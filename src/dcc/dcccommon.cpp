/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "dcccommon.h"

#include "preferences.h"
#include "server.h"
#include "konversation_log.h"

#include <config-konversation.h>

#include <cstdlib>
#include <sys/types.h>
#ifndef Q_OS_WIN
#   include <netinet/in.h>
#   include <sys/socket.h>
#   include <net/if.h>
#   include <sys/ioctl.h>
#   if HAVE_STROPTS_H
#       include <stropts.h>
#   endif
#   include <arpa/inet.h>
#endif

#include <QHostAddress>
#include <QTcpServer>


namespace Konversation
{
    namespace DCC
    {
        //TODO: IPv6 support, CHECK ME
        QString DccCommon::textIpToNumericalIp( const QString& ipString )
        {
            QHostAddress ip;
            ip.setAddress( ipString );
            switch (ip.protocol())
            {
            case QAbstractSocket::IPv4Protocol:
                return QString::number( ip.toIPv4Address() );

            case QAbstractSocket::IPv6Protocol:
            //ipv6 is not numerical, it is just normal text, "0:c00:0:0:1f::" for example
                return ip.toString();

            default:
                qCDebug(KONVERSATION_LOG) << "unsupported protocol: " << ipString;
                return QString();
            }
        }

        QString DccCommon::numericalIpToTextIp( const QString& numericalIp )
        {
            QHostAddress ip;

            //Only IPV6 can contain ':'
            if (numericalIp.contains(QLatin1Char(':'))) {
                return numericalIp;
            }
            //ipv4 comes as numericalip
            else
            {
                ip.setAddress( numericalIp.toULong() );
            }

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

            qCDebug(KONVERSATION_LOG) << ownIp;
            return ownIp;
        }

        QString DccCommon::ipv6FallbackAddress(const QString& address)
        {
            QString fallbackIp = address;
            QHostAddress ip(address);
            if (ip.protocol() == QAbstractSocket::IPv6Protocol)
            {
#ifndef Q_OS_WIN
                /* This is fucking ugly but there is no KDE way to do this yet :| -cartman */
                struct ifreq ifr;
                const QByteArray addressBa = Preferences::self()->dccIPv4FallbackIface().toLatin1();
                const char* address = addressBa.constData();
                int sock = socket(AF_INET, SOCK_DGRAM, 0);
                strncpy(ifr.ifr_name, address, IF_NAMESIZE - 1);
                ifr.ifr_name[IF_NAMESIZE - 1] = '\0';
                ifr.ifr_addr.sa_family = AF_INET;

                if (ioctl( sock, SIOCGIFADDR, &ifr ) >= 0)
                {
                    struct sockaddr_in sock;
                    memcpy(&sock, &ifr.ifr_addr, sizeof(ifr.ifr_addr));
                    fallbackIp = QString::fromLatin1(inet_ntoa(sock.sin_addr));
                }
                qCDebug(KONVERSATION_LOG) << "Falling back to IPv4 address " << fallbackIp;
#else
                qCDebug(KONVERSATION_LOG) << "TODO: implement ipv6 fallback";
#endif
            }
            return fallbackIp;
        }

        QTcpServer* DccCommon::createServerSocketAndListen( QObject* parent, QString* failedReason, int minPort, int maxPort )
        {
            auto* socket = new QTcpServer( parent );

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
                    return nullptr;
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
                    return nullptr;
                }
            }

            return socket;
        }
    }
}
