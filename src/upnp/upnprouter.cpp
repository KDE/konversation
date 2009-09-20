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

#include "upnprouter.h"
#include "upnpdescriptionparser.h"
#include "soap.h"

#include <QCoreApplication>
#include <QNetworkReply>

#include <KDebug>
#include <KLocale>
#include <KIO/Job>
#include <KIO/NetAccess>
#include <kio/jobclasses.h>

#include <stdlib.h>


namespace Konversation
{
    namespace UPnP
    {
        UPnPService::UPnPService()
        {
            ready = false;
        }

        UPnPService::UPnPService(const UPnPService & s)
        {
            this->servicetype = s.servicetype;
            this->controlurl = s.controlurl;
            this->eventsuburl = s.eventsuburl;
            this->serviceid = s.serviceid;
            this->scpdurl = s.scpdurl;

            ready = false;
        }

        void UPnPService::setProperty(const QString & name,const QString & value)
        {
            if (name == "serviceType")
                servicetype = value;
            else if (name == "controlURL")
                controlurl = value;
            else if (name == "eventSubURL")
                eventsuburl = value;
            else if (name == "SCPDURL")
                scpdurl = value;
            else if (name == "serviceId")
                serviceid = value;
        }

        void UPnPService::clear()
        {
            servicetype = controlurl = eventsuburl = scpdurl = serviceid = "";
        }

        UPnPService & UPnPService::operator = (const UPnPService & s)
        {
            this->servicetype = s.servicetype;
            this->controlurl = s.controlurl;
            this->eventsuburl = s.eventsuburl;
            this->serviceid = s.serviceid;
            this->scpdurl = s.scpdurl;
            return *this;
        }

        ///////////////////////////////////////

        void UPnPDeviceDescription::setProperty(const QString & name,const QString & value)
        {
            if (name == "friendlyName")
                friendlyName = value;
            else if (name == "manufacturer")
                manufacturer = value;
            else if (name == "modelDescription")
                modelDescription = value;
            else if (name == "modelName")
                modelName = value;
            else if (name == "modelNumber")
                modelNumber == value;
        }

        ///////////////////////////////////////

        UPnPRouter::UPnPRouter(const QString & server,const KUrl & location,const QString & uuid) : server(server),location(location),uuid(uuid)
        {
        }


        UPnPRouter::~UPnPRouter()
        {
            QListIterator<Forwarding*> itr(forwards);
            while (itr.hasNext())
            {
                Forwarding *check = itr.next();
                undoForward(check->port, check->proto);
            }

            // We need to give time for the QNetworkManager to process the undo forward commands. Continue
            // Processing the event loop from here until there are no more forwards.
            while(forwards.size() > 0)
            {
                QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
            }
        }

        void UPnPRouter::addService(const UPnPService & s)
        {
            if (!( s.servicetype.contains("WANIPConnection") ||
                   s.servicetype.contains("WANPPPConnection") ))
                return;

            // Confirm this service is connected. Place in pending queue.
            KJob *req = getStatusInfo(s);

            if (req) pending_services[req] = s;
        }

        void UPnPRouter::downloadFinished(KJob* j)
        {
            if (j->error())
            {
                error = i18n("Failed to download %1 : %2",location.prettyUrl(),j->errorString());
                kDebug() << error << endl;
                return;
            }

            KIO::StoredTransferJob* st = (KIO::StoredTransferJob*)j;
            // load in the file (target is always local)
            UPnPDescriptionParser desc_parse;
            bool ret = desc_parse.parse(st->data(),this);
            if (!ret)
            {
                error = i18n("Error parsing router description.");
            }

            emit xmlFileDownloaded(this,ret);
        }

        void UPnPRouter::downloadXMLFile()
        {
            error.clear();
            // downlaod XML description into a temporary file in /tmp
            kDebug() << "Downloading XML file " << location << endl;
            KIO::Job* job = KIO::storedGet(location,KIO::NoReload, KIO::Overwrite | KIO::HideProgressInfo);
            connect(job,SIGNAL(result(KJob *)),this,SLOT(downloadFinished( KJob* )));
        }

        KJob *UPnPRouter::getStatusInfo(UPnPService s)
        {
            kDebug() << "UPnP - Checking service status: " << s.servicetype << endl;

            QString action = "GetStatusInfo";
            QString comm = SOAP::createCommand(action,s.servicetype);

            return sendSoapQuery(comm,s.servicetype + '#' + action,s.controlurl);
        }

        bool UPnPRouter::forward(const QHostAddress & host, quint16 port, QAbstractSocket::SocketType proto)
        {
            kDebug() << "Forwarding port " << host.toString() << port << " (" << (proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP") << ")" << endl;

            if (service.ready)
            {
                // add all the arguments for the command
                QList<SOAP::Arg> args;
                SOAP::Arg a;
                a.element = "NewRemoteHost";
                args.append(a);

                // the external port
                a.element = "NewExternalPort";
                a.value = QString::number(port);
                args.append(a);

                // the protocol
                a.element = "NewProtocol";
                a.value = proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP";
                args.append(a);

                // the local port
                a.element = "NewInternalPort";
                a.value = QString::number(port);
                args.append(a);

                // the local IP address
                a.element = "NewInternalClient";
                a.value = host.toString();
                args.append(a);

                a.element = "NewEnabled";
                a.value = '1';
                args.append(a);

                a.element = "NewPortMappingDescription";
                a.value = QString("Konversation UPNP");
                args.append(a);

                a.element = "NewLeaseDuration";
                a.value = '0';
                args.append(a);

                QString action = "AddPortMapping";
                QString comm = SOAP::createCommand(action,service.servicetype,args);

                Forwarding *forward = new Forwarding;

                forward->port = port;
                forward->host = host;
                forward->proto = proto;

                if (KJob *req = sendSoapQuery(comm,service.servicetype + '#' + action,service.controlurl))
                {
                    // erase old forwarding if one exists
                    // The UPnP spec states if an IGD receives a forward request that matches an existing request that it must accept it.
                    QListIterator<Forwarding*> itr(forwards);
                    while (itr.hasNext())
                    {
                        Forwarding *check = itr.next();

                        if (check->port == forward->port && check->host == forward->host && check->proto == forward->proto)
                        {
                            forwards.removeAll(check);
                            delete check;
                        }
                    }

                    forwards.append(forward);
                    pending_forwards[req] = forward;

                    return true;
                }

                kDebug() << "Forwarding Failed: Failed to send SOAP query.";
                delete forward;
            }

            kDebug() << "Forwarding Failed: No UPnP Service.";
            return false;
        }

