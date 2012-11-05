/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "chatwindow.h"
#include "channel.h"
#include "query.h"
#include "ircview.h"
#include "ircinput.h"
#include "server.h"
#include "application.h"
#include "logfilereader.h"
#include "viewcontainer.h"

#include <QDateTime>
#include <QDir>
#include <QTextCodec>
#include <QKeyEvent>
#include <QScrollBar>

#include <KDialog>
#include <KUser>


ChatWindow::ChatWindow(QWidget* parent) : KVBox(parent)
{
    setName("ChatWindowObject");
    setTextView(0);
    setInputBar(0);
    firstLog = true;
    m_server = 0;
    m_recreationScheduled = false;
    m_notificationsEnabled = true;
    m_channelEncodingSupported = false;
    m_currentTabNotify = Konversation::tnfNone;

    setMargin(margin());
    setSpacing(spacing());
}

ChatWindow::~ChatWindow()
{
    if (getInputBar() && getServer())
    {
        const QString& language = getInputBar()->spellCheckingLanguage();

        if (!language.isEmpty())
        {
            Konversation::ServerGroupSettingsPtr serverGroup = getServer()->getConnectionSettings().serverGroup();

            if (serverGroup)
                Preferences::setSpellCheckingLanguage(serverGroup, getName(), language);
            else
                Preferences::setSpellCheckingLanguage(getServer()->getDisplayName(), getName(), language);
        }
    }

    emit closing(this);
    m_server=0;
}

// reimplement this if your window needs special close treatment
bool ChatWindow::closeYourself(bool /* askForConfirmation */)
{
    deleteLater();

    return true;
}

void ChatWindow::cycle()
{
    m_recreationScheduled = true;

    closeYourself(false);
}

void ChatWindow::updateAppearance()
{
    if (getTextView()) getTextView()->updateAppearance();

    // The font size of the KTabWidget container may be inappropriately
    // small due to the "Tab bar" font size setting.
    setFont(KGlobalSettings::generalFont());
}

void ChatWindow::setName(const QString& newName)
{
    name=newName;
    emit nameChanged(this,newName);
}

QString ChatWindow::getName() const
{
    return name;
}

QString ChatWindow::getTitle() const
{
    QString title;
    if (getType() == Channel)
    {
       title = QString("%1 (%2)")
             .arg(getName())
             .arg(getServer()->getDisplayName());
    }
    else
    {
       title = getName();
    }

    return title;
}

QString ChatWindow::getURI(bool passNetwork)
{
    QString protocol;
    QString url;
    QString port;
    QString server;
    QString channel;

    if (getServer()->getUseSSL())
        protocol = "ircs://";
    else
        protocol = "irc://";

    if (getType() == Channel)
        channel = getName().replace(QRegExp("^#"), QString());

    if (passNetwork)
    {
        server = getServer()->getDisplayName();

        QUrl test(protocol+server);

        // QUrl (ultimately used by the bookmark system, which is the
        // primary consumer here) doesn't like spaces in hostnames as
        // well as other things which are possible in user-chosen net-
        // work names, so let's fall back to the hostname if we can't
        // get the network name by it.
        if (!test.isValid())
            passNetwork = false;
    }

    if (!passNetwork)
    {
        server = getServer()->getServerName();
        port = ':'+QString::number(getServer()->getPort());
    }

    if (server.contains(':')) // IPv6
        server = '['+server+']';

    url = protocol+server+port+'/'+channel;

    return url;
}

void ChatWindow::setType(WindowType newType)
{
    type=newType;
}

ChatWindow::WindowType ChatWindow::getType() const
{
    return type;
}

void ChatWindow::setServer(Server* newServer)
{
    if (!newServer)
    {
        kDebug() << "ChatWindow::setServer(0)!";
    }
    else
    {
        m_server=newServer;
        connect(m_server,SIGNAL (serverOnline(bool)),this,SLOT (serverOnline(bool)) );

        // check if we need to set up the signals
        if(getType() != ChannelList)
        {
            if(textView) textView->setServer(newServer);
            else kDebug() << "textView==0!";
        }

        serverOnline(m_server->isConnected());
    }

    if (getInputBar())
    {
        QString language;

        Konversation::ServerGroupSettingsPtr serverGroup = newServer->getConnectionSettings().serverGroup();

        if (serverGroup)
            language = Preferences::spellCheckingLanguage(serverGroup, getName());
        else
            language = Preferences::spellCheckingLanguage(newServer->getDisplayName(), getName());

        if (!language.isEmpty())
            getInputBar()->setSpellCheckingLanguage(language);
    }
}

