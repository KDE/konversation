/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "upnpmcastsocket.h"

#include "konversation_log.h"

#include <QStringList>
#include <QUrl>

#ifdef Q_OS_WIN
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#endif


namespace Konversation
{
    namespace UPnP
    {

        UPnPMCastSocket::UPnPMCastSocket() : QUdpSocket ()
        {
            QObject::connect(this, &UPnPMCastSocket::readyRead, this, &UPnPMCastSocket::onReadyRead);
            QObject::connect(this, &UPnPMCastSocket::errorOccurred, this, &UPnPMCastSocket::onError);
            for (quint32 i = 0;i < 10;i++)
            {
                if (!bind(1900 + i,QUdpSocket::ShareAddress))
                    qCDebug(KONVERSATION_LOG) << "Cannot bind to UDP port 1900 : " << errorString();
                else
                    break;
            }

            joinUPnPMCastGroup();
        }


        UPnPMCastSocket::~UPnPMCastSocket()
        {
            qDeleteAll(pending_routers);
            qDeleteAll(routers);
            leaveUPnPMCastGroup();
        }

        void UPnPMCastSocket::discover()
        {
            qCDebug(KONVERSATION_LOG) << "Trying to find UPnP devices on the local network";

            // send a HTTP M-SEARCH message to 239.255.255.250:1900
            const char* data = "M-SEARCH * HTTP/1.1\r\n"
                    "HOST: 239.255.255.250:1900\r\n"
                    "ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
                    "MAN:\"ssdp:discover\"\r\n"
                    "MX:3\r\n"
                    "\r\n\0";

            writeDatagram(data,strlen(data),QHostAddress(QStringLiteral("239.255.255.250")),1900);
        }

        void UPnPMCastSocket::onXmlFileDownloaded(UPnPRouter* r,bool success)
        {
            pending_routers.remove(r);
            if (!success)
            {
                // we couldn't download and parse the XML file so
                // get rid of it
                r->deleteLater();
            }
            else
            {
                // add it to the list and emit the signal
                if (!routers.contains(r->getUUID()))
                {
                    routers.insert(r->getUUID(),r);
                    Q_EMIT discovered(r);
                }
                else
                {
                    r->deleteLater();
                }
            }
        }

        void UPnPMCastSocket::onReadyRead()
        {
            QByteArray data(pendingDatagramSize(),0);
            if (readDatagram(data.data(),pendingDatagramSize()) == -1)
                return;

            // try to make a router of it
            UPnPRouter* r = parseResponse(data);
            if (r)
            {
                QObject::connect(r,&UPnPRouter::xmlFileDownloaded,
                        this,&UPnPMCastSocket::onXmlFileDownloaded);

                // download it's xml file
                r->downloadXMLFile();
                pending_routers.insert(r);
            }
        }

        UPnPRouter* UPnPMCastSocket::parseResponse(const QByteArray & arr)
        {
            QStringList lines = QString::fromLatin1(arr).split(QStringLiteral("\r\n"));
            QString server;
            QUrl location;
            QString uuid;

            // first read first line and see if contains a HTTP 200 OK message
            QString line = lines.first();
            if (line.contains(QLatin1String("HTTP")))
            {
                // it is either a 200 OK or a NOTIFY
                if (!line.contains(QLatin1String("NOTIFY")) && !line.contains(QLatin1String("200 OK")))
                    return nullptr;
            }
            else
                return nullptr;

            // quick check that the response being parsed is valid
            bool validDevice = false;
            for (int idx = 0;idx < lines.count() && !validDevice; idx++)
            {
                line = lines[idx];
                if ((line.contains(QLatin1String("ST:")) || line.contains(QLatin1String("NT:"))) && line.contains(QLatin1String("InternetGatewayDevice")))
                {
                    validDevice = true;
                }
            }
            if (!validDevice)
            {
            //	qCDebug(KONVERSATION_LOG) << "Not a valid Internet Gateway Device";
                return nullptr;
            }

            // read all lines and try to find the server and location fields
            for (int i = 1;i < lines.count();i++)
            {
                line = lines[i];
                if (line.startsWith(QLatin1String("Location"), Qt::CaseInsensitive))
                {
                    location = QUrl(line.mid(line.indexOf(QLatin1Char(':')) + 1).trimmed());
                    if (!location.isValid())
                        return nullptr;
                }
                else if (line.startsWith(QLatin1String("Server"), Qt::CaseInsensitive))
                {
                    server = line.mid(line.indexOf(QLatin1Char(':')) + 1).trimmed();
                    if (server.isEmpty())
                        return nullptr;

                }
                else if (line.contains(QLatin1String("USN"), Qt::CaseInsensitive) && line.contains(QLatin1String("uuid"), Qt::CaseInsensitive))
                {
                    uuid = line.split(QLatin1Char(':')).at(2);

                    if (uuid.isEmpty())
                        return nullptr;
                }
            }

            if (routers.contains(uuid))
            {
                return nullptr;
            }
            else
            {
                qCDebug(KONVERSATION_LOG) << "Detected IGD " << server << "UUID" << uuid;
                // everything OK, make a new UPnPRouter
                return new UPnPRouter(server,location,uuid);
            }
        }

        void UPnPMCastSocket::onError(QAbstractSocket::SocketError )
        {
            qCDebug(KONVERSATION_LOG) << "UPnPMCastSocket Error : " << errorString();
        }

        void UPnPMCastSocket::joinUPnPMCastGroup()
        {
            int fd = socketDescriptor();
            struct ip_mreq mreq;

            memset(&mreq,0,sizeof(struct ip_mreq));

            mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    #ifndef Q_OS_WIN
            if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
    #else
            if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
    #endif
            {
                qCDebug(KONVERSATION_LOG) << "Failed to join multicast group 239.255.255.250";
            }
        }

        void UPnPMCastSocket::leaveUPnPMCastGroup()
        {
            int fd = socketDescriptor();
            struct ip_mreq mreq;

            memset(&mreq,0,sizeof(struct ip_mreq));

            mreq.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    #ifndef Q_OS_WIN
            if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,&mreq,sizeof(struct ip_mreq)) < 0)
    #else
            if (setsockopt(fd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mreq,sizeof(struct ip_mreq)) < 0)
    #endif
            {
                qCDebug(KONVERSATION_LOG) << "Failed to leave multicast group 239.255.255.250";
            }
        }
    }
}

#include "moc_upnpmcastsocket.cpp"
