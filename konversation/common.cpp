/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#include "common.h"

#include <qstring.h>
#include <qregexp.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpainter.h>

#include "konversationapplication.h"
#include "preferences.h"

namespace Konversation
{

QString removeIrcMarkup(const QString& text)
{
  QString escaped = text;
  QRegExp colorRegExp("((\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)|\x02|\x09|\x13|\x16|\x1f)");
  // Escape text decoration
  escaped.remove(colorRegExp);

  // Replace Mirc's 0x03 characters too, they show up as rectangles
  escaped.remove(QChar(0x03));

  return escaped;
}

QString tagURLs(const QString& text, const QString& fromNick)
{
  QString filteredLine = text;
  QString linkColor = KonversationApplication::preferences.getColor("LinkMessage");
  int pos = 0;

  QRegExp channelPattern("^#(\\S)+|"
			 "\\s#(\\S)+"
			 );
  
  channelPattern.setCaseSensitive(false);
  
  while(channelPattern.search(filteredLine, pos) != -1) {
    
    // Remember where we found the url
    pos = channelPattern.pos();
    
    // Extract channel
    QString channel = channelPattern.capturedTexts()[0];
    QString space;
    
    QString href(channel.stripWhiteSpace());
    if(href.length() != channel.length())
      space=" "; // We eated some space so we will put it before channel link
    
    href = "#" + href;
    QString link = "<font color=\"#" + linkColor + "\">"+space+"<a href=\"" + href + "\">" + channel.stripWhiteSpace() + "</a></font>";
    
    filteredLine.replace(pos,channel.length(),link);
    pos += link.length();
    
  }
  pos = 0;
    
  QRegExp urlPattern("(((http://|https://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
                  "(([-_.%\\d\\w]*(:[-_.%\\d\\w]*)?@)|)"
                  // IP Address
                  "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|"
                  // Decimal IP address
                  "[0-9]{1,12}|"
                  // Standard host name
                  "[a-z0-9][\\.%a-z0-9_-]+\\.[a-z]{2,}"
                  // Port number, path to document
                  ")(:[0-9]{1,5})?((:|)(/[^>\"\\s]*))?|"
                  // eDonkey2000 links need special treatment
                  "ed2k://\\|([^|]+\\|){4})|"
		  "(mailto:|)((([a-z]|\\d)+[\\w\\x2E\\x2D]+)\\x40([\\w\\x2E\\x2D]{2,})\\x2E(\\w{2,})))"
		  );

  urlPattern.setCaseSensitive(false);
  KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

  while(urlPattern.search(filteredLine, pos) != -1) {
      // Remember where we found the url
      pos = urlPattern.pos();

      // Extract url
      QString url = urlPattern.capturedTexts()[0];
      QString href(url);

      // clean up href for browser
      if(href.startsWith("www.")) href = "http://" + href;
      else if(href.startsWith("ftp.")) href = "ftp://" + href;
      else if(href.find(QRegExp("(([a-z]+[\\w\\x2E\\x2D]+)\\x40)")) == 0) href = "mailto:" + href;
  
      // Fix &amp; back to & in href ... kludgy but I don't know a better way.
      href.replace("&amp;", "&");
      // Replace all spaces with %20 in href
      href.replace(" ", "%20");
      // Build rich text link
      QString link = "<font color=\"#" + linkColor + "\"><u><a href=\"" + href + "\">" + url + "</a></u></font>";

      // replace found url with built link
      filteredLine.replace(pos, url.length(), link);
      // next search begins right after the link
      pos += link.length();
      // tell the program that we have found a new url

      konvApp->storeUrl(fromNick, href);
  }

  return filteredLine;
}

//TODO: there's room for optimization as pahlibar said. (strm)

// the below two functions were taken from kopeteonlinestatus.cpp.
QBitmap overlayMasks( const QBitmap *under, const QBitmap *over )
{
  if ( !under && !over ) return QBitmap();
  if ( !under ) return *over;
  if ( !over ) return *under;

  QBitmap result = *under;
  bitBlt( &result, 0, 0, over, 0, 0, over->width(), over->height(), Qt::OrROP );
  return result;
}

QPixmap overlayPixmaps( const QPixmap &under, const QPixmap &over )
{
  if ( over.isNull() ) return under;

  QPixmap result = under;
  result.setMask( overlayMasks( under.mask(), over.mask() ) );

  QPainter p( &result );
  p.drawPixmap( 0, 0, over );
  return result;
}

}
