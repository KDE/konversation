/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircview.cpp  -  the text widget used for all text based panels
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

// Comment this #define to try a different text widget
// #define TABLE_VERSION

#include <qstylesheet.h>
#include <qtextcodec.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>

// Check if we use special QT versions to keep text widget from displaying
// all lines after another without line breaks
#if QT_VERSION == 303
#define ADD_LINE_BREAKS
#endif

#include <kdebug.h>

#include "konversationapplication.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"

IRCView::IRCView(QWidget* parent,Server* newServer) : KTextBrowser(parent)
{
  kdDebug() << "IRCView::IRCView()" << endl;

  setVScrollBarMode(AlwaysOn);
  setHScrollBarMode(AlwaysOff);

  installEventFilter(this);

  setServer(newServer);
  setFont(KonversationApplication::preferences.getTextFont());

#ifndef TABLE_VERSION
  setText("<qt>\n");
#endif
}

IRCView::~IRCView()
{
  kdDebug() << "IRCView::~IRCView()" << endl;
}

void IRCView::setServer(Server* newServer)
{
  server=newServer;
}

void IRCView::clear()
{
  buffer="";
  KTextBrowser::clear();
}

void IRCView::replaceDecoration(QString& line,char decoration,char replacement)
{
  int pos;
  bool decorated=false;

  while((pos=line.find(decoration))!=-1)
  {
    line.replace(pos,1,(decorated) ? QString("</%1>").arg(replacement) : QString("<%1>").arg(replacement));
    decorated=!decorated;
  }
}

