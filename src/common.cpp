/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
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

    static QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
    static QRegExp urlPattern("((www\\.(?!\\.)|(fish|irc|amarok|(f|sf|ht)tp(|s))://)(\\.?[\\d\\w/,\\':~\\?=;#@\\-\\+\\%\\*\\{\\}\\!\\(\\)\\[\\]]|&)+)|"
        "([-.\\d\\w]+@[-.\\d\\w]{2,}\\.[\\w]{2,})");
    static QRegExp tdlPattern("(.*)\\.(\\w+),$");

    QHash<QChar,QString> initChanModesHash()
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
        myHash.insert('r', i18n("server reop"));
        myHash.insert('c', i18n("no colors allowed"));
        myHash.insert('l', i18n("user throttling"));

        return myHash;
    }

    const QHash<QChar,QString> ChanModes::m_hash = initChanModesHash();

    QHash<QChar,QString> getChannelModesHash()
    {
        return ChanModes::m_hash;
    }

    QString removeIrcMarkup(const QString& text)
    {
        QString escaped = text;
        // Escape text decoration
        escaped.remove(colorRegExp);

        // Remove Mirc's 0x03 characters too, they show up as rectangles
        escaped.remove(QChar(0x03));

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
        line.replace("%I","\x09");       // replace %I with italics char
        line.replace("%O","\x0f");       // replace %O with reset to default char
        line.replace("%S","\x13");       // replace %S with strikethru char
        // line.replace(QRegExp("%?"),"\x15");
        line.replace("%R","\x16");       // replace %R with reverse char
        line.replace("%U","\x1f");       // replace %U with underline char
        line.replace("%\x01","%");       // restore double %% as single %

        return line;
    }

    QString tagUrls(const QString& text, const QString& fromNick, bool useCustomColor)
    {
        TextUrlData data = extractUrlData(text, fromNick, false, true, useCustomColor);

        return data.htmlText;
    }

    QList<QPair<int, int> > getUrlRanges(const QString& text)
    {
        TextUrlData data = extractUrlData(text, QString(), true, false, false);

        return data.urlRanges;
    }

    TextUrlData extractUrlData(const QString& text, const QString& fromNick, bool doUrlRanges,
        bool doHyperlinks, bool useCustomHyperlinkColor)
    {
        // QTime timer;
        // timer.start();

        TextUrlData data;
        data.htmlText = text;

        int pos = 0;
        int urlLen = 0;

        QString link;
        QString insertText;
        QString protocol;
        QString href;
        QString append;

        urlPattern.setCaseSensitivity(Qt::CaseInsensitive);

        if (doHyperlinks)
        {
            QString linkColor = Preferences::self()->color(Preferences::Hyperlink).name();

            if (useCustomHyperlinkColor)
                link = "<a href=\"#%1\" style=\"color:" + linkColor + "\">%2</a>";
            else
                link = "<a href=\"#%1\">%2</a>";

            if (data.htmlText.contains("#"))
            {
                QRegExp chanExp("(^|\\s|^\"|\\s\"|,|'|\\(|\\:|!|@|%|\\+)(#[^,\\s;\\)\\:\\/\\(\\<\\>]*[^.,\\s;\\)\\:\\/\\(\"\''\\<\\>])");

                while ((pos = chanExp.indexIn(data.htmlText, pos)) >= 0)
                {
                    href = chanExp.cap(2);
                    urlLen = href.length();
                    pos += chanExp.cap(1).length();

                    insertText = link.arg(href, href);
                    data.htmlText.replace(pos, urlLen, insertText);
                    pos += insertText.length();
                }
            }

            if (useCustomHyperlinkColor)
                link = "<a href=\"%1%2\" style=\"color:" + linkColor + "\">%3</a>";
            else
                link = "<a href=\"%1%2\">%3</a>";

            pos = 0;
            urlLen = 0;
        }

        while ((pos = urlPattern.indexIn(data.htmlText, pos)) >= 0)
        {
            urlLen = urlPattern.matchedLength();

            // check if the matched text is already replaced as a channel
            if (doHyperlinks && data.htmlText.lastIndexOf("<a", pos ) > data.htmlText.lastIndexOf("</a>", pos))
            {
                ++pos;
                continue;
            }

            protocol.clear();
            href = data.htmlText.mid(pos, urlLen);
            append.clear();

            // Don't consider trailing comma part of link.
            if (href.right(1) == ",")
            {
                href.truncate(href.length()-1);
                append = ',';
            }

            // Don't consider trailing semicolon part of link.
            if (href.right(1) == ";")
            {
                href.truncate(href.length()-1);
                append = ';';
            }

            // Don't consider trailing closing parenthesis part of link when
            // there's an opening parenthesis preceding the beginning of the
            // URL or there is no opening parenthesis in the URL at all.
            if (href.right(1) == ")" && (data.htmlText.mid(pos-1, 1) == "(" || !href.contains("(")))
            {
                href.truncate(href.length()-1);
                append.prepend(")");
            }

            if (doHyperlinks)
            {
                // Qt doesn't support (?<=pattern) so we do it here
                if ((pos > 0) && data.htmlText[pos-1].isLetterOrNumber())
                {
                    pos++;
                    continue;
                }

                if (urlPattern.cap(1).startsWith(QLatin1String("www."), Qt::CaseInsensitive))
                    protocol = "http://";
                else if (urlPattern.cap(1).isEmpty())
                    protocol = "mailto:";

                // Use \x0b as a placeholder for & so we can read them after changing all & in the normal text to &amp;
                insertText = link.arg(protocol, QString(href).replace('&', "\x0b"), href) + append;

                data.htmlText.replace(pos, urlLen, insertText);

                Application::instance()->storeUrl(fromNick, href, QDateTime::currentDateTime());
            }
            else
                insertText = href + append;

            if (doUrlRanges)
                data.urlRanges << QPair<int, int>(pos, href.length());

            pos += insertText.length();
        }

        if (doHyperlinks)
        {
            // Change & to &amp; to prevent html entities to do strange things to the text
            data.htmlText.replace('&', "&amp;");
            data.htmlText.replace("\x0b", "&");
        }

        // kDebug() << "Took (msecs) : " << timer.elapsed() << " for " << data.htmlText;

        return data;
    }

    bool isUrl(const QString& text)
    {
        return urlPattern.exactMatch(text);
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
            if (c.category() == QChar::Other_Surrogate)
            {
                if (!c.isHighSurrogate() || (!(i+1 < s.length()) && !s.at(i+1).isLowSurrogate()))
                    Q_ASSERT("something let a bad surrogate pair through! send the backtrace, tell us how it happened");

                QChar next = s.at(i+1);
                if ((next.unicode()&0x3FE) == 0x3FE && (c.unicode()&0x3F) == 0x3F)
                    s.replace(i, 2, QChar(0xFFFD)); //its one of the last two of the plane, replace it

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
