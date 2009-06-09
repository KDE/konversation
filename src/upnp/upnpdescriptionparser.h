/***************************************************************************
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   Copyright (C) 2009 by Michael Kreitzer                                *
 *   joris.guisson@gmail.com                                               *
 *   mrgrim@gr1m.org                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef KTUPNPDESCRIPTIONPARSER_H
#define KTUPNPDESCRIPTIONPARSER_H


namespace Konversation
{
    namespace UPnP
    {
        class UPnPRouter;

        /**
        * @author Joris Guisson
        *
        * Parses the xml description of a router.
        */
        class UPnPDescriptionParser
        {
        public:
            UPnPDescriptionParser();
            virtual ~UPnPDescriptionParser();

            /**
            * Parse the xml description.
            * @param file File it is located in
            * @param router The router off the xml description
            * @return true upon success
            */
            bool parse(const QString & file,UPnPRouter* router);


            /**
            * Parse the xml description.
            * @param data QByteArray with the data
            * @param router The router off the xml description
            * @return true upon success
            */
            bool parse(const QByteArray & data,UPnPRouter* router);
        };
    }
}

#endif
