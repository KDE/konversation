/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
  Copyright (C) 2006 Michael Kreitzer <mrgrim@gr1m.org>
*/

#include "common.h"
#include "application.h"
#include "config/preferences.h"

#include <QString>
#include <QRegExp>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>

#include "guess_ja.cpp"
#include "unicode.cpp"

namespace Konversation
{
    void initChanModesHash()
    {
        QHash<QChar,QString> myHash;

        myHash.insert('t', i18n("topic protection"));
        myHash.insert('n', i18n("no messages from outside"));
        myHash.insert('s', i18n("secret"));
        myHash.insert('i', i18n("invite only"));
        myHash.insert('p', i18n("private"));
        myHash.insert('m', i18n("moderated"));
        myHash.insert('k', i18n("password protected"));
        myHash.insert('a', i18n("anonymous"));
        myHash.insert('c', i18n("no colors allowed"));
        myHash.insert('l', i18n("user limit"));

        m_modesHash = myHash;
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
            QList<QPair<int, int> > urlRanges = getUrlRanges(text);

            if (urlRanges.isEmpty())
                return replaceFormattingCodes(text);
            else
            {
                QString line;

                QPair<int, int> pair;
                int startPos = 0;
                int length = 0;

                QListIterator<QPair<int, int> > i(urlRanges);

                while (i.hasNext())
                {
                    pair = i.next();

                    length = pair.first - startPos;

                    line += replaceFormattingCodes(text.mid(startPos, length));

                    startPos = pair.first + pair.second;

                    line += text.mid(pair.first, pair.second);
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
        line.replace("%%","%\x01");      // make sure to protect double %%
        line.replace("%B","\x02");       // replace %B with bold char
        line.replace("%C","\x03");       // replace %C with color char
        line.replace("%G","\x07");       // replace %G with ASCII BEL 0x07
        line.replace("%I","\x1d");       // replace %I with italics char
        line.replace("%O","\x0f");       // replace %O with reset to default char
        line.replace("%S","\x13");       // replace %S with strikethru char
        // line.replace(QRegExp("%?"),"\x15");
        line.replace("%R","\x16");       // replace %R with reverse char
        line.replace("%U","\x1f");       // replace %U with underline char
        line.replace("%\x01","%");       // restore double %% as single %

        return line;
    }

    QString replaceIRCMarkups(const QString& text)
    {
        QString line(text);

        line.replace('\x02', "%B");      // replace bold char with %B
        line.replace('\x03', "%C");       // replace color char with %C
        line.replace('\x07', "%G");       // replace ASCII BEL 0x07 with %G
        line.replace('\x1d', "%I");       // replace italics char with %I
        line.replace('\x0f', "%O");       // replace reset to default char with %O
        line.replace('\x13', "%S");       // replace strikethru char with %S
        line.replace('\x16', "%R");       // replace reverse char with %R
        // underline char send by kvirc
        line.replace('\x1f', "%U");       // replace underline char with %U
        // underline char send by mirc
        line.replace('\x15', "%U");       // replace underline char with %U

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
        urlPattern.setCaseSensitivity(Qt::CaseInsensitive);

        int pos = 0;
        int urlLen = 0;

        QString protocol;
        QString href;

        while ((pos = urlPattern.indexIn(htmlText, pos)) >= 0)
        {
            urlLen = urlPattern.matchedLength();
            href = htmlText.mid(pos, urlLen);

            data.urlRanges << QPair<int, int>(pos, href.length());
            pos += href.length();

            if (doUrlFixup)
            {
                protocol.clear();
                if (urlPattern.cap(2).isEmpty())
                {
                    QString urlPatternCap1(urlPattern.cap(1));
                    if (urlPatternCap1.contains('@'))
                        protocol = "mailto:";
                    else if (urlPatternCap1.startsWith(QLatin1String("ftp."), Qt::CaseInsensitive))
                        protocol = "ftp://";
                    else
                        protocol = "http://";
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

        while ((pos = chanExp.indexIn(ircText, pos)) >= 0)
        {
            channel = chanExp.cap(2);
            chanLen = channel.length();

            // we want the pos where #channel starts
            // indexIn gives us the first match and the first match may be
            // "#test", " #test" or " \"test", so the first Index is off by some chars
            pos = chanExp.pos(2);

            data.channelRanges << QPair<int, int>(pos, chanLen);
            pos += chanLen;

            if (doChannelFixup)
            {
                channel = removeIrcMarkup(channel);
                data.fixedChannels.append(channel);
            }
        }
        return data;
    }

    bool isUrl(const QString& text)
    {
        return urlPattern.exactMatch(text);
    }

    QString extractColorCodes(const QString& _text)
    {
        QString text(_text);
        int pos = 0;
        QString ret;
        QString match;
        while ((pos = colorRegExp.indexIn(text, pos)) >= 0)
        {
            match = colorRegExp.cap(0);
            ret += match;
            text.remove(pos, match.length());
        }
        return ret;
    }

    QPixmap overlayPixmaps( const QPixmap &under, const QPixmap &over )
    {
        if (over.isNull() && under.isNull())
                return QPixmap();
        else if (under.isNull())
            return QPixmap(over);
        else if (over.isNull())
            return QPixmap(under);

        QPixmap result(under);
        QPainter painter(&result);
        painter.drawPixmap(QPoint(0,0), over);
        return result;
    }

    uint colorForNick(const QString& nickname)
    {
        int nickvalue = 0;

        for (int index = 0; index < nickname.length(); index++)
        {
            nickvalue += nickname[index].unicode();
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
        for (int i = 0; i < s.length(); ++i)
        {
            QChar c(s.at(i));
            if ((c.category() == QChar::Other_NotAssigned) //perhaps Qt will use QChar::Other_NotAssigned some day
                || (c.unicode() >= 0xFDD0 && c.unicode() <= 0xFDEF) //Unicode class Cn on BMP only
                || (c.unicode() == 0xFFFE || (c.unicode() == 0xFFFF)) //Unicode class Cn on all planes
                || (c.category() == QChar::Other_Surrogate)
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
        for (int i = 0; i < list.count(); ++i)
            sterilizeUnicode(list[i]);

        return list;
    }

    /// Copy the list argument and return it, filtered
    QStringList sterilizeUnicode(const QStringList& list)
    {
        QStringList out(list);
        return sterilizeUnicode(out);
    }

}
