/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircview.cpp  -  description
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

#include <kdebug.h>

#include "konversationapplication.h"
#include "ircview.h"
#include "highlight.h"

IRCView::IRCView(QWidget* parent) : KTextBrowser(parent)
{
  kdDebug() << "IRCView::IRCView()" << endl;

  setVScrollBarMode(AlwaysOn);
  setHScrollBarMode(AlwaysOff);

  installEventFilter(this);

#ifndef TABLE_VERSION
  setText("<qt>\n");
#endif
}

IRCView::~IRCView()
{
  kdDebug() << "IRCView::~IRCView()" << endl;
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

QString IRCView::filter(const QString& line,bool doHilight)
{
  QString filteredLine(line);

  /* Replace all & with &amp; */
  filteredLine.replace(QRegExp("&"),"&amp;");
  /* Replace all < with &lt; */
  filteredLine.replace(QRegExp("\\<"),"&lt;");
  /* Replace all > with &gt; */
  filteredLine.replace(QRegExp("\\>"),"&gt;");
  /* Replace all 0x0f (reset color) with \0x031,0 */
  filteredLine.replace(QRegExp("\017"),"\0031,0;");
//  while((pos=filteredLine.find('\017'))!=-1) filteredLine.replace(pos,1,"\0031,0");

  /* replace \003 codes with rich text color codes */
  /* TODO: use QRegExp for this */

  /* How many chars to replace? */
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

    /* TODO: make these configurable */
    const char* colorCodes[]={"ffffff","000000","000080","008000","ff0000","a52a2a","800080","ff8000",
                              "808000","00ff00","008080","00ffff","0000ff","ffc0cb","a0a0a0","c0c0c0"};

    /* remove leading \003 */
    filteredLine.replace(pos,1,"");
    replace=0;
    colorString="";

    colChar=filteredLine[digitPos];          /* get first char */
    if(colChar.isDigit())                    /* is this a digit? */
    {
      foregroundColor=colChar.digitValue();  /* take this digit as color */
      replace++;

      colChar=filteredLine[++digitPos];      /* get next char */
      if(foregroundColor<2)                  /* maybe a two digit color? */
      {
        if(colChar.isDigit())                /* is this a digit? */
        {
          if(colChar.digitValue()<6)         /* would this be a color from 10 to 15?  */
          {
            foregroundColor=foregroundColor*10+colChar.digitValue();
            replace++;
            colChar=filteredLine[++digitPos];  /* get next char */
          }
        }
      }
    }
    if(colChar==',')
    {
      replace++;
      colChar=filteredLine[++digitPos];        /* get first char */
      if(colChar.isDigit())                    /* is this a digit? */
      {
        backgroundColor=colChar.digitValue();  /* take this digit as color */
        replace++;

        if(backgroundColor<2)                  /* maybe a two digit color? */
        {
          colChar=filteredLine[++digitPos];    /* get next char */
          if(colChar.isDigit())                /* is this a digit? */
          {
            if(colChar.digitValue()<6)         /* would this be a color from 10 to 15?  */
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

  /* Replace all text decorations */
  replaceDecoration(filteredLine,'\x02','b');
  replaceDecoration(filteredLine,'\x09','i');
//  replaceDecoration(filteredLine,'\x13',"strikethrough");
  replaceDecoration(filteredLine,'\x15','u');
  replaceDecoration(filteredLine,'\x1f','u');

  /* Hilight */
  if(doHilight)
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
    }
  }

  /* URL Catcher */
  QString linkMessageColor = KonversationApplication::preferences.getLinkMessageColor();

  QRegExp pattern("((http://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
                  "([\\.@%a-z_-])+\\.[a-z]{2,}(/[^)>\"'!\\s]*)?|"
                  /* eDonkey2000 links need special treatment */
                  "ed2k://\\|([^|]+\\|){4})");

  pos=0;
  while(pattern.search(filteredLine,pos)!=-1)
  {
    /* Remember where we found the url */
    pos=pattern.pos();

    /* Extract url */
    QString url=pattern.capturedTexts()[0];
    QString href(url);

    /* clean up href for browser */
    if(href.startsWith("www.")) href="http://"+href;
    else if(href.startsWith("ftp.")) href="ftp://"+href;

    /* Fix &amp; back to & in href ... kludgy but I don't know a better way. */
    href.replace(QRegExp("&amp;"),"&");
    /* Replace all spaces with %20 in href */
    href.replace(QRegExp(" "),"%20");
    /* Build rich text link */
    QString link("<font color=\"#"+linkMessageColor+"\"><a href=\""+href+"\">"+url+"</a></font>");

    /* replace found url with built link */
    filteredLine.replace(pos,url.length(),link);
    /* next search begins right after the link */
    pos+=link.length();
    /* tell the program that we have found a new url */
    emit newURL(url);
  }

  /*

***** Old URL Catcher. Remove when new routine proves itself stable *****

  QString urlString=filteredLine;

  QStringList matchList;
  matchList.append("http://");
  matchList.append("www.");
  matchList.append("ftp://");
  matchList.append("ftp.");
  matchList.append("news://");
  matchList.append("nntp://");
  matchList.append("gopher://");
  matchList.append("ed2k://");

  bool foundSomething;

  pos=0;
  do
  {
    foundSomething=false;

    for(int index=0;matchList[index]!=0;index++)
    {
      pos=urlString.find(matchList[index]);
      if(pos!=-1)
      {
        int end=urlString.find(' ',pos);

        if(end==-1) end=urlString.length();
        int len=end-pos;

        QString url=urlString.mid(pos,len);

        // Try to clean up URLs by cutting off rightmost "junk"
        QRegExp smartChars("[().,>=\"'-]$");
        while(url.find(smartChars)!=-1) url=url.left(url.length()-1);

        // Remove URL from search string
        urlString.replace(pos,url.length(),"");

        QString link="<font color=\"#"+linkMessageColor+"\"><u><a href=\"";
        if(url.startsWith("www")) link+="http://";
        else if(url.startsWith("ftp")) link+="ftp://";

        // Fix &amp; back to & in link ... kludgy but I don't know a better way.
        link+=url;
        while((pos=link.find("&amp;"))!=-1) link.replace(pos,5,"&");

        link.append("\">"+url+"</a></u></font>");
        // Replace link in original line
        pos=filteredLine.find(url);
        filteredLine.replace(pos,url.length(),link);

        emit newURL(url);

        foundSomething=true;
      }
    }
  } while(foundSomething);
*/
  /* Replace multiple Spaces with "<space>&nbsp;" */
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
  QString line=QString("<tr><td><font color=\"#"+channelMessageColor()+"\">%1:</font></td><td><font color=\"#"+channelMessageColor()+"\">%2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+channelMessageColor+"\">&lt;%1&gt; %2</font><br>\n").arg(filter(nick,false)).arg(filter(message));
#endif

  emit textToLog(QString("%1:\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendQuery(const char* nick,const char* message)
{
  QString queryMessageColor = KonversationApplication::preferences.getQueryMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+queryMessageColor+"\">*%1*</font></td><td><font color=\"#"+queryMessageColor+"\">%2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+queryMessageColor+"\">*%1* %2</font><br>\n").arg(filter(nick,false)).arg(filter(message));
#endif

  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const char* nick,const char* message)
{
  QString actionMessageColor = KonversationApplication::preferences.getActionMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td>&nbsp;</td><td><font color=\"#"+actionMessageColor+"\">* %1 %2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+actionMessageColor+"\">* %1 %2</font><br>\n").arg(filter(nick,false)).arg(filter(message));
#endif

  emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendServerMessage(const char* type,const char* message)
{
  QString serverMessageColor = KonversationApplication::preferences.getServerMessageColor();

  /* Fixed width font option for MOTD */
  /* TODO: Make this configurable */
  QString motd("MOTD");
  QString fixed;
  if(motd==type) fixed=" face=\"courier\"";
#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+serverMessageColor+"\">%1</font></td><td><font color=\"#"+serverMessageColor+"\""+fixed+">%2</font></td></tr>\n").arg(type).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+serverMessageColor+"\""+fixed+">[%1] %2</font></td></tr><br>\n").arg(type).arg(filter(message));
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
  QString line=QString("<font color=\"#"+commandMessageColor+"\">*** %2</font><br>\n").arg(filter(message));
#endif
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendBacklogMessage(const char* firstColumn,const char* message)
{
  QString backlogMessageColor = KonversationApplication::preferences.getBacklogMessageColor();

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+backlogMessageColor+"\">%1</font></td><td><font color=\"#"+backlogMessageColor+"\">%2</font></td></tr>\n").arg(firstColumn).arg(filter(message));
#else
  QString line=QString("<font color=\"#"+backlogMessageColor+"\">%1 %2</font><br>\n").arg(firstColumn).arg(filter(message));
#endif

  doAppend(line);
}

void IRCView::doAppend(QString line)
{
  /* Add line to buffer */
  buffer+=line;
  emit newText();

#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#else
  KTextBrowser::append(line);
#endif
  // contentsHeight() seems to return wrong values when the widget is hidden
  ensureVisible(0,contentsHeight());
}

/* Workaround to scroll to the end of the TextView when it's shown */
void IRCView::showEvent(QShowEvent* event)
{
  /* Suppress Compiler Warning */
  event->type();

#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#endif
  ensureVisible(0,contentsHeight());
  /* Set focus to input line (must be connected) */
  emit gotFocus();
}

void IRCView::focusInEvent(QFocusEvent* event)
{
  /* Suppress Compiler Warning */
  event->type();
  /* Set focus to input line (must be connected) */
  emit gotFocus();
}

/* Copy selected text into clipboard immediately */
bool IRCView::eventFilter(QObject* object,QEvent* event)
{
  if(event->type()==QEvent::MouseButtonRelease)
  {
    if(hasSelectedText()) copy();
  }

  return KTextBrowser::eventFilter(object,event);
}
