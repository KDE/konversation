/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2004, 2005 John Tapsell <john@geola.co.uk>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include <QtSystemDetection>

#include "transferrecv.h"
#include "dcccommon.h"
#include "transfermanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "upnprouter.h"
#include "konversation_log.h"

#include <QDateTime>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFileInfo>

#include <KUser>
#include <KAuthorized>
#include <KIO/Job>
#include <KIO/StatJob>
#include <KIO/MkdirJob>
#include <KIO/StoredTransferJob>

#ifdef Q_OS_WIN
// Prevent windows system header files from defining min/max as macros.
#define NOMINMAX 1
#include <winsock2.h>
#endif

/*
 *flow chart*

 TransferRecv()

 start()              : called from TransferPanel when user pushes the accept button
  | \
  | requestResume()   : called when user chooses to resume in ResumeDialog. it emits the signal ResumeRequest()
  |
  | startResume()     : called by "Server"
  | |
connectToSender()

connectionSuccess()  : called by recvSocket

*/

namespace Konversation
{
    namespace DCC
    {
        TransferRecv::TransferRecv(QObject *parent)
            : Transfer(Transfer::Receive, parent)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            m_serverSocket = nullptr;
            m_recvSocket = nullptr;
            m_writeCacheHandler = nullptr;

            m_connectionTimer = new QTimer(this);
            m_connectionTimer->setSingleShot(true);
            connect(m_connectionTimer, &QTimer::timeout, this, &TransferRecv::connectionTimeout);
            //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
        }

