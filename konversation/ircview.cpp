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

// #include <qlabel.h>
#include <qstylesheet.h>
#include <qtextcodec.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>

#include <kdebug.h>

#include "konversationapplication.h"
#include "ircview.h"

// IRCView::IRCView(QWidget* parent)
//IRCView::IRCView(QWidget* parent) : QScrollView(parent)
IRCView::IRCView(QWidget* parent) : KTextBrowser(parent)
{
//  grid=new QGrid(2,parent);


// ScrollView stuff
//  enableClipper(true);
//  grid=new QGrid(2,this->viewport());
//  addChild(grid);

// QListView stuff
//  addColumn("cerrNick");
//  addColumn("Text");

  kdDebug() << "IRCView::IRCView()" << endl;
  // setTextFormat(Qt::RichText);
  setVScrollBarMode(AlwaysOn);
  setHScrollBarMode(AlwaysOff);

  installEventFilter(this);

#ifndef TABLE_VERSION
//  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">\n"
//          "<tr><td>WWWWWWWWWW</td><td></td></tr>\n");
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
	QString linkMessageColor = KonversationApplication::preferences.getLinkMessageColor();
  QString filteredLine(line);

  /* -1 to make next search work (pos+1) */
  int pos=-1;
  /* Replace all & with &amp; */
  while((pos=filteredLine.find('&',pos+1))!=-1) filteredLine.insert(pos+1,"amp;");
  /* Replace all < with &lt; */
  while((pos=filteredLine.find('<'))!=-1) filteredLine.replace(pos,1,"&lt;");
  /* Replace all > with &gt; */
  while((pos=filteredLine.find('>'))!=-1) filteredLine.replace(pos,1,"&gt;");
  /* Replace all 0x0f (reset color) with \0x031,0 */
  /* TODO: place default fore/background colors here */
  while((pos=filteredLine.find('\017'))!=-1) filteredLine.replace(pos,1,"\0031,0");

  /* How many chars to replace? */
  int replace;
  bool firstColor=true;
  QChar colChar;
  QString colorString;

  while((pos=filteredLine.find('\003'))!=-1)
  {
    int digitPos=pos;
    int foregroundColor=1; /* TODO: replace with default foreground */
    int backgroundColor=0; /* TODO: replace with default background */
    const char* colorCodes[]={"ffffff","000000","000080","008000","ff0000","a52a2a","800080","ff8000",
                              "808000","00ff00","008080","00ffff","0000ff","ffc0cb","a0a0a0","c0c0c0"};
    /* remove \003 */

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
    QStringList hilightList=KonversationApplication::preferences.getHilightList();
    QString hilightColor=KonversationApplication::preferences.getHilightColor();

    unsigned int index;

    for(index=0;index<hilightList.count();index++)
    {
      QString needle=hilightList[index].lower();
      if(filteredLine.lower().find(needle)!=-1)
      {
        filteredLine=QString("<font color=\"#"+hilightColor+"\">")+filteredLine+QString("</font>");
        break;
      }
    }
  }

  /* URL Catcher matches */
  QStringList matchList;
  matchList.append("http://");
  matchList.append("www.");
  matchList.append("ftp://");
  matchList.append("news://");
  matchList.append("nntp://");
  matchList.append("gopher://");
  matchList.append("ed2k://");

  QString urlString=filteredLine;

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

        /* Try to clean up URLs by cutting off rightmost "junk" */
        QRegExp smartChars("[().,>=\"'-]$");
        while(url.find(smartChars)!=-1) url=url.left(url.length()-1);

        /* Remove URL from search string*/
        urlString.replace(pos,url.length(),"");

        QString link="<font color=\"#"+linkMessageColor+"\"><u><a href=\"";
        if(url.startsWith("www")) link+="http://";

        /* Fix &amp; back to & in link ... kludgy but I don't know a better way. */
        link+=url;
        while((pos=link.find("&amp;"))!=-1) link.replace(pos,5,"&");

        link.append("\">"+url+"</a></u></font>");
        /* Replace link in original line */
        pos=filteredLine.find(url);
        filteredLine.replace(pos,url.length(),link);

        emit newURL(url);

        foundSomething=true;
      }
    }
  } while(foundSomething);

  /* Replace multiple Spaces with "<space>&nbsp;" */
  do
  {
    pos=filteredLine.find("  ");
    if(pos!=-1) filteredLine.replace(pos+1,1,"&nbsp;");
  } while(pos!=-1);

//  cout << filteredLine << endl;
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
//  line="qwrtrwetzrtzu qqwrtrwetzrtzqwrtrwetzrtzu u wrtqwrtrweqwrt rwetzrqqwrtrqwrtrwetzrtzu wetzrtzu  wrtrwetzrqwrtrwetzrtzu tzu tzu tzrtzu qwrtrqwrtrwetzrtzu qwqwrtrwetzrqwrtrwetzrtzu tzu qwrtrwetzrtzuqwrtrwetzrtzu  rtrwetzrqwrtrwetzrtzu tzu wetqwrtrwetzrtzu zrtzu rqwrtrwetzrtzu wetzqwrqwrtrwetzrtzu trweqwrtrwetqwrtrwetzrtzu zrtzu tzrtqwrtrwetzrtzu zu rtzu qqqwrtrwetzrtzu wrtrwetzrtzu wrtrwetzrtzu<br>";
	kdDebug() << line << endl;
  buffer+=line;
/*
//  QLabel* newLine=new QLabel(line.left(line.length()-1),this->grid);
  QTextBrowser* x=new QTextBrowser(this->grid);
  x->setText("X");
  x=new QTextBrowser(this->grid);
  x->setText(line.left(line.length()-1));
//  QListViewItem* newLine=new QListViewItem(this,line.left(line.length()-1));
*/
  emit newText();

#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#else
  KTextBrowser::append(line);
#endif
  ensureVisible(0,contentsHeight()); // contentsHeight() seems to return wrong values when the widget is hidden
}

void IRCView::showEvent(QShowEvent* event)
{
  /* Suppress Compiler Warning */
  event->type();
  /* Workaround to scroll to the end of the TextView when it's shown */
//  setText(buffer);
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