        bool UPnPRouter::undoForward(quint16 port, QAbstractSocket::SocketType proto)
        {
            kDebug() << "Undoing forward of port " << port
                    << " (" << (proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP") << ")" << endl;

            if (service.ready)
            {
                Forwarding *forward = NULL;

                QListIterator<Forwarding*> itr(forwards);
                while (itr.hasNext())
                {
                    Forwarding *check = itr.next();

                    if (check->port == port && check->proto == proto)
                        forward = check;
                }

                if (forward == NULL || pending_forwards.keys(forward).size() > 0)
                    return false; // Either forward not found or forward is still pending

                // add all the arguments for the command
                QList<SOAP::Arg> args;
                SOAP::Arg a;
                a.element = "NewRemoteHost";
                args.append(a);

                // the external port
                a.element = "NewExternalPort";
                a.value = QString::number(forward->port);
                args.append(a);

                // the protocol
                a.element = "NewProtocol";
                a.value = forward->proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP";
                args.append(a);

                QString action = "DeletePortMapping";
                QString comm = SOAP::createCommand(action,service.servicetype,args);

                if (KJob *req = sendSoapQuery(comm,service.servicetype + '#' + action,service.controlurl))
                {
                    pending_unforwards[req] = forward;

                    return true;
                }
                
                kDebug() << "Undo forwarding Failed: Failed to send SOAP query.";
            }

            kDebug() << "Undo forwarding Failed: No UPnP Service.";
            return false;
        }

        KJob *UPnPRouter::sendSoapQuery(const QString & query,const QString & soapact,const QString & controlurl)
        {
            // if port is not set, 0 will be returned
            // thanks to Diego R. Brogna for spotting this bug
            if (location.port()<=0)
                location.setPort(80);

            KUrl address;

            address.setProtocol(QString("http"));
            address.setHost(location.host());
            address.setPort(location.port());
            address.setPath(controlurl);

            KIO::TransferJob *req = KIO::http_post( address, query.toAscii(), KIO::HideProgressInfo );
            
            req->addMetaData("content-type", QString("text/xml"));
            req->addMetaData("UserAgent", QString("Konversation UPnP"));
            req->addMetaData("customHTTPHeader", QString("SOAPAction: ") + soapact);

            soap_data_out[req] = QByteArray();
            soap_data_in[req]  = QByteArray();
            
            connect( req, SIGNAL(data( KIO::Job *, const QByteArray & )), this, SLOT(recvSoapData( KIO::Job*, const QByteArray & )) );
            connect( req, SIGNAL(dataReq( KIO::Job *, QByteArray & )), this, SLOT(sendSoapData( KIO::Job*, QByteArray & )) );
            
            connect( req, SIGNAL(result(KJob *)), this, SLOT(onRequestFinished( KJob* )) );
            
            return req;
        }

        void UPnPRouter::sendSoapData(KIO::Job *job, QByteArray &data)
        {
            data.append(soap_data_out[job]);
            soap_data_out[job].clear();
        }

        void UPnPRouter::recvSoapData(KIO::Job *job, const QByteArray &data)
        {
            soap_data_in[job].append(data);
        }

        void UPnPRouter::onRequestFinished(KJob *r)
        {
            if (r->error())
            {
                kDebug() << "UPnPRouter : Error: " << r->errorString() << endl;
                
                if (pending_services.contains(r))
                {
                    pending_services.remove(r);
                }
                else if (pending_forwards.contains(r))
                {
                    emit forwardComplete(true, pending_forwards[r]->port);
                    
                    forwards.removeAll(pending_forwards[r]);
                    pending_forwards.remove(r);
                }
                else if (pending_unforwards.contains(r))
                {
                    emit unforwardComplete(true, pending_unforwards[r]->port);
                    
                    forwards.removeAll(pending_unforwards[r]);
                    pending_unforwards.remove(r);
                }
            }
            else
            {
                QString reply(soap_data_in[r]);
                soap_data_in[r].clear();

                kDebug() << "UPnPRouter : OK:" << endl;
                
                if (pending_services.contains(r))
                {
                    if (reply.contains("Connected"))
                    {
                        // Lets just deal with one connected service for now. Last one wins.
                        service = pending_services[r];
                        service.ready = true;
                        
                        kDebug() << "Found connected service: " << service.servicetype << endl;
                    }
                    
                    pending_services.remove(r);
                }
                else if (pending_forwards.contains(r))
                {
                    emit forwardComplete(false, pending_forwards[r]->port);
                    
                    pending_forwards.remove(r);
                }
                else if (pending_unforwards.contains(r))
                {
                    emit unforwardComplete(false, pending_unforwards[r]->port);
                    
                    forwards.removeAll(pending_unforwards[r]);
                    pending_unforwards.remove(r);
                }
            }
            
            soap_data_in.remove(r);
            soap_data_out.remove(r);
        }
    }
}
