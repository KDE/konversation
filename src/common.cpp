/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
    SPDX-FileCopyrightText: 2006-2008 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2006 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "common.h"
// use <> include to allow test build to find the mock version
#include <config/preferences.h>

#include <QString>

#include <KLocalizedString>

#include "guess_ja.cpp"
#include "unicode.cpp"

namespace Konversation
{
    void initChanModesHash()
    {
        m_modesHash = {
            { QLatin1Char('t'), i18n("topic protection") },
            { QLatin1Char('n'), i18n("no messages from outside") },
            { QLatin1Char('s'), i18n("secret") },
            { QLatin1Char('i'), i18n("invite only") },
            { QLatin1Char('p'), i18n("private") },
            { QLatin1Char('m'), i18n("moderated") },
            { QLatin1Char('k'), i18n("password protected") },
            { QLatin1Char('a'), i18n("anonymous") },
            { QLatin1Char('c'), i18n("no colors allowed") },
            { QLatin1Char('l'), i18n("user limit") },
        };
    }

    QHash<QChar,QString> getChannelModesHash()
    {
        if(m_modesHash.isEmpty())
            initChanModesHash();

        return m_modesHash;
    }

    QString removeIrcMarkup(const QString& text)
    {
        QString escaped(text);
        // Escape text decoration
        escaped.remove(colorRegExp);

        return escaped;
    }

    QString doVarExpansion(const QString& text)
    {
        if (!Preferences::self()->disableExpansion())
        {
            const QList<QPair<int, int>> urlRanges = getUrlRanges(text);

            if (urlRanges.isEmpty())
                return replaceFormattingCodes(text);
            else
            {
                QString line;

                int startPos = 0;
                int length = 0;

                for (QPair<int, int> pair : urlRanges) {
                    length = pair.first - startPos;

                    line += replaceFormattingCodes(text.mid(startPos, length));

                    startPos = pair.first + pair.second;

                    line += QStringView(text).mid(pair.first, pair.second);
                }

                if (startPos <= text.length() - 1)
                    line += replaceFormattingCodes(text.mid(startPos));

                return line;
            }
        }

        return text;
    }

    QString replaceFormattingCodes(const QString& text)
    {
        QString line = text;

        // Replace placeholders.
        line.replace(QStringLiteral("%%"),QStringLiteral("%\x01"));      // make sure to protect double %%
        line.replace(QStringLiteral("%B"),QStringLiteral("\x02"));       // replace %B with bold char
        line.replace(QStringLiteral("%C"),QStringLiteral("\x03"));       // replace %C with color char
        line.replace(QStringLiteral("%G"),QStringLiteral("\x07"));       // replace %G with ASCII BEL 0x07
        line.replace(QStringLiteral("%I"),QStringLiteral("\x1d"));       // replace %I with italics char
        line.replace(QStringLiteral("%O"),QStringLiteral("\x0f"));       // replace %O with reset to default char
        line.replace(QStringLiteral("%S"),QStringLiteral("\x1e"));       // replace %S with strikethru char
        // line.replace(QRegExp("%?"),"\x15");
        line.replace(QStringLiteral("%R"),QStringLiteral("\x16"));       // replace %R with reverse char
        line.replace(QStringLiteral("%U"),QStringLiteral("\x1f"));       // replace %U with underline char
        line.replace(QStringLiteral("%M"),QStringLiteral("\x11"));       // replace %M with monospace char
        line.replace(QStringLiteral("%\x01"),QStringLiteral("%"));       // restore double %% as single %

        return line;
    }

    QString replaceIRCMarkups(const QString& text)
    {
        QString line(text);

        line.replace(QLatin1Char('\x02'), QStringLiteral("%B"));      // replace bold char with %B
        line.replace(QLatin1Char('\x03'), QStringLiteral("%C"));       // replace color char with %C
        line.replace(QLatin1Char('\x07'), QStringLiteral("%G"));       // replace ASCII BEL 0x07 with %G
        line.replace(QLatin1Char('\x1d'), QStringLiteral("%I"));       // replace italics char with %I
        line.replace(QLatin1Char('\x0f'), QStringLiteral("%O"));       // replace reset to default char with %O
        // historic code
        line.replace(QLatin1Char('\x13'), QStringLiteral("%S"));       // replace strikethru char with %S
        // modern
        line.replace(QLatin1Char('\x1e'), QStringLiteral("%S"));       // replace strikethru char with %S
        line.replace(QLatin1Char('\x16'), QStringLiteral("%R"));       // replace reverse char with %R
        // underline char send by kvirc
        line.replace(QLatin1Char('\x1f'), QStringLiteral("%U"));       // replace underline char with %U
        // underline char send by mirc
        line.replace(QLatin1Char('\x15'), QStringLiteral("%U"));       // replace underline char with %U
        line.replace(QLatin1Char('\x11'), QStringLiteral("%M"));       // replace monospace char with %M

        return line;
    }

    QList<QPair<int, int> > getUrlRanges(const QString& text)
    {
        TextUrlData data = extractUrlData(text, false);

        return data.urlRanges;
    }

    QList< QPair< int, int > > getChannelRanges(const QString& text)
    {
        TextChannelData data = extractChannelData(text, false);

        return data.channelRanges;
    }

