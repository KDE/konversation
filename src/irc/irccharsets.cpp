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


namespace Konversation
{

    struct IRCCharsetsSingleton
    {
        IRCCharsets charsets;
    };

}

Q_GLOBAL_STATIC(Konversation::IRCCharsetsSingleton, s_charsets)

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
        return KCharsets::charsets()->encodingForName( descriptiveName );
    }

    QString IRCCharsets::ambiguousNameToShortName( const QString& ambiguousName )
    {
        // simplify ambiguousName
        QString simplifiedAmbiguousName( ambiguousName.toLower() );
        simplifiedAmbiguousName.remove( QRegExp( QStringLiteral("[^a-z0-9]") ));

        // search m_simplifiedShortNames
        if(m_simplifiedShortNames.contains(simplifiedAmbiguousName))
            return m_simplifiedShortNames[simplifiedAmbiguousName];

        // failed
        return QString();
    }

    int IRCCharsets::shortNameToIndex( const QString& shortName )
    {
        int index = 0;
        for (auto & m_shortName : m_shortNames)
        {
            if ( m_shortName == shortName )
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
        // Special cases
        // don't add conditions for the languages for which QTextCodec::codecForLocale() returns a correct codec.
        // Japanese networks prefer jis7 over Utf-8.
        if (QLocale::system().name() == QStringLiteral("ja_JP"))
            return QStringLiteral("ISO-2022-JP");

        // it's a little hacky..
        for (auto & m_shortName : m_shortNames)
            if (QTextCodec::codecForName( m_shortName.toLatin1() ) == QTextCodec::codecForLocale())
                return m_shortName;

        return QStringLiteral("UTF-8");
    }

    QTextCodec* IRCCharsets::codecForName( const QString& shortName )
    {
        // Qt 5 / KCharsets seem to no longer support jis7 in common builds, but we have
        // to assume existing user config.
        if (shortName == QStringLiteral("jis7"))
            return KCharsets::charsets()->codecForName("ISO-2022-JP");
        else
            return KCharsets::charsets()->codecForName(shortName.toLatin1());
    }

    IRCCharsets::IRCCharsets()
    {
        // Add some aliases
        // use only [a-z0-9] for keys!
        m_simplifiedShortNames[QStringLiteral("unicode")] = QStringLiteral("UTF-8");
        m_simplifiedShortNames[QStringLiteral("latin1")] = QStringLiteral("ISO 8859-1");

        // setup m_shortNames, m_descriptiveNames, m_simplifiedShortNames
        QRegExp reSimplify( QStringLiteral("[^a-zA-Z0-9]") );
        m_descriptiveNames = KCharsets::charsets()->descriptiveEncodingNames();
        QStringList::Iterator it = m_descriptiveNames.begin();
        while ( it != m_descriptiveNames.end() )
        {
            QString encodingName = KCharsets::charsets()->encodingForName( *it );
            // exclude encodings which are not supported on IRC
            // 10646-UCS-2 & ucs2 are both UTF-16
            if ( encodingName == QStringLiteral("ISO 10646-UCS-2") ||
                 encodingName == QStringLiteral("ucs2") ||
                 encodingName == QStringLiteral("UTF-16") ||
                 encodingName == QStringLiteral("utf7") )
            {
                it = m_descriptiveNames.erase( it );
            }
            else
            {
                m_shortNames.append( encodingName );
                m_simplifiedShortNames.insert( encodingName.remove( reSimplify ).toLower(), m_shortNames.last() );

                if(encodingName == QStringLiteral("jis7"))        // Add iso-2022-jp which is same as jis7 but not in Qt
                {
                    it = m_descriptiveNames.insert(it, QStringLiteral("Japanese ( ISO 2022-JP )"));
                    m_shortNames.append( QStringLiteral("ISO 2022-JP") );
                    m_simplifiedShortNames.insert( QStringLiteral("iso2022jp"), QStringLiteral("ISO 2022-JP") );
                    ++it;
                }
                ++it;
            }

        }
    }

}