Server* ChatWindow::getServer() const
{
    return m_server;
}

void ChatWindow::serverOnline(bool /* state */)
{
    //emit online(this,state);
}

void ChatWindow::setTextView(IRCView* newView)
{
    textView = newView;

    if(!textView)
    {
        return;
    }

    textView->setVerticalScrollBarPolicy(Preferences::self()->showIRCViewScrollBar() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

    textView->setChatWin(this);
    connect(textView,SIGNAL(textToLog(QString)), this,SLOT(logText(QString)));
    connect(textView,SIGNAL(setStatusBarTempText(QString)), this, SIGNAL(setStatusBarTempText(QString)));
    connect(textView,SIGNAL(clearStatusBarTempText()), this, SIGNAL(clearStatusBarTempText()));
}

void ChatWindow::appendRaw(const QString& message, bool self)
{
    if(!textView) return;
    textView->appendRaw(message, self);
}

void ChatWindow::appendLog(const QString& message)
{
    if(!textView) return;
    textView->appendLog(message);
}

void ChatWindow::append(const QString& nickname,const QString& message)
{
    if(!textView) return ;
    textView->append(nickname,message);
}

void ChatWindow::appendQuery(const QString& nickname,const QString& message, bool inChannel)
{
    if(!textView) return ;
    textView->appendQuery(nickname,message, inChannel);
}

void ChatWindow::appendAction(const QString& nickname, const QString& message)
{
    if(!textView) return;

    if (getType() == Query || getType() == DccChat)
        textView->appendQueryAction(nickname, message);
    else
        textView->appendChannelAction(nickname, message);
}

void ChatWindow::appendServerMessage(const QString& type,const QString& message, bool parseURL)
{
    if(!textView) return ;
    textView->appendServerMessage(type,message, parseURL);
}

void ChatWindow::appendCommandMessage(const QString& command, const QString& message, bool parseURL, bool self)
{
    if(!textView) return ;
    textView->appendCommandMessage(command,message,parseURL,self);
}

void ChatWindow::appendBacklogMessage(const QString& firstColumn,const QString& message)
{
    if(!textView) return ;
    textView->appendBacklogMessage(firstColumn,Konversation::sterilizeUnicode(message));
}

void ChatWindow::clear()
{
    if (!textView) return;

    textView->clear();

    resetTabNotification();

    if (m_server)
        m_server->getViewContainer()->unsetViewNotification(this);
}

void ChatWindow::cdIntoLogPath()
{
    QString home = KUser(KUser::UseRealUserID).homeDir();
    QString logPath = Preferences::self()->logfilePath().pathOrUrl().replace("$HOME", home);

    QDir logDir(home);

    // Try to "cd" into the logfile path.
    if (!logDir.cd(logPath))
    {
        // Only create log path if logging is enabled.
        if (log())
        {
            // Try to create the logfile path and "cd" into it again.
            logDir.mkpath(logPath);
            logDir.cd(logPath);
        }
    }

    // Add the logfile name to the path.
    logfile.setFileName(logDir.path() + '/' + logName);
}

void ChatWindow::setLogfileName(const QString& name)
{
    // Only change name of logfile if the window was new.
    if(firstLog)
    {
        if (getTextView())
            getTextView()->setContextMenuOptions(IrcContextMenus::ShowLogAction, true);

        // status panels get special treatment here, since they have no server at the beginning
        if (getType() == Status || getType() == DccChat)
        {
            logName = name + ".log";
        }
        else if (m_server)
        {
            // make sure that no path delimiters are in the name
            logName = QString(m_server->getDisplayName().toLower()).append('_').append(name).append(".log").replace('/','_');
        }

        // load backlog to show
        if(Preferences::self()->showBacklog())
        {
            // "cd" into log path or create path, if it's not there
            cdIntoLogPath();
            // Show last log lines. This idea was stole ... um ... inspired by PMP :)
            // Don't do this for the server status windows, though
            if((getType() != Status) && logfile.open(QIODevice::ReadOnly))
            {
                qint64 filePosition;

                QString backlogLine;
                QTextStream backlog(&logfile);
                backlog.setCodec(QTextCodec::codecForName("UTF-8"));
                backlog.setAutoDetectUnicode(true);

                QStringList firstColumns;
                QStringList messages;
                int offset = 0;
                qint64 lastPacketHeadPosition = backlog.device()->size();
                const unsigned int packetSize = 4096;
                while(messages.count() < Preferences::self()->backlogLines() && backlog.device()->size() > packetSize * offset)
                {
                    QStringList firstColumnsInPacket;
                    QStringList messagesInPacket;

                    // packetSize * offset < size <= packetSize * ( offset + 1 )

                    // Check if the log is bigger than packetSize * ( offset + 1 )
                    if(backlog.device()->size() > packetSize * ( offset + 1 ))
                    {
                        // Set file pointer to the packet size above the offset
                        backlog.seek(backlog.device()->size() - packetSize * ( offset + 1 ));
                        // Skip first line, since it may be incomplete
                        backlog.readLine();
                    }
                    else
                    {
                        // Set file pointer to the head

                        // Qt 4.5 Doc: Note that when using a QTextStream on a
                        // QFile, calling reset() on the QFile will not have the
                        // expected result because QTextStream buffers the  file.
                        // Use the QTextStream::seek() function instead.
                        // backlog.device()->reset();
                        backlog.seek( 0 );
                    }

                    // remember actual file position to check for deadlocks
                    filePosition = backlog.pos();

                    qint64 currentPacketHeadPosition = filePosition;

                    // Loop until end of file reached
                    while(!backlog.atEnd() && filePosition < lastPacketHeadPosition)
                    {
                        backlogLine = backlog.readLine();

                        // check for deadlocks
                        if(backlog.pos() == filePosition)
                        {
                            backlog.seek(filePosition + 1);
                        }

                        // if a tab character is present in the line, meaning it is a valid chatline
                        if (backlogLine.contains('\t'))
                        {
                            // extract first column from log
                            QString backlogFirst = backlogLine.left(backlogLine.indexOf('\t'));
                            // cut first column from line
                            backlogLine = backlogLine.mid(backlogLine.indexOf('\t') + 1);
                            // Logfile is in utf8 so we don't need to do encoding stuff here
                            // append backlog with time and first column to text view
                            firstColumnsInPacket << backlogFirst;
                            messagesInPacket << backlogLine;
                        }

                        // remember actual file position to check for deadlocks
                        filePosition = backlog.pos();
                    } // while

                    // remember the position not to read the same lines again
                    lastPacketHeadPosition = currentPacketHeadPosition;
                    ++offset;

                    firstColumns = firstColumnsInPacket + firstColumns;
                    messages = messagesInPacket + messages;
                }
                backlog.setDevice(0);
                logfile.close();

                // trim
                int surplus = messages.count() - Preferences::self()->backlogLines();
                // "surplus" can be a minus value. (when the backlog is too short)
                if(surplus > 0)
                {
                    for(int i = 0 ; i < surplus ; ++i)
                    {
                        firstColumns.pop_front();
                        messages.pop_front();
                    }
                }

                QStringList::Iterator itFirstColumn = firstColumns.begin();
                QStringList::Iterator itMessage = messages.begin();
                for( ; itFirstColumn != firstColumns.end() ; ++itFirstColumn, ++itMessage )
                    appendBacklogMessage(*itFirstColumn, *itMessage);
            }
        } // if(Preferences::showBacklog())
    }
}

void ChatWindow::logText(const QString& text)
{
    if(log())
    {
        // "cd" into log path or create path, if it's not there
        cdIntoLogPath();

        if(logfile.open(QIODevice::WriteOnly | QIODevice::Append))
        {
            // wrap the file into a stream
            QTextStream logStream(&logfile);
            // write log in utf8 to help i18n
            logStream.setCodec(QTextCodec::codecForName("UTF-8"));
            logStream.setAutoDetectUnicode(true);

            if(firstLog)
            {
                QString intro(i18n("\n*** Logfile started\n*** on %1\n\n", QDateTime::currentDateTime().toString()));
                logStream << intro;
                firstLog=false;
            }

            QDateTime dateTime = QDateTime::currentDateTime();
            QString logLine(QString("[%1] [%2] %3\n").arg(KGlobal::locale()->formatDate(dateTime.date(), KLocale::LongDate)).
                arg(KGlobal::locale()->formatTime(dateTime.time(), true)).arg(text));

            logStream << logLine;

            // detach stream from file
            logStream.setDevice(0);

            // close file
            logfile.close();
        }
        else kWarning() << "open(QIODevice::Append) for " << logfile.fileName() << " failed!";
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
    if(Preferences::self()->useSpacing())
        return Preferences::self()->spacing();
    else
        return KDialog::spacingHint();
}

int ChatWindow::margin()
{
    if(Preferences::self()->useSpacing())
        return Preferences::self()->margin();
    else
        return 0;
}

// Accessors
IRCView* ChatWindow::getTextView() const
{
  return textView;
}

bool ChatWindow::log()
{
  return Preferences::self()->log();
}

// reimplement this in all panels that have user input
QString ChatWindow::getTextInLine()
{
    if (m_inputBar)
        return m_inputBar->toPlainText();
    else
        return QString();
}

bool ChatWindow::canBeFrontView()
{
  return false;
}

bool ChatWindow::searchView()
{
  return false;
}

// reimplement this in all panels that have user input
void ChatWindow::indicateAway(bool)
{
}

// reimplement this in all panels that have user input
void ChatWindow::appendInputText(const QString& text, bool fromCursor)
{
    if (!fromCursor)
        m_inputBar->append(text);
    else
    {
        const int position = m_inputBar->textCursor().position();
        m_inputBar->textCursor().insertText(text);
        QTextCursor cursor = m_inputBar->textCursor();
        cursor.setPosition(position + text.length());
        m_inputBar->setTextCursor(cursor);
    }
}

bool ChatWindow::eventFilter(QObject* watched, QEvent* e)
{
    if(e->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);

        bool scrollMod = (Preferences::self()->useMultiRowInputBox() ? false : (ke->modifiers() == Qt::ShiftModifier));

        if(ke->key() == Qt::Key_Up && scrollMod)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() - sbar->singleStep());
            }

            return true;
        }
        else if(ke->key() == Qt::Key_Down && scrollMod)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() + sbar->singleStep());
            }

            return true;
        }
        else if(ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_PageUp)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() - sbar->pageStep());
            }

            return true;
        }
        else if(ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_PageDown)
        {
            if(textView)
            {
                QScrollBar* sbar = textView->verticalScrollBar();
                sbar->setValue(sbar->value() + sbar->pageStep());
            }

            return true;
        }

    }

    return KVBox::eventFilter(watched, e);
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

