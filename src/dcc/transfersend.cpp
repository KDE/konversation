/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2004, 2005 John Tapsell <john@geola.co.uk>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include <QtSystemDetection>

#include "transfersend.h"
#include "dcccommon.h"
#include "transfermanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "upnprouter.h"
#include "konversation_log.h"

#include <QTimer>
#include <QTcpSocket>
#include <QTcpServer>
#include <QInputDialog>
#include <QTemporaryFile>

// TODO: remove the dependence
#include <KAuthorized>

#include <KIO/Job>
#include <KIO/StatJob>
#include <KIO/CopyJob>

#ifdef Q_OS_WIN
// Prevent windows system header files from defining min/max as macros.
#define NOMINMAX 1
#include <winsock2.h>
#endif

using namespace Konversation::UPnP;

namespace Konversation
{
    namespace DCC
    {
        TransferSend::TransferSend(QObject *parent)
            : Transfer(Transfer::Send, parent)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            m_serverSocket = nullptr;
            m_sendSocket = nullptr;
            m_tmpFile = nullptr;

            m_connectionTimer = new QTimer(this);
            m_connectionTimer->setSingleShot(true);
            connect(m_connectionTimer, &QTimer::timeout, this, &TransferSend::slotConnectionTimeout);

            // set defualt values
            m_reverse = Preferences::self()->dccPassiveSend();
        }

        TransferSend::~TransferSend()
        {
            cleanUp();
        }

        void TransferSend::cleanUp()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            stopConnectionTimer();
            disconnect(m_connectionTimer, nullptr, nullptr, nullptr);

            finishTransferLogger();
            if (m_tmpFile)
            {
                delete m_tmpFile;
                m_tmpFile = nullptr;
            }

