/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2006 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include "irccharsets.h"

#include <QLocale>
#include <QRegularExpression>
#include <QTextCodec>

#include <KCharsets>
#include <kcodecs.h>


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

    int IRCCharsets::availableEncodingsCount() const
    {
        return m_shortNames.count();
    }

    QString IRCCharsets::shortNameToDescriptiveName(const QString& shortName) const
    {
        return m_descriptiveNames[ shortNameToIndex( shortName ) ];
    }

    QString IRCCharsets::descriptiveNameToShortName(const QString& descriptiveName) const
    {
        return KCharsets::charsets()->encodingForName( descriptiveName );
    }

    QString IRCCharsets::ambiguousNameToShortName(const QString& ambiguousName) const
    {
        // simplify ambiguousName
        QString simplifiedAmbiguousName( ambiguousName.toLower() );
        simplifiedAmbiguousName.remove( QRegularExpression( QStringLiteral("[^a-z0-9]") ));

        // search m_simplifiedShortNames
        if(m_simplifiedShortNames.contains(simplifiedAmbiguousName))
            return m_simplifiedShortNames[simplifiedAmbiguousName];

        // failed
        return QString();
    }

    int IRCCharsets::shortNameToIndex(const QString& shortName) const
    {
        int index = 0;
        for (const QString& name : m_shortNames) {
            if (name == shortName)
                return index;
            ++index;
        }
        return -1;
    }

    bool IRCCharsets::isValidEncoding(const QString& shortName) const
    {
        return m_shortNames.contains(shortName);
    }

    QString IRCCharsets::encodingForLocale() const
    {
        // Special cases
        // don't add conditions for the languages for which QTextCodec::codecForLocale() returns a correct codec.
        // Japanese networks prefer jis7 over Utf-8.
        if (QLocale::system().name() == QLatin1String("ja_JP"))
            return QStringLiteral("ISO-2022-JP");

        // it's a little hacky..
        for (const QString& shortName : m_shortNames) {
            if (QTextCodec::codecForName(shortName.toLatin1()) == QTextCodec::codecForLocale())
                return shortName;
        }

        return QStringLiteral("UTF-8");
    }

    QTextCodec* IRCCharsets::codecForName(const QString& shortName) const
    {
        // KF6 removed `KCodecs::Codec::codecForName`. assuming that this
        // exists on Qt. Someone with better understanding of codecs, what 
        // should I do here?
        //
        // a KCodec is not convertible to a QTextCodec
        // KCodecs::Codec *codec = KCodecs::Codec::codecForName("ISO-2022-JP");
        // return ???

        return QTextCodec::codecForName(shortName.toLocal8Bit());
    }

    IRCCharsets::IRCCharsets()
    {
        // Add some aliases
        // use only [a-z0-9] for keys!
        m_simplifiedShortNames[QStringLiteral("unicode")] = QStringLiteral("UTF-8");
        m_simplifiedShortNames[QStringLiteral("latin1")] = QStringLiteral("ISO 8859-1");

        // setup m_shortNames, m_descriptiveNames, m_simplifiedShortNames
        const QRegularExpression reSimplify( QStringLiteral("[^a-zA-Z0-9]") );
        m_descriptiveNames = KCharsets::charsets()->descriptiveEncodingNames();
        QStringList::Iterator it = m_descriptiveNames.begin();
        while ( it != m_descriptiveNames.end() )
        {
            QString encodingName = KCharsets::charsets()->encodingForName( *it );
            // exclude encodings which are not supported on IRC
            // 10646-UCS-2 & ucs2 are both UTF-16
            if ( encodingName == QLatin1String("ISO 10646-UCS-2") ||
                 encodingName == QLatin1String("ucs2") ||
                 encodingName == QLatin1String("UTF-16") ||
                 encodingName == QLatin1String("utf7") )
            {
                it = m_descriptiveNames.erase( it );
            }
            else
            {
                m_shortNames.append( encodingName );
                m_simplifiedShortNames.insert( encodingName.remove( reSimplify ).toLower(), m_shortNames.last() );

                if(encodingName == QLatin1String("jis7"))        // Add iso-2022-jp which is same as jis7 but not in Qt
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
