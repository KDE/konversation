/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2007 Shintaro Matsuoka <shin@shoegazed.org>
    SPDX-FileCopyrightText: 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef DCCCOMMON_H
#define DCCCOMMON_H

#include <QString>

class QObject;
class QTcpServer;

class Server;

namespace Konversation
{
    namespace DCC
    {
        class DccCommon
        {
            public:
                // converts an IP text like "127.0.0.1" to a number.
                static QString textIpToNumericalIp( const QString& ipString );

                // converts a numerical IP text like "12345678" to a normal IP text.
                static QString numericalIpToTextIp( const QString& numericalIp );

                // returns the self IP following the setting.
                static QString getOwnIp( Server* server = nullptr );

                static QString ipv6FallbackAddress(const QString& address);

                // creates an instance of QTcpServer following the DCC settings
                static QTcpServer* createServerSocketAndListen( QObject* parent = nullptr, QString* failedReason = nullptr, int minPort = 0, int maxPort = 0 );

            private:
                DccCommon();
        };
    }
}

#endif  // DCCCOMMON_H
