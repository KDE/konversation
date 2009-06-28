/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include "dcccommon.h"
#include "preferences.h"
#include "server.h"

#include <arpa/inet.h>

#include <QHostAddress>
#include <QTcpServer>

namespace Konversation
{
    namespace DCC
    {
        //TODO: IPv6 support
        QString DccCommon::textIpToNumericalIp( const QString& ipString )
        {
            QHostAddress ip;
            ip.setAddress( ipString );
            switch (ip.protocol())
            {
            case QAbstractSocket::IPv4Protocol:
                return QString::number( ip.toIPv4Address() );

            case QAbstractSocket::IPv6Protocol:
                kDebug() << "TODO: implement me for ipv6";
                return "";

            default:
                kDebug() << "unspported protocol: " << ipString;
                return "";
            }
        }

        QString DccCommon::numericalIpToTextIp( const QString& numericalIp )
        {
            QHostAddress ip;
            ip.setAddress( numericalIp.toULong() );

            return ip.toString();
        }

        QString DccCommon::getOwnIp( Server* server )
        {
            QString ownIp;
            int methodId = Preferences::self()->dccMethodToGetOwnIp();

            if ( methodId == 1 && server )
            {
                // by the WELCOME message or the USERHOST message from the server
                ownIp = server->getOwnIpByServerMessage();
            }
            else if ( methodId == 2 && !Preferences::self()->dccSpecificOwnIp().isEmpty() )
            {
                // manual
                QHostInfo res = QHostInfo::fromName(Preferences::self()->dccSpecificOwnIp());
                if(res.error() == QHostInfo::NoError && !res.addresses().isEmpty())
                {
                    ownIp = res.addresses().first().toString();
                }
            }

            // fallback or methodId == 0 (network interface)
            if ( ownIp.isEmpty() && server )
            {
                ownIp = server->getOwnIpByNetworkInterface();
            }

            kDebug() << ownIp;
            return ownIp;
        }

        QTcpServer* DccCommon::createServerSocketAndListen( QObject* parent, QString* failedReason, int minPort, int maxPort )
        {
            QTcpServer* socket = new QTcpServer( parent );

            if ( minPort > 0 && maxPort >= minPort )  // ports are configured manually
            {
                // set port
                bool found = false;                       // whether succeeded to set port
                for ( int port = minPort; port <= maxPort ; ++port )
                {
                    bool success = socket->listen( QHostAddress::Any, port );
                    if ( ( found = ( success && socket->isListening() ) ) )
                        break;
                    socket->close();
                }
                if ( !found )
                {
                    if ( failedReason )
                        *failedReason = i18n( "No vacant port" );
                    delete socket;
                    return 0;
                }
            }
            else
            {
                // Let the operating system choose a port
                if ( !socket->listen() )
                {
                    if ( failedReason )
                        *failedReason = i18n( "Could not open a socket" );
                    delete socket;
                    return 0;
                }
            }

            return socket;
        }

    }
}
