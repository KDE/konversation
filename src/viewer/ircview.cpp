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
  Copyright (C) 2004-2009 Eli Mackenzie <argonel@gmail.com>
*/

#include "ircview.h"
#include "channel.h"
#include "dcc/chat.h"
#include "application.h"
#include "mainwindow.h"
#include "viewcontainer.h"
#include "connectionmanager.h"
#include "highlight.h"
#include "server.h"
#include "sound.h"
#include "common.h"
#include "emoticons.h"
#include "notificationhandler.h"

#include <qstringlist.h>
#include <qregexp.h>
#include <qclipboard.h>
#include <qbrush.h>
#include <qevent.h>
#include <qmap.h>
#include <qcolor.h>
#include <qscrollbar.h>
#include <qcursor.h>

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
#include <kauthorized.h>
#include <KActionCollection>
#include <KToggleAction>
#include <KToolInvocation>

class QPixmap;
class QDropEvent;
class QDragEnterEvent;
class QEvent;

class KMenu;

class Server;
class ChatWindow;
class SearchBar;

#if 0
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
    m_nickPopup = 0;
    m_channelPopup = 0;

    m_rememberLineParagraph = -1;
    m_rememberLineDirtyBit = false;

    //m_disableEnsureCursorVisible = false;
    //m_wasPainted = false;

    setUndoRedoEnabled(0);
    //setLinkUnderline(false);
    document()->setDefaultStyleSheet("a.nick:link {text-decoration: none}");
    //setVScrollBarMode(AlwaysOn);
    //setHScrollBarMode(AlwaysOff);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    //setNotifyClick(true); // TODO FIXME import the rest of the link handling
    setFocusPolicy(Qt::ClickFocus);
    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    viewport()->setMouseTracking(true);

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
    m_popup->insertItem(KIcon("edit-copy"),i18n("&Copy"),Copy);
    m_popup->insertItem(i18n("Select All"),SelectAll);
    m_popup->insertItem(KIcon("edit-find"),i18n("Find Text..."),Search);

    setServer(newServer);

    setViewBackground(Preferences::self()->color(Preferences::TextViewBackground),QString());

    if (Preferences::self()->customTextFont())
        setFont(Preferences::self()->textFont());
    else
        setFont(KGlobalSettings::generalFont());

    if (Preferences::self()->useParagraphSpacing()) enableParagraphSpacing();

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
        m_popup->addSeparator();
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

bool IRCView::search(const QString& pattern, bool caseSensitive, bool wholeWords, bool forward, bool fromCursor)
{
    if (pattern.isEmpty())
        return true;

    m_pattern       = pattern;
    m_forward       = forward;
    m_searchFlags = 0;
    if (caseSensitive)
        m_searchFlags |= QTextDocument::FindCaseSensitively;
    if (wholeWords)
        m_searchFlags |= QTextDocument::FindWholeWords;
    if (!fromCursor)
        m_forward ? moveCursor(QTextCursor::Start) : moveCursor(QTextCursor::End);

    return searchNext();
}

bool IRCView::searchNext(bool reversed)
{
    bool fwd = (reversed ? !m_forward : m_forward);
    if (fwd) {
        moveCursor(QTextCursor::EndOfWord);
        m_searchFlags &= ~QTextDocument::FindBackward;
    }
    else {
        moveCursor(QTextCursor::StartOfWord);
        m_searchFlags |= QTextDocument::FindBackward;
    }
    return find(m_pattern, m_searchFlags);
}

