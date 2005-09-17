/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Base class for all chat panels
  begin:     Fri Feb 1 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qdatetime.h>
#include <qdir.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <qtooltip.h>
#include <qlayout.h>

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <kaction.h>

#include "channel.h"
#include "chatwindow.h"
#include "ircview.h"
#include "server.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "logfilereader.h"

ChatWindow::ChatWindow(QWidget* parent) : QVBox(parent)
{
    setName("ChatWindowObject");
    parentWidget=parent;
    firstLog=true;
    m_server=0;
    m_notificationsEnabled = true;
    m_channelEncodingSupported = false;
    m_mainWindow=NULL;
    m_currentTabNotify = Konversation::tnfNone;

    setMargin(margin());
    setSpacing(spacing());
}

ChatWindow::~ChatWindow()
{
}

void ChatWindow::setName(const QString& newName)
{
    name=newName;
    emit nameChanged(this,newName);
}

QString ChatWindow::getName()
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

        m_server=newServer;
        setMainWindow(m_server->getMainWindow());
        connect(m_server,SIGNAL (serverOnline(bool)),this,SLOT (serverOnline(bool)) );

        // check if we need to set up the signals
        if(getType()!=ChannelList)
        {
            if(textView) textView->setServer(newServer);
            else kdDebug() << "ChatWindow::setServer(): textView==0!" << endl;
        }

        emit serverOnline(m_server->isConnected());
    }
}

void ChatWindow::setMainWindow(KonversationMainWindow *mainWindow)
{
    m_mainWindow = mainWindow;
}

Server* ChatWindow::getServer()
{
    return m_server;
}

void ChatWindow::serverOnline(bool state)
{
    emit online(this,state);
}

void ChatWindow::setIdentity(const Identity *newIdentity)
{
    Q_ASSERT(newIdentity);  if(!newIdentity) return;
    identity=*newIdentity;
}

void ChatWindow::setTextView(IRCView* newView)
{
    textView = newView;

    if(!textView)
    {
        return;
    }

    textView->setChatWin(this);
    connect(textView,SIGNAL (textToLog(const QString&)),this,SLOT (logText(const QString&)) );
}

void ChatWindow::insertRememberLine()
{
    Q_ASSERT(textView);  if(!textView) return;
    kdDebug() << "Inserting remember line" << endl;
    textView->appendRaw("<br><hr color=\""+Preferences::color(Preferences::CommandMessage).name()+"\" noshade>", true, true);
}

void ChatWindow::appendRaw(const QString& message, bool suppressTimestamps)
{
    Q_ASSERT(textView);  if(!textView) return;
    textView->appendRaw(message, suppressTimestamps);
}

void ChatWindow::append(const QString& nickname,const QString& message)
{
    Q_ASSERT(textView);  if(!textView) return ;
    textView->append(nickname,message);
}

void ChatWindow::appendQuery(const QString& nickname,const QString& message, bool)
{
    Q_ASSERT(textView);  if(!textView) return ;
    textView->appendQuery(nickname,message);
}

void ChatWindow::appendAction(const QString& nickname,const QString& message, bool)
{
    Q_ASSERT(textView);  if(!textView) return ;
    textView->appendAction(nickname,message);
}

void ChatWindow::appendServerMessage(const QString& type,const QString& message)
{
    Q_ASSERT(this); if(!this) return;             //I think there are still cases where chatwindow is being called even after it's deleted.
    Q_ASSERT(textView);  if(!textView) return ;
    textView->appendServerMessage(type,message);
}

void ChatWindow::appendCommandMessage(const QString& command,const QString& message, bool important, bool parseURL, bool self)
{
    Q_ASSERT(textView);  if(!textView) return ;
    textView->appendCommandMessage(command,message,important, parseURL, self);
}

void ChatWindow::appendBacklogMessage(const QString& firstColumn,const QString& message)
{
    Q_ASSERT(textView);  if(!textView) return ;
    textView->appendBacklogMessage(firstColumn,message);
}