QString IRCView::filter(const QString& line,const QString& whoSent,bool doHilight)
{
  QString filteredLine(line);

  // make sure that own lines don't always get highlight
  QString who;
  if(whoSent!=server->getNickname() || KonversationApplication::preferences.getHilightOwnLines()) who=whoSent;

  // Replace all & with &amp;
  filteredLine.replace(QRegExp("&"),"&amp;");
  // Replace all < with &lt;
  filteredLine.replace(QRegExp("\\<"),"&lt;");
  // Replace all > with &gt;
  filteredLine.replace(QRegExp("\\>"),"&gt;");
  // Replace all 0x0f (reset color) with \0x031,0
  filteredLine.replace(QRegExp("\017"),"\0031,0");

  // replace \003 codes with rich text color codes
  // TODO: use QRegExp for this

  // How many chars to replace?
  int replace;
  bool firstColor=true;
  QChar colChar;
  QString colorString;

  int pos;
  while((pos=filteredLine.find('\003'))!=-1)
  {
    int digitPos=pos;
    int foregroundColor=1;
    int backgroundColor=0;

    // TODO: make these configurable
    const char* colorCodes[]={"ffffff","000000","000080","008000","ff0000","a52a2a","800080","ff8000",
                              "808000","00ff00","008080","00ffff","0000ff","ffc0cb","a0a0a0","c0c0c0"};

    // remove leading \003
    filteredLine.replace(pos,1,"");
    replace=0;
    colorString="";

    colChar=filteredLine[digitPos];          // get first char
    if(colChar.isDigit())                    // is this a digit?
    {
      foregroundColor=colChar.digitValue();  // take this digit as color
      replace++;

      colChar=filteredLine[++digitPos];      // get next char
      if(foregroundColor<2)                  // maybe a two digit color?
      {
        if(colChar.isDigit())                // is this a digit?
        {
          if(colChar.digitValue()<6)         // would this be a color from 10 to 15?
          {
            foregroundColor=foregroundColor*10+colChar.digitValue();
            replace++;
            colChar=filteredLine[++digitPos];  // get next char
          }
        }
      }
    }
    if(colChar==',')
    {
      replace++;
      colChar=filteredLine[++digitPos];        // get first char
      if(colChar.isDigit())                    // is this a digit?
      {
        backgroundColor=colChar.digitValue();  // take this digit as color
        replace++;

        if(backgroundColor<2)                  // maybe a two digit color?
        {
          colChar=filteredLine[++digitPos];    // get next char
          if(colChar.isDigit())                // is this a digit?
          {
            if(colChar.digitValue()<6)         // would this be a color from 10 to 15?
            {
              backgroundColor=backgroundColor*10+colChar.digitValue();
              replace++;
            }
          }
        }
      }
    }

    colorString=(firstColor) ? "" : "</font>";
    colorString+="<font color=\"#"+QString(colorCodes[foregroundColor])+"\">";

    filteredLine.replace(pos,replace,colorString);
    firstColor=false;
  } // while

  if(!firstColor) filteredLine+="</font>";

  // Replace all text decorations
  replaceDecoration(filteredLine,'\x02','b');
  replaceDecoration(filteredLine,'\x09','i');
  replaceDecoration(filteredLine,'\x13','u'); // should be strikethru
  replaceDecoration(filteredLine,'\x15','u');
  replaceDecoration(filteredLine,'\x16','b'); // should be reverse
  replaceDecoration(filteredLine,'\x1f','u');

  // URL Catcher
  QString linkMessageColor = KonversationApplication::preferences.getLinkMessageColor();

  QRegExp pattern("((http://|https://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
                  // IP Address
                  "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|"
                  // Decimal IP address
                  "[0-9]{1,12}|"
                  // Standard host name
                  "([\\.@%a-z0-9_-])+\\.[a-z]{2,}"
                  // Port number, path to document
                  ")(:[0-9]{1,5})?(/[^)>\"'\\s]*)?|"
                  // eDonkey2000 links need special treatment
                  "ed2k://\\|([^|]+\\|){4})");

  pattern.setCaseSensitive(false);

  pos=0;
  while(pattern.search(filteredLine,pos)!=-1)
  {
    // Remember where we found the url
    pos=pattern.pos();

    // Extract url
    QString url=pattern.capturedTexts()[0];
    QString href(url);

    // clean up href for browser
    if(href.startsWith("www.")) href="http://"+href;
    else if(href.startsWith("ftp.")) href="ftp://"+href;

    // Fix &amp; back to & in href ... kludgy but I don't know a better way.
    href.replace(QRegExp("&amp;"),"&");
    // Replace all spaces with %20 in href
    href.replace(QRegExp(" "),"%20");
    // Build rich text link
    QString link("<font color=\"#"+linkMessageColor+"\"><a href=\""+href+"\">"+url+"</a></font>");

    // replace found url with built link
    filteredLine.replace(pos,url.length(),link);
    // next search begins right after the link
    pos+=link.length();
    // tell the program that we have found a new url
    emit newURL(url);
  }

  // Hilight
  if(doHilight)
  {
    if(KonversationApplication::preferences.getHilightNick() &&
       filteredLine.lower().find(server->getNickname().lower())!=-1)
    {
      QColor hilightNickColor=KonversationApplication::preferences.getHilightNickColor();
      filteredLine=QString("<font color=\""+hilightNickColor.name()+"\">")+filteredLine+QString("</font>");
    }
    else
    {
      QPtrList<Highlight> hilightList=KonversationApplication::preferences.getHilightList();
      unsigned int index;

      for(index=0;index<hilightList.count();index++)
      {
        QString needle=hilightList.at(index)->getText().lower();
        if(filteredLine.lower().find(needle)!=-1)
        {
          filteredLine=QString("<font color=\""+hilightList.at(index)->getColor().name()+"\">")+filteredLine+QString("</font>");
          break;
        }
        if(who!=NULL && who.lower().find(needle)!=-1)
        {
          filteredLine=QString("<font color=\""+KonversationApplication::preferences.getHilightOwnLinesColor().name()+"\">")+filteredLine+QString("</font>");
          break;
        }
     } // endfor
    }
  }

  // Replace multiple Spaces with "<space>&nbsp;"
  do
  {
    pos=filteredLine.find("  ");
    if(pos!=-1) filteredLine.replace(pos+1,1,"&nbsp;");
  } while(pos!=-1);

  return filteredLine;
}

