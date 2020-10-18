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
#include "konversation_log.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

#include <QCoreApplication>
#include <QNetworkReply>

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
            if (name == QLatin1String("serviceType"))
                servicetype = value;
            else if (name == QLatin1String("controlURL"))
                controlurl = value;
            else if (name == QLatin1String("eventSubURL"))
                eventsuburl = value;
            else if (name == QLatin1String("SCPDURL"))
                scpdurl = value;
            else if (name == QLatin1String("serviceId"))
                serviceid = value;
        }

        void UPnPService::clear()
        {
            servicetype = controlurl = eventsuburl = scpdurl = serviceid = QString();
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
            if (name == QLatin1String("friendlyName"))
                friendlyName = value;
            else if (name == QLatin1String("manufacturer"))
                manufacturer = value;
            else if (name == QLatin1String("modelDescription"))
                modelDescription = value;
            else if (name == QLatin1String("modelName"))
                modelName = value;
            else if (name == QLatin1String("modelNumber"))
                modelNumber = value;
        }

        ///////////////////////////////////////

        UPnPRouter::UPnPRouter(const QString & server,const QUrl &location,const QString & uuid) : server(server),location(location),uuid(uuid)
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
            if (!( s.servicetype.contains(QLatin1String("WANIPConnection")) ||
                   s.servicetype.contains(QLatin1String("WANPPPConnection")) ))
                return;

            // Confirm this service is connected. Place in pending queue.
            KJob *req = getStatusInfo(s);

            if (req) pending_services[req] = s;
        }

        void UPnPRouter::downloadFinished(KJob* j)
        {
            if (j->error())
            {
                error = i18n("Failed to download %1: %2",location.url(),j->errorString());
                qCDebug(KONVERSATION_LOG) << error;
                return;
            }

            KIO::StoredTransferJob* st = qobject_cast<KIO::StoredTransferJob*>(j);
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
            qCDebug(KONVERSATION_LOG) << "Downloading XML file " << location;
            KIO::Job* job = KIO::storedGet(location,KIO::NoReload, KIO::Overwrite | KIO::HideProgressInfo);
            connect(job, &KIO::Job::result, this, &UPnPRouter::downloadFinished);
        }

        KJob *UPnPRouter::getStatusInfo(const UPnPService &s)
        {
            qCDebug(KONVERSATION_LOG) << "UPnP - Checking service status: " << s.servicetype;

            QString action = QStringLiteral("GetStatusInfo");
            QString comm = SOAP::createCommand(action,s.servicetype);

            return sendSoapQuery(comm,s.servicetype + QLatin1Char('#') + action,s.controlurl);
        }

        bool UPnPRouter::forward(const QHostAddress & host, quint16 port, QAbstractSocket::SocketType proto)
        {
            qCDebug(KONVERSATION_LOG) << "Forwarding port " << host.toString() << port << " (" << (proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP") << ")";

            if (service.ready)
            {
                // add all the arguments for the command
                QList<SOAP::Arg> args;
                SOAP::Arg a;
                a.element = QStringLiteral("NewRemoteHost");
                args.append(a);

                // the external port
                a.element = QStringLiteral("NewExternalPort");
                a.value = QString::number(port);
                args.append(a);

                // the protocol
                a.element = QStringLiteral("NewProtocol");
                a.value = (proto == QAbstractSocket::TcpSocket) ? QStringLiteral("TCP") : QStringLiteral("UDP");
                args.append(a);

                // the local port
                a.element = QStringLiteral("NewInternalPort");
                a.value = QString::number(port);
                args.append(a);

                // the local IP address
                a.element = QStringLiteral("NewInternalClient");
                a.value = host.toString();
                args.append(a);

                a.element = QStringLiteral("NewEnabled");
                a.value = QStringLiteral("1");
                args.append(a);

                a.element = QStringLiteral("NewPortMappingDescription");
                a.value = QStringLiteral("Konversation UPNP");
                args.append(a);

                a.element = QStringLiteral("NewLeaseDuration");
                a.value = QStringLiteral("0");
                args.append(a);

                QString action = QStringLiteral("AddPortMapping");
                QString comm = SOAP::createCommand(action,service.servicetype,args);

                Forwarding *forward = new Forwarding;

                forward->port = port;
                forward->host = host;
                forward->proto = proto;

                if (KJob *req = sendSoapQuery(comm,service.servicetype + QLatin1Char('#') + action,service.controlurl))
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

                qCDebug(KONVERSATION_LOG) << "Forwarding Failed: Failed to send SOAP query.";
                delete forward;
            }

            qCDebug(KONVERSATION_LOG) << "Forwarding Failed: No UPnP Service.";
            return false;
        }

        bool UPnPRouter::undoForward(quint16 port, QAbstractSocket::SocketType proto)
        {
            qCDebug(KONVERSATION_LOG) << "Undoing forward of port " << port
                    << " (" << (proto == QAbstractSocket::TcpSocket ? "TCP" : "UDP") << ")";

            if (service.ready)
            {
                Forwarding *forward = nullptr;

                QListIterator<Forwarding*> itr(forwards);
                while (itr.hasNext())
                {
                    Forwarding *check = itr.next();

                    if (check->port == port && check->proto == proto)
                        forward = check;
                }

                if (forward == nullptr || pending_forwards.keys(forward).size() > 0)
                    return false; // Either forward not found or forward is still pending

                // add all the arguments for the command
                QList<SOAP::Arg> args;
                SOAP::Arg a;
                a.element = QStringLiteral("NewRemoteHost");
                args.append(a);

                // the external port
                a.element = QStringLiteral("NewExternalPort");
                a.value = QString::number(forward->port);
                args.append(a);

                // the protocol
                a.element = QStringLiteral("NewProtocol");
                a.value = (forward->proto == QAbstractSocket::TcpSocket) ? QStringLiteral("TCP") : QStringLiteral("UDP");
                args.append(a);

                QString action = QStringLiteral("DeletePortMapping");
                QString comm = SOAP::createCommand(action,service.servicetype,args);

                if (KJob *req = sendSoapQuery(comm,service.servicetype + QLatin1Char('#') + action,service.controlurl))
                {
                    pending_unforwards[req] = forward;

                    return true;
                }

                qCDebug(KONVERSATION_LOG) << "Undo forwarding Failed: Failed to send SOAP query.";
            }

            qCDebug(KONVERSATION_LOG) << "Undo forwarding Failed: No UPnP Service.";
            return false;
        }

        KJob *UPnPRouter::sendSoapQuery(const QString & query,const QString & soapact,const QString & controlurl)
        {
            // if port is not set, 0 will be returned
            // thanks to Diego R. Brogna for spotting this bug
            if (location.port()<=0)
                location.setPort(80);

            QUrl address;

            address.setScheme(QStringLiteral("http"));
            address.setHost(location.host());
            address.setPort(location.port());
            address.setPath(controlurl);

            KIO::TransferJob *req = KIO::http_post( address, query.toLatin1(), KIO::HideProgressInfo );

            req->addMetaData(QStringLiteral("content-type"), QStringLiteral("text/xml"));
            req->addMetaData(QStringLiteral("UserAgent"), QStringLiteral("Konversation UPnP"));
            req->addMetaData(QStringLiteral("customHTTPHeader"), QStringLiteral("SOAPAction: ") + soapact);

            soap_data_out[req] = QByteArray();
            soap_data_in[req]  = QByteArray();

            connect(req, &KIO::TransferJob::data, this, &UPnPRouter::recvSoapData);
            connect(req, &KIO::TransferJob::dataReq, this, &UPnPRouter::sendSoapData);

            connect(req, &KIO::TransferJob::result, this, &UPnPRouter::onRequestFinished);

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
                qCDebug(KONVERSATION_LOG) << "UPnPRouter : Error: " << r->errorString();

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
                const QString reply = QString::fromUtf8(soap_data_in[r]);
                soap_data_in[r].clear();

                qCDebug(KONVERSATION_LOG) << "UPnPRouter : OK:";

                if (pending_services.contains(r))
                {
                    if (reply.contains(QLatin1String("Connected")))
                    {
                        // Lets just deal with one connected service for now. Last one wins.
                        service = pending_services[r];
                        service.ready = true;

                        qCDebug(KONVERSATION_LOG) << "Found connected service: " << service.servicetype;
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