    TextUrlData extractUrlData(const QString& text, bool doUrlFixup)
    {
        TextUrlData data;
        QString htmlText(text);
        urlPattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

        int pos = 0;
        int urlLen = 0;

        QString protocol;
        QString href;

        QRegularExpressionMatch rmatch;
        while ((pos = htmlText.indexOf(urlPattern, pos, &rmatch)) >= 0)
        {
            urlLen = rmatch.capturedLength(0);
            href = htmlText.mid(pos, urlLen);

            data.urlRanges << QPair<int, int>(pos, href.length());
            pos += href.length();

            if (doUrlFixup)
            {
                protocol.clear();
                if (rmatch.captured(2).isEmpty())
                {
                    QString urlPatternCap1(rmatch.captured(1));
                    if (urlPatternCap1.contains(QLatin1Char('@')))
                        protocol = QStringLiteral("mailto:");
                    else if (urlPatternCap1.startsWith(QLatin1String("ftp."), Qt::CaseInsensitive))
                        protocol = QStringLiteral("ftp://");
                    else
                        protocol = QStringLiteral("http://");
                }

                href = protocol + removeIrcMarkup(href);
                data.fixedUrls.append(href);
            }
        }
        return data;
    }

    TextChannelData extractChannelData(const QString& text, bool doChannelFixup)
    {
        TextChannelData data;
        QString ircText(text);

        int pos = 0;
        int chanLen = 0;
        QString channel;

        QRegularExpressionMatch rmatch;
        while ((pos = ircText.indexOf(chanExp, pos, &rmatch)) >= 0)
        {
            channel = rmatch.captured(2);
            chanLen = channel.length();

            // we want the pos where #channel starts
            // indexIn gives us the first match and the first match may be
            // "#test", " #test" or " \"test", so the first Index is off by some chars
            pos = rmatch.capturedStart(2);

            data.channelRanges << QPair<int, int>(pos, chanLen);
            pos += chanLen;

            if (doChannelFixup)
            {
                channel = removeIrcMarkup(channel);
                channel = QString::fromLatin1(QUrl::toPercentEncoding(channel));
                data.fixedChannels.append(channel);
            }
        }
        return data;
    }

    bool isUrl(const QString& text)
    {
        QRegularExpression re(urlPattern);
        re.setPattern(QRegularExpression::anchoredPattern(urlPattern.pattern()));

        return re.match(text).hasMatch();
    }

    QString extractColorCodes(const QString& _text)
    {
        QString text(_text);
        int pos = 0;
        QString ret;
        while (pos < text.size()) {
            QRegularExpressionMatch match = colorRegExp.match(text, pos);
            if (!match.hasMatch()) {
                break;
            }
            ret += match.captured();
            pos = match.capturedStart();
            text.remove(pos, match.capturedLength());
        }
        return ret;
    }

    uint colorForNick(const QString& nickname)
    {
        int nickvalue = 0;

        for (QChar c : nickname) {
            nickvalue += c.unicode();
        }

        return (nickvalue % 8);
    }

    /// Replace invalid codepoints so the string can be converted to Utf8.
    /// @param s a const reference to the QString to copy and change
    /// @retval s new QString
    QString sterilizeUnicode(const QString& s)
    {
        QString copy(s);
        sterilizeUnicode(copy);
        return copy;
    }

    /// Replace invalid codepoints so the string can be converted to Utf8.
    /// @param s a reference to the QString to change, a reference so it works with m_inputbuffer.back() in server.cpp
    /// @retval s reference to the argument
    QString& sterilizeUnicode(QString& s)
    {
        // HACK work around undocumented requirement to vet Unicode text sent over DBUS.
        // strips noncharacters, the private use characters are presumably safe
        for (int i = 0; i < s.length(); ++i)
        {
            QChar c(s.at(i));
            if (c.category() == QChar::Other_Surrogate)
            {
                if (!c.isHighSurrogate() || (!(i+1 < s.length()) && !s.at(i+1).isLowSurrogate()))
                {
                    //stomp on this bad char now, next trip through the loop (if there is one) eats the other
                    s.replace(i, 1, QChar(0xFFFD));
                    continue;
                }

                QChar next = s.at(i+1);
                if ((next.unicode()&0x3FE) == 0x3FE && (c.unicode()&0x3F) == 0x3F)
                {
                    s.replace(i, 1, QChar(0xFFFD)); //its one of the last two of the plane, replace them
                    s.replace(i+1, 1, QChar(0xFFFD));
                }

                ++i; // skip the high surrogate now, the loop takes care of the low
            }
            else if ((c.category() == QChar::Other_NotAssigned) //perhaps Qt will use QChar::Other_NotAssigned some day
                || (c.unicode() >= 0xFDD0 && c.unicode() <= 0xFDEF) //Unicode class Cn on BMP only
                || (c.unicode() == 0xFFFE || (c.unicode() == 0xFFFF)) //Unicode class Cn on all planes
                )
            {
                s.replace(i, 1, QChar(0xFFFD));
            }
        }
        return s;
    }

    /// Run a QStringList through sterilizeUnicode
    /// @param list a reference to the list
    /// @retval list
    QStringList& sterilizeUnicode(QStringList& list)
    {
        for (QString& s : list)
            sterilizeUnicode(s);

        return list;
    }

    /// Copy the list argument and return it, filtered
    QStringList sterilizeUnicode(const QStringList& list)
    {
        QStringList out(list);
        return sterilizeUnicode(out);
    }

}
