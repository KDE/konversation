/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  chatwindow.cpp  -  description
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qdatetime.h>
#include <qdir.h>

#include <klocale.h>
#include <kdebug.h>

#include "chatwindow.h"
#include "konversationapplication.h"

ChatWindow::ChatWindow(QWidget* parent)
{
  parentWidget=parent;
  firstLog=true;

  // FIXME: Make this configurable!
//  font.setFamily("Arial");
//  font.setPointSize(24);
  
//  setFont(font);
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::setName(QString newName)
{
  name=newName;
}

QString& ChatWindow::getName()
{
  return name;
}

void ChatWindow::setType(WindowType newType)
{
  type=newType;
}

ChatWindow::WindowType ChatWindow::getType()
{
  return type;
}

void ChatWindow::setServer(Server* newServer)
{
  server=newServer;
  connect(&filter,SIGNAL (openQuery(const QString&,const QString&)),
           server,SLOT   (addQuery(const QString&,const QString&)) );
  connect(&filter,SIGNAL (openDccPanel()),
           server,SLOT   (requestDccPanel()) );
  connect(&filter,SIGNAL (openDccSend(QString,QString)),
           server,SLOT   (addDccSend(QString,QString)) );
  connect(&filter,SIGNAL (requestDccSend(QString)),
           server,SLOT   (requestDccSend(QString)) );
}

void ChatWindow::setTextView(IRCView* newView)
{
  textView=newView;
  connect(textView,SIGNAL (textToLog(const QString&)),this,SLOT (logText(const QString&)) );
}

void ChatWindow::append(const char* nickname,const char* message)
{
  textView->append(nickname,message);
}

void ChatWindow::appendQuery(const char* nickname,const char* message)
{
  textView->appendQuery(nickname,message);
}

void ChatWindow::appendAction(const char* nickname,const char* message)
{
  textView->appendAction(nickname,message);
}

void ChatWindow::appendServerMessage(const char* type,const char* message)
{
  textView->appendServerMessage(type,message);
}

void ChatWindow::appendCommandMessage(const char* command,const char* message)
{
  textView->appendCommandMessage(command,message);
}

void ChatWindow::appendBacklogMessage(const char* firstColumn,const char* message)
{
  textView->appendBacklogMessage(firstColumn,message);
}

void ChatWindow::cdIntoLogPath()
{
  QDir logPath=QDir::home();
  // Try to "cd" into the logfile path
  if(!logPath.cd(KonversationApplication::preferences.getLogPath(),true))
  {
    // Only create log path if logging is enabled
    if(log)
    {
      // Try to create the logfile path and "cd" into it again
      logPath.mkdir(KonversationApplication::preferences.getLogPath(),true);
      logPath.cd(KonversationApplication::preferences.getLogPath(),true);
    }
  }

  // add the logfile name to the path
  logfile.setName(logPath.path()+"/"+logName);
}

void ChatWindow::setLogfileName(const QString& name)
{
  // Only change name of logfile if the window was new.
  if(firstLog)
  {
    logName=name;
    // "cd" into log path or create path, if it's not there
    cdIntoLogPath();
    // Show last log lines. This idea was stole ... um ... inspired by PMP :)
    if(logfile.open(IO_ReadOnly))
    {
      QString backlogLine;
      // Set file pointer to 1 kB from the end
      logfile.at(logfile.size()-1024);
      // Skip first line, since it may be incomplete
      logfile.readLine(backlogLine,1024);
      // Loop until end of file reached
      while(!logfile.atEnd())
      {
        logfile.readLine(backlogLine,1024);
        // if a tab character is present in the line
        if(backlogLine.find('\t')!=-1)
        {
          // extract timestamp from log
          QString backlogTime=backlogLine.left(backlogLine.find(' '));
          // cut timestamp from line
          backlogLine=backlogLine.mid(backlogLine.find(' ')+1);
          // extract first column from log
          QString backlogFirst=backlogLine.left(backlogLine.find('\t'));
          // cut first column from line
          backlogLine=backlogLine.mid(backlogLine.find('\t')+1);
          // append backlog with time and first column to text view
          appendBacklogMessage(backlogFirst,backlogTime+' '+backlogLine);
        }
      } // while
      logfile.close();
    }
  }
}

void ChatWindow::logText(const QString& text)
{
  // "cd" into log path or create path, if it's not there
  cdIntoLogPath();

  if(logfile.open(IO_WriteOnly | IO_Append))
  {
    if(firstLog)
    {
      QString intro(i18n("\n*** Logfile started\n*** on %1\n\n").arg(QDateTime::currentDateTime().toString()));
      logfile.writeBlock(intro,intro.length());
      firstLog=false;
    }

    QTime time=QTime::currentTime();
    QString logLine(QString("[%1] %2\n").arg(time.toString("hh:mm:ss")).arg(text));
    logfile.writeBlock(logLine,logLine.length());
    logfile.close();
  }
  else kdWarning() << "ChatWindow::logText(): open(IO_Append) for " << logfile.name() << " failed!" << endl;
}
