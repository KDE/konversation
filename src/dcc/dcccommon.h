/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef DCCCOMMON_H
#define DCCCOMMON_H

#include <qstring.h>
#include <QTcpServer>

class QObject;

class Server;

class DccCommon
{
    public:
        // converts an IP text like "127.0.0.1" to a number.
        static QString textIpToNumericalIp( const QString& ipString );

        // converts a numerical IP text like "12345678" to a normal IP text.
        static QString numericalIpToTextIp( const QString& numericalIp );

        // returns the self IP following the setting.
        static QString getOwnIp( Server* server = 0 );

        // creates an instance of QTcpServer following the DCC settings
        static QTcpServer* createServerSocketAndListen( QObject* parent = 0, QString* failedReason = 0, int minPort = 0, int maxPort = 0 );

    private:
        DccCommon();
};

#endif  // DCCCOMMON_H
