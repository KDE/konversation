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
*/

#include <qdatetime.h>
#include <qdir.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <qtooltip.h>

#ifdef USE_MDI
#include <qlayout.h>
#else
#include <kmdichildview.h>
#endif

#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kactioncollection.h>
#include <kaction.h>

#include "chatwindow.h"
#include "ircview.h"
#include "server.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "logfilereader.h"

#ifdef USE_MDI
ChatWindow::ChatWindow(QString caption) : KMdiChildView(caption)
#else
ChatWindow::ChatWindow(QWidget* parent) : QVBox(parent)
#endif
{
#ifdef USE_MDI
  setName(caption);
  setLedColor(0);
  setLabelColor(QString::null);
  setOn(0);
  state=Off;
  blinkOn=false;
#else
  setName("ChatWindowObject");
  parentWidget=parent;
#endif
  firstLog=true;
  server=0;
  m_notificationsEnabled = true;
  m_channelEncodingSupported = false;

#ifdef USE_MDI
  mainLayout=new QVBoxLayout(this);
  mainLayout->setAutoAdd(true);
  mainLayout->setMargin(margin());
  mainLayout->setSpacing(spacing());
#else
  setMargin(margin());
  setSpacing(spacing());
#endif

#ifdef USE_MDI
  connect(this,SIGNAL(childWindowCloseRequest(KMdiChildView*)),this,SLOT(closeRequest(KMdiChildView*)));
  connect(&blinkTimer,SIGNAL(timeout()),this,SLOT(blinkTimeout()));
  blinkTimer.start(500);
#endif
  m_mainWindow=NULL;

}

ChatWindow::~ChatWindow()
{
}

#ifdef USE_MDI
void ChatWindow::setLedColor(int newColor)
{
  Images* images=KonversationApplication::instance()->images();
  ledColor=newColor;
  iconOn=images->getLed(newColor,true,true);
  iconOff=images->getLed(newColor,false,true);
}

void ChatWindow::setLabelColor(const QString& color)
{
  labelColor=color;
}

void ChatWindow::setOn(bool on,bool important)
{
  if(on)
  {
    if(important)         blinkTimer.changeInterval(500);
    else if(state!=Fast)  blinkTimer.changeInterval(1000);
    state=important ? Fast : Slow;
  }
  else state=Off;

  emit setNotification(this,(state!=Off) ? iconOn : iconOff,QString::null);
}
#endif

void ChatWindow::blinkTimeout() // USE_MDI
{
#ifdef USE_MDI
  if(state!=Off)
  {
    // if the user wants us to blink, toggle LED blink status
    if(KonversationApplication::preferences.getBlinkingTabs())
    {
      blinkOn=!blinkOn;
      // draw the new LED
      emit setNotification(this,(blinkOn) ? iconOn : iconOff,(blinkOn) ? labelColor : QString::null);
    }
    // else LED should be always on
    else
    {
      // only change state when LED was off until now
      if(!blinkOn)
      {
        // switch LED on
        blinkOn=true;
        emit setNotification(this,iconOn,labelColor);
      }
    }
  }
#endif
}

void ChatWindow::setName(const QString& newName)
{
  name=newName;
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
    setMainWindow(server->getMainWindow());
#ifdef USE_MDI
    connect(server,SIGNAL(serverQuit(const QString&)),this,SLOT(serverQuit(const QString&)));
#endif
    // check if we need to set up the signals
    if(getType()!=ChannelList)
    {
      if(textView) textView->setServer(newServer);
      else kdDebug() << "ChatWindow::setServer(): textView==0!" << endl;
    }
  }
}
void ChatWindow::setMainWindow(KonversationMainWindow *mainWindow) {
  m_mainWindow = mainWindow;
  //setMainWindow may be called in a constructor, so make sure we call adjust
  //focus after it is set up.

  // FIXME: Please double-check if this is really needed. It steals focus to a hidden tab!
  //  QTimer::singleShot(0, this, SLOT(adjustFocus()));
}

Server* ChatWindow::getServer()
{
  return server;
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
  textView=newView;
  connect(textView,SIGNAL (textToLog(const QString&)),this,SLOT (logText(const QString&)) );
}

