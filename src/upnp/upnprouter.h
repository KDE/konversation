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

#ifndef KTUPNPROUTER_H
#define KTUPNPROUTER_H

#include <QHostAddress>

#include <kurl.h>
#include <kjob.h>
#include <kio/jobclasses.h>

namespace Konversation
{
    namespace UPnP
    {
        /**
        * Structure describing a UPnP service found in an xml file.
        */
        struct UPnPService
        {
            QString serviceid;
            QString servicetype;
            QString controlurl;
            QString eventsuburl;
            QString scpdurl;

            bool ready;

            UPnPService();
            UPnPService(const UPnPService & s);

            /**
            * Set a property of the service.
            * @param name Name of the property (matches to variable names)
            * @param value Value of the property
            */
            void setProperty(const QString & name,const QString & value);

            /**
            * Set all strings to empty.
            */
            void clear();

            /// Print the data of this service
            void debugPrintData();

            /**
            * Assignment operator
            * @param s The service to copy
            * @return *this
            */
            UPnPService & operator = (const UPnPService & s);
        };

        /**
        *  Struct to hold the description of a device
        */
        struct UPnPDeviceDescription
        {
            QString friendlyName;
            QString manufacturer;
            QString modelDescription;
            QString modelName;
            QString modelNumber;

            /**
            * Set a property of the description
            * @param name Name of the property (matches to variable names)
            * @param value Value of the property
            */
            void setProperty(const QString & name,const QString & value);
        };

        /**
        * @author Joris Guisson
        *
        * Class representing a UPnP enabled router. This class is also used to communicate
        * with the router.
        */
        class UPnPRouter : public QObject
        {
            Q_OBJECT

        private:
            struct Forwarding
            {
                quint16 port;
                QHostAddress host;
                QAbstractSocket::SocketType proto;
            };

            QString server;
            KUrl location;
            QString uuid;
            UPnPDeviceDescription desc;

            UPnPService service;
            QList<Forwarding*> forwards;

            QHash<KJob*, UPnPService> pending_services;
            QHash<KJob*, Forwarding*> pending_forwards;
            QHash<KJob*, Forwarding*> pending_unforwards;

            QHash<KJob*, QByteArray>  soap_data_in;
            QHash<KJob*, QByteArray>  soap_data_out;

            QString error;
            
        public:
            /**
            * Construct a router.
            * @param server The name of the router
            * @param location The location of it's xml description file
            * @param verbose Print lots of debug info
            */
            UPnPRouter(const QString & server,const KUrl & location,const QString & uuid);
            virtual ~UPnPRouter();

            /// Get the name  of the server
            QString getServer() const {return server;}

            /// Get the uuid of the server
            QString getUUID() const {return uuid;}

            /// Get the location of it's xml description
            KUrl getLocation() const {return location;}

            /// Get the device description
            UPnPDeviceDescription & getDescription() {return desc;}

            /// Get the device description (const version)
            const UPnPDeviceDescription & getDescription() const {return desc;}

            /**
            * Download the XML File of the router.
            */
            void downloadXMLFile();

            /**
            * Add a service to the router.
            * @param s The service
            */
            void addService(const UPnPService & s);

            /**
            * Forward a local port
            * @param port The local port to forward
            */
            bool forward(const QHostAddress & host, quint16 port, QAbstractSocket::SocketType proto);

            /**
            * Undo forwarding
            * @param port The port
            * @param waitjob When this is set the jobs needs to be added to the waitjob,
            * so we can wait for their completeion at exit
            */
            bool undoForward(quint16 port, QAbstractSocket::SocketType proto);

            quint32 getNumForwardedPorts() const {return forwards.size();}

            /// Get the current error (null string if there is none)
            QString getError() const {return error;}

        private slots:
            void onRequestFinished(KJob *reply);
            void downloadFinished(KJob* j);
            
            void sendSoapData(KIO::Job *job, QByteArray &data);
            void recvSoapData(KIO::Job *job, const QByteArray &data);
            
        signals:
            /**
            * Signal which indicates that the XML was downloaded successfully or not.
            * @param r The router which emitted the signal
            * @param success Whether or not it succeeded
            */
            void xmlFileDownloaded(UPnPRouter* r,bool success);

            void forwardComplete(bool error, quint16 port);
            void unforwardComplete(bool error, quint16 port);

        private:

            KJob *sendSoapQuery(const QString & query,const QString & soapact,const QString & controlurl);
            KJob *getStatusInfo(UPnPService s);
        };
    }
}

#endif
