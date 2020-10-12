/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005-2007 Joris Guisson <joris.guisson@gmail.com>
    SPDX-FileCopyrightText: 2009 Michael Kreitzer <mrgrim@gr1m.org>
*/

#ifndef KTSOAP_H
#define KTSOAP_H

#include <QList>
#include <QString>


namespace Konversation
{
    namespace UPnP
    {

        /**
        @author Joris Guisson
        */
        class SOAP
        {
        public:

            /**
            * Create a simple UPnP SOAP command without parameters.
            * @param action The name of the action
            * @param service The name of the service
            * @return The command
            */
            static QString createCommand(const QString & action,const QString & service);

            struct Arg
            {
                QString element;
                QString value;
            };

            /**
            * Create a UPnP SOAP command with parameters.
            * @param action The name of the action
            * @param service The name of the service
            * @param args Arguments for command
            * @return The command
            */
            static QString createCommand(const QString & action,const QString & service,const QList<Arg> & args);
        };
    }
}

#endif
