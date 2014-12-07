/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005-2007 Joris Guisson <joris.guisson@gmail.com>
  Copyright (C) 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "upnpmcastsocket.h"

#include <QStringList>

#include <QUrl>
#include <QDebug>


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
            QObject::connect(this, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

            for (quint32 i = 0;i < 10;i++)
            {
                if (!bind(1900 + i,QUdpSocket::ShareAddress))
                    qDebug() << "Cannot bind to UDP port 1900 : " << errorString() << endl;
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
            qDebug() << "Trying to find UPnP devices on the local network" << endl;

            // send a HTTP M-SEARCH message to 239.255.255.250:1900
            const char* data = "M-SEARCH * HTTP/1.1\r\n"
                    "HOST: 239.255.255.250:1900\r\n"
                    "ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
                    "MAN:\"ssdp:discover\"\r\n"
                    "MX:3\r\n"
                    "\r\n\0";

            writeDatagram(data,strlen(data),QHostAddress("239.255.255.250"),1900);
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
                    emit discovered(r);
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
                QObject::connect(r,SIGNAL(xmlFileDownloaded(UPnPRouter*,bool)),
                        this,SLOT(onXmlFileDownloaded(UPnPRouter*,bool)));

                // download it's xml file
                r->downloadXMLFile();
                pending_routers.insert(r);
            }
        }

        UPnPRouter* UPnPMCastSocket::parseResponse(const QByteArray & arr)
        {
            QStringList lines = QString::fromLatin1(arr).split("\r\n");
            QString server;
            QUrl location;
            QString uuid;

            // first read first line and see if contains a HTTP 200 OK message
            QString line = lines.first();
            if (line.contains("HTTP"))
            {
                // it is either a 200 OK or a NOTIFY
                if (!line.contains("NOTIFY") && !line.contains("200 OK"))
                    return 0;
            }
            else
                return 0;

            // quick check that the response being parsed is valid
            bool validDevice = false;
            for (int idx = 0;idx < lines.count() && !validDevice; idx++)
            {
                line = lines[idx];
                if ((line.contains("ST:") || line.contains("NT:")) && line.contains("InternetGatewayDevice"))
                {
                    validDevice = true;
                }
            }
            if (!validDevice)
            {
            //	qDebug() << "Not a valid Internet Gateway Device" << endl;
                return 0;
            }

            // read all lines and try to find the server and location fields
            for (int i = 1;i < lines.count();i++)
            {
                line = lines[i];
                if (line.startsWith(QLatin1String("Location"), Qt::CaseInsensitive))
                {
                    location = QUrl(line.mid(line.indexOf(':') + 1).trimmed());
                    if (!location.isValid())
                        return 0;
                }
                else if (line.startsWith(QLatin1String("Server"), Qt::CaseInsensitive))
                {
                    server = line.mid(line.indexOf(':') + 1).trimmed();
                    if (server.length() == 0)
                        return 0;

                }
                else if (line.contains("USN", Qt::CaseInsensitive) && line.contains("uuid", Qt::CaseInsensitive))
                {
                    uuid = line.split(':').at(2);

                    if (uuid.length() == 0)
                        return 0;
                }
            }

            if (routers.contains(uuid))
            {
                return 0;
            }
            else
            {
                qDebug() << "Detected IGD " << server << "UUID" << uuid << endl;
                // everything OK, make a new UPnPRouter
                return new UPnPRouter(server,location,uuid);
            }
        }

        void UPnPMCastSocket::onError(QAbstractSocket::SocketError )
        {
            qDebug() << "UPnPMCastSocket Error : " << errorString() << endl;
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
                qDebug() << "Failed to join multicast group 239.255.255.250" << endl;
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
                qDebug() << "Failed to leave multicast group 239.255.255.250" << endl;
            }
        }
    }
}




