/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#ifndef KTUPNPMCASTSOCKET_H
#define KTUPNPMCASTSOCKET_H

#include "upnprouter.h"

#include <QSet>
#include <QUdpSocket>


namespace Konversation
{
    namespace UPnP
    {
        class UPnPRouter;

        /**
        * @author Joris Guisson
        *
        * Socket used to discover UPnP devices. This class will keep track
        * of all discovered devices.
        */
        class UPnPMCastSocket : public QUdpSocket
        {
            Q_OBJECT
        public:
            UPnPMCastSocket();
            ~UPnPMCastSocket() override;

            /// Get the number of routers discovered
            quint32 getNumDevicesDiscovered() const {return routers.count();}

            /// Find a router using it's server name
            UPnPRouter* findDevice(const QString & name) const {return routers.find(name).value();}

        public Q_SLOTS:
            /**
            * Try to discover a UPnP device on the network.
            * A signal will be emitted when a device is found.
            */
            void discover();

        public:
            UPnPRouter* parseResponse(const QByteArray & arr);

        Q_SIGNALS:
            /**
            * Emitted when a router or Internet gateway device is detected.
            * @param router The router
            */
            void discovered(Konversation::UPnP::UPnPRouter* router);

        private Q_SLOTS:
            void onReadyRead();
            void onError(QAbstractSocket::SocketError err);
            void onXmlFileDownloaded(UPnPRouter* r,bool success);

        private:
            void joinUPnPMCastGroup();
            void leaveUPnPMCastGroup();

        private:
            QHash<QString,UPnPRouter*> routers;
            QSet<UPnPRouter*> pending_routers; // routers which we are downloading the XML file from

            Q_DISABLE_COPY(UPnPMCastSocket)
        };
    }
}

#endif