void IRCView::append(const char* nick,const char* message)
{
  QString channelMessageColor = KonversationApplication::preferences.getChannelMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+channelMessageColor()+"\">%1:</font></td><td><font color=\"#"+channelMessageColor()+"\">%2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+channelMessageColor+"\"><b>&lt;%1&gt;</b> %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick));
#else
  QString line=QString("<font color=\"#"+channelMessageColor+"\"><b>&lt;%1&gt;</b> %2</font>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("%1:\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendQuery(const char* nick,const char* message)
{
  QString queryMessageColor = KonversationApplication::preferences.getQueryMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+queryMessageColor+"\">*%1*</font></td><td><font color=\"#"+queryMessageColor+"\">%2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+queryMessageColor+"\"><b>*%1*</b> %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
  QString line=QString("<font color=\"#"+queryMessageColor+"\"><b>*%1*</b> %2</font>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const char* nick,const char* message)
{
  QString actionMessageColor = KonversationApplication::preferences.getActionMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td>&nbsp;</td><td><font color=\"#"+actionMessageColor+"\">* %1 %2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+actionMessageColor+"\">* %1 %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
  QString line=QString("<font color=\"#"+actionMessageColor+"\">* %1 %2</font>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendServerMessage(const char* type,const char* message)
{
  QString serverMessageColor = KonversationApplication::preferences.getServerMessageColor();

  // Fixed width font option for MOTD
  QString fixed;
  if(KonversationApplication::preferences.getFixedMOTD())
  {
    if(QString("MOTD")==type) fixed=" face=\"courier\"";
  }

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+serverMessageColor+"\">%1</font></td><td><font color=\"#"+serverMessageColor+"\""+fixed+">%2</font></td></tr>\n").arg(type).arg(filter(message));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+serverMessageColor+"\""+fixed+"><b>[%1]</b> %2</font><br>\n").arg(type).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+serverMessageColor+"\""+fixed+"><b>[%1]</b> %2</font>\n").arg(type).arg(filter(message));
#endif
#endif
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendCommandMessage(const char* type,const char* message)
{
  QString commandMessageColor = KonversationApplication::preferences.getCommandMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+commandMessageColor+"\">%1</font></td><td><font color=\"#"+commandMessageColor+"\">%2</font></td></tr>\n").arg(type).arg(filter(message));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+commandMessageColor+"\">*** %2</font><br>\n").arg(filter(message));
#else
  QString line=QString("<font color=\"#"+commandMessageColor+"\">*** %2</font>\n").arg(filter(message));
#endif
#endif
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendBacklogMessage(const char* firstColumn,const char* rawMessage)
{
  QString time;
  QString message(rawMessage);
  QString backlogMessageColor = KonversationApplication::preferences.getBacklogMessageColor();

  // extract timestamp from message string
  if(message.startsWith("["))
  {
    time=message.section(' ',0,0);
    message=message.section(' ',1);
  }

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+backlogMessageColor+"\">%1</font></td><td><font color=\"#"+backlogMessageColor+"\">%2 %3</font></td></tr>\n").arg(time).arg(firstColumn).arg(filter(message,NULL,false));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+backlogMessageColor+"\">%1 %2 %3</font><br>\n").arg(time).arg(firstColumn).arg(filter(message,NULL,false));
#else
  QString line=QString("<font color=\"#"+backlogMessageColor+"\">%1 %2 %3</font>\n").arg(time).arg(firstColumn).arg(filter(message,NULL,false));
#endif
#endif

  doAppend(line);
}

void IRCView::doAppend(QString line)
{
  kdDebug() << "IRCView::doAppend("<< line << ")" << endl;
  // Add line to buffer
  buffer+=line;
  emit newText();

  // scroll view only if the scroll bar is already at the bottom
#if QT_VERSION == 303
  bool doScroll=true;
#else
  bool doScroll=((contentsHeight()-visibleHeight())==contentsY());
#endif

#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#else
  KTextBrowser::append(line);
#endif

  if(doScroll) ensureVisible(0,contentsHeight());
}

// Workaround to scroll to the end of the TextView when it's shown
void IRCView::showEvent(QShowEvent* event)
{
  kdDebug() << "IRCView::showEvent()" << endl;
  // Suppress Compiler Warning
  event->type();

#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#endif
  ensureVisible(0,contentsHeight());
  // Set focus to input line (must be connected)
  emit gotFocus();
}

void IRCView::focusInEvent(QFocusEvent* event)
{
  // Suppress Compiler Warning
  event->type();
  // Set focus to input line (must be connected)
  emit gotFocus();
}

// Copy selected text into clipboard immediately
bool IRCView::eventFilter(QObject* object,QEvent* event)
{
  if(event->type()==QEvent::MouseButtonRelease)
  {
    if(hasSelectedText()) copy();
  }

  return KTextBrowser::eventFilter(object,event);
}