        TransferRecv::~TransferRecv()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            cleanUp();
        }

        void TransferRecv::cleanUp()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            stopConnectionTimer();
            disconnect(m_connectionTimer, nullptr, nullptr, nullptr);

            finishTransferLogger();
            if (m_serverSocket)
            {
                m_serverSocket->close();
                m_serverSocket = nullptr;

                if (m_reverse && Preferences::self()->dccUPnP())
                {
                    UPnP::UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                    if (router)
                    {
                        router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                    }
                }
            }
            if (m_recvSocket)
            {
                disconnect(m_recvSocket, nullptr, nullptr, nullptr);
                m_recvSocket->close();
                m_recvSocket = nullptr;                         // the instance will be deleted automatically by its parent
            }
            if (m_writeCacheHandler)
            {
                m_writeCacheHandler->closeNow();
                m_writeCacheHandler->deleteLater();
                m_writeCacheHandler = nullptr;
            }
            Transfer::cleanUp();
        }

        void TransferRecv::setPartnerIp(const QString &ip)
        {
            if (getStatus() == Configuring)
            {
                m_partnerIp = ip;
            }
        }

        void TransferRecv::setPartnerPort(quint16 port)
        {
            if (getStatus() == Configuring)
            {
                m_partnerPort = port;
            }
        }

        void TransferRecv::setFileSize(quint64 fileSize)
        {
            if (getStatus() == Configuring)
            {
                m_fileSize = fileSize;
            }
        }

        void TransferRecv::setFileName(const QString &fileName)
        {
            if (getStatus() == Configuring)
            {
                m_fileName = fileName;
                m_saveFileName = m_fileName;
            }
        }

        void TransferRecv::setFileURL(const QUrl &url)
        {
            if (getStatus() == Preparing || getStatus() == Configuring || getStatus() == Queued)
            {
                m_fileURL = url;
                m_saveFileName = url.fileName();
            }
        }

        void TransferRecv::setReverse(bool reverse, const QString &reverseToken)
        {
            if (getStatus() == Configuring)
            {
                m_reverse = reverse;
                if (reverse)
                {
                    m_partnerPort = 0;
                    m_reverseToken = reverseToken;
                }
            }
        }

        bool TransferRecv::queue()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            if (getStatus() != Configuring)
            {
                return false;
            }

            if (m_partnerIp.isEmpty())
            {
                return false;
            }

            if (m_ownIp.isEmpty())
            {
                m_ownIp = DccCommon::getOwnIp(Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId));
            }

            if (!KAuthorized::authorizeAction(QStringLiteral("allow_downloading")))
            {
                //note we have this after the initialisations so that item looks okay
                //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
                failed(i18n("The admin has restricted the right to receive files"));
                return false;
            }

            // check if the sender IP is valid
            if (m_partnerIp == QLatin1String("0.0.0.0"))
            {
                failed(i18n("Invalid sender address (%1)", m_partnerIp));
                return false;
            }

            // TODO: should we support it?
            if (m_fileSize == 0)
            {
                failed(i18n("Unsupported negotiation (filesize=0)"));
                return false;
            }

            if (m_fileName.isEmpty())
            {
                m_fileName = QLatin1String("unnamed_file_") + QDateTime::currentDateTime().toString(Qt::ISODate).remove(QLatin1Char(':'));
                m_saveFileName = m_fileName;
            }

            if (m_fileURL.isEmpty())
            {
                // determine default incoming file URL

                // set default folder
                if (!Preferences::self()->dccPath().isEmpty())
                {
                    m_fileURL = Preferences::self()->dccPath();
                }
                else
                {
                    m_fileURL.setPath(KUser(KUser::UseRealUserID).homeDir());  // default folder is *not* specified
                }

                //buschinski TODO CHECK ME
                // add a slash if there is none
                //m_fileURL.adjustPath(KUrl::AddTrailingSlash);

                // Append folder with partner's name if wanted
                if (Preferences::self()->dccCreateFolder())
                {
                    m_fileURL = m_fileURL.adjusted(QUrl::StripTrailingSlash);
                    m_fileURL.setPath(m_fileURL.path() + QDir::separator() + m_partnerNick);
                }

                // Just incase anyone tries to do anything nasty
                QString fileNameSanitized = sanitizeFileName(m_saveFileName);

                // Append partner's name to file name if wanted
                if (Preferences::self()->dccAddPartner())
                {
                    m_fileURL = m_fileURL.adjusted(QUrl::StripTrailingSlash);
                    m_fileURL.setPath(m_fileURL.path() + QDir::separator() + m_partnerNick + QLatin1Char('.') + fileNameSanitized);
                }
                else
                {
                    m_fileURL = m_fileURL.adjusted(QUrl::StripTrailingSlash);
                    m_fileURL.setPath(m_fileURL.path() + QDir::separator() + fileNameSanitized);
                }
            }

            return Transfer::queue();
        }

        void TransferRecv::abort()                     // public slot
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            if (getStatus() == Transfer::Queued)
            {
                Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
                if (server)
                {
                    server->dccRejectSend(m_partnerNick, transferFileName(m_fileName));
                }
            }

            if(m_writeCacheHandler)
            {
                m_writeCacheHandler->write(true);       // flush
            }

            cleanUp();
            setStatus(Aborted);
            Q_EMIT done(this);
        }

        void TransferRecv::start()                     // public slot
        {
            qCDebug(KONVERSATION_LOG) << "[BEGIN]";

            if (getStatus() != Queued)
            {
                return;
            }

            setStatus(Preparing);

            prepareLocalKio(false, false);

            qCDebug(KONVERSATION_LOG) << "[END]";
        }

        void TransferRecv::prepareLocalKio(bool overwrite, bool resume, KIO::fileoffset_t startPosition)
        {
            qCDebug(KONVERSATION_LOG)
                << "URL: " << m_fileURL
                << "\nOverwrite: " << overwrite
                << "\nResume: " << resume << " (Position: " << startPosition << ")";

            m_resumed = resume;
            m_transferringPosition = startPosition;

            if (!createDirs(KIO::upUrl(m_fileURL)))
            {
                askAndPrepareLocalKio(i18n("<b>Cannot create the folder or destination is not writable.</b><br/>"
                    "Folder: %1<br/>",
                    KIO::upUrl(m_fileURL).toString()),
                    ResumeDialog::RA_Rename | ResumeDialog::RA_Cancel | ResumeDialog::RA_OverwriteDefaultPath,
                    ResumeDialog::RA_Rename);
                return;
            }

            if (Application::instance()->getDccTransferManager()->isLocalFileInWritingProcess(m_fileURL))
            {
                askAndPrepareLocalKio(i18n("<b>The file is used by another transfer.</b><br/>"
                    "%1<br/>",
                    m_fileURL.toString()),
                    ResumeDialog::RA_Rename | ResumeDialog::RA_Cancel,
                    ResumeDialog::RA_Rename);
                return;
            }

            KIO::JobFlags flags;
            if(overwrite)
            {
                flags |= KIO::Overwrite;
            }
            if(m_resumed)
            {
                flags |= KIO::Resume;
            }
            //for now, maybe later
            flags |= KIO::HideProgressInfo;
            KIO::TransferJob *transferJob = KIO::put(m_fileURL, -1, flags);

            if (!transferJob)
            {
                qCDebug(KONVERSATION_LOG) << "KIO::put() returned NULL. what happened?";
                failed(i18n("Could not create a KIO instance"));
                return;
            }

            transferJob->setAutoDelete(true);
            connect(transferJob, &KIO::TransferJob::canResume, this, &TransferRecv::slotLocalCanResume);
            connect(transferJob, &KIO::TransferJob::result, this, &TransferRecv::slotLocalGotResult);
            connect(transferJob, &KIO::TransferJob::dataReq, this, &TransferRecv::slotLocalReady);
        }

        void TransferRecv::askAndPrepareLocalKio(const QString &message, int enabledActions, ResumeDialog::ReceiveAction defaultAction, KIO::fileoffset_t startPosition)
        {
            switch (ResumeDialog::ask(this, message, enabledActions, defaultAction))
            {
                case ResumeDialog::RA_Resume:
                    prepareLocalKio(false, true, startPosition);
                    break;
                case ResumeDialog::RA_Overwrite:
                    prepareLocalKio(true, false);
                    break;
                case ResumeDialog::RA_Rename:
                    prepareLocalKio(false, false);
                    break;
                case ResumeDialog::RA_Cancel:
                default:
                    setStatus(Queued);
            }
        }

        bool TransferRecv::createDirs(const QUrl &dirURL) const
        {
            QUrl kurl(dirURL);

            //First we split directories until we reach to the top,
            //since we need to create directories one by one

            QList<QUrl> dirList;
            while (kurl != KIO::upUrl(kurl))
            {
                dirList.prepend(kurl);
                kurl = KIO::upUrl(kurl);
            }

            //Now we create the directories

            for (const QUrl& dir : std::as_const(dirList)) {
                KIO::StatJob* statJob = KIO::stat(dir, KIO::StatJob::SourceSide, KIO::StatNoDetails);
                statJob->exec();
                if (statJob->error())
                {
                    KIO::MkdirJob* job = KIO::mkdir(dir, -1);
                    if (!job->exec())
                    {
                        return false;
                    }
                }
            }

#ifndef Q_OS_WIN
            QFileInfo dirInfo(dirURL.toLocalFile());
            if (!dirInfo.isWritable())
            {
                return false;
            }
#else
            //!TODO find equivalent windows solution
            //from 4.7 QFile Doc:
            // File permissions are handled differently on Linux/Mac OS X and Windows.
            // In a non writable directory on Linux, files cannot be created.
            // This is not always the case on Windows, where, for instance,
            // the 'My Documents' directory usually is not writable, but it is still
            // possible to create files in it.
#endif

            return true;
        }

        void TransferRecv::slotLocalCanResume(KIO::Job *job, KIO::filesize_t size)
        {
            qCDebug(KONVERSATION_LOG) << "[BEGIN]\n"
                << "size: " << size;

            auto* transferJob = qobject_cast<KIO::TransferJob*>(job);
            if (!transferJob)
            {
                qCDebug(KONVERSATION_LOG) << "not a TransferJob? returning";
                return;
            }

            if (size != 0)
            {
                disconnect(transferJob, nullptr, nullptr, nullptr);
                if (Preferences::self()->dccAutoResume())
                {
                    prepareLocalKio(false, true, size);
                }
                else
                {
                    askAndPrepareLocalKio(i18np(
                        "<b>A partial file exists:</b><br/>"
                        "%2<br/>"
                        "Size of the partial file: 1 byte.<br/>",
                        "<b>A partial file exists:</b><br/>"
                        "%2<br/>"
                        "Size of the partial file: %1 bytes.<br/>",
                        size,
                        m_fileURL.toString()),
                        ResumeDialog::RA_Resume | ResumeDialog::RA_Overwrite | ResumeDialog::RA_Rename | ResumeDialog::RA_Cancel,
                        ResumeDialog::RA_Resume,
                        size);
                }
                transferJob->putOnHold();
            }

            qCDebug(KONVERSATION_LOG) << "[END]";
        }

        void TransferRecv::slotLocalGotResult(KJob *job)
        {
            qCDebug(KONVERSATION_LOG) << "[BEGIN]";

            auto* transferJob = static_cast<KIO::TransferJob*>(job);
            disconnect(transferJob, nullptr, nullptr, nullptr);

            switch (transferJob->error())
            {
                case 0:                                   // no error
                    qCDebug(KONVERSATION_LOG) << "job->error() returned 0.\n"
                        << "Why was I called in spite of no error?";
                    break;
                case KIO::ERR_FILE_ALREADY_EXIST:
                    askAndPrepareLocalKio(i18nc("%1=fileName, %2=local filesize, %3=sender filesize",
                                                "<b>The file already exists.</b><br/>"
                                                "%1 (%2)<br/>"
                                                "Sender reports file size of %3<br/>",
                                                m_fileURL.toString(), KIO::convertSize(QFileInfo(m_fileURL.path()).size()),
                                                KIO::convertSize(m_fileSize)),
                                          ResumeDialog::RA_Overwrite | ResumeDialog::RA_Rename | ResumeDialog::RA_Cancel,
                                          ResumeDialog::RA_Overwrite);
                    break;
                default:
                    askAndPrepareLocalKio(i18n("<b>Could not open the file.<br/>"
                        "Error: %1</b><br/>"
                        "%2<br/>",
                        transferJob->error(),
                        m_fileURL.toString()),
                        ResumeDialog::RA_Rename | ResumeDialog::RA_Cancel,
                        ResumeDialog::RA_Rename);
            }

            qCDebug(KONVERSATION_LOG) << "[END]";
        }

        void TransferRecv::slotLocalReady(KIO::Job *job)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            auto* transferJob = static_cast<KIO::TransferJob*>(job);

            disconnect(transferJob, nullptr, nullptr, nullptr);           // WriteCacheHandler will control the job after this

            m_writeCacheHandler = new TransferRecvWriteCacheHandler(transferJob);

            connect(m_writeCacheHandler, &TransferRecvWriteCacheHandler::done, this, &TransferRecv::slotLocalWriteDone);
            connect(m_writeCacheHandler, &TransferRecvWriteCacheHandler::gotError, this, &TransferRecv::slotLocalGotWriteError);

            if (!m_resumed)
            {
                connectWithSender();
            }
            else
            {
                requestResume();
            }
        }

        void TransferRecv::connectWithSender()
        {
            if (m_reverse)
            {
                if (!startListeningForSender())
                {
                    return;
                }

                Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
                if (!server)
                {
                    failed(i18n("Could not send Reverse DCC SEND acknowledgement to the partner via the IRC server."));
                    return;
                }

                m_ownIp = DccCommon::getOwnIp(server);
                m_ownPort = m_serverSocket->serverPort();

                if (Preferences::self()->dccUPnP())
                {
                    UPnP::UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();

                    if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), m_ownPort, QAbstractSocket::TcpSocket))
                    {
                        connect(router, &UPnP::UPnPRouter::forwardComplete, this, &TransferRecv::sendReverseAck);
                    }
                    else
                    {
                        sendReverseAck(true, 0); // Try anyways on error
                    }
                }
                else
                {
                    sendReverseAck(false, 0);
                }
            }
            else
            {
                connectToSendServer();
            }
        }

        void TransferRecv::sendReverseAck(bool error, quint16 port)
        {
            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (!server)
            {
                failed(i18n("Could not send Reverse DCC SEND acknowledgement to the partner via the IRC server."));
                return;
            }

            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                if (port != m_ownPort) return; // Somebody elses forward succeeded

                auto* router = qobject_cast<UPnP::UPnPRouter*>(this->sender());
                disconnect(router, &UPnP::UPnPRouter::forwardComplete, this, &TransferRecv::sendReverseAck);

                if (error)
                {
                    server->appendMessageToFrontmost(i18nc("Universal Plug and Play", "UPnP"), i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)), QHash<QString, QString>(), false);
                }
            }

            setStatus(WaitingRemote, i18n("Waiting for connection"));

            server->dccReverseSendAck(m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp(m_ownIp), m_ownPort, m_fileSize, m_reverseToken);
        }

        void TransferRecv::requestResume()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            setStatus(WaitingRemote, i18n("Waiting for remote host's acceptance"));

            startConnectionTimer(30);

            qCDebug(KONVERSATION_LOG) << "Requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort;

            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (!server)
            {
                qCDebug(KONVERSATION_LOG) << "Could not retrieve the instance of Server. Connection id: " << m_connectionId;
                failed(i18n("Could not send DCC RECV resume request to the partner via the IRC server."));
                return;
            }

            if (m_reverse)
            {
                server->dccPassiveResumeGetRequest(m_partnerNick, transferFileName(m_fileName), m_partnerPort, m_transferringPosition, m_reverseToken);
            }
            else
            {
                server->dccResumeGetRequest(m_partnerNick, transferFileName(m_fileName), m_partnerPort, m_transferringPosition);
            }
        }

                                                          // public slot
        void TransferRecv::startResume(quint64 position)
        {
            qCDebug(KONVERSATION_LOG) << "Position:" << position;

            stopConnectionTimer();

            if ((quint64)m_transferringPosition != position)
            {
                qCDebug(KONVERSATION_LOG) << "remote responded with an unexpected position\n"
                    << "expected: " << m_transferringPosition << "\nremote response: " << position;
                failed(i18n("Unexpected response from remote host"));
                return;
            }

            connectWithSender();
        }

        void TransferRecv::connectToSendServer()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            // connect to sender

            setStatus(Connecting);

            startConnectionTimer(30);

            m_recvSocket = new QTcpSocket(this);

            connect(m_recvSocket, &QTcpSocket::connected, this, &TransferRecv::startReceiving);
            connect(m_recvSocket, &QTcpSocket::errorOccurred, this, &TransferRecv::connectionFailed);

            qCDebug(KONVERSATION_LOG) << "Attempting to connect to " << m_partnerIp << ":" << m_partnerPort;

            m_recvSocket->connectToHost(m_partnerIp, m_partnerPort);
        }

        bool TransferRecv::startListeningForSender()
        {
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
                return false;
            }

            connect(m_serverSocket, &QTcpServer::newConnection, this, &TransferRecv::slotServerSocketReadyAccept);

            startConnectionTimer(30);

            return true;
        }

        void TransferRecv::slotServerSocketReadyAccept()
        {
            //reverse dcc

            m_recvSocket = m_serverSocket->nextPendingConnection();
            if (!m_recvSocket)
            {
                failed(i18n("Could not accept the connection (socket error)."));
                return;
            }

            connect(m_recvSocket, &QTcpSocket::errorOccurred, this, &TransferRecv::connectionFailed);

            // we don't need ServerSocket anymore
            m_serverSocket->close();
            m_serverSocket = nullptr; // Will be deleted by parent

            if (Preferences::self()->dccUPnP())
            {
                UPnP::UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
                if (router)
                {
                    router->undoForward(m_ownPort, QAbstractSocket::TcpSocket);
                }
            }

            startReceiving();
        }

        void TransferRecv::startReceiving()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            stopConnectionTimer();

            connect(m_recvSocket, &QTcpSocket::readyRead, this, &TransferRecv::readData);

            m_transferStartPosition = m_transferringPosition;

            //we don't need the original filename anymore, overwrite it to display the correct one in transfermanager/panel
            m_fileName = m_saveFileName;

            m_ownPort = m_recvSocket->localPort();

            startTransferLogger();                          // initialize CPS counter, ETA counter, etc...

            setStatus(Transferring);
        }

                                                          // slot
        void TransferRecv::connectionFailed(QAbstractSocket::SocketError errorCode)
        {
            qCDebug(KONVERSATION_LOG) << "Code = " << errorCode << ", string = " << m_recvSocket->errorString();
            failed(m_recvSocket->errorString());
        }

        void TransferRecv::readData()                  // slot
        {
            //qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            qint64 actual = m_recvSocket->read(m_buffer, m_bufferSize);
            if (actual > 0)
            {
                //actual is the size we read in, and is guaranteed to be less than m_bufferSize
                m_transferringPosition += actual;
                m_writeCacheHandler->append(m_buffer, actual);
                m_writeCacheHandler->write(false);
                //in case we could not read all the data, leftover data could get lost
                if (m_recvSocket->bytesAvailable() > 0)
                {
                    readData();
                }
                else
                {
                    sendAck();
                }
            }
        }

        void TransferRecv::sendAck()                   // slot
        {
            //qCDebug(KONVERSATION_LOG) << m_transferringPosition << "/" << (KIO::fileoffset_t)m_fileSize;

            //It is bound to be 32bit according to dcc specs, -> 4GB limit.
            //But luckily no client ever reads this value,
            //except for old mIRC versions, but they couldn't send or receive files over 4GB anyway.
            //Note: The resume and filesize are set via dcc send command and can be over 4GB

            quint32 pos = htonl(static_cast<quint32>(m_transferringPosition));

            m_recvSocket->write((char*)&pos, 4);
            if (m_transferringPosition == static_cast<KIO::fileoffset_t>(m_fileSize))
            {
                qCDebug(KONVERSATION_LOG) << "Sent final ACK.";
                disconnect(m_recvSocket, nullptr, nullptr, nullptr);
                m_writeCacheHandler->close();             // WriteCacheHandler will send the signal done()
            }
            else if (m_transferringPosition > static_cast<KIO::fileoffset_t>(m_fileSize))
            {
                qCDebug(KONVERSATION_LOG) << "The remote host sent larger data than expected: " << m_transferringPosition;
                failed(i18n("Transfer error"));
            }
        }

        void TransferRecv::slotLocalWriteDone()        // <-WriteCacheHandler::done()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            cleanUp();
            setStatus(Done);
            Q_EMIT done(this);
        }

                                                          // <- WriteCacheHandler::gotError()
        void TransferRecv::slotLocalGotWriteError(const QString &errorString)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            failed(i18n("KIO error: %1", errorString));
        }

        void TransferRecv::startConnectionTimer(int secs)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            m_connectionTimer->start(secs * 1000);
        }

        void TransferRecv::stopConnectionTimer()
        {
            if (m_connectionTimer->isActive())
            {
                m_connectionTimer->stop();
                qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            }
        }

        void TransferRecv::connectionTimeout()         // slot
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            failed(i18n("Timed out"));
        }

        // WriteCacheHandler

        TransferRecvWriteCacheHandler::TransferRecvWriteCacheHandler(KIO::TransferJob *transferJob)
            : m_transferJob(transferJob)
        {
            m_writeReady = true;
            m_cacheStream = nullptr;

            connect(m_transferJob, &KIO::TransferJob::dataReq, this, &TransferRecvWriteCacheHandler::slotKIODataReq);
            connect(m_transferJob, &KIO::TransferJob::result, this, &TransferRecvWriteCacheHandler::slotKIOResult);

            m_transferJob->setAsyncDataEnabled(m_writeAsyncMode = true);
        }

        TransferRecvWriteCacheHandler::~TransferRecvWriteCacheHandler()
        {
            closeNow();
        }

                                                          // public
        void TransferRecvWriteCacheHandler::append(char *data, int size)
        {
            // sendAsyncData() and dataReq() cost a lot of time, so we should pack some caches.

            static const int maxWritePacketSize = 1 * 1024 * 1024; // 1meg

            if (m_cacheList.isEmpty() || m_cacheList.back().size() + size > maxWritePacketSize)
            {
                m_cacheList.append(QByteArray());
                delete m_cacheStream;
                m_cacheStream = new QDataStream(&m_cacheList.back(), QIODevice::WriteOnly);
            }

            m_cacheStream->writeRawData(data, size);
        }

                                                          // public
        bool TransferRecvWriteCacheHandler::write(bool force)
        {
            // force == false: return without doing anything when the whole cache size is smaller than maxWritePacketSize

            if (m_cacheList.isEmpty() || !m_transferJob || !m_writeReady || !m_writeAsyncMode)
            {
                return false;
            }

            if (!force && m_cacheList.count() < 2)
            {
                return false;
            }

            // do write
            m_writeReady = false;

            m_transferJob->sendAsyncData(m_cacheList.front());
            //qCDebug(KONVERSATION_LOG) << "wrote " << m_cacheList.front().size() << " bytes.";
            m_cacheList.pop_front();

            return true;
        }

        void TransferRecvWriteCacheHandler::close()    // public
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            write(true);                                // write once if kio is ready to write
            m_transferJob->setAsyncDataEnabled(m_writeAsyncMode = false);
            qCDebug(KONVERSATION_LOG) << "switched to synchronized mode.";
            qCDebug(KONVERSATION_LOG) << "flushing... (remaining caches: " << m_cacheList.count() << ")";
        }

        void TransferRecvWriteCacheHandler::closeNow() // public
        {
            write(true);                                // flush
            if (m_transferJob)
            {
                m_transferJob->kill();
                m_transferJob = nullptr;
            }
            m_cacheList.clear();
            delete m_cacheStream;
            m_cacheStream = nullptr;
        }

        void TransferRecvWriteCacheHandler::slotKIODataReq(KIO::Job *job, QByteArray &data)
        {
            Q_UNUSED(job)

            // We are in writeAsyncMode if there is more data to be read in from dcc
            if (m_writeAsyncMode)
            {
                m_writeReady = true;
            }
            else
            {
                // No more data left to read from incoming dcctransfer
                if (!m_cacheList.isEmpty())
                {
                    // once we write everything in cache, the file is complete.
                    // This function will be called once more after this last data is written.
                    data = m_cacheList.front();
                    qCDebug(KONVERSATION_LOG) << "will write " << m_cacheList.front().size() << " bytes.";
                    m_cacheList.pop_front();
                }
                else
                {
                    // finally, no data left to write or read.
                    qCDebug(KONVERSATION_LOG) << "flushing done.";
                    m_transferJob = nullptr;
                    Q_EMIT done();                          // -> TransferRecv::slotLocalWriteDone()
                }
            }
        }

        void TransferRecvWriteCacheHandler::slotKIOResult(KJob *job)
        {
            Q_ASSERT(m_transferJob);

            disconnect(m_transferJob, nullptr, nullptr, nullptr);
            m_transferJob = nullptr;

            if (job->error())
            {
                QString errorString = job->errorString();
                closeNow();
                Q_EMIT gotError(errorString);             // -> TransferRecv::slotLocalGotWriteError()
            }
        }

    }
}

#include "moc_transferrecv.cpp"
