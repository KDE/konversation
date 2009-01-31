// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005-2007 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "ircview.h"
#include "channel.h"
#include "dcc/chat.h"
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "viewcontainer.h"
#include "connectionmanager.h"
#include "highlight.h"
#include "server.h"
#include "sound.h" ////// header renamed
#include "common.h"
#include "emoticons.h"
#include "notificationhandler.h"

#include <qstringlist.h>
#include <qregexp.h>
#include <qclipboard.h>
#include <qbrush.h>
#include <qevent.h>
#include <q3dragobject.h>
#include <q3popupmenu.h>
#include <qmap.h>
#include <qcolor.h>
#include <qscrollbar.h>
#include <qcursor.h>
//Added by qt3to4:
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <Q3StrList>
#include <QPixmap>
#include <QMouseEvent>
#include <QShowEvent>
#include <Q3ValueList>
#include <QKeyEvent>
#include <QHideEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <Q3CString>
#include <Q3PtrList>

#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>
//#include <kurldrag.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdeversion.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <kiconloader.h>
#include <kshell.h>
#include <kmenu.h>
#include <kaction.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kmenubar.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kstdaccel.h>
#include <kglobal.h>
#include <QTextDocument>
#include <kauthorized.h>
#include <KActionCollection>
#include <KToggleAction>

class QPixmap;
class Q3StrList;
class QDropEvent;
class QDragEnterEvent;
class QEvent;

class KMenu;

class Server;
class ChatWindow;
class SearchBar;

#if chew
//IRCView::getPopup() const
//IRCView::searchNext(bool)
IRCView::clear()
//IRCView::search(QString const&, bool, bool, bool, bool)
//IRCView::setNickAndChannelContextMenusEnabled(bool)
//IRCView::setupNickPopupMenu()
//IRCView::enableParagraphSpacing()
//IRCView::setViewBackground(QColor const&, QString const&)
//IRCView::getContextNick() const
//IRCView::setupQueryPopupMenu() { m_nickPopup = 0; }
//IRCView::hasLines()
//IRCView::setupChannelPopupMenu()
#endif


IRCView::IRCView(QWidget* parent, Server* newServer) : QPlainTextEdit(parent)
{

    m_copyUrlMenu = false;
    m_resetScrollbar = true;
    m_offset = 0;
    m_mousePressed = false;
    m_isOnNick = false;
    m_isOnChannel = false;
    m_chatWin = 0;
    m_findParagraph=0;
    m_findIndex=0;
    m_nickPopup = 0;
    m_channelPopup = 0;

    m_rememberLineParagraph = -1;
    m_rememberLineDirtyBit = false;

    //m_disableEnsureCursorVisible = false;
    //m_wasPainted = false;

    setUndoRedoEnabled(0);
    //setLinkUnderline(false);
    //setVScrollBarMode(AlwaysOn);
    //setHScrollBarMode(AlwaysOff);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    //setNotifyClick(true); // TODO FIXME import the rest of the link handling
    setFocusPolicy(Qt::ClickFocus);
    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);


//     // set basic style sheet for <p> to make paragraph spacing possible
//     Q3StyleSheet* sheet=new Q3StyleSheet(this,"ircview_style_sheet");
//     new Q3StyleSheetItem(sheet,"p");
//     setStyleSheet(sheet);

    m_popup = new KMenu(this);
    m_popup->setObjectName("ircview_context_menu");
    toggleMenuBarSeparator = m_popup->insertSeparator();
    m_popup->setItemVisible(toggleMenuBarSeparator, false);
    copyUrlMenuSeparator = m_popup->insertSeparator();
    m_popup->setItemVisible(copyUrlMenuSeparator, false);
    m_popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
    m_popup->insertItem(i18n("Select All"),SelectAll);
    m_popup->insertItem(SmallIcon("find"),i18n("Find Text..."),Search);

    setServer(newServer);

    setViewBackground(Preferences::color(Preferences::TextViewBackground),QString());

    if (Preferences::customTextFont())
        setFont(Preferences::textFont());
    else
        setFont(KGlobalSettings::generalFont());

    if (Preferences::useParagraphSpacing()) enableParagraphSpacing();

    connect(this, SIGNAL(highlighted(const QString&)), this, SLOT(highlightedSlot(const QString&)));
}

IRCView::~IRCView()
{
    delete m_popup;
}

void IRCView::setServer(Server* newServer)
{
    m_server = newServer;

/*    if (newServer)
    {
        KAction *action = newServer->getViewContainer()->actionCollection()->action("open_logfile");
        Q_ASSERT(action);
        if(!action) return;
        m_popup->insertSeparator();
        action->plug(m_popup);
    }
*/
}

