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
#include "application.h" ////// header renamed
#include "config/preferences.h"

#include <qstring.h>
#include <qregexp.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <klocale.h>


namespace Konversation
{

    #include "guess_ja.cpp"
    #include "unicode.cpp"

    static QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
    static QRegExp urlPattern("((www\\.(?!\\.)|(fish|irc|(f|sf|ht)tp(|s))://)(\\.?[\\d\\w/,\\':~\\?=;#@\\-\\+\\%\\*\\{\\}\\!\\(\\)]|&)+)|"
        "([-.\\d\\w]+@[-.\\d\\w]{2,}\\.[\\w]{2,})");
    static QRegExp tdlPattern("(.*)\\.(\\w+),$");

    QString removeIrcMarkup(const QString& text)
    {
        QString escaped = text;
        // Escape text decoration
        escaped.remove(colorRegExp);

        // Remove Mirc's 0x03 characters too, they show up as rectangles
        escaped.remove(QChar(0x03));

        return escaped;
    }

    QString tagURLs(const QString& text, const QString& fromNick, bool useCustomColor)
    {
        // QTime timer;
        // timer.start();

        QString filteredLine = text;
        QString linkColor = Preferences::self()->color(Preferences::Hyperlink).name();
        QString link;
        QString insertText;
        int pos = 0;
        int urlLen = 0;
        QString href;

        if(useCustomColor)
        {
            link = "<font color=\""+linkColor+"\"><a href=\"#%1\">%2</a></font>";
        }
        else
        {
            link = "<a href=\"#%1\">%2</a>";
        }

        if(filteredLine.contains("#"))
        {
            QRegExp chanExp("(^|\\s|^\"|\\s\"|,|'|\\(|\\:|!|@|%|\\+)(#[^,\\s;\\)\\:\\/\\(\\<\\>]*[^.,\\s;\\)\\:\\/\\(\"\''\\<\\>])");
            while ((pos = chanExp.indexIn(filteredLine, pos)) >= 0)
            {
                href = chanExp.cap(2);
                urlLen = href.length();
                pos += chanExp.cap(1).length();

                // HACK:Use space as a placeholder for \ as Qt tries to be clever and does a replace to / in urls in QTextEdit
                insertText = link.arg(QString(href).replace('\\', " "), href);
                filteredLine.replace(pos, urlLen, insertText);
                pos += insertText.length();
            }
        }

        pos = 0;
        urlLen = 0;

        urlPattern.setCaseSensitivity(Qt::CaseInsensitive);
        QString protocol;

        // FIXME this should probably go away with the text control upgrade
        if(useCustomColor)
        {
            link = QString("<font color=\"" + linkColor + "\"><u><a href=\"%1%2\">%3</a></u></font>");
        }
        else
        {
            link = QString("<u><a href=\"%1%2\">%3</a></u>");
        }

        while ((pos = urlPattern.indexIn(filteredLine, pos)) >= 0)
        {
            QString append;

            // check if the matched text is already replaced as a channel
            if ( filteredLine.lastIndexOf( "<a", pos ) > filteredLine.lastIndexOf( "</a>", pos ) )
            {
                ++pos;
                continue;
            }

            protocol="";
            urlLen = urlPattern.matchedLength();
            href = filteredLine.mid( pos, urlLen );

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
            if (href.right(1) == ")" && (filteredLine.mid(pos-1,1) == "(" || !href.contains("(")))
            {
                href.truncate(href.length()-1);
                append.prepend(")");
            }

            // Qt doesn't support (?<=pattern) so we do it here
            if((pos > 0) && filteredLine[pos-1].isLetterOrNumber())
            {
                pos++;
                continue;
            }

            if (urlPattern.cap(1).startsWith(QLatin1String("www."), Qt::CaseInsensitive))
                protocol = "http://";
            else if (urlPattern.cap(1).isEmpty())
                protocol = "mailto:";

            // Use \x0b as a placeholder for & so we can readd them after changing all & in the normal text to &amp;
            // HACK Replace % with \x03 in the url to keep Qt from doing stupid things
            insertText = link.arg(protocol, QString(href).replace('&', "\x0b").replace('%', "\x03"), href) + append;
            filteredLine.replace(pos, urlLen, insertText);
            pos += insertText.length();
            KonversationApplication::instance()->storeUrl(fromNick, href);
        }

        // Change & to &amp; to prevent html entities to do strange things to the text
        filteredLine.replace('&', "&amp;");
        filteredLine.replace("\x0b", "&");

        // kDebug() << "Took (msecs) : " << timer.elapsed() << " for " << filteredLine;

        return filteredLine;
    }

    //TODO: there's room for optimization as pahlibar said. (strm)

    // the below two functions were taken from kopeteonlinestatus.cpp.
/*
    QBitmap overlayMasks( const QBitmap *under, const QBitmap *over )
    {
        if ( !under && !over ) return QBitmap();
        if ( !under ) return *over;
        if ( !over ) return *under;

        QBitmap result = *under;
        bitBlt( &result, 0, 0, over, 0, 0, over->width(), over->height(), Qt::OrROP );
        return result;
    }
*/
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
}
