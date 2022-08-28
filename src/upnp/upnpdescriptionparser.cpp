/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "upnpdescriptionparser.h"

#include "upnprouter.h"
#include "konversation_log.h"

#include <QXmlStreamReader>
#include <QStack>


namespace Konversation
{
    namespace UPnP
    {

        class XMLContentHandler
        {
            enum Status
            {
                TOPLEVEL,ROOT,DEVICE,SERVICE,FIELD,OTHER
            };

            QString tmp;
            UPnPRouter* router;
            UPnPService curr_service;
            QStack<Status> status_stack;

            Q_DISABLE_COPY(XMLContentHandler)

        public:
            XMLContentHandler(UPnPRouter* router);
            ~XMLContentHandler();

            bool parse(const QByteArray& data);

        private:
            bool startDocument();
            bool endDocument();
            bool startElement(QStringView namespaceUri, QStringView localName,
                              QStringView qName, const QXmlStreamAttributes& atts);
            bool endElement(QStringView namespaceUri, QStringView localName,
                            QStringView qName);
            bool characters(QStringView chars);

            static bool interestingDeviceField(QStringView name);
            static bool interestingServiceField(QStringView name);
        };


        UPnPDescriptionParser::UPnPDescriptionParser()
        {}


        UPnPDescriptionParser::~UPnPDescriptionParser()
        {}

        bool UPnPDescriptionParser::parse(const QByteArray & data,UPnPRouter* router)
        {
            XMLContentHandler chandler(router);
            const bool ret = chandler.parse(data);

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

        bool XMLContentHandler::parse(const QByteArray& data)
        {
            QXmlStreamReader reader(data);

            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.hasError())
                    return false;

                switch (reader.tokenType()) {
                case QXmlStreamReader::StartDocument:
                    if (!startDocument()) {
                        return false;
                    }
                    break;
                case QXmlStreamReader::EndDocument:
                    if (!endDocument()) {
                        return false;
                    }
                    break;
                case QXmlStreamReader::StartElement:
                    if (!startElement(reader.namespaceUri(), reader.name(),
                                      reader.qualifiedName(), reader.attributes())) {
                        return false;
                    }
                    break;
                case QXmlStreamReader::EndElement:
                    if (!endElement(reader.namespaceUri(), reader.name(),
                                    reader.qualifiedName())) {
                        return false;
                    }
                    break;
                case QXmlStreamReader::Characters:
                    if (!reader.isWhitespace() && !reader.text().trimmed().isEmpty()) {
                        if (!characters(reader.text()))
                            return false;
                    }
                    break;
                default:
                    break;
                }
            }

            if (!reader.isEndDocument())
                return false;

            return true;
        }

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

        bool XMLContentHandler::interestingDeviceField(QStringView name)
        {
            return name == QLatin1String("friendlyName") || name == QLatin1String("manufacturer") || name == QLatin1String("modelDescription") ||
                    name == QLatin1String("modelName") || name == QLatin1String("modelNumber");
        }


        bool XMLContentHandler::interestingServiceField(QStringView name)
        {
            return name == QLatin1String("serviceType") || name == QLatin1String("serviceId") || name == QLatin1String("SCPDURL") ||
                    name == QLatin1String("controlURL") || name == QLatin1String("eventSubURL");
        }

        bool XMLContentHandler::startElement(QStringView namespaceUri, QStringView localName,
                                             QStringView qName, const QXmlStreamAttributes& atts)
        {
            Q_UNUSED(namespaceUri)
            Q_UNUSED(qName)
            Q_UNUSED(atts)

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

        bool XMLContentHandler::endElement(QStringView namespaceUri, QStringView localName,
                                           QStringView qName)
        {
            Q_UNUSED(namespaceUri)
            Q_UNUSED(qName)

            switch (status_stack.top())
            {
            case FIELD:
                // we have a field so set it
                status_stack.pop();
                if (status_stack.top() == DEVICE)
                {
                    // if we are in a device
                    router->getDescription().setProperty(localName, tmp);
                }
                else if (status_stack.top() == SERVICE)
                {
                    // set a property of a service
                    curr_service.setProperty(localName, tmp);
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


        bool XMLContentHandler::characters(QStringView ch)
        {
            tmp.append(ch);

            return true;
        }
    }
}
