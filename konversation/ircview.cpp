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
*/

#include <iostream>

// #include <qlabel.h>
#include <qstylesheet.h>
#include <qtextcodec.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>

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
//  addColumn("Nick");
//  addColumn("Text");

  cerr << "IRCView::IRCView()" << endl;
  // setTextFormat(Qt::RichText);
  setVScrollBarMode(AlwaysOn);
  setHScrollBarMode(AlwaysOff);

  installEventFilter(this);
}

IRCView::~IRCView()
{
  cerr << "IRCView::~IRCView()" << endl;
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

  /* -1 to make next search work (pos+1) */
  int pos=-1;
  /* Replace all & with &amp; */
  while((pos=filteredLine.find('&',pos+1))!=-1) filteredLine.insert(pos+1,"amp;");
  /* Replace all < with &lt; */
  while((pos=filteredLine.find('<'))!=-1) filteredLine.replace(pos,1,"&lt;");
  /* Replace all > with &gt; */
  while((pos=filteredLine.find('>'))!=-1) filteredLine.replace(pos,1,"&gt;");

  /* How many chars to replace? */
  int replace;
  bool firstColor=true;
  QChar colChar;
  QString colorString;

  while((pos=filteredLine.find('\003'))!=-1)
  {
    int digitPos=pos;
    int foregroundColor=1; /* replace with default foreground */
    int backgroundColor=0; /* replace with default background */
    char* colorCodes[]={"ffffff","000000","000080","008000","ff0000","a52a2a","800080","ff8000",
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

      if(foregroundColor<2)                  /* maybe a two digit color? */
      {
        colChar=filteredLine[++digitPos];    /* get next char */
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
  }

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

        QString link="<font color=\"#0000ff\"><u><a href=\"";
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
  QString line=QString("<tr><td><font color=\"#000000\">%1:</font></td><td><font color=\"#000000\">%2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
  emit textToLog(QString("%1:\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendQuery(const char* nick,const char* message)
{
  QString line=QString("<tr><td><font color=\"#8e0000\">*%1*</font></td><td><font color=\"#8e0000\">%2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const char* nick,const char* message)
{
  QString line=QString("<tr><td>&nbsp;</td><td><font color=\"#000070\">* %1 %2</font></td></tr>\n").arg(filter(nick,false)).arg(filter(message));
  emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendServerMessage(const char* type,const char* message)
{
  /* Fixed width font option for MOTD */
  /* TODO: Make this configurable */
  QString motd("MOTD");
  QString fixed;
  if(motd==type) fixed=" face=\"courier\"";

  QString line=QString("<tr><td><font color=\"#91640a\">%1</font></td><td><font color=\"#91640a\""+fixed+">%2</font></td></tr>\n").arg(type).arg(filter(message));
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendCommandMessage(const char* type,const char* message)
{
  QString line=QString("<tr><td><font color=\"#960096\">%1</font></td><td><font color=\"#960096\">%2</font></td></tr>\n").arg(type).arg(filter(message));
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendBacklogMessage(const char* firstColumn,const char* message)
{
  QString line=QString("<tr><td><font color=\"#aaaaaa\">%1</font></td><td><font color=\"#aaaaaa\">%2</font></td></tr>\n").arg(firstColumn).arg(filter(message));

  doAppend(line);
}

void IRCView::doAppend(QString line)
{
  /* Add line to buffer */
//  line="qwrtrwetzrtzu qqwrtrwetzrtzqwrtrwetzrtzu u wrtqwrtrweqwrt rwetzrqqwrtrqwrtrwetzrtzu wetzrtzu  wrtrwetzrqwrtrwetzrtzu tzu tzu tzrtzu qwrtrqwrtrwetzrtzu qwqwrtrwetzrqwrtrwetzrtzu tzu qwrtrwetzrtzuqwrtrwetzrtzu  rtrwetzrqwrtrwetzrtzu tzu wetqwrtrwetzrtzu zrtzu rqwrtrwetzrtzu wetzqwrqwrtrwetzrtzu trweqwrtrwetqwrtrwetzrtzu zrtzu tzrtqwrtrwetzrtzu zu rtzu qqqwrtrwetzrtzu wrtrwetzrtzu wrtrwetzrtzu<br>";
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

//  setText(buffer);
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
  ensureVisible(0,contentsHeight()); // contentsHeight() seems to return wrong values when the widget is hidden
}

void IRCView::showEvent(QShowEvent* event)
{
  /* Workaround to scroll to the end of the TextView when it's shown */
//  setText(buffer);
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
  ensureVisible(0,contentsHeight());
  /* Set focus to input line (must be connected) */
  emit gotFocus();
}

void IRCView::focusInEvent(QFocusEvent* event)
{
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