void IRCView::searchAgain(){}
void IRCView::insertRememberLine(){}
void IRCView::cancelRememberLine(){}
void IRCView::insertMarkerLine(){}
void IRCView::clearLines(){}
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
    QString channelColor = Preferences::self()->color(Preferences::ChannelMessage).name();

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
    QColor channelColor=Preferences::self()->color(Preferences::ChannelMessage);
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
    QString queryColor=Preferences::self()->color(Preferences::QueryMessage).name();

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
    QString actionColor=Preferences::self()->color(Preferences::ActionMessage).name();

    QString nickLine = createNickLine(nick, false);

    QString line;
    line = "<p><font color=\"" + actionColor + "\">%1 * " + nickLine + " %3</font></p>\n";
    line = line.arg(timeStamp(), nick, filter(message, actionColor, nick, true));

    emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendServerMessage(const QString& type, const QString& message, bool parseURL)
{
    QString serverColor = Preferences::self()->color(Preferences::ServerMessage).name();
    m_tabNotification = Konversation::tnfControl;

    // Fixed width font option for MOTD
    QString fixed;
    if(Preferences::self()->fixedMOTD() && !m_fontDataBase.isFixedPitch(font().family()))
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
    if (Preferences::self()->hideUnimportantEvents() && !important)
        return;

    QString commandColor = Preferences::self()->color(Preferences::CommandMessage).name();
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
    QString backlogColor = Preferences::self()->color(Preferences::BacklogMessage).name();
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

    int scrollMax = Preferences::self()->scrollbackMax();
    if (scrollMax != 0) {
        //don't remove lines if the user has scrolled up to read old lines
        bool atBottom = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
        setMaximumBlockCount(atBottom ? scrollMax : maximumBlockCount() + 1);
    }

    line.remove('\n'); // TODO why have newlines? we get <p>, so the \n are unnecessary...

    appendHtml(line);

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
    if(Preferences::self()->timestamping())
    {
        QTime time = QTime::currentTime();
        QString timeColor = Preferences::self()->color(Preferences::Time).name();
        QString timeFormat = Preferences::self()->timestampFormat();
        QString timeString;

        if(!Preferences::self()->showDate())
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

    if(Preferences::self()->useColoredNicks() && m_server)
    {
        QString nickColor;

        if (nick != m_server->getNickname())
            nickColor = Preferences::self()->nickColor(m_server->obtainNickInfo(nick)->getNickColor()).name();
        else
            nickColor =  Preferences::self()->nickColor(8).name();

        if(nickColor == "#000000")
        {
            nickColor = "#000001";                    // HACK Working around QTextBrowser's auto link coloring
        }

        nickLine = "<font color=\"" + nickColor + "\">"+nickLine+"</font>";
    }
    //FIXME: Another last-minute hack to get DCC Chat colored nicknames
    // working. We can't use NickInfo::getNickColor() because we don't
    // have a server.
    else if (Preferences::self()->useColoredNicks() && m_chatWin->getType() == ChatWindow::DccChat)
    {
        QString ownNick = static_cast<DccChat*>(m_chatWin)->getOwnNick();
        QString nickColor;

        if (nick != ownNick)
        {
            int nickvalue = 0;

            for (int index = 0; index < nick.length(); index++)
            {
                nickvalue += nick[index].unicode();
            }

            nickColor = Preferences::self()->nickColor((nickvalue % 8)).name();
        }
        else
            nickColor =  Preferences::self()->nickColor(8).name();

        if(nickColor == "#000000")
        {
            nickColor = "#000001";                    // HACK Working around QTextBrowser's auto link coloring
        }

        nickLine = "<font color=\"" + nickColor + "\">"+nickLine+"</font>";
    }

    if(Preferences::self()->useClickableNicks())
    {
        // HACK:Use space as a placeholder for \ as Qt tries to be clever and does a replace to / in urls in QTextEdit
        nickLine = "<a class=\"nick\" href=\"#" + QString(nick).replace('\\', " ") + "\">" + nickLine + "</a>";
    }

    if(privMsg)
    {
        nickLine.prepend ("-&gt; ");
    }

    if(encapsulateNick)
        nickLine = "&lt;" + nickLine + "&gt;";

    if(Preferences::self()->useBoldNicks())
        nickLine = "<b>" + nickLine + "</b>";

    return nickLine;
}

void IRCView::replaceDecoration(QString& line, char decoration, char replacement)
{
    int pos;
    bool decorated = false;

    while((pos=line.indexOf(decoration))!=-1)
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

    if(filteredLine.contains('\x07'))
    {
        if(Preferences::self()->beep())
        {
            kapp->beep();
        }
    }

    // replace \003 and \017 codes with rich text color codes
    // captures          1    2                   23 4                   4 3     1
    QRegExp colorRegExp("(\003([0-9]|0[0-9]|1[0-5]|)(,([0-9]|0[0-9]|1[0-5])|,|)|\017)");

    int pos;
    bool allowColors = Preferences::self()->allowColorCodes();
    bool firstColor = true;
    QString colorString;

    while((pos=colorRegExp.indexIn(filteredLine))!=-1)
    {
        if(!allowColors)
        {
            colorString = QString();
        }
        else
        {
            colorString = (firstColor) ? QString() : QString("</font>");

            // reset colors on \017 to default value
            if(colorRegExp.cap(1) == "\017")
                colorString += "<font color=\""+defaultColor+"\">";
            else
            {
                if(!colorRegExp.cap(2).isEmpty())
                {
                    int foregroundColor = colorRegExp.cap(2).toInt();
                    colorString += "<font color=\"" + Preferences::self()->ircColorCode(foregroundColor).name() + "\">";
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

        if(Preferences::self()->highlightNick() &&
            filteredLine.toLower().contains(QRegExp("(^|[^\\d\\w])" +
            QRegExp::escape(ownNick.toLower()) +
            "([^\\d\\w]|$)")))
        {
            // highlight current nickname
            highlightColor = Preferences::self()->highlightNickColor().name();
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
                    needleReg.setCaseSensitivity(Qt::CaseInsensitive);
                                                  // highlight regexp in text
                    patternFound = ((filteredLine.contains(needleReg)) ||
                                                  // highlight regexp in nickname
                        (whoSent.contains(needleReg)));

                    // remember captured patterns for later
                    captures=needleReg.capturedTexts();

                }
                else
                {
                    QString needle=highlight->getPattern();
                                                  // highlight patterns in text
                    patternFound = ((filteredLine.contains(needle, Qt::CaseInsensitive)) ||
                                                  // highlight patterns in nickname
                        (whoSent.contains(needle, Qt::CaseInsensitive)));
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

                if(Preferences::self()->highlightSoundsEnabled() && m_chatWin->notificationsEnabled())
                {
                    konvApp->sound()->play(highlight->getSoundURL());
                }

                konvApp->notificationHandler()->highlight(m_chatWin, whoSent, line);
                m_autoTextToSend = highlight->getAutoText();

                // replace %0 - %9 in regex groups
                for(int capture=0;capture<captures.count();capture++)
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
    else if(doHighlight && (whoSent == ownNick) && Preferences::self()->highlightOwnLines())
    {
        // highlight own lines
        filteredLine = "<font color=\"" + Preferences::self()->highlightOwnLinesColor().name() +
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


// Mouse tracking

// HACK -- QPlainTextEdit doesn't provide an implementation of hitTest that QAbstractTextDocumentLayout::anchorAt can call, nor does it override QTextControl::mouseMoveEvent in a useful way, so lets track the mouse cursor

/* //Version from konvi3 for reference
void IRCView::contentsMouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressed && (m_pressPosition - ev->pos()).manhattanLength() > QApplication::startDragDistance()) {
        m_mousePressed = false;
        removeSelection();
        KURL ux = KURL::fromPathOrURL(m_urlToDrag);

        if (m_server && m_urlToDrag.startsWith("##")) {
            //FIXME consistent IRC URL serialization
            ux = QString("irc://%1:%2/%3").arg(m_server->getServerName()).arg(m_server->getPort()).arg(m_urlToDrag.mid(2));
        }
        else if (m_urlToDrag.startsWith("#"))
            ux = m_urlToDrag.mid(1);
        KURLDrag* u = new KURLDrag(ux, viewport());
        u->drag();
        return;
    }
    KTextBrowser::contentsMouseMoveEvent(ev);
}
*/
void IRCView::mouseMoveEvent(QMouseEvent *e)
{
    const QPoint pos = e->pos();
    QTextCharFormat fmt=cursorForPosition(pos).charFormat();
    if (m_fmtUnderMouse != fmt)
    {
        m_fmtUnderMouse = fmt;
        if (fmt.isAnchor())
            m_highlightedURL = fmt.anchorHref();
        else
            m_highlightedURL=QString();
    }
    highlightedSlot(m_highlightedURL);
    //it doesn't seem to do anything we're overly concerned about
    QPlainTextEdit::mouseMoveEvent(e);
}

void IRCView::mousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_urlToDrag = m_highlightedURL;

        if (!m_urlToDrag.isNull())
        {
            m_mousePressed = true;
            m_pressPosition = ev->pos();
            return;
        }
    }

    QPlainTextEdit::mousePressEvent(ev);
}

void IRCView::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::MidButton)
    {
        if (m_copyUrlMenu)
        {
            openLink(m_urlToCopy,true);
            return;
        }
        else
        {
            emit textPasted(true);
            return;
        }
    }
    if (ev->button() == Qt::LeftButton)
    {
        if (m_mousePressed && !m_highlightedURL.isNull())
        {
            if (ev->modifiers() == Qt::ShiftModifier)
                ;//saveLinkAs(m_highlightedURL);
            else
                openLink(m_highlightedURL);

            m_mousePressed = false;
            return;
        }
    }

    QPlainTextEdit::mouseReleaseEvent(ev);
}

// FIXME do we still care about newtab? looks like konqi has lots of config now..
void IRCView::openLink(const QString& url, bool)
{
    if (!url.isEmpty() && !url.startsWith("#"))
    {
        if (url.startsWith("irc://"))
        {
            KonversationApplication* konvApp = KonversationApplication::instance();
            konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, url);
        }
        else if (!Preferences::self()->useCustomBrowser() || url.startsWith("mailto:"))
        {
            if (url.startsWith("mailto:"))
                KToolInvocation::invokeMailer(KUrl(url));
            else
                KToolInvocation::invokeBrowser(url);
        }
        else
        {
            QString cmd = Preferences::self()->webBrowserCmd();
            cmd.replace("%u", url);
            KProcess *proc = new KProcess;
            QStringList cmdAndArgs = KShell::splitArgs(cmd);
            *proc << cmdAndArgs;
            //      This code will also work, but starts an extra shell process.
            //      kdDebug() << "IRCView::urlClickSlot(): cmd = " << cmd << endl;
            //      *proc << cmd;
            //      proc->setUseShell(true);
            proc->startDetached();
            delete proc;
        }
    }
    //FIXME: Don't do channel links in DCC Chats to begin with since they don't have a server.
    else if (url.startsWith("##") && m_server && m_server->isConnected())
    {
        QString channel(url);
        channel.replace("##", "#");
        m_server->sendJoinCommand(channel);
    }
    //FIXME: Don't do user links in DCC Chats to begin with since they don't have a server.
    else if (url.startsWith("#") && m_server && m_server->isConnected()) 
    {
        QString recipient(url);
        recipient.remove("#");
        NickInfoPtr nickInfo = m_server->obtainNickInfo(recipient);
        m_server->addQuery(nickInfo, true /*we initiated*/);
    }
}

void IRCView::highlightedSlot(const QString& _link)
{
    QString link = _link;
    // HACK Replace % with \x03 in the url to keep Qt from doing stupid things
    link = link.replace ('\x03', "%");
    //Hack to handle the fact that we get a decoded url
    //FIXME someone who knows what it looks like when we get a decoded url can reenable this if necessary...
    //link = KUrl(link).url();

    // HACK:Use space as a placeholder for \ as Qt tries to be clever and does a replace to / in urls in QTextEdit
    if(link.startsWith("#"))
    {
        link = link.replace(' ', "\\");
    }

    //we just saw this a second ago.  no need to reemit.
    if (link == m_lastStatusText && !link.isEmpty())
        return;

    // remember current URL to overcome link clicking problems in QTextBrowser
    //m_highlightedURL = link;

    if (link.isEmpty())
    {
        if (!m_lastStatusText.isEmpty()) 
        {
            emit clearStatusBarTempText();
            m_lastStatusText = QString();
        }
    } else
    {
        m_lastStatusText = link;
    }

    if(!link.startsWith("#"))
    {
        m_isOnNick = false;
        m_isOnChannel = false;

        if (!link.isEmpty()) {
            //link therefore != m_lastStatusText  so emit with this new text
            emit setStatusBarTempText(link);
        }
        if (link.isEmpty() && m_copyUrlMenu)
        {
            m_popup->removeItem(CopyUrl);
            m_popup->removeItem(Bookmark);
            m_popup->removeItem(SaveAs);
            m_popup->setItemVisible(copyUrlMenuSeparator, false);
            m_copyUrlMenu = false;

        }
        else if (!link.isEmpty() && !m_copyUrlMenu)
        {
            m_popup->setItemVisible(copyUrlMenuSeparator, true);
            m_popup->insertItem(SmallIcon("editcopy"), i18n("Copy URL to Clipboard"), CopyUrl, 1);
            m_popup->insertItem(SmallIcon("bookmark"), i18n("Add to Bookmarks"), Bookmark, 2);
            m_popup->insertItem(SmallIcon("filesaveas"), i18n("Save Link As..."), SaveAs, 3);
            m_copyUrlMenu = true;
            m_urlToCopy = link;
        }
    }
    else if (link.startsWith("#") && !link.startsWith("##"))
    {
        m_currentNick = link.mid(1);
        //FIXME how are menu titles done now? /me is too tired
        //m_nickPopup->changeTitle(m_nickPopupId,m_currentNick);
        m_isOnNick = true;
        emit setStatusBarTempText(i18n("Open a query with %1", m_currentNick));
    }
    else
    {
        // link.startsWith("##")
        m_currentChannel = link.mid(1);

        QString prettyId = m_currentChannel;

        if (prettyId.length()>15)
        {
            prettyId.truncate(15);
            prettyId.append("...");
        }

        //m_channelPopup->changeTitle(m_channelPopupId,prettyId);
        m_isOnChannel = true;
        emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
    }
}

// **WARNING** the selectionChange signal comes BEFORE the selection has actually been changed, hook cursorPositionChanged too
//void IRCView::mouseDoubleClickEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos)

