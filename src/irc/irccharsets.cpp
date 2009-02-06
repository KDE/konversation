// A wrapper for KCharsets
// Copyright (C) 2004, 2006 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "irccharsets.h"

#include <qglobal.h>
#include <qregexp.h>
#include <qlocale.h>
#include <qtextcodec.h>
#include <kcharsets.h>
#include <kdebug.h>
#include <kglobal.h>


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

    QStringList IRCCharsets::availableEncodingShortNames()
    {
        return m_shortNames;
    }

    QStringList IRCCharsets::availableEncodingDescriptiveNames()
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
        simplifiedAmbiguousName.replace( QRegExp( "[^a-z0-9]" ), "" );

        // search m_simplifiedShortNames
        int index = 0;
        for ( QStringList::iterator it = m_simplifiedShortNames.begin() ; it != m_simplifiedShortNames.end() ; ++it )
        {
            if ( (*it) == simplifiedAmbiguousName )
                return m_shortNames[index];
            ++index;
        }

        // search m_shortNameAliases
        if ( m_shortNameAliases.contains( simplifiedAmbiguousName ) )
            return m_shortNameAliases[ simplifiedAmbiguousName ];

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

        return "utf8";
    }

    QTextCodec* IRCCharsets::codecForName( const QString& shortName )
    {
        if(shortName == "iso-2022-jp")
            return QTextCodec::codecForName( "jis7" );
        else
            return QTextCodec::codecForName( shortName.toAscii() );
    }

    IRCCharsets::IRCCharsets()
    {
        // setup m_shortNameAliases
        // use only [a-z0-9] for keys!
        m_shortNameAliases["unicode"] = "utf8";
        m_shortNameAliases["latin1"] = "iso-8859-1";

        // setup m_shortNames, m_descriptiveNames, m_simplifiedShortNames
        QRegExp reSimplify( "[^a-zA-Z0-9]" );
        m_descriptiveNames = KGlobal::charsets()->descriptiveEncodingNames();
        QStringList::Iterator it = m_descriptiveNames.begin();
        while ( it != m_descriptiveNames.end() )
        {
            QString encodingName = KGlobal::charsets()->encodingForName( *it );
            // exclude encodings which are not supported on IRC
            if ( encodingName == "iso-10646-ucs-2" ||
                 encodingName == "utf16" ||
                 encodingName == "utf16" ||
                 encodingName == "utf7" )
            {
                it = m_descriptiveNames.erase( it );
            }
            else
            {
                m_shortNames.append( encodingName );
                m_simplifiedShortNames.append( encodingName.replace( reSimplify, "" ) );

                if(encodingName == "jis7")        // Add iso-2022-jp which is same as jis7 but not in Qt
                {
                    it = m_descriptiveNames.insert(it, "Japanese ( iso-2022-jp )");
                    m_shortNames.append( "iso-2022-jp" );
                    m_simplifiedShortNames.append( "ISO-2022-JP" );
                    ++it;
                }
                ++it;
            }

        }
    }

}
