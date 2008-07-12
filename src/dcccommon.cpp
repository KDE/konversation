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
#include "channel.h"
#include "preferences.h"
#include "server.h"

#include <arpa/inet.h>

#include <qhostaddress.h>

#include <klocale.h>
#include <kresolver.h>
#include <kserversocket.h>


QString DccCommon::textIpToNumericalIp( const QString& ipString )
{
    QHostAddress ip;
    ip.setAddress( ipString );

    return QString::number( ip.ip4Addr() );
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
    int methodId = Preferences::dccMethodToGetOwnIp();

    if ( methodId == 1 && server )
    {
        // by the WELCOME message or the USERHOST message from the server
        ownIp = server->getOwnIpByServerMessage();
    }
    else if ( methodId == 2 && !Preferences::dccSpecificOwnIp().isEmpty() )
    {
        // manual
        KNetwork::KResolverResults res = KNetwork::KResolver::resolve(Preferences::dccSpecificOwnIp(), "");
        if(res.error() == KResolver::NoError && res.size() > 0)
        {
            ownIp = res.first().address().nodeName();
        }
    }

    // fallback or methodId == 3 (network interface)
    if ( ownIp.isEmpty() && server )
    {
        ownIp = server->getOwnIpByNetworkInterface();
    }

    kdDebug() << "DccCommon::getOwnIp(): " << ownIp << endl;
    return ownIp;
}

KNetwork::KServerSocket* DccCommon::createServerSocketAndListen( QObject* parent, QString* failedReason, int minPort, int maxPort )
{
    KNetwork::KServerSocket* socket = new KNetwork::KServerSocket( parent );
    socket->setFamily( KNetwork::KResolver::InetFamily );

    if ( minPort > 0 && maxPort >= minPort )  // ports are configured manually
    {
        // set port
        bool found = false;                       // whether succeeded to set port
        for ( int port = minPort; port <= maxPort ; ++port )
        {
            socket->setAddress( QString::number( port ) );
            bool success = socket->listen();
            if ( ( found = ( success && socket->error() == KNetwork::KSocketBase::NoError ) ) )
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
        socket->setAddress( "0" );
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

int DccCommon::getServerSocketPort( KNetwork::KServerSocket* serverSocket )
{
    KNetwork::KSocketAddress ipAddr = serverSocket->localAddress();
    const struct sockaddr_in* socketAddress = (sockaddr_in*)ipAddr.address();
    return ntohs( socketAddress->sin_port );
}