void ChatWindow::cdIntoLogPath()
{
    QDir logPath=QDir::home();
    // Try to "cd" into the logfile path
    if(!logPath.cd(Preferences::logfilePath(),true))
    {
        // Only create log path if logging is enabled
        if(log)
        {
            // Try to create the logfile path and "cd" into it again
            logPath.mkdir(Preferences::logfilePath(),true);
            logPath.cd(Preferences::logfilePath(),true);
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
        // status panels get special treatment here, since they have no server at the beginning
        if(getType() == Status)
        {
            logName = name + ".log";
        }
        else if(m_server)
        {
            // make sure that no path delimiters are in the name
            logName = QString(m_server->getServerGroup().lower()).append('_').append(name).append(".log").replace('/','_');
        }

        // "cd" into log path or create path, if it's not there
        cdIntoLogPath();
        // Show last log lines. This idea was stole ... um ... inspired by PMP :)
        // Don't do this for the server status windows, though
        if((getType() != Status) && logfile.open(IO_ReadOnly))
        {
            unsigned long filePosition;

            QString backlogLine;
            QTextStream backlog(&logfile);
            backlog.setEncoding(QTextStream::UnicodeUTF8);
            // Check if the log is actually big enough
            if(backlog.device()->size() > 1024)
            {
                // Set file pointer to 1 kB from the end
                backlog.device()->at(backlog.device()->size() - 1024);
                // Skip first line, since it may be incomplete
                backlog.readLine();
            }

            // Loop until end of file reached
            while(!backlog.atEnd())
            {
                // remember actual file position to check for deadlocks
                filePosition = backlog.device()->at();
                backlogLine = backlog.readLine();

                // check for deadlocks
                if(backlog.device()->at() == filePosition) backlog.device()->at(filePosition + 1);

                // if a tab character is present in the line
                if(backlogLine.find('\t') != -1)
                {
                    // extract first column from log
                    QString backlogFirst = backlogLine.left(backlogLine.find('\t'));
                    // cut first column from line
                    backlogLine = backlogLine.mid(backlogLine.find('\t') + 1);
                    // Logfile is in utf8 so we don't need to do encoding stuff here
                    // append backlog with time and first column to text view
                    appendBacklogMessage(backlogFirst, backlogLine);
                }
            }                                     // while

            backlog.unsetDevice();
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
            // write log in utf8 to help i18n
            logStream.setEncoding(QTextStream::UnicodeUTF8);

            if(firstLog)
            {
                QString intro(i18n("\n*** Logfile started\n*** on %1\n\n").arg(QDateTime::currentDateTime().toString()));
                logStream << intro;
                firstLog=false;
            }

            QTime time=QTime::currentTime();
            QString logLine(QString("[%1] [%2] %3\n").arg(QDate::currentDate(Qt::LocalTime).toString()).
                arg(time.toString("hh:mm:ss")).arg(text));

            logStream << logLine;

            // detach stream from file
            logStream.unsetDevice();

            // close file
            logfile.close();
        }
        else kdWarning() << "ChatWindow::logText(): open(IO_Append) for " << logfile.name() << " failed!" << endl;
    }
}

void ChatWindow::setChannelEncodingSupported(bool enabled)
{
    m_channelEncodingSupported = enabled;
}

bool ChatWindow::isChannelEncodingSupported() const
{
    return m_channelEncodingSupported;
}

int ChatWindow::spacing()
{
    if(Preferences::useSpacing())
        return Preferences::spacing();
    else
        return KDialog::spacingHint();
}

int ChatWindow::margin()
{
    if(Preferences::useSpacing())
        return Preferences::margin();
    else
        return 0;
}

// Accessors
IRCView* ChatWindow::getTextView() const { return textView; }
void ChatWindow::setLog(bool activate) { log=activate; }

// reimplement this in all panels that have user input
QString ChatWindow::getTextInLine()    { return QString::null; }

bool ChatWindow::canBeFrontView()           { return false; }
bool ChatWindow::searchView()          { return false; }

// reimplement this in all panels that have user input
void ChatWindow::indicateAway(bool)
{
}

// reimplement this in all panels that have user input
void ChatWindow::appendInputText(const QString&)
{
}

// reimplement this if your window needs special close treatment
bool ChatWindow::closeYourself()
{
    return true;
}

bool ChatWindow::eventFilter(QObject* watched, QEvent* e)
{
    if(e->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);

        if(ke->key() == Qt::Key_Up && ke->state() == Qt::ShiftButton)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() - sbar->lineStep());
            }

            return true;
        }
        else if(ke->key() == Qt::Key_Down && ke->state() == Qt::ShiftButton)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() + sbar->lineStep());
            }

            return true;
        }
        else if(ke->key() == Qt::Key_Prior)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() - sbar->pageStep());
            }

            return true;
        }
        else if(ke->key() == Qt::Key_Next)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() + sbar->pageStep());
            }

            return true;
        }

    }

    return QVBox::eventFilter(watched, e);
}

void ChatWindow::adjustFocus()
{
    childAdjustFocus();
}

void ChatWindow::emitUpdateInfo()
{
    QString info = getName();
    emit updateInfo(info);
}

void ChatWindow::activateTabNotification(Konversation::TabNotifyType type)
{
    if(type >= m_currentTabNotify)
    {
        return;
    }

    m_currentTabNotify = type;

    QString colorString;

    switch(type)
    {
        case Konversation::tnfNick:
            colorString = Preferences::highlightNickColor();
            break;

        case Konversation::tnfHighlight:
            colorString = "#55c8c0";
            break;

        case Konversation::tnfNormal:
            colorString = "#0000ff";
            break;

        case Konversation::tnfControl:
            colorString = "#4eb959";
            break;

        default:
            break;
    }

    emit updateTabNotification(this, colorString);
}

void ChatWindow::resetTabNotification()
{
    m_currentTabNotify = Konversation::tnfNone;
}

#include "chatwindow.moc"
