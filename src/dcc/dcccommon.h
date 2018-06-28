/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef DCCCOMMON_H
#define DCCCOMMON_H

#include <QString>
#include <QHostAddress>

#include "upnprouter.h"
#include "preferences.h"
#include "transfermanager.h"
#include "server.h"
#include "application.h"

class QObject;
class QTcpServer;

namespace Konversation
{
    namespace DCC
    {
        template<typename T, typename MethodPtr>
        void callMethodIfDccUpnp(T thisPointer, MethodPtr methodPointer, Server *server, quint16 ownPort)
        {
            using namespace Konversation::UPnP;

            // small helper lambda to call a method if we are in dccUPnP mode.
            if (!Preferences::self()->dccUPnP())
            {
                (thisPointer->*methodPointer)(false, 0);
            }
            UPnPRouter *router = Application::instance()->getDccTransferManager()->getUPnPRouter();
            if (router && router->forward(QHostAddress(server->getOwnIpByNetworkInterface()), ownPort, QAbstractSocket::TcpSocket))
            {
                QObject::connect(router, &UPnPRouter::forwardComplete, thisPointer, methodPointer);
            }
            else
            {
                (thisPointer->*methodPointer)(true, 0); // On error try anyways
            }
        }

        class DccCommon
        {
            public:
                // converts an IP text like "127.0.0.1" to a number.
                static QString textIpToNumericalIp( const QString& ipString );

                // converts a numerical IP text like "12345678" to a normal IP text.
                static QString numericalIpToTextIp( const QString& numericalIp );

                // returns the self IP following the setting.
                static QString getOwnIp(Server* server = nullptr);

                static QString ipv6FallbackAddress(const QString& address);

                // creates an instance of QTcpServer following the DCC settings
                static QTcpServer* createServerSocketAndListen( QObject* parent = nullptr, QString* failedReason = nullptr, int minPort = 0, int maxPort = 0 );

            private:
                DccCommon() = delete;
        };
    }
}

#endif  // DCCCOMMON_H