void ChatWindow::insertRememberLine()
{
  Q_ASSERT(textView);  if(!textView) return;
  kdDebug() << "Inserting remember line" << endl;
  textView->appendRaw("<br><hr color=\"#"+KonversationApplication::preferences.getColor("CommandMessage")+"\" noshade>", true);
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

void ChatWindow::appendQuery(const QString& nickname,const QString& message, bool usenotifications)
{
  Q_ASSERT(textView);  if(!textView) return ;
  textView->appendQuery(nickname,message);

  // OnScreen Message
  if(usenotifications && KonversationApplication::preferences.getOSDShowQuery() && notificationsEnabled())
  {
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    konvApp->osd->showOSD(i18n( "(Query) <%1> %2" ).arg(nickname).arg(message));
  }

}

void ChatWindow::appendAction(const QString& nickname,const QString& message, bool usenotifications)
{
  Q_ASSERT(textView);  if(!textView) return ;
  textView->appendAction(nickname,message);

  // OnScreen Message
  if(usenotifications && KonversationApplication::preferences.getOSDShowQuery() && notificationsEnabled())
  {
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
    konvApp->osd->showOSD(i18n( "(Query) * %1 %2" ).arg(nickname).arg(message));
  }

}

void ChatWindow::appendServerMessage(const QString& type,const QString& message)
{
  Q_ASSERT(this); if(!this) return; //I think there are still cases where chatwindow is being called even after it's deleted.
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
    // status panels get special treatment here, since they have no server at the beginning
    if(getType()==Status) {
      logName=name+".log";
    } else if(server) {
      // make sure that no path delimiters are in the name
      logName=server->getServerGroup().lower().replace("/","_")+"_"+name+".log";
    }

    // "cd" into log path or create path, if it's not there
    cdIntoLogPath();
    // Show last log lines. This idea was stole ... um ... inspired by PMP :)
    // Don't do this for the server status windows, though
    if(getType()!=Status && logfile.open(IO_ReadOnly))
    {
      unsigned long filePosition;

      QString backlogLine;
      QTextStream backlog(&logfile);
      backlog.setEncoding(QTextStream::UnicodeUTF8);
      // Check if the log is actually big enough
      if(backlog.device()->size()>1024)
      {
        // Set file pointer to 1 kB from the end
        backlog.device()->at(backlog.device()->size()-1024);
        // Skip first line, since it may be incomplete
        backlog.readLine();
      }

      // Loop until end of file reached
      while(!backlog.atEnd())
      {
        // remember actual file position to check for deadlocks
        filePosition=backlog.device()->at();
        backlogLine=backlog.readLine();

        // check for deadlocks
        if(backlog.device()->at()==filePosition) backlog.device()->at(filePosition+1);
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
          // Logfile is in utf8 so we don't need to do encoding stuff here
          // append backlog with time and first column to text view
          appendBacklogMessage(backlogFirst,backlogTime+' '+backlogLine);
        }
      } // while
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
    return 0;
}

// Accessors
IRCView* ChatWindow::getTextView()     { return textView; }
void ChatWindow::setLog(bool activate) { log=activate; }

// reimplement this in all panels that have user input
QString ChatWindow::getTextInLine()    { return QString::null; }

// reimplement this to return true in all classes that can become front view
bool ChatWindow::frontView()           { return false; }
// reimplement this to return true in all classes that can become search view
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

#ifdef USE_MDI
// reimplement this if your window needs special close treatment
void ChatWindow::closeYourself(ChatWindow*)
{
}
#endif

void ChatWindow::serverQuit(const QString&) // USE_MDI
{
}

#ifdef USEMDI
void ChatWindow::closeRequest(KMdiChildView* view) // USE_MDI
{
  closeYourself(static_cast<ChatWindow*>(view));
}
#else
void ChatWindow::closeRequest(KMdiChildView*) {}
#endif
bool ChatWindow::eventFilter(QObject* watched, QEvent* e)
{
  if(e->type() == QEvent::KeyPress) {
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);

    if(ke->key() == Qt::Key_Up && ke->state() == Qt::ShiftButton) {
      if(textView) {
        QScrollBar* sbar = textView->verticalScrollBar();
        sbar->setValue(sbar->value() - sbar->lineStep());
      }

      return true;
    } else if(ke->key() == Qt::Key_Down && ke->state() == Qt::ShiftButton) {
      if(textView) {
        QScrollBar* sbar = textView->verticalScrollBar();
        sbar->setValue(sbar->value() + sbar->lineStep());
      }

      return true;
    } else if(ke->key() == Qt::Key_Prior) {
      if(textView) {
        QScrollBar* sbar = textView->verticalScrollBar();
        sbar->setValue(sbar->value() - sbar->pageStep());
      }

      return true;
    } else if(ke->key() == Qt::Key_Next) {
      if(textView) {
        QScrollBar* sbar = textView->verticalScrollBar();
        sbar->setValue(sbar->value() + sbar->pageStep());
      }

      return true;
    }


  }

#ifdef USE_MDI
  return KMdiChildView::eventFilter(watched, e);
#else
  return QVBox::eventFilter(watched, e);
#endif
}

void ChatWindow::adjustFocus() {
  if(m_mainWindow && m_mainWindow->actionCollection()) {
    KAction *action;
    action = m_mainWindow->actionCollection()->action("insert_remember_line");
    if(action) action->setEnabled(textView!=NULL); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("insert_character");
    if(action) action->setEnabled(isInsertCharacterSupported()); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("irc_colors");
    if(action) action->setEnabled(areIRCColorsSupported()); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("clear_window");
    if(action) action->setEnabled(textView!=NULL); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("edit_find");
    if(action) action->setEnabled(textView!=NULL); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("edit_find_next");
    if(action) action->setEnabled(textView!=NULL); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("open_channel_list");
    if(action) action->setEnabled(server!=NULL); else Q_ASSERT(action);
    action = m_mainWindow->actionCollection()->action("open_logfile");
    if(action) {
	    action->setEnabled(!logName.isEmpty());
	    if(logName.isEmpty())
		    action->setText(i18n("&Open Logfile"));
	    else
		    action->setText(i18n("&Open Logfile for %1").arg(getName()));
    } else
	    Q_ASSERT(action);
  }
  childAdjustFocus();
}


#include "chatwindow.moc"

