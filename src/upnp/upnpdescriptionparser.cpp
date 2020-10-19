/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "upnpdescriptionparser.h"

#include "upnprouter.h"
#include "konversation_log.h"

#include <QXmlAttributes>
#include <QStack>


namespace Konversation
{
    namespace UPnP
    {

        class XMLContentHandler : public QXmlDefaultHandler
        {
            enum Status
            {
                TOPLEVEL,ROOT,DEVICE,SERVICE,FIELD,OTHER
            };

            QString tmp;
            UPnPRouter* router;
            UPnPService curr_service;
            QStack<Status> status_stack;
        public:
            XMLContentHandler(UPnPRouter* router);
            ~XMLContentHandler() override;


            bool startDocument() override;
            bool endDocument() override;
            bool startElement(const QString &, const QString & localName, const QString &,
                            const QXmlAttributes & atts) override;
            bool endElement(const QString & , const QString & localName, const QString &  ) override;
            bool characters(const QString & ch) override;

            bool interestingDeviceField(const QString & name);
            bool interestingServiceField(const QString & name);
        };


        UPnPDescriptionParser::UPnPDescriptionParser()
        {}


        UPnPDescriptionParser::~UPnPDescriptionParser()
        {}

        bool UPnPDescriptionParser::parse(const QString & file,UPnPRouter* router)
        {
            bool ret = true;
            QFile fptr(file);
            if (!fptr.open(QIODevice::ReadOnly))
                return false;

            QXmlInputSource input(&fptr);
            XMLContentHandler chandler(router);
            QXmlSimpleReader reader;

            reader.setContentHandler(&chandler);
            ret = reader.parse(&input,false);

            if (!ret)
            {
                qCDebug(KONVERSATION_LOG) << "Error parsing XML";
                return false;
            }
            return true;
        }

        bool UPnPDescriptionParser::parse(const QByteArray & data,UPnPRouter* router)
        {
            bool ret = true;
            QXmlInputSource input;
            input.setData(data);
            XMLContentHandler chandler(router);
            QXmlSimpleReader reader;

            reader.setContentHandler(&chandler);
            ret = reader.parse(&input,false);

            if (!ret)
            {
                qCDebug(KONVERSATION_LOG) << "Error parsing XML";
                return false;
            }
            return true;
        }

        /////////////////////////////////////////////////////////////////////////////////


        XMLContentHandler::XMLContentHandler(UPnPRouter* router) : router(router)
        {}

        XMLContentHandler::~XMLContentHandler()
        {}


        bool XMLContentHandler::startDocument()
        {
            status_stack.push(TOPLEVEL);
            return true;
        }

        bool XMLContentHandler::endDocument()
        {
            status_stack.pop();
            return true;
        }

        bool XMLContentHandler::interestingDeviceField(const QString & name)
        {
            return name == QLatin1String("friendlyName") || name == QLatin1String("manufacturer") || name == QLatin1String("modelDescription") ||
                    name == QLatin1String("modelName") || name == QLatin1String("modelNumber");
        }


        bool XMLContentHandler::interestingServiceField(const QString & name)
        {
            return name == QLatin1String("serviceType") || name == QLatin1String("serviceId") || name == QLatin1String("SCPDURL") ||
                    name == QLatin1String("controlURL") || name == QLatin1String("eventSubURL");
        }

        bool XMLContentHandler::startElement(const QString &, const QString & localName, const QString &,
                                            const QXmlAttributes & )
        {
            tmp = QString();
            switch (status_stack.top())
            {
            case TOPLEVEL:
                // from toplevel we can only go to root
                if (localName == QLatin1String("root"))
                    status_stack.push(ROOT);
                else
                    return false;
                break;
            case ROOT:
                // from the root we can go to device or specVersion
                // we are not interested in the specVersion
                if (localName == QLatin1String("device"))
                    status_stack.push(DEVICE);
                else
                    status_stack.push(OTHER);
                break;
            case DEVICE:
                // see if it is a field we are interested in
                if (interestingDeviceField(localName))
                    status_stack.push(FIELD);
                else
                    status_stack.push(OTHER);
                break;
            case SERVICE:
                if (interestingServiceField(localName))
                    status_stack.push(FIELD);
                else
                    status_stack.push(OTHER);
                break;
            case OTHER:
                if (localName == QLatin1String("service"))
                    status_stack.push(SERVICE);
                else if (localName == QLatin1String("device"))
                    status_stack.push(DEVICE);
                else
                    status_stack.push(OTHER);
                break;
            case FIELD:
                break;
            }
            return true;
        }

        bool XMLContentHandler::endElement(const QString & , const QString & localName, const QString &  )
        {
            switch (status_stack.top())
            {
            case FIELD:
                // we have a field so set it
                status_stack.pop();
                if (status_stack.top() == DEVICE)
                {
                    // if we are in a device
                    router->getDescription().setProperty(localName,tmp);
                }
                else if (status_stack.top() == SERVICE)
                {
                    // set a property of a service
                    curr_service.setProperty(localName,tmp);
                }
                break;
            case SERVICE:
                // add the service
                router->addService(curr_service);
                curr_service.clear();
                // pop the stack
                status_stack.pop();
                break;
            default:
                status_stack.pop();
                break;
            }

            // reset tmp
            tmp = QString();
            return true;
        }


        bool XMLContentHandler::characters(const QString & ch)
        {
            if (!ch.isEmpty()) {
                tmp += ch;
            }
            return true;
        }
    }
}