            m_file.close();
            if (m_sendSocket)
            {
                disconnect(m_sendSocket, nullptr, nullptr, nullptr);
                m_sendSocket->close();
                m_sendSocket = nullptr;                         // the instance will be deleted automatically by its parent
            }
            if (m_serverSocket)
            {
                m_serverSocket->close();
                m_serverSocket = nullptr;                       // the instance will be deleted automatically by its parent

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                    if (router)
                    {
                        router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                    }
                }
            }
            Transfer::cleanUp();
        }

        void TransferSend::setFileURL(const QUrl &url)
        {
            if (getStatus() == Configuring)
            {
                m_fileURL = url;
            }
        }

        void TransferSend::setFileName(const QString &fileName)
        {
            if (getStatus() == Configuring)
            {
                m_fileName = fileName;
            }
        }

        void TransferSend::setOwnIp(const QString &ownIp)
        {
            if (getStatus() == Configuring)
            {
                m_ownIp = ownIp;
            }
        }

        void TransferSend::setFileSize(KIO::filesize_t fileSize)
        {
            if (getStatus() == Configuring)
            {
                m_fileSize = fileSize;
            }
        }

        void TransferSend::setReverse(bool reverse)
        {
            if (getStatus() == Configuring)
            {
                m_reverse = reverse;
            }
        }

        bool TransferSend::queue()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            if (getStatus() != Configuring)
            {
                return false;
            }

            if (m_ownIp.isEmpty())
            {
                m_ownIp = DccCommon::getOwnIp(Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId));
            }

            if (!KAuthorized::authorizeAction(QStringLiteral("allow_downloading")))
            {
                //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
                //Note this is after the initialisation so the view looks correct still
                failed(i18n("The admin has restricted the right to send files"));
                return false;
            }

            if (m_fileName.isEmpty())
            {
                m_fileName = sanitizeFileName(m_fileURL.fileName());
            }

            if (Preferences::self()->dccIPv4Fallback())
            {
                m_ownIp = DCC::DccCommon::ipv6FallbackAddress(m_ownIp);
            }

            m_fastSend = Preferences::self()->dccFastSend();
            qCDebug(KONVERSATION_LOG) << "Fast DCC send: " << m_fastSend;

            //Check the file exists
            KIO::StatJob* statJob = KIO::stat(m_fileURL, KIO::StatJob::SourceSide, KIO::StatNoDetails);
            statJob->exec();
            if (statJob->error())
            {
                failed(i18n("The url \"%1\" does not exist", m_fileURL.toString()));
                return false;
            }

            //Some protocols, like http, maybe not return a filename, and altFileName may be empty, So prompt the user for one.
            if (m_fileName.isEmpty())
            {
                bool pressedOk;
                m_fileName = QInputDialog::getText(nullptr, i18n("Enter Filename"),
                                                   i18n("<qt>The file that you are sending to <i>%1</i> does not have a filename.<br/>Please enter a filename to be presented to the receiver, or cancel the dcc transfer</qt>", getPartnerNick()),
                                                   QLineEdit::EchoMode::Normal, i18n("unknown"),
                                                   &pressedOk);

                if (!pressedOk)
                {
                    failed(i18n("No filename was given"));
                    return false;
                }
            }

            //FIXME: if "\\\"" works well on other IRC clients, replace "\"" with "\\\""
            m_fileName.replace(QLatin1Char('\"'), QLatin1Char('_'));
            if (Preferences::self()->dccSpaceToUnderscore())
            {
                m_fileName.replace(QLatin1Char(' '), QLatin1Char('_'));
            }

            if (!m_fileURL.isLocalFile())
            {
                m_tmpFile = new QTemporaryFile();
                m_tmpFile->open(); // create the file, and thus create m_tmpFile.fileName
                m_tmpFile->close(); // no need to keep the file open, it isn't deleted until the destructor is called

                QUrl tmpUrl = QUrl::fromLocalFile(m_tmpFile->fileName());
                KIO::CopyJob *fileCopyJob = KIO::copy(m_fileURL, tmpUrl, KIO::Overwrite);

                connect(fileCopyJob, &KIO::CopyJob::result, this, &TransferSend::slotLocalCopyReady);
                fileCopyJob->start();
                setStatus(Preparing);
                return false; // not ready to send yet
            }

            slotLocalCopyReady(nullptr);
            return true;
        }

        void TransferSend::slotLocalCopyReady(KJob *job)
        {
            QString fn = m_fileURL.toDisplayString();
            bool remoteFile = job != nullptr;
            int error = job ? job->error() : 0;

            qCDebug(KONVERSATION_LOG) << "m_tmpFile: " << fn << "error: " << error << "remote file: " << remoteFile;

            if (error)
            {
                failed(i18n("Could not retrieve \"%1\"", fn));
                return;
            }

            if (remoteFile)
            {
                m_file.setFileName(m_tmpFile->fileName());
            }
            else
            {
                m_file.setFileName(m_fileURL.toLocalFile());
            }

            if (m_fileSize == 0)
            {
                m_fileSize = m_file.size();

                qCDebug(KONVERSATION_LOG) << "filesize 0, new filesize: " << m_fileSize;

                if (m_fileSize == 0)
                {
                    failed(i18n("Unable to send a 0 byte file."));
                    return;
                }
            }

            setStatus(Queued);
            if (remoteFile)
            {
                start(); // addDccSend would have done it for us if this was a local file
            }
        }

        void TransferSend::reject()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            failed(i18n("DCC SEND request was rejected"));
        }

        void TransferSend::abort()                     // public slot
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            cleanUp();
            setStatus(Aborted);
            Q_EMIT done(this);
        }

        void TransferSend::start()                     // public slot
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            if (getStatus() != Queued)
            {
                return;
            }

            // common procedure

            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (!server)
            {
                qCDebug(KONVERSATION_LOG) << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed(i18n("Could not send a DCC SEND request to the partner via the IRC server."));
                return;
            }

            if (!m_reverse)
            {
                // Normal DCC SEND
                qCDebug(KONVERSATION_LOG) << "normal DCC SEND";

                // Set up server socket
                QString failedReason;
                if (Preferences::self()->dccSpecificSendPorts())
                {
                    m_serverSocket = DccCommon::createServerSocketAndListen(this, &failedReason, Preferences::self()->dccSendPortsFirst(), Preferences::self()->dccSendPortsLast());
                }
                else
                {
                    m_serverSocket = DccCommon::createServerSocketAndListen(this, &failedReason);
                }

                if (!m_serverSocket)
                {
                    failed(failedReason);
                    return;
                }

                connect(m_serverSocket, &QTcpServer::newConnection, this, &TransferSend::acceptClient);

                // Get own port number
                m_ownPort = m_serverSocket->serverPort();

                qCDebug(KONVERSATION_LOG) << "Own Address=" << m_ownIp << ":" << m_ownPort;

                if (Preferences::self()->dccUPnP())
                {
                    UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();

                    if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), m_ownPort, QAbstractSocket::TcpSocket))
                    {
                        connect(router, &UPnPRouter::forwardComplete, this, &TransferSend::sendRequest);
                    }
                    else
                    {
                        sendRequest(true, 0); // Just try w/o UPnP on failure
                    }
                }
                else
                {
                    sendRequest(false, 0);
                }
            }
            else
            {
                // Passive DCC SEND
                qCDebug(KONVERSATION_LOG) << "Passive DCC SEND";

                int tokenNumber = Application::instance()->getDccTransferManager()->generateReverseTokenNumber();
                // TODO: should we append a letter "T" to this token?
                m_reverseToken = QString::number(tokenNumber);

                qCDebug(KONVERSATION_LOG) << "Passive DCC key(token): " << m_reverseToken;

                startConnectionTimer(Preferences::self()->dccSendTimeout());

                server->dccPassiveSendRequest(m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp(m_ownIp), m_fileSize, m_reverseToken);
            }

            setStatus(WaitingRemote, i18n("Awaiting acceptance by remote user..."));
        }

        void TransferSend::sendRequest(bool error, quint16 port)
        {
            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (!server)
            {
                qCDebug(KONVERSATION_LOG) << "could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed(i18n("Could not send a DCC SEND request to the partner via the IRC server."));
                return;
            }

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                if (port != m_ownPort) return; // Somebody elses forward succeeded

                auto* router = qobject_cast<UPnP::UPnPRouter*>(this->sender());
                disconnect(router, &UPnPRouter::forwardComplete, this, &TransferSend::sendRequest);

                if (error)
                {
                    server->appendMessageToFrontmost(i18nc("Universal Plug and Play", "UPnP"), i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)), QHash<QString, QString>(), false);
                }
            }

            startConnectionTimer(Preferences::self()->dccSendTimeout());

            server->dccSendRequest(m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp(m_ownIp), m_ownPort, m_fileSize);
        }

        void TransferSend::connectToReceiver(const QString &partnerHost, quint16 partnerPort)
        {
            qCDebug(KONVERSATION_LOG) << "host:" << partnerHost << "port:" << partnerPort;
            // Reverse DCC

            startConnectionTimer(Preferences::self()->dccSendTimeout());

            m_partnerIp = partnerHost;
            m_partnerPort = partnerPort;

            m_sendSocket = new QTcpSocket(this);

            connect(m_sendSocket, &QTcpSocket::connected, this, &TransferSend::startSending);
            connect(m_sendSocket, &QTcpSocket::errorOccurred, this, &TransferSend::slotGotSocketError);

            setStatus(Connecting);

            m_sendSocket->connectToHost(partnerHost, partnerPort);
        }
                                                          // public
        bool TransferSend::setResume(quint64 position)
        {
            qCDebug(KONVERSATION_LOG) << "Position=" << position;

            if (getStatus() > WaitingRemote)
            {
                return false;
            }

            if (position >= m_fileSize)
            {
                return false;
            }

            m_resumed = true;
            m_transferringPosition = position;
            return true;
        }

        void TransferSend::acceptClient()                     // slot
        {
            // Normal DCC

            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            stopConnectionTimer();

            m_sendSocket = m_serverSocket->nextPendingConnection();
            if (!m_sendSocket)
            {
                failed(i18n("Could not accept the connection (socket error)."));
                return;
            }
            connect(m_sendSocket, &QTcpSocket::errorOccurred, this, &TransferSend::slotGotSocketError);

            // we don't need ServerSocket anymore
            m_serverSocket->close();
            m_serverSocket = nullptr; // the instance will be deleted automatically by its parent

            if (Preferences::self()->dccUPnP())
            {
                UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                if (router)
                {
                    router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                }
            }

            startSending();
        }

        void TransferSend::startSending()
        {
            stopConnectionTimer();

            connect(m_sendSocket, &QTcpSocket::bytesWritten, this, &TransferSend::bytesWritten);
            connect(m_sendSocket, &QTcpSocket::readyRead, this, &TransferSend::getAck);

            m_partnerIp = m_sendSocket->peerAddress().toString();
            m_partnerPort = m_sendSocket->peerPort();
            m_ownPort = m_sendSocket->localPort();

            if (m_file.open(QIODevice::ReadOnly))
            {
                // seek to file position to make resume work
                m_file.seek(m_transferringPosition);
                m_transferStartPosition = m_transferringPosition;

                writeData();
                startTransferLogger();                      // initialize CPS counter, ETA counter, etc...
                setStatus(Transferring);
            }
            else
            {
                failed(getQFileErrorString(m_file.error()));
            }
        }

        void TransferSend::bytesWritten(qint64 bytes)
        {
            if (bytes > 0)
            {
                m_transferringPosition += bytes;
                if ((KIO::fileoffset_t)m_fileSize <= m_transferringPosition)
                {
                    Q_ASSERT((KIO::fileoffset_t)m_fileSize == m_transferringPosition);
                    qCDebug(KONVERSATION_LOG) << "Done.";
                }
            }

            if (m_sendSocket)
            {
                if (m_fastSend && m_sendSocket->bytesToWrite() <= (qint64)m_bufferSize)
                {
                    writeData();
                }
                else if (!m_fastSend && bytes == 0)
                {
                    writeData();
                }
            }
        }

        void TransferSend::writeData()                 // slot
        {
            //qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            qint64 actual = m_file.read(m_buffer, m_bufferSize);
            if (actual > 0)
            {
                m_sendSocket->write(m_buffer, actual);
            }
        }

        void TransferSend::getAck()                    // slot
        {
            //qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            if (m_transferringPosition < (KIO::fileoffset_t)m_fileSize)
            {
                //don't write data directly, in case we get spammed with ACK we try so send too fast
                bytesWritten(0);
            }

            quint32 pos;
            while (m_sendSocket->bytesAvailable() >= 4)
            {
                m_sendSocket->read((char*)&pos, 4);
                pos = ntohl(pos);

                //qCDebug(KONVERSATION_LOG) << pos  << "/" << m_fileSize;
                if (pos == m_fileSize)
                {
                    qCDebug(KONVERSATION_LOG) << "Received final ACK.";
                    cleanUp();
                    setStatus(Done);
                    Q_EMIT done(this);
                    break;                                // for safe
                }
            }
        }

        void TransferSend::slotGotSocketError(QAbstractSocket::SocketError errorCode)
        {
            stopConnectionTimer();
            qCDebug(KONVERSATION_LOG) << "code =  " << errorCode << " string = " << m_sendSocket->errorString();
            failed(i18n("Socket error: %1", m_sendSocket->errorString()));
        }

        void TransferSend::startConnectionTimer(int secs)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            //start also restarts, no need for us to double check it
            m_connectionTimer->start(secs * 1000);
        }

        void TransferSend::stopConnectionTimer()
        {
            if (m_connectionTimer->isActive())
            {
                qCDebug(KONVERSATION_LOG) << "stop";
                m_connectionTimer->stop();
            }
        }

        void TransferSend::slotConnectionTimeout()         // slot
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            failed(i18n("Timed out"));
        }
                                                          // protected, static
        QString TransferSend::getQFileErrorString(int code) const
        {
            QString errorString;

            switch(code)
            {
                case QFile::NoError:
                    errorString = i18n("The operation was successful. Should never happen in an error dialog.");
                    break;
                case QFile::ReadError:
                    errorString = i18n("Could not read from file \"%1\".", m_fileName);
                    break;
                case QFile::WriteError:
                    errorString = i18n("Could not write to file \"%1\".", m_fileName);
                    break;
                case QFile::FatalError:
                    errorString = i18n("A fatal unrecoverable error occurred.");
                    break;
                case QFile::OpenError:
                    errorString = i18n("Could not open file \"%1\".", m_fileName);
                    break;

                    // Same case value? Damn!
                    //        case IO_ConnectError:
                    //          errorString="Could not connect to the device.";
                    //        break;

                case QFile::AbortError:
                    errorString = i18n("The operation was unexpectedly aborted.");
                    break;
                case QFile::TimeOutError:
                    errorString = i18n("The operation timed out.");
                    break;
                case QFile::UnspecifiedError:
                    errorString = i18n("An unspecified error happened on close.");
                    break;
                default:
                    errorString = i18n("Unknown error. Code %1",code);
                    break;
            }

            return errorString;
        }

    }
}

#include "moc_transfersend.cpp"
