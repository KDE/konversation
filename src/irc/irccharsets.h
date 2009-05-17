// A wrapper for KCharsets
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef KONVERSATION_IRCCHARSETS_H
#define KONVERSATION_IRCCHARSETS_H

#include <QHash>
#include <qstringlist.h>


namespace Konversation
{
    struct IRCCharsetsSingleton;

    class IRCCharsets
    {
        friend struct IRCCharsetsSingleton;

        private:
            IRCCharsets();

        public:
            static IRCCharsets *self();

            /**
             * Lists all available encoding names.
             * e.g. "utf8", "iso 8859-1"
             * Encodings which don't work on IRC are excluded. (e.g. utf16)
             * @note It's guaranteed that the order of this list is same with that of @ref availableEncodingDescriptiveNames() .
             */
            QStringList availableEncodingShortNames();

            /**
             * Lists all available encoding descriptions.
             * e.g. "Unicode ( utf8 )", "Western European ( iso 8859-1 )"
             * Encodings which don't work on IRC are excluded. (e.g. utf16)
             */
            QStringList availableEncodingDescriptiveNames();

            int availableEncodingsCount();

            QString shortNameToDescriptiveName( const QString& shortName );
            QString descriptiveNameToShortName( const QString& descriptiveName );

            /**
             * Converts the ambiguous encoding name to a short encoding name
             * Like : iso8859-9 -> iso 8859-9, iso-8859-9 -> iso 8859-9
             * If the ambiguous name is invalid, returns QString:null.
             * @return a short encoding name or QString::null
             */
            QString ambiguousNameToShortName( const QString& ambiguousName );

            /**
             * Returns the encoding index in the short names list or the descriptions list.
             * If the encoding name is invalid, returns -1.
             * @return an index number of the encoding
             */
            int shortNameToIndex( const QString& shortName );

            /**
             * Checks if the encoding name is in the short encoding names.
             * @see availableEncodingShortNames()
             */
            bool isValidEncoding( const QString& shortName );

            /**
             * Returns the short name of the most suitable encoding for this locale.
             * @return a short encoding name
             */
            QString encodingForLocale();

            QTextCodec* codecForName( const QString& shortName );

        private:
            /**
             * short names list
             * you can get this list with @ref availableEncodingShortNames()
             * e.g. iso 8859-1
             */
            QStringList m_shortNames;

            /**
             * descriptive names list
             * you can get this list with @ref availableEncodingDescriptiveNames();
             * e.g. Western European ( iso 8859-1 )
             */
            QStringList m_descriptiveNames;

            /**
             * simplified short names list (for internal use)
             * e.g. iso88591
             * used in @ref ambiguousNameToShortName()
             */
            QHash<QString, QString> m_simplifiedShortNames;
    };

}
#endif                                            // KONVERSATION_IRCCHARSETS_H
