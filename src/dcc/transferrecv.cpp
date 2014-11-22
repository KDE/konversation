/*
  receive a file on DCC protocol
  begin:     Mit Aug 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/
/*
  Copyright (C) 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2004,2005 John Tapsell <john@geola.co.uk>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/
/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "transferrecv.h"
#include "dcccommon.h"
#include "transfermanager.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "upnprouter.h"

#include <QDateTime>
#include <QTcpSocket>
#include <QTcpServer>
#include <QFileInfo>

#include <KUser>
#include <KAuthorized>
#include <KIO/Job>

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
            qDebug();

            m_serverSocket = 0;
            m_recvSocket = 0;
            m_writeCacheHandler = 0;

            m_connectionTimer = new QTimer(this);
            m_connectionTimer->setSingleShot(true);
            connect(m_connectionTimer, &QTimer::timeout, this, &TransferRecv::connectionTimeout);
            //timer hasn't started yet.  qtimer will be deleted automatically when 'this' object is deleted
        }

        TransferRecv::~TransferRecv()
        {
            qDebug();
            cleanUp();
        }

        void TransferRecv::cleanUp()
        {
            qDebug();

            stopConnectionTimer();
            disconnect(m_connectionTimer, 0, 0, 0);

            finishTransferLogger();
            if (m_serverSocket)
            {
                m_serverSocket->close();
                m_serverSocket = 0;

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
                disconnect(m_recvSocket, 0, 0, 0);
                m_recvSocket->close();
                m_recvSocket = 0;                         // the instance will be deleted automatically by its parent
            }
            if (m_writeCacheHandler)
            {
                m_writeCacheHandler->closeNow();
                m_writeCacheHandler->deleteLater();
                m_writeCacheHandler = 0;
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
            qDebug();

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

            if (!KAuthorized::authorizeKAction("allow_downloading"))
            {
                //note we have this after the initialisations so that item looks okay
                //Do not have the rights to send the file.  Shouldn't have gotten this far anyway
                failed(i18n("The admin has restricted the right to receive files"));
                return false;
            }

            // check if the sender IP is valid
            if (m_partnerIp == "0.0.0.0")
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
                m_fileName = "unnamed_file_" + QDateTime::currentDateTime().toString(Qt::ISODate).remove(':');
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
                    m_fileURL.setPath(m_fileURL.path() + QDir::separator() + m_partnerNick + '.' + fileNameSanitized);
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
            qDebug();

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
            emit done(this);
        }

        void TransferRecv::start()                     // public slot
        {
            qDebug() << "[BEGIN]";

            if (getStatus() != Queued)
            {
                return;
            }

            setStatus(Preparing);

            prepareLocalKio(false, false);

            qDebug() << "[END]";
        }

        void TransferRecv::prepareLocalKio(bool overwrite, bool resume, KIO::fileoffset_t startPosition)
        {
            qDebug()
                << "URL: " << m_fileURL << endl
                << "Overwrite: " << overwrite << endl
                << "Resume: " << resume << " (Position: " << startPosition << ")";

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
                qDebug() << "KIO::put() returned NULL. what happened?";
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

            QList<QUrl>::ConstIterator it;
            for (it=dirList.constBegin(); it != dirList.constEnd(); ++it)
            {
                KIO::StatJob* statJob = KIO::stat(*it, KIO::StatJob::SourceSide, 0);
                statJob->exec();
                if (statJob->error())
                {
                    KIO::MkdirJob* job = KIO::mkdir(*it, -1);
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
            qDebug() << "[BEGIN]" << endl
                << "size: " << size;

            KIO::TransferJob* transferJob = dynamic_cast<KIO::TransferJob*>(job);
            if (!transferJob)
            {
                qDebug() << "not a TransferJob? returning";
                return;
            }

            if (size != 0)
            {
                disconnect(transferJob, 0, 0, 0);
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

            qDebug() << "[END]";
        }

        void TransferRecv::slotLocalGotResult(KJob *job)
        {
            qDebug() << "[BEGIN]";

            KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>(job);
            disconnect(transferJob, 0, 0, 0);

            switch (transferJob->error())
            {
                case 0:                                   // no error
                    qDebug() << "job->error() returned 0." << endl
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

            qDebug() << "[END]";
        }

        void TransferRecv::slotLocalReady(KIO::Job *job)
        {
            qDebug();

            KIO::TransferJob* transferJob = static_cast<KIO::TransferJob*>(job);

            disconnect(transferJob, 0, 0, 0);           // WriteCacheHandler will control the job after this

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

            qDebug();

            if (Preferences::self()->dccUPnP() && this->sender())
            {
                if (port != m_ownPort) return; // Somebody elses forward succeeded

                disconnect (this->sender(), SIGNAL(forwardComplete(bool,quint16)), this, SLOT(sendRequest(bool,quint16)));

                if (error)
                {
                    server->appendMessageToFrontmost(i18nc("Universal Plug and Play", "UPnP"), i18n("Failed to forward port %1. Sending DCC request to remote user regardless.", QString::number(m_ownPort)), false);
                }
            }

            setStatus(WaitingRemote, i18n("Waiting for connection"));

            server->dccReverseSendAck(m_partnerNick, transferFileName(m_fileName), DccCommon::textIpToNumericalIp(m_ownIp), m_ownPort, m_fileSize, m_reverseToken);
        }

        void TransferRecv::requestResume()
        {
            qDebug();

            setStatus(WaitingRemote, i18n("Waiting for remote host's acceptance"));

            startConnectionTimer(30);

            qDebug() << "Requesting resume for " << m_partnerNick << " file " << m_fileName << " partner " << m_partnerPort;

            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (!server)
            {
                qDebug() << "Could not retrieve the instance of Server. Connection id: " << m_connectionId;
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
            qDebug() << "Position:" << position;

            stopConnectionTimer();

            if ((quint64)m_transferringPosition != position)
            {
                qDebug() << "remote responsed an unexpected position"<< endl
                    << "expected: " << m_transferringPosition << endl
                    << "remote response: " << position;
                failed(i18n("Unexpected response from remote host"));
                return;
            }

            connectWithSender();
        }

        void TransferRecv::connectToSendServer()
        {
            qDebug();

            // connect to sender

            setStatus(Connecting);

            startConnectionTimer(30);

            m_recvSocket = new QTcpSocket(this);

            connect(m_recvSocket, &QTcpSocket::connected, this, &TransferRecv::startReceiving);
            connect(m_recvSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &TransferRecv::connectionFailed);

            qDebug() << "Attempting to connect to " << m_partnerIp << ":" << m_partnerPort;

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

            connect(m_recvSocket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, &TransferRecv::connectionFailed);

            // we don't need ServerSocket anymore
            m_serverSocket->close();
            m_serverSocket = 0; // Will be deleted by parent

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
            qDebug();
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
            qDebug() << "Code = " << errorCode << ", string = " << m_recvSocket->errorString();
            failed(m_recvSocket->errorString());
        }

        void TransferRecv::readData()                  // slot
        {
            //qDebug();
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
            //qDebug() << m_transferringPosition << "/" << (KIO::fileoffset_t)m_fileSize;

            //It is bound to be 32bit according to dcc specs, -> 4GB limit.
            //But luckily no client ever reads this value,
            //except for old mIRC versions, but they couldn't send or receive files over 4GB anyway.
            //Note: The resume and filesize are set via dcc send command and can be over 4GB

            quint32 pos = intel((quint32)m_transferringPosition);

            m_recvSocket->write((char*)&pos, 4);
            if (m_transferringPosition == (KIO::fileoffset_t)m_fileSize)
            {
                qDebug() << "Sent final ACK.";
                disconnect(m_recvSocket, 0, 0, 0);
                m_writeCacheHandler->close();             // WriteCacheHandler will send the signal done()
            }
            else if (m_transferringPosition > (KIO::fileoffset_t)m_fileSize)
            {
                qDebug() << "The remote host sent larger data than expected: " << m_transferringPosition;
                failed(i18n("Transfer error"));
            }
        }

        void TransferRecv::slotLocalWriteDone()        // <-WriteCacheHandler::done()
        {
            qDebug();
            cleanUp();
            setStatus(Done);
            emit done(this);
        }

                                                          // <- WriteCacheHandler::gotError()
        void TransferRecv::slotLocalGotWriteError(const QString &errorString)
        {
            qDebug();
            failed(i18n("KIO error: %1", errorString));
        }

        void TransferRecv::startConnectionTimer(int secs)
        {
            qDebug();
            m_connectionTimer->start(secs * 1000);
        }

        void TransferRecv::stopConnectionTimer()
        {
            if (m_connectionTimer->isActive())
            {
                m_connectionTimer->stop();
                qDebug();
            }
        }

        void TransferRecv::connectionTimeout()         // slot
        {
            qDebug();
            failed(i18n("Timed out"));
        }

        // WriteCacheHandler

        TransferRecvWriteCacheHandler::TransferRecvWriteCacheHandler(KIO::TransferJob *transferJob)
            : m_transferJob(transferJob)
        {
            m_writeReady = true;
            m_cacheStream = 0;

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
            //qDebug() << "wrote " << m_cacheList.front().size() << " bytes.";
            m_cacheList.pop_front();

            return true;
        }

        void TransferRecvWriteCacheHandler::close()    // public
        {
            qDebug();
            write(true);                                // write once if kio is ready to write
            m_transferJob->setAsyncDataEnabled(m_writeAsyncMode = false);
            qDebug() << "switched to synchronized mode.";
            qDebug() << "flushing... (remaining caches: " << m_cacheList.count() << ")";
        }

        void TransferRecvWriteCacheHandler::closeNow() // public
        {
            write(true);                                // flush
            if (m_transferJob)
            {
                m_transferJob->kill();
                m_transferJob = 0;
            }
            m_cacheList.clear();
            delete m_cacheStream;
            m_cacheStream = 0;
        }

        void TransferRecvWriteCacheHandler::slotKIODataReq(KIO::Job *job, QByteArray &data)
        {
            Q_UNUSED(job);

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
                    qDebug() << "will write " << m_cacheList.front().size() << " bytes.";
                    m_cacheList.pop_front();
                }
                else
                {
                    // finally, no data left to write or read.
                    qDebug() << "flushing done.";
                    m_transferJob = 0;
                    emit done();                          // -> TransferRecv::slotLocalWriteDone()
                }
            }
        }

        void TransferRecvWriteCacheHandler::slotKIOResult(KJob *job)
        {
            Q_ASSERT(m_transferJob);

            disconnect(m_transferJob, 0, 0, 0);
            m_transferJob = 0;

            if (job->error())
            {
                QString errorString = job->errorString();
                closeNow();
                emit gotError(errorString);             // -> TransferRecv::slotLocalGotWriteError()
            }
        }

    }
}


