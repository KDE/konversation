/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  chatwindow.cpp  -  Base class for all chat panels
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qdatetime.h>
#include <qdir.h>

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <krootpixmap.h>

#include "server.h"
#include "chatwindow.h"
#include "konversationapplication.h"

ChatWindow::ChatWindow(QWidget* parent)
{
  setName("ChatWindowObject");
  parentWidget=parent;
  firstLog=true;

  setMargin(margin());
  setSpacing(spacing());

  connect(&filter,SIGNAL(launchScript(const QString&)),
    &scriptLauncher,SLOT(launchScript(const QString&)) );
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::setName(const QString& newName)
{
  name=newName;
  scriptLauncher.setTargetName(newName);
  emit nameChanged(this,newName);
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
  if(newServer==0) kdDebug("ChatWindow::setServer(0)!") << endl;
  else
  {
    server=newServer;

    // check if we need to set up the signals
    if(getType()!=ChannelList)
    {
      if(textView) textView->setServer(newServer);
      else kdDebug() << "ChatWindow::setServer(): textView==0!" << endl;

      connect(&filter,SIGNAL (openQuery(const QString&,const QString&)),
               server,SLOT   (addQuery(const QString&,const QString&)) );
      connect(&filter,SIGNAL (openDccPanel()),
               server,SLOT   (requestDccPanel()) );
      connect(&filter,SIGNAL (closeDccPanel()),
               server,SLOT   (requestCloseDccPanel()) );
      connect(&filter,SIGNAL (openDccSend(const QString &, const QString &)),
               server,SLOT   (addDccSend(const QString &, const QString &)) );
      connect(&filter,SIGNAL (requestDccSend()),
               server,SLOT   (requestDccSend()) );
      connect(&filter,SIGNAL (requestDccSend(const QString &)),
               server,SLOT   (requestDccSend(const QString &)) );
      connect(&filter,SIGNAL (away()),
               server,SLOT   (away()) );
      connect(&filter,SIGNAL (unAway()),
               server,SLOT   (unAway()) );
      connect(&filter,SIGNAL (sendToAllChannels(const QString&)),
               server,SLOT   (sendToAllChannels(const QString&)) );
      connect(&filter,SIGNAL (banUsers(const QStringList&,const QString&,const QString&)),
               server,SLOT   (requestBan(const QStringList&,const QString&,const QString&)) );

      connect(&filter,SIGNAL (openRawLog(bool)), server,SLOT (addRawLog(bool)) );
      connect(&filter,SIGNAL (closeRawLog()),server,SLOT (closeRawLog()) );

      scriptLauncher.setServerName(server->getServerName());

      connect(&scriptLauncher,SIGNAL (scriptNotFound(const QString&)),
                         server,SLOT (scriptNotFound(const QString&)) );
      connect(&scriptLauncher,SIGNAL (scriptExecutionError(const QString&)),
                         server,SLOT (scriptExecutionError(const QString&)) );
    }
  }
}

Server* ChatWindow::getServer()
{
  return server;
}

void ChatWindow::setIdentity(const Identity *newIdentity)
{
  identity=newIdentity;
  filter.setIdentity(identity);
}

void ChatWindow::setTextView(IRCView* newView)
{
  textView=newView;
  connect(textView,SIGNAL (textToLog(const QString&)),this,SLOT (logText(const QString&)) );
}

void ChatWindow::appendRaw(const QString& message)
{
  textView->appendRaw(message);
}

void ChatWindow::append(const QString& nickname,const QString& message)
{
  textView->append(nickname,message);
}

void ChatWindow::appendQuery(const QString& nickname,const QString& message)
{
  textView->appendQuery(nickname,message);
}

void ChatWindow::appendAction(const QString& nickname,const QString& message)
{
  textView->appendAction(nickname,message);
}

void ChatWindow::appendServerMessage(const QString& type,const QString& message)
{
  textView->appendServerMessage(type,message);
}

void ChatWindow::appendCommandMessage(const QString& command,const QString& message)
{
  textView->appendCommandMessage(command,message);
}

void ChatWindow::appendBacklogMessage(const QString& firstColumn,const QString& message)
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
    // Don't do this for the server status windows, though
    if(getType()!=Status && logfile.open(IO_ReadOnly))
    {
      unsigned long filePosition;

      QString backlogLine;
      // Set file pointer to 1 kB from the end
      logfile.at(logfile.size()-1024);
      // Skip first line, since it may be incomplete
      logfile.readLine(backlogLine,1024);
      // Loop until end of file reached
      while(!logfile.atEnd())
      {
        // remember actual file position to check for deadlocks
        filePosition=logfile.at();

        logfile.readLine(backlogLine,1024);
        // check for deadlocks
        if(logfile.at()==filePosition) logfile.at(filePosition+1);
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
  if(log)
  {
    // "cd" into log path or create path, if it's not there
    cdIntoLogPath();

    if(logfile.open(IO_WriteOnly | IO_Append))
    {
      // wrap the file into a stream
      QTextStream logStream(&logfile);

      if(firstLog)
      {
        QString intro(i18n("\n*** Logfile started\n*** on %1\n\n").arg(QDateTime::currentDateTime().toString()));
        logStream << intro;
        firstLog=false;
      }

      QTime time=QTime::currentTime();
      QString logLine(QString("[%1] %2\n").arg(time.toString("hh:mm:ss")).arg(text));
      logStream << logLine;

      // detach stream from file
      logStream.unsetDevice();

      // close file
      logfile.close();
    }
    else kdWarning() << "ChatWindow::logText(): open(IO_Append) for " << logfile.name() << " failed!" << endl;
  }
}

int ChatWindow::spacing()
{
  if(KonversationApplication::preferences.getUseSpacing())
    return KonversationApplication::preferences.getSpacing();
  else
    return KDialog::spacingHint();
}

int ChatWindow::margin()
{
  if(KonversationApplication::preferences.getUseSpacing())
    return KonversationApplication::preferences.getMargin();
  else
    return KDialog::marginHint();
}

// Accessors

IRCView* ChatWindow::getTextView()     { return textView; }
void ChatWindow::setLog(bool activate) { log=activate; }

// reimplement this if your window needs special close treatment
void ChatWindow::closeYourself()
{
}

#include "chatwindow.moc"