void IRCView::setChatWin(ChatWindow* chatWin)
{
    m_chatWin = chatWin;

    if(m_chatWin->getType()==ChatWindow::Channel)
        setupNickPopupMenu();
    else
        setupQueryPopupMenu();

    setupChannelPopupMenu();
}

void IRCView::search()
{
    emit doSearch();
}
bool IRCView::search(QString const&, bool, bool, bool, bool) { return false; }
void IRCView::searchAgain(){}
void IRCView::insertRememberLine(){}
void IRCView::cancelRememberLine(){}
void IRCView::insertMarkerLine(){}
void IRCView::clearLines(){}
bool IRCView::searchNext(bool) { return false; }
bool IRCView::hasLines() { return false; }

// TODO FIXME can't do this anymore, need to find another way
/* void IRCView::clear()
{
    m_buffer = QString();
    KTextBrowser::setText("");
    wipeLineParagraphs();
}
*/

void IRCView::enableParagraphSpacing() {}
void IRCView::setViewBackground(QColor const&, QString const&) {}

// Data insertion

void IRCView::append(const QString& nick, const QString& message)
{
    QString channelColor = Preferences::color(Preferences::ChannelMessage).name();

    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick);

    QString line;
    line = "<p><font color=\"" + channelColor + "\">%1" + nickLine + " %3</font></p>\n";
    line = line.arg(timeStamp(), nick, filter(message, channelColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendRaw(const QString& message, bool suppressTimestamps, bool self)
{
    QColor channelColor=Preferences::color(Preferences::ChannelMessage);
    m_tabNotification = Konversation::tnfNone;

    QString line;
    if (suppressTimestamps)
        line = QString("<p><font color=\"" + channelColor.name() + "\">" + message + "</font></p>\n");
    else
        line = QString("<p>" + timeStamp() + " <font color=\"" + channelColor.name() + "\">" + message + "</font></p>\n");

    doAppend(line, self);
}

void IRCView::appendQuery(const QString& nick, const QString& message, bool inChannel)
{
    QString queryColor=Preferences::color(Preferences::QueryMessage).name();

    m_tabNotification = Konversation::tnfPrivate;

    QString nickLine = createNickLine(nick, true, inChannel);

    QString line;
    line = "<p><font color=\"" + queryColor + "\">%1 " + nickLine + " %3</font></p>\n";
    line = line.arg(timeStamp(), nick, filter(message, queryColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendChannelAction(const QString& nick, const QString& message)
{
    m_tabNotification = Konversation::tnfNormal;
    appendAction(nick, message);
}

void IRCView::appendQueryAction(const QString& nick, const QString& message)
{
    m_tabNotification = Konversation::tnfPrivate;
    appendAction(nick, message);
}

void IRCView::appendAction(const QString& nick, const QString& message)
{
    QString actionColor=Preferences::color(Preferences::ActionMessage).name();

    QString nickLine = createNickLine(nick, false);

    QString line;
    line = "<p><font color=\"" + actionColor + "\">%1 * " + nickLine + " %3</font></p>\n";
    line = line.arg(timeStamp(), nick, filter(message, actionColor, nick, true));

    emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendServerMessage(const QString& type, const QString& message, bool parseURL)
{
    QString serverColor = Preferences::color(Preferences::ServerMessage).name();
    m_tabNotification = Konversation::tnfControl;

    // Fixed width font option for MOTD
    QString fixed;
    if(Preferences::fixedMOTD() && !m_fontDataBase.isFixedPitch(font().family()))
    {
        if(type == i18n("MOTD"))
            fixed=" face=\"" + KGlobalSettings::fixedFont().family() + "\"";
    }

    QString line;
    line = "<p><font color=\"" + serverColor + "\"" + fixed + ">%1 <b>[</b>%2<b>]</b> %3</font></p>\n";
    if(type != i18n("Notify"))
        line = line.arg(timeStamp(), type, filter(message, serverColor, 0 , true, parseURL));
    else
        line = "<font color=\"" + serverColor + "\">"+line.arg(timeStamp(), type, message)+"</font>";

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool important, bool parseURL, bool self)
{
    if (Preferences::hideUnimportantEvents() && !important)
        return;

    QString commandColor = Preferences::color(Preferences::CommandMessage).name();
    QString prefix="***";
    m_tabNotification = Konversation::tnfControl;

    if(type == i18n("Join"))
    {
        prefix="-->";
        parseURL=false;
    }
    else if(type == i18n("Part") || type == i18n("Quit"))
    {
        prefix="<--";
    }

    prefix=Qt::escape(prefix);

    QString line;
    line = "<p><font color=\"" + commandColor + "\">%1 %2 %3</font></p>\n";

    line = line.arg(timeStamp(), prefix, filter(message, commandColor, 0, true, parseURL, self));

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line, self);
}

void IRCView::appendBacklogMessage(const QString& firstColumn,const QString& rawMessage)
{
    QString time;
    QString message = rawMessage;
    QString nick = firstColumn;
    QString backlogColor = Preferences::color(Preferences::BacklogMessage).name();
    m_tabNotification = Konversation::tnfNone;

    time = nick.section(' ', 0, 4);
    nick = nick.section(' ', 5);

    if(!nick.isEmpty() && !nick.startsWith("<") && !nick.startsWith("*"))
    {
        nick = '|' + nick + '|';
    }

    // Nicks are in "<nick>" format so replace the "<>"
    nick.replace("<","&lt;");
    nick.replace(">","&gt;");

    QString line;

    line = "<p><font color=\"" + backlogColor + "\">%1 %2 %3</font></p>\n";
    line = line.arg(time, nick, filter(message, backlogColor, NULL, false, false));

    doAppend(line);
}

void IRCView::doAppend(const QString& newLine, bool self)
{
    QString line(newLine);

    if (!self && m_chatWin)
        m_chatWin->activateTabNotification(m_tabNotification);

    // scroll view only if the scroll bar is already at the bottom
    bool doScroll = (verticalScrollBar()->value() == verticalScrollBar()->maximum());

    line.remove('\n'); // TODO why have newlines? we get <p>, so the \n are unnecessary...

    appendHtml(line);

    if (doScroll)
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());

    //FIXME: Disable auto-text for DCC Chats since we don't have a server to parse wildcards.
    if (!m_autoTextToSend.isEmpty() && m_server)
    {
        // replace placeholders in autoText
        QString sendText = m_server->parseWildcards(m_autoTextToSend,m_server->getNickname(),
            QString(), QString(), QString(), QString());
        // avoid recursion due to signalling
        m_autoTextToSend.clear();
        // send signal only now
        emit autoText(sendText);
    }
    else
    {
        m_autoTextToSend.clear();
    }

    if (!m_lastStatusText.isEmpty())
        emit clearStatusBarTempText();
}

QString IRCView::timeStamp()
{
    if(Preferences::timestamping())
    {
        QTime time = QTime::currentTime();
        QString timeColor = Preferences::color(Preferences::Time).name();
        QString timeFormat = Preferences::timestampFormat();
        QString timeString;

        if(!Preferences::showDate())
        {
            timeString = QString("<font color=\"" + timeColor + "\">[%1]</font> ").arg(time.toString(timeFormat));
        }
        else
        {
            QDate date = QDate::currentDate();
            timeString = QString("<font color=\"" +
                timeColor + "\">[%1 %2]</font> ")
                    .arg(KGlobal::locale()->formatDate(date, KLocale::ShortDate),
                         time.toString(timeFormat));
        }

        return timeString;
    }

    return QString();
}

QString IRCView::createNickLine(const QString& nick, bool encapsulateNick, bool privMsg)
{
    QString nickLine = "%2";

    if(Preferences::useClickableNicks())
    {
        // HACK:Use space as a placeholder for \ as Qt tries to be clever and does a replace to / in urls in QTextEdit
        nickLine = "<a href=\"#" + QString(nick).replace('\\', " ") + "\">%2</a>";
    }

    if(privMsg)
    {
        nickLine.prepend ("-&gt; ");
    }

    if(encapsulateNick)
        nickLine = "&lt;" + nickLine + "&gt;";

    if(Preferences::useColoredNicks() && m_server)
    {
        QString nickColor;

        if (nick != m_server->getNickname())
            nickColor = Preferences::nickColor(m_server->obtainNickInfo(nick)->getNickColor()).name();
        else
            nickColor =  Preferences::nickColor(8).name();

        if(nickColor == "#000000")
        {
            nickColor = "#000001";                    // HACK Working around QTextBrowser's auto link coloring
        }

        nickLine = "<font color=\"" + nickColor + "\">"+nickLine+"</font>";
    }
    //FIXME: Another last-minute hack to get DCC Chat colored nicknames
    // working. We can't use NickInfo::getNickColor() because we don't
    // have a server.
    else if (Preferences::useColoredNicks() && m_chatWin->getType() == ChatWindow::DccChat)
    {
        QString ownNick = static_cast<DccChat*>(m_chatWin)->getOwnNick();
        QString nickColor;

        if (nick != ownNick)
        {
            int nickvalue = 0;

            for (uint index = 0; index < nick.length(); index++)
            {
                nickvalue += nick[index].unicode();
            }

            nickColor = Preferences::nickColor((nickvalue % 8)).name();
        }
        else
            nickColor =  Preferences::nickColor(8).name();

        if(nickColor == "#000000")
        {
            nickColor = "#000001";                    // HACK Working around QTextBrowser's auto link coloring
        }

        nickLine = "<font color=\"" + nickColor + "\">"+nickLine+"</font>";
    }

    if(Preferences::useBoldNicks())
        nickLine = "<b>" + nickLine + "</b>";

    return nickLine;
}

void IRCView::replaceDecoration(QString& line, char decoration, char replacement)
{
    int pos;
    bool decorated = false;

    while((pos=line.find(decoration))!=-1)
    {
        line.replace(pos,1,(decorated) ? QString("</%1>").arg(replacement) : QString("<%1>").arg(replacement));
        decorated = !decorated;
    }
}

QString IRCView::filter(const QString& line, const QString& defaultColor, const QString& whoSent,
bool doHighlight, bool parseURL, bool self)
{
    QString filteredLine(line);
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    //Since we can't turn off whitespace simplification withouteliminating text wrapping,
    //  if the line starts with a space turn it into a non-breaking space.
    //    (which magically turns back into a space on copy)

    if (filteredLine[0]==' ')
        filteredLine[0]='\xA0';

    // TODO: Use QStyleSheet::escape() here
    // Replace all < with &lt;
    filteredLine.replace("<","\x0blt;");
    // Replace all > with &gt;
    filteredLine.replace(">", "\x0bgt;");

    #if 0
    if(!Preferences::disableExpansion())
    {
        QRegExp boldRe("\\*\\*([a-zA-Z0-9]+)\\*\\*");
        QRegExp underRe("\\_\\_([a-zA-Z0-9]+)\\_\\_");
        int position = 0;
        QString replacement;

        while( position >= 0)
        {
            position = boldRe.search(filteredLine, position);
            if( position > -1)
            {
                replacement = boldRe.cap(1);
                replacement = "\x02"+replacement+"\x02";
                filteredLine.replace(position,replacement.length()+2,replacement);
            }
            position += boldRe.matchedLength();
        }

        position = 0;
        while( position >= 0)
        {
            position = underRe.search(filteredLine, position);
            if( position > -1)
            {
                replacement = underRe.cap(1);
                replacement = "\x1f"+replacement+"\x1f";
                filteredLine.replace(position,replacement.length()+2,replacement);
            }
            position += underRe.matchedLength();
        }
    }
    #endif

    if(filteredLine.find("\x07") != -1)
    {
        if(Preferences::beep())
        {
            kapp->beep();
        }
    }

    // replace \003 and \017 codes with rich text color codes
    // captures          1    2                   23 4                   4 3     1
    QRegExp colorRegExp("(\003([0-9]|0[0-9]|1[0-5]|)(,([0-9]|0[0-9]|1[0-5])|,|)|\017)");

    int pos;
    bool allowColors = Preferences::allowColorCodes();
    bool firstColor = true;
    QString colorString;

    while((pos=colorRegExp.search(filteredLine))!=-1)
    {
        if(!allowColors)
        {
            colorString = QString();
        }
        else
        {
            colorString = (firstColor) ? QString::null : QString("</font>");

            // reset colors on \017 to default value
            if(colorRegExp.cap(1) == "\017")
                colorString += "<font color=\""+defaultColor+"\">";
            else
            {
                if(!colorRegExp.cap(2).isEmpty())
                {
                    int foregroundColor = colorRegExp.cap(2).toInt();
                    colorString += "<font color=\"" + Preferences::ircColorCode(foregroundColor).name() + "\">";
                }
                else
                {
                    colorString += "<font color=\""+defaultColor+"\">";
                }
            }

            firstColor = false;
        }

        filteredLine.replace(pos, colorRegExp.cap(0).length(), colorString);
    }

    if(!firstColor)
        filteredLine+="</font>";

    // Replace all text decorations
    // TODO: \017 should reset all textt decorations to plain text
    replaceDecoration(filteredLine,'\x02','b');
    replaceDecoration(filteredLine,'\x09','i');
    replaceDecoration(filteredLine,'\x13','s');
    replaceDecoration(filteredLine,'\x15','u');
    replaceDecoration(filteredLine,'\x16','b');   // should be inverse
    replaceDecoration(filteredLine,'\x1f','u');

    if(parseURL)
    {
        filteredLine = Konversation::tagURLs(filteredLine, whoSent);
    }
    else
    {
        // Change & to &amp; to prevent html entities to do strange things to the text
        filteredLine.replace('&', "&amp;");
        filteredLine.replace("\x0b", "&");
    }

    filteredLine = Konversation::Emoticons::parseEmoticons(filteredLine);

    // Highlight
    QString ownNick;

    if (m_server)
    {
        ownNick = m_server->getNickname();
    }
    else if (m_chatWin->getType() == ChatWindow::DccChat)
    {
        ownNick = static_cast<DccChat*>(m_chatWin)->getOwnNick();
    }

    if(doHighlight && (whoSent != ownNick) && !self)
    {
        QString highlightColor;

        if(Preferences::highlightNick() &&
            filteredLine.toLower().find(QRegExp("(^|[^\\d\\w])" +
            QRegExp::escape(ownNick.toLower()) +
            "([^\\d\\w]|$)")) != -1)
        {
            // highlight current nickname
            highlightColor = Preferences::highlightNickColor().name();
            m_tabNotification = Konversation::tnfNick;
        }
        else
        {
            Q3PtrList<Highlight> highlightList = Preferences::highlightList();
            Q3PtrListIterator<Highlight> it(highlightList);
            Highlight* highlight = it.current();
            bool patternFound = false;
            int index = 0;

            QStringList captures;
            while(highlight)
            {
                if(highlight->getRegExp())
                {
                    QRegExp needleReg(highlight->getPattern());
                    needleReg.setCaseSensitive(false);
                                                  // highlight regexp in text
                    patternFound = ((filteredLine.find(needleReg) != -1) ||
                                                  // highlight regexp in nickname
                        (whoSent.find(needleReg) != -1));

                    // remember captured patterns for later
                    captures=needleReg.capturedTexts();

                }
                else
                {
                    QString needle=highlight->getPattern();
                                                  // highlight patterns in text
                    patternFound = ((filteredLine.find(needle, 0, false) != -1) ||
                                                  // highlight patterns in nickname
                        (whoSent.find(needle, 0, false) != -1));
                }

                if(!patternFound)
                {
                    ++it;
                    highlight = it.current();
                    ++index;
                }
                else
                {
                    break;
                }
            }

            if(patternFound)
            {
                highlightColor = highlight->getColor().name();
                m_highlightColor = highlightColor;
                m_tabNotification = Konversation::tnfHighlight;

                if(Preferences::highlightSoundsEnabled() && m_chatWin->notificationsEnabled())
                {
                    konvApp->sound()->play(highlight->getSoundURL());
                }

                konvApp->notificationHandler()->highlight(m_chatWin, whoSent, line);
                m_autoTextToSend = highlight->getAutoText();

                // replace %0 - %9 in regex groups
                for(unsigned int capture=0;capture<captures.count();capture++)
                {
                  m_autoTextToSend.replace(QString("%%1").arg(capture),captures[capture]);
                }
                m_autoTextToSend.replace(QRegExp("%[0-9]"),QString());
            }
        }

        // apply found highlight color to line
        if(!highlightColor.isEmpty())
        {
            filteredLine = "<font color=\"" + highlightColor + "\">" + filteredLine + "</font>";
        }
    }
    else if(doHighlight && (whoSent == ownNick) && Preferences::highlightOwnLines())
    {
        // highlight own lines
        filteredLine = "<font color=\"" + Preferences::highlightOwnLinesColor().name() +
            "\">" + filteredLine + "</font>";
    }

    // Replace pairs of spaces with "<space>&nbsp;" to preserve some semblance of text wrapping
    filteredLine.replace("  "," \xA0");
    return filteredLine;
}


//Context Menu

const QString& IRCView::getContextNick() const
{
    return m_currentNick;
}

void IRCView::clearContextNick()
{
    m_currentNick = QString();
}

KMenu* IRCView::getPopup() const
{
    return m_popup;
}

void IRCView::setNickAndChannelContextMenusEnabled(bool enable)
{
    if (m_nickPopup) m_nickPopup->setEnabled(enable);
    if (m_channelPopup) m_channelPopup->setEnabled(enable);
}

void IRCView::setupNickPopupMenu() { m_nickPopup = 0; }

void IRCView::setupQueryPopupMenu() { m_nickPopup = 0; }

void IRCView::setupChannelPopupMenu() { m_nickPopup = 0; }
