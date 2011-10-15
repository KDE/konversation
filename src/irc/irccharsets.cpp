// A wrapper for KCharsets
// Copyright (C) 2004, 2006 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "irccharsets.h"

#include <QLocale>
#include <QTextCodec>

#include <KCharsets>
#include <KGlobal>


namespace Konversation
{

    struct IRCCharsetsSingleton
    {
        IRCCharsets charsets;
    };

}

K_GLOBAL_STATIC(Konversation::IRCCharsetsSingleton, s_charsets)

namespace Konversation
{

    IRCCharsets *IRCCharsets::self()
    {
        return &s_charsets->charsets;
    }

    QStringList IRCCharsets::availableEncodingShortNames() const
    {
        return m_shortNames;
    }

    QStringList IRCCharsets::availableEncodingDescriptiveNames() const
    {
        return m_descriptiveNames;
    }

    int IRCCharsets::availableEncodingsCount()
    {
        return m_shortNames.count();
    }

    QString IRCCharsets::shortNameToDescriptiveName( const QString& shortName )
    {
        return m_descriptiveNames[ shortNameToIndex( shortName ) ];
    }

    QString descriptiveNameToShortName( const QString& descriptiveName )
    {
        return KGlobal::charsets()->encodingForName( descriptiveName );
    }

    QString IRCCharsets::ambiguousNameToShortName( const QString& ambiguousName )
    {
        // simplify ambiguousName
        QString simplifiedAmbiguousName( ambiguousName.toLower() );
        simplifiedAmbiguousName.remove( QRegExp( "[^a-z0-9]" ));

        // search m_simplifiedShortNames
        if(m_simplifiedShortNames.contains(simplifiedAmbiguousName))
            return m_simplifiedShortNames[simplifiedAmbiguousName];

        // failed
        return QString();
    }

    int IRCCharsets::shortNameToIndex( const QString& shortName )
    {
        int index = 0;
        for ( QStringList::iterator it = m_shortNames.begin() ; it != m_shortNames.end() ; ++it )
        {
            if ( (*it) == shortName )
                return index;
            ++index;
        }
        return -1;
    }

    bool IRCCharsets::isValidEncoding( const QString& shortName )
    {
        return ( m_shortNames.contains( shortName ) > 0 );
    }

    QString IRCCharsets::encodingForLocale()
    {
        QString locale = QLocale::system().name();

        // Special cases
        // don't add conditions for the languages for which QTextCodec::codecForLocale() returns a correct codec.
        if ( locale == "ja_JP" )
            return "jis7";

        // it's a little hacky..
        for ( QStringList::iterator it = m_shortNames.begin() ; it != m_shortNames.end() ; ++it )
            if ( QTextCodec::codecForName( (*it).toAscii() ) == QTextCodec::codecForLocale() )
                return *it;

        return "UTF-8";
    }

    QTextCodec* IRCCharsets::codecForName( const QString& shortName )
    {
        if(shortName == "ISO 2022-JP")
            return QTextCodec::codecForName( "jis7" );
        else
            return QTextCodec::codecForName( shortName.toAscii() );
    }

    IRCCharsets::IRCCharsets()
    {
        // Add some aliases
        // use only [a-z0-9] for keys!
        m_simplifiedShortNames["unicode"] = "UTF-8";
        m_simplifiedShortNames["latin1"] = "ISO 8859-1";

        // setup m_shortNames, m_descriptiveNames, m_simplifiedShortNames
        QRegExp reSimplify( "[^a-zA-Z0-9]" );
        m_descriptiveNames = KGlobal::charsets()->descriptiveEncodingNames();
        QStringList::Iterator it = m_descriptiveNames.begin();
        while ( it != m_descriptiveNames.end() )
        {
            QString encodingName = KGlobal::charsets()->encodingForName( *it );
            // exclude encodings which are not supported on IRC
            // 10646-UCS-2 & ucs2 are both UTF-16
            if ( encodingName == "ISO 10646-UCS-2" ||
                 encodingName == "ucs2" ||
                 encodingName == "UTF-16" ||
                 encodingName == "utf7" )
            {
                it = m_descriptiveNames.erase( it );
            }
            else
            {
                m_shortNames.append( encodingName );
                m_simplifiedShortNames.insert( encodingName.remove( reSimplify ).toLower(), m_shortNames.last() );

                if(encodingName == "jis7")        // Add iso-2022-jp which is same as jis7 but not in Qt
                {
                    it = m_descriptiveNames.insert(it, "Japanese ( ISO 2022-JP )");
                    m_shortNames.append( "ISO 2022-JP" );
                    m_simplifiedShortNames.insert( "iso2022jp", "ISO 2022-JP" );
                    ++it;
                }
                ++it;
            }

        }
    }

}
