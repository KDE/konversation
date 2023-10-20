/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "upnprouter.h"

#include "upnpdescriptionparser.h"
#include "soap.h"
#include "konversation_log.h"

#include <KLocalizedString>
#include <KIO/StoredTransferJob>

#include <QCoreApplication>

#include <cstdlib>


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

        void UPnPService::setProperty(QStringView name, const QString & value)
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

        void UPnPDeviceDescription::setProperty(QStringView name, const QString & value)
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
            for (Forwarding *check : std::as_const(forwards)) {
                undoForward(check->port, check->proto);
            }

            // We need to give time for the QNetworkManager to process the undo forward commands. Continue
            // Processing the event loop from here until there are no more forwards.
            while (!forwards.isEmpty()) {
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

            auto* st = static_cast<KIO::StoredTransferJob*>(j);
            // load in the file (target is always local)
            UPnPDescriptionParser desc_parse;
            bool ret = desc_parse.parse(st->data(),this);
            if (!ret)
            {
                error = i18n("Error parsing router description.");
            }

            Q_EMIT xmlFileDownloaded(this,ret);
        }

        void UPnPRouter::downloadXMLFile()
        {
            error.clear();
            // download XML description into a temporary file in /tmp
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
                const QList<SOAP::Arg> args = {
                    { QStringLiteral("NewRemoteHost"),             QString() },
                    // the external port
                    { QStringLiteral("NewExternalPort"),           QString::number(port) },
                    // the protocol
                    { QStringLiteral("NewProtocol"),
                      (proto == QAbstractSocket::TcpSocket) ? QStringLiteral("TCP") : QStringLiteral("UDP") },
                    // the local port
                    { QStringLiteral("NewInternalPort"),           QString::number(port) },
                    // the local IP address
                    { QStringLiteral("NewInternalClient"),         host.toString() },
                    { QStringLiteral("NewEnabled"),                QStringLiteral("1") },
                    { QStringLiteral("NewPortMappingDescription"), QStringLiteral("Konversation UPNP") },
                    { QStringLiteral("NewLeaseDuration"),          QStringLiteral("0") },
                };

                QString action = QStringLiteral("AddPortMapping");
                QString comm = SOAP::createCommand(action,service.servicetype,args);

                auto *forward = new Forwarding;

                forward->port = port;
                forward->host = host;
                forward->proto = proto;

                if (KJob *req = sendSoapQuery(comm,service.servicetype + QLatin1Char('#') + action,service.controlurl))
                {
                    // erase old forwarding if one exists
                    // The UPnP spec states if an IGD receives a forward request that matches an existing request that it must accept it.
                    auto it = forwards.begin();
                    while (it != forwards.end()) {
                        Forwarding *check = *it;

                        if (check->port == forward->port && check->host == forward->host && check->proto == forward->proto)
                        {
                            it = forwards.erase(it);
                            delete check;
                        } else {
                            ++it;
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

                for (Forwarding *check : std::as_const(forwards)) {
                    if (check->port == port && check->proto == proto)
                        forward = check;
                }

                if (forward == nullptr || !pending_forwards.keys(forward).isEmpty())
                    return false; // Either forward not found or forward is still pending

                // add all the arguments for the command
                const QList<SOAP::Arg> args = {
                    { QStringLiteral("NewRemoteHost"),   QString() },
                    // the external port
                    { QStringLiteral("NewExternalPort"), QString::number(forward->port) },
                    // the protocol
                    { QStringLiteral("NewProtocol"),
                      (forward->proto == QAbstractSocket::TcpSocket) ? QStringLiteral("TCP") : QStringLiteral("UDP") },
                };

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
                    Q_EMIT forwardComplete(true, pending_forwards[r]->port);

                    Forwarding * forward = pending_forwards[r];
                    forwards.removeAll(forward);
                    pending_forwards.remove(r);
                    delete forward;
                }
                else if (pending_unforwards.contains(r))
                {
                    Q_EMIT unforwardComplete(true, pending_unforwards[r]->port);

                    Forwarding * forward = pending_unforwards[r];
                    forwards.removeAll(forward);
                    pending_unforwards.remove(r);
                    delete forward;
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
                    Q_EMIT forwardComplete(false, pending_forwards[r]->port);

                    pending_forwards.remove(r);
                }
                else if (pending_unforwards.contains(r))
                {
                    Q_EMIT unforwardComplete(false, pending_unforwards[r]->port);

                    Forwarding * forward = pending_unforwards[r];
                    forwards.removeAll(forward);
                    pending_unforwards.remove(r);
                    delete forward;
                }
            }

            soap_data_in.remove(r);
            soap_data_out.remove(r);
        }
    }
}

#include "moc_upnprouter.cpp"