QColor ChatWindow::highlightColor()
{
    return getTextView()->highlightColor();
}

void ChatWindow::activateTabNotification(Konversation::TabNotifyType type)
{
    if (!notificationsEnabled())
        return;

    if(type > m_currentTabNotify)
        return;

    m_currentTabNotify = type;

    emit updateTabNotification(this,type);
}

void ChatWindow::resetTabNotification()
{
    m_currentTabNotify = Konversation::tnfNone;
}

void ChatWindow::msgHelper(const QString& recipient, const QString& message)
    {
    // A helper method for handling the 'msg' and 'query' (with a message
    // payload) commands. When the user uses either, we show a visualiza-
    // tion of what he/she has sent in the form of '<-> target> message>'
    // in the chat view of the tab the command was issued in, as well as
    // add the resulting message to the target view (if present), in that
    // order. The order is especially important as the origin and target
    // views may be the same, and the two messages may thus appear toge-
    // ther and should be sensibly ordered.

    if (recipient.isEmpty() || message.isEmpty())
        return;

    bool isAction = false;
    QString result = message;
    QString visualization;

    if (result.startsWith(Preferences::self()->commandChar() + "me"))
    {
        isAction = true;

        result = result.mid(4);
        visualization = QString("* %1 %2").arg(m_server->getNickname()).arg(result);
    }
    else
        visualization = result;

    appendQuery(recipient, visualization, true);

    if (!getServer())
        return;

    ::Query* query = m_server->getQueryByName(recipient);

    if (query)
    {
        if (isAction)
            query->appendAction(m_server->getNickname(), result);
        else
            query->appendQuery(m_server->getNickname(), result);

        return;
    }

    ::Channel* channel = m_server->getChannelByName(recipient);

    if (channel)
    {
        if (isAction)
            channel->appendAction(m_server->getNickname(), result);
        else
            channel->append(m_server->getNickname(), result);
    }
}

#include "chatwindow.moc"
