/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002-2004 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2004-2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2004, 2005 John Tapsell <john@geola.co.uk>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "transfer.h"

#include "application.h"
#include "connectionmanager.h"
#include "notificationhandler.h"
#include "preferences.h"
#include "konversation_log.h"

#include <config-konversation.h>

#include <kio_version.h>
#include <KIO/JobUiDelegateFactory>

#include <KIO/OpenUrlJob>

#include <QFileInfo>

namespace Konversation
{
    namespace DCC
    {
        Transfer::Transfer(Type dccType, QObject *parent)
            : QObject(parent)
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;

            m_type = dccType;

            m_status = Configuring;

            m_ownPort = 0;
            m_fileSize = 0;
            m_resumed = false;
            m_reverse = false;
            m_connectionId = -1;  // Not configured
            m_timeLeft = Transfer::NotInTransfer;
            m_transferringPosition = 0;
            m_transferStartPosition = 0;
            m_averageSpeed = 0.0;
            m_currentSpeed = 0.0;

            m_bufferSize = Preferences::self()->dccBufferSize();
            m_buffer = new char[m_bufferSize];

            connect(&m_loggerTimer, &QTimer::timeout, this, &Transfer::logTransfer);

            m_timeOffer = QDateTime::currentDateTime();
        }

        Transfer::~Transfer()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
        }

        void Transfer::setConnectionId(int id)
        {
            if (getStatus() == Configuring || getStatus() == Queued)
            {
                m_connectionId = id;
            }
        }

        void Transfer::setPartnerNick(const QString &nick)
        {
            if (getStatus() == Configuring || getStatus() == Queued)
            {
                m_partnerNick = nick;
            }
        }

        bool Transfer::queue()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            if (getStatus() != Configuring)
            {
                return false;
            }

            if (m_fileName.isEmpty())
            {
                return false;
            }

            if (m_connectionId == -1 || m_partnerNick.isEmpty())
            {
                return false;
            }

            setStatus(Queued);
            return true;
        }

        void Transfer::runFile()
        {
            if (getType() == Transfer::Send || getStatus() == Transfer::Done)
            {
                auto *job = new KIO::OpenUrlJob(getFileURL());
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
                job->setRunExecutables(false);
                job->start();
            }
        }

        void Transfer::startTransferLogger()
        {
            m_timeTransferStarted = QDateTime::currentDateTime();
            m_loggerBaseTime.start();
            m_loggerTimer.start(100);
        }

        void Transfer::finishTransferLogger()
        {
            if (m_timeTransferFinished.isNull())
            {
                m_timeTransferFinished = QDateTime::currentDateTime();
            }
            m_loggerTimer.stop();
            updateTransferMeters();
        }

        // called by m_loggerTimer
        void Transfer::logTransfer()
        {
            m_transferLogTime.append(m_loggerBaseTime.elapsed());
            m_transferLogPosition.append(m_transferringPosition);
            updateTransferMeters();
        }

        void Transfer::cleanUp()
        {
            qCDebug(KONVERSATION_LOG) << __FUNCTION__;
            delete[] m_buffer;
            m_buffer = nullptr;
            m_loggerTimer.stop();
        }

        void Transfer::removedFromView()
        {
            Q_EMIT removed(this);
        }

        // just for convenience
        void Transfer::failed(const QString &errorMessage)
        {
            cleanUp();
            Application *konv_app = Application::instance();
            Server *server = konv_app->getConnectionManager()->getServerByConnectionId(m_connectionId);
            if (server)
            {
                qCDebug(KONVERSATION_LOG) << "notification:" << errorMessage;
                konv_app->notificationHandler()->dccError(server->getStatusView(), errorMessage);
            }
            setStatus(Failed, errorMessage);
            Q_EMIT done(this);
        }

        void Transfer::setStatus(Status status, const QString &statusDetail)
        {
            bool changed = (status != m_status);
            Status oldStatus = m_status;
            m_status = status;
            m_statusDetail = statusDetail;
            if (changed)
            {
                Q_EMIT statusChanged(this, m_status, oldStatus);
            }

            if (m_status == Done)
            {
                Application *konv_app = Application::instance();
                Server *server = konv_app->getConnectionManager()->getServerByConnectionId(m_connectionId);
                if (server)
                {
                    qCDebug(KONVERSATION_LOG) << "notification:" << m_fileName;
                    konv_app->notificationHandler()->dccTransferDone(server->getStatusView(), m_fileName, this);
                }
            }
        }

        void Transfer::updateTransferMeters()
        {
            const int timeToCalc = 5;

            if (getStatus() == Transferring)
            {
                // update CurrentSpeed

                // remove too old data
                QList<int>::iterator itTime = m_transferLogTime.begin();
                QList<KIO::fileoffset_t>::iterator itPos = m_transferLogPosition.begin();
                while (itTime != m_transferLogTime.end() && (m_transferLogTime.last() - (*itTime) > timeToCalc * 1000))
                {
                    itTime = m_transferLogTime.erase(itTime);
                    itPos = m_transferLogPosition.erase(itPos);
                }

                // shift the base of the time (m_transferLoggerBaseTime)
                // reason: QTime can't handle a time longer than 24 hours
                int shiftOffset = m_loggerBaseTime.restart();
                itTime = m_transferLogTime.begin();
                for (; itTime != m_transferLogTime.end(); ++itTime)
                {
                    (*itTime) = (*itTime) - shiftOffset;
                }

                // The logTimer is 100ms, as 200ms is below 1sec we get "undefined" speed
                if (m_transferLogTime.count() >= 2 && m_timeTransferStarted.secsTo(QDateTime::currentDateTime()) > 0)
                {
                    // FIXME: precision of average speed is too bad
                    m_averageSpeed = (double)(m_transferringPosition - m_transferStartPosition) / (double)m_timeTransferStarted.secsTo(QDateTime::currentDateTime());
                    m_currentSpeed = (double)(m_transferLogPosition.last() - m_transferLogPosition.front()) / (double)(m_transferLogTime.last() - m_transferLogTime.front()) * 1000;
                }
                else // avoid zero devision
                {
                    m_averageSpeed = Transfer::Calculating;
                    m_currentSpeed = Transfer::Calculating;
                }

                // update the remaining time
                if  (m_transferringPosition == (KIO::fileoffset_t)m_fileSize)
                {
                    m_timeLeft = 0;
                }
                else if (m_currentSpeed <= 0)
                {
                    m_timeLeft = Transfer::InfiniteValue;
                }
                else
                {
                    m_timeLeft = (int)((double)(m_fileSize - m_transferringPosition) / m_currentSpeed);
                }
            }
            else if (m_status >= Done)
            {
                if (m_timeTransferStarted.secsTo(m_timeTransferFinished) > 1)
                {
                    m_averageSpeed = (double)(m_transferringPosition - m_transferStartPosition) / (double)m_timeTransferStarted.secsTo(m_timeTransferFinished);
                }
                else
                {
                    m_averageSpeed = Transfer::InfiniteValue;
                }

                m_currentSpeed = 0;
                if (m_status == Done)
                {
                    m_timeLeft = 0;
                }
                else
                {
                    m_timeLeft = Transfer::NotInTransfer;
                }
            }
            else
            {
                m_averageSpeed = 0;
                m_currentSpeed = 0;
                m_timeLeft = Transfer::NotInTransfer;
            }
        }

        QString Transfer::sanitizeFileName(const QString &fileName)
        {
            QString fileNameTmp = QFileInfo(fileName).fileName();
            if (fileNameTmp.startsWith(QLatin1Char('.'))) {
                fileNameTmp.replace(0, 1, QLatin1Char('_'));         // Don't create hidden files
            }
            if (fileNameTmp.isEmpty())
            {
                fileNameTmp = QStringLiteral("unnamed");
            }
            return fileNameTmp;
        }

        Transfer::Type Transfer::getType() const
        {
            return m_type;
        }

        Transfer::Status Transfer::getStatus() const
        {
            return m_status;
        }

        const QString &Transfer::getStatusDetail() const
        {
            return m_statusDetail;
        }

        QDateTime Transfer::getTimeOffer() const
        {
            return m_timeOffer;
        }

        int Transfer::getConnectionId() const
        {
            return m_connectionId;
        }

        QString Transfer::getOwnIp() const
        {
            return m_ownIp;
        }

        quint16 Transfer::getOwnPort() const
        {
            return m_ownPort;
        }

        QString Transfer::getPartnerNick() const
        {
            return m_partnerNick;
        }

        QString Transfer::getPartnerIp() const
        {
            return m_partnerIp;
        }

        quint16 Transfer::getPartnerPort() const
        {
            return m_partnerPort;
        }

        QString Transfer::getFileName() const
        {
            return m_fileName;
        }

        KIO::filesize_t Transfer::getFileSize() const
        {
            return m_fileSize;
        }

        KIO::fileoffset_t Transfer::getTransferringPosition() const
        {
            return m_transferringPosition;
        }

        KIO::fileoffset_t Transfer::getTransferStartPosition() const
        {
            return m_transferStartPosition;
        }

        QUrl Transfer::getFileURL() const
        {
            return m_fileURL;
        }

        bool Transfer::isResumed() const
        {
            return m_resumed;
        }

        bool Transfer::isReverse() const
        {
            return m_reverse;
        }

        QString Transfer::getReverseToken() const
        {
            return m_reverseToken;
        }

        transferspeed_t Transfer::getAverageSpeed() const
        {
            return m_averageSpeed;
        }

        transferspeed_t Transfer::getCurrentSpeed() const
        {
            return m_currentSpeed;
        }

        int Transfer::getTimeLeft() const
        {
            return m_timeLeft;
        }

        int Transfer::getProgress() const
        {
            if (getFileSize() == 0)
            {
                return 0;
            }
            else
            {
                return (int)(((double)getTransferringPosition() / (double)getFileSize()) * 100.0);
            }
        }

        QDateTime Transfer::getTimeTransferStarted() const
        {
            return m_timeTransferStarted;
        }

        QDateTime Transfer::getTimeTransferFinished() const
        {
            return m_timeTransferFinished;
        }

        QString Transfer::transferFileName(const QString & fileName)
        {
            if (fileName.contains(QLatin1Char(' ')) && !(fileName.startsWith(QLatin1Char('\"')) && fileName.endsWith(QLatin1Char('\"')))) {
                return QLatin1Char('\"') + fileName + QLatin1Char('\"');
            }

            return fileName;
        }

    }
}

#include "moc_transfer.cpp"
