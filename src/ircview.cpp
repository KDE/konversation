// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  the text widget used for all text based panels
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <private/qrichtext_p.h>

#include <qstylesheet.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>
#include <qclipboard.h>
#include <qbrush.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qpopupmenu.h>
#include <qwhatsthis.h>
#include <qmap.h>
#include <qcolor.h>
#include <qscrollbar.h>
#include <qcursor.h>

#include <dcopref.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdeversion.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <kprocess.h>
#include <kiconloader.h>
#include <kshell.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kmenubar.h>

#include "channel.h"
#include "dccchat.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "viewcontainer.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"
#include "konversationsound.h"
#include "common.h"
#include "emoticon.h"
#include "notificationhandler.h"

IRCView::IRCView(QWidget* parent, Server* newServer) : KTextBrowser(parent)
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
    m_lastInsertionWasLine = false;

    setAutoFormatting(QTextEdit::AutoNone);
    setUndoRedoEnabled(0);
    setLinkUnderline(false);
    setVScrollBarMode(AlwaysOn);
    setHScrollBarMode(AlwaysOff);
    setWrapPolicy(QTextEdit::AtWordOrDocumentBoundary);
    setNotifyClick(true);
    setFocusPolicy(QWidget::ClickFocus);

    // set basic style sheet for <p> to make paragraph spacing possible
    QStyleSheet* sheet=new QStyleSheet(this,"ircview_style_sheet");
    new QStyleSheetItem(sheet,"p");
    setStyleSheet(sheet);

    m_popup = new QPopupMenu(this,"ircview_context_menu");
    toggleMenuBarSeparator = m_popup->insertSeparator();
    m_popup->setItemVisible(toggleMenuBarSeparator, false);
    m_popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
    m_popup->insertItem(i18n("Select All"),SelectAll);
    m_popup->insertSeparator();
    m_popup->insertItem(SmallIcon("find"),i18n("Find Text..."),Search);

    setServer(newServer);

    setViewBackground(Preferences::color(Preferences::TextViewBackground),QString::null);

    if (Preferences::customTextFont())
        setFont(Preferences::textFont());
    else
        setFont(KGlobalSettings::generalFont());

    connect(this, SIGNAL(highlighted(const QString&)), this, SLOT(highlightedSlot(const QString&)));
}

IRCView::~IRCView()
{
    delete m_popup;
}

void IRCView::updateStyleSheet()
{
    // set style sheet for <p> to define paragraph spacing
    QStyleSheet* sheet = styleSheet();

    if(!sheet)
        return;

    int paragraphSpacing;

    if(Preferences::useParagraphSpacing())
        paragraphSpacing=Preferences::paragraphSpacing();
    else
        paragraphSpacing = 0;

    QStyleSheetItem* style=sheet->item("p");

    if(!sheet)
    {
        kdDebug() << "IRCView::updateStyleSheet(): style == 0!" << endl;
        return;
    }

    style->setDisplayMode(QStyleSheetItem::DisplayBlock);
    style->setMargin(QStyleSheetItem::MarginVertical,paragraphSpacing);
    style->setSelfNesting(false);
}

void IRCView::setViewBackground(const QColor& backgroundColor, const QString& pixmapName)
{
    QPixmap backgroundPixmap;
    backgroundPixmap.load(pixmapName);

    if(backgroundPixmap.isNull())
    {
        setPaper(backgroundColor);
    }
    else
    {
        QBrush backgroundBrush;
        backgroundBrush.setColor(backgroundColor);
        backgroundBrush.setPixmap(backgroundPixmap);
        setPaper(backgroundBrush);
    }
}

void IRCView::setServer(Server* newServer)
{
    m_server = newServer;

    if (newServer)
    {
        KAction *action = newServer->getViewContainer()->actionCollection()->action("open_logfile");
        Q_ASSERT(action);
        if(!action) return;
        m_popup->insertSeparator();
        action->plug(m_popup);
    }

}

const QString& IRCView::getContextNick() const
{
    return m_currentNick;
}

void IRCView::clearContextNick()
{
    m_currentNick = QString::null;
}

void IRCView::clear()
{
    m_buffer = QString::null;
    KTextBrowser::clear();
}

void IRCView::highlightedSlot(const QString& _link)
{
    //Hack to handle the fact that we get a decoded url
    QString link = KURL::fromPathOrURL(_link).url();
    //we just saw this a second ago.  no need to reemit.
    if (link == m_lastStatusText && !link.isEmpty())
        return;

    // remember current URL to overcome link clicking problems in QTextBrowser
    m_highlightedURL = link;

    if (link.isEmpty())
    {
        if (!m_lastStatusText.isEmpty()) 
        {
            emit clearStatusBarTempText();
            m_lastStatusText = QString::null;
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
            m_copyUrlMenu = false;
        }
        else if (!link.isEmpty() && !m_copyUrlMenu)
        {
            m_popup->insertItem(i18n("Copy URL to Clipboard"),CopyUrl,1);
            m_popup->insertItem(i18n("Add to Bookmarks"),Bookmark,2);
            m_copyUrlMenu = true;
            m_urlToCopy = link;
        }
    }
    else if (link.startsWith("#") && !link.startsWith("##"))
    {
        m_currentNick = link.mid(1);
        m_nickPopup->changeTitle(m_nickPopupId,m_currentNick);
        m_isOnNick = true;
        emit setStatusBarTempText(i18n("Open a query with %1").arg(m_currentNick));
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

        m_channelPopup->changeTitle(m_channelPopupId,prettyId);
        m_isOnChannel = true;
        emit setStatusBarTempText(i18n("Join the channel %1").arg(m_currentChannel));
    }
}

void IRCView::openLink(const QString& url, bool newTab)
{
    if (!url.isEmpty() && !url.startsWith("#"))
    {
        // Always use KDE default mailer.
        if (!Preferences::useCustomBrowser() || url.startsWith("mailto:"))
        {
            if(newTab && !url.startsWith("mailto:"))
            {
                QCString foundApp, foundObj;
                QByteArray data;
                QDataStream str(data, IO_WriteOnly);
                if( KApplication::dcopClient()->findObject("konqueror*", "konqueror-mainwindow*",
                    "windowCanBeUsedForTab()", data, foundApp, foundObj, false, 3000))
                {
                    DCOPRef ref(foundApp, foundObj);
                    ref.call("newTab", url);
                }
            } else
                new KRun(KURL::fromPathOrURL(url));
        }
        else
        {
            QString cmd = Preferences::webBrowserCmd();
            cmd.replace("%u", url);
            KProcess *proc = new KProcess;
            QStringList cmdAndArgs = KShell::splitArgs(cmd);
            *proc << cmdAndArgs;
            //      This code will also work, but starts an extra shell process.
            //      kdDebug() << "IRCView::urlClickSlot(): cmd = " << cmd << endl;
            //      *proc << cmd;
            //      proc->setUseShell(true);
            proc->start(KProcess::DontCare);
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

    // Replace all 0x03 without color number (reset color) with \0x031,0 or \0x030,1, depending on which one fits
    // with the users chosen colours, based on the relative brightness. TODO defaultColor needs explanation

    bool inverted = false;                        // TODO this flag should be stored somewhere
    {
        QColor fg(Preferences::color(Preferences::ChannelMessage));
        QColor  bg(Preferences::color(Preferences::TextViewBackground));

        int h = 0, s = 0,fv = 0,bv = 0;
        fg.getHsv(&h,&s,&fv);
        bg.getHsv(&h,&s,&bv);

        if (bv <= fv)
        {
            inverted = false;
        }
    }

    if(inverted)
    {
        filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0030,1\\1");
    }
    else
    {
        filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0031,0\\1");
    }

    if(filteredLine.find("\x07") != -1)
    {
        if(Preferences::beep())
        {
            kapp->beep();
        }
    }

    // replace \003 and \017 codes with rich text color codes
    // captures          1    2                   23 4                   4 3     1
    QRegExp colorRegExp("(\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)");

    int pos;
    bool allowColors = Preferences::allowColorCodes();
    bool firstColor = true;
    QString colorString;

    while((pos=colorRegExp.search(filteredLine))!=-1)
    {
        if(!allowColors)
        {
            colorString = QString::null;
        }
        else
        {
            colorString = (firstColor) ? QString::null : QString("</font>");

            // reset colors on \017 to default value
            if(colorRegExp.cap(1) == "\017")
                colorString += "<font color=\""+defaultColor+"\">";
            else
            {
                int foregroundColor = colorRegExp.cap(2).toInt();
                colorString += "<font color=\"" + Preferences::ircColorCode(foregroundColor).name() + "\">";
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

    filteredLine = Konversation::EmotIcon::filter(filteredLine, fontMetrics());

    // Highlight
    QString ownNick;

    if (m_server)
    {
        ownNick = m_server->getNickname();
    }
    else if (m_chatWin->getType() == ChatWindow::DccChat)
    {
        ownNick = static_cast<DccChat*>(m_chatWin)->getMyNick();
    }

    if(doHighlight && (whoSent != ownNick) && !self)
    {
        QString highlightColor;

        if(Preferences::highlightNick() &&
            filteredLine.lower().find(QRegExp("(^|[^\\d\\w])" +
            QRegExp::escape(ownNick.lower()) +
            "([^\\d\\w]|$)")) != -1)
        {
            // highlight current nickname
            highlightColor = Preferences::highlightNickColor().name();
            m_tabNotification = Konversation::tnfNick;
        }
        else
        {
            QPtrList<Highlight> highlightList = Preferences::highlightList();
            QPtrListIterator<Highlight> it(highlightList);
            Highlight* highlight = it.current();
            bool patternFound = false;
            int index = 0;

            QStringList captures;
            while(highlight)
            {
                if(highlight->getRegExp())
                {
                    QRegExp needleReg=highlight->getPattern();
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
                    QString needle=highlight->getPattern().lower();
                                                  // highlight patterns in text
                    patternFound = ((filteredLine.lower().find(needle) != -1) ||
                                                  // highlight patterns in nickname
                        (whoSent.lower().find(needle) != -1));
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

                if(Preferences::highlightSoundsEnabled())
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
                m_autoTextToSend.replace(QRegExp("%[0-9]"),QString::null);
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

    // Replace multiple Spaces with "<space>&nbsp;"
    while((pos = filteredLine.find("  ")) != -1)
    {
        filteredLine.replace(pos + (pos == 0 ? 0 : 1), 1, "&nbsp;");
    }

    return filteredLine;
}

QString IRCView::createNickLine(const QString& nick, bool encapsulateNick)
{
    QString nickLine = "%2";

    if(Preferences::useClickableNicks())
        nickLine = "<a href=\"#" + nick + "\">%2</a>";

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
        QString ownNick = static_cast<DccChat*>(m_chatWin)->getMyNick();
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

void IRCView::append(const QString& nick,const QString& message)
{
    QString channelColor = Preferences::color(Preferences::ChannelMessage).name();

    if(channelColor  == "#000000")
    {
        channelColor = "#000001";              // HACK Working around QTextBrowser's auto link coloring
    }

    QString line;
    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick);

    if(basicDirection(message) == QChar::DirR)
    {
        line = RLE;
        line += LRE;
        line += "<p><font color=\"" + channelColor + "\">" + nickLine + " %1" + PDF + " %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + channelColor + "\">%1" + nickLine + " %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, channelColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    m_lastInsertionWasLine = false;

    doAppend(line);
}

void IRCView::appendLine()
{
    QColor channelColor=Preferences::color(Preferences::ChannelMessage);
    m_tabNotification = Konversation::tnfNone;

    QString line = "<p><font color=\"" + channelColor.name() + "\"><br><hr color=\""+Preferences::color(Preferences::CommandMessage).name()+"\" noshade></font></p>\n";
    if (!m_lastInsertionWasLine)
    {
        doAppend(line);
        m_lastInsertionWasLine = true;
    }
}

void IRCView::appendRaw(const QString& message, bool suppressTimestamps, bool self)
{
    QColor channelColor=Preferences::color(Preferences::ChannelMessage);
    QString line;
    m_tabNotification = Konversation::tnfNone;

    if(suppressTimestamps)
    {
        line = QString("<p><font color=\"" + channelColor.name() + "\">" + message + "</font></p>\n");
    }
    else
    {
        line = QString("<p>" + timeStamp() + " <font color=\"" + channelColor.name() + "\">" + message + "</font></p>\n");
    }

    m_lastInsertionWasLine = false;

    doAppend(line, true, self);
}

void IRCView::appendQuery(const QString& nick,const QString& message)
{
    QString queryColor=Preferences::color(Preferences::QueryMessage).name();

    if(queryColor  == "#000000")
    {
        queryColor = "#000001";                // HACK Working around QTextBrowser's auto link coloring
    }

    QString line;
    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick);

    if(basicDirection(message) == QChar::DirR)
    {
        line = RLE;
        line += LRE;
        line += "<p><font color=\"" + queryColor + "\">" + nickLine + " %1" + PDF + " %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + queryColor + "\">%1 " + nickLine + " %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, queryColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    m_lastInsertionWasLine = false;

    doAppend(line);
}

void IRCView::appendAction(const QString& nick,const QString& message)
{
    QString actionColor=Preferences::color(Preferences::ActionMessage).name();

    if(actionColor  == "#000000")
    {
        actionColor = "#000001";               // HACK Working around QTextBrowser's auto link coloring
    }

    QString line;
    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick, false);

    if(basicDirection(message) == QChar::DirR)
    {
        line = RLE;
        line += LRE;
        line += "<p><font color=\"" + actionColor + "\">" + nickLine + " * %1" + PDF + " %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + actionColor + "\">%1 * " + nickLine + " %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, actionColor, nick, true));

    emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

    m_lastInsertionWasLine = false;

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

    if(basicDirection(message) == QChar::DirR)
    {
        line = RLE;
        line += LRE;
        line += "<p><font color=\"" + serverColor + "\"" + fixed + "><b>[</b>%2<b>]</b> %1" + PDF + " %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + serverColor + "\"" + fixed + ">%1 <b>[</b>%2<b>]</b> %3</font></p>\n";
    }

    if(type != i18n("Notify"))
        line = line.arg(timeStamp(), type, filter(message, serverColor, 0 , true, parseURL));
    else
        line = "<font color=\"" + serverColor + "\">"+line.arg(timeStamp(), type, message)+"</font>";

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    m_lastInsertionWasLine = false;

    doAppend(line);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool important, bool parseURL, bool self)
{
    QString commandColor = Preferences::color(Preferences::CommandMessage).name();
    QString line;
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

    prefix=QStyleSheet::escape(prefix);

    if(basicDirection(message) == QChar::DirR)
    {
        line = RLE;
        line += LRE;
        line += "<p><font color=\"" + commandColor + "\">%2 %1" + PDF + " %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + commandColor + "\">%1 %2 %3</font></p>\n";
    }

    line = line.arg(timeStamp(), prefix, filter(message, commandColor, 0, true, parseURL, self));

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    m_lastInsertionWasLine = false;

    doAppend(line, important, self);
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

    if(basicDirection(message) == QChar::DirR)
    {
        line = "<p><font color=\"" + backlogColor + "\">%2 %1 %3</font></p>\n";
    }
    else
    {
        line = "<p><font color=\"" + backlogColor + "\">%1 %2 %3</font></p>\n";
    }

    line = line.arg(time, nick, filter(message, backlogColor, NULL, false, false));

    m_lastInsertionWasLine = false;

    doAppend(line);
}

//without any display update stuff that freaks out the scrollview
void IRCView::removeSelectedText( int selNum )
{
    QTextDocument* doc=document();

    for ( int i = 0; i < (int)doc->numSelections(); ++i )
    {
        if ( i == selNum )
            continue;
        doc->removeSelection( i );
    }
    // ...snip...

    doc->removeSelectedText( selNum, QTextEdit::textCursor() );

    // ...snip...
}

void IRCView::scrollToBottom()
{
    // QTextEdit::scrollToBottom does sync() too, but we don't want it because its slow
    setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
}

void IRCView::doAppend(const QString& newLine, bool important, bool self)
{
    // Add line to buffer
    QString line(newLine);

    if(important || !Preferences::hideUnimportantEvents())
    {
        if(!self)
        {
            emit updateTabNotification(m_tabNotification);
        }

        // scroll view only if the scroll bar is already at the bottom
        bool doScroll = ( KTextBrowser::verticalScrollBar()->value() == KTextBrowser::verticalScrollBar()->maxValue());

        line.remove('\n');                        // TODO why have newlines? we get <p>, so the \n are unnecessary...

        bool up = KTextBrowser::viewport()->isUpdatesEnabled();

        KTextBrowser::viewport()->setUpdatesEnabled(false);
        KTextBrowser::append(line);

        document()->lastParagraph()->format();
        resizeContents(contentsWidth(), document()->height());

        //Explanation: the scrolling mechanism cannot handle the buffer changing when the scrollbar is not
        // at an end, so the scrollbar wets its pants and forgets who it is for ten minutes
        // TODO: make this eat multiple lines at once when the preference is changed so it doesn't take so long
        if (doScroll)
        {
            int sbm = Preferences::scrollbackMax();
            if (sbm)
            {
                //loop for two reasons: 1) preference changed 2) lines added while scrolled up
                for(sbm = paragraphs() - sbm; sbm > 0; --sbm)
                    removeParagraph(0);
                resizeContents(contentsWidth(), document()->height());
            }
        }

        KTextBrowser::viewport()->setUpdatesEnabled(up);

        if(doScroll)
        {
            setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
            repaintContents(false);
        }
    }

    //FIXME: Disable auto-text for DCC Chats since we don't have a server
    // to parse wildcards.
    if (!m_autoTextToSend.isEmpty() && m_server)
    {
        // replace placeholders in autoText
        QString sendText = m_server->parseWildcards(m_autoTextToSend,m_server->getNickname(),
            QString::null, QString::null, QString::null, QString::null);
        // avoid recursion due to signalling
        m_autoTextToSend = QString::null;
        // send signal only now
        emit autoText(sendText);
    }
    else
    {
        m_autoTextToSend = QString::null;
    }

    if (!m_lastStatusText.isEmpty()) emit clearStatusBarTempText();
}

// remember if scrollbar was positioned at the end of the text or not
void IRCView::hideEvent(QHideEvent* /* event */) {
    m_resetScrollbar = ((contentsHeight()-visibleHeight()) == contentsY());
}

// Workaround to scroll to the end of the TextView when it's shown
void IRCView::showEvent(QShowEvent* event) {
    Q_UNUSED(event);
    // did the user scroll the view to the end of the text before hiding?
    if(m_resetScrollbar)
    {
        moveCursor(MoveEnd,false);
        ensureVisible(0,contentsHeight());
    }
}

void IRCView::contentsMouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::MidButton)
    {
        if(m_copyUrlMenu)
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

    if (ev->button() == QMouseEvent::LeftButton)
    {
        if (m_mousePressed)
        {
            openLink(m_highlightedURL);
            m_mousePressed = false;
            return;
        }
    }

    KTextBrowser::contentsMouseReleaseEvent(ev);
}

void IRCView::contentsMousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == QMouseEvent::LeftButton)
    {
        m_urlToDrag = m_highlightedURL;

        if (!m_urlToDrag.isNull())
        {
            m_mousePressed = true;
            m_pressPosition = ev->pos();
            return;
        }
    }

    KTextBrowser::contentsMousePressEvent(ev);
}

void IRCView::contentsMouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressed && (m_pressPosition - ev->pos()).manhattanLength() > QApplication::startDragDistance())
    {
        m_mousePressed = false;
        removeSelection();
        KURL ux = KURL::fromPathOrURL(m_urlToDrag);

        if (m_server && m_urlToDrag.startsWith("##"))
        {
            ux = QString("irc://%1:%2/%3").arg(m_server->getServerName()).arg(m_server->getPort()).arg(m_urlToDrag.mid(2));
        }
        else if (m_urlToDrag.startsWith("#"))
        {
            ux = m_urlToDrag.mid(1);
        }

        KURLDrag* u = new KURLDrag(ux, viewport());
        u->drag();
        return;
    }

    KTextBrowser::contentsMouseMoveEvent(ev);
}

void IRCView::contentsContextMenuEvent(QContextMenuEvent* ev)
{
    bool block = contextMenu(ev);
    // Hack to counter the fact that we're given an decoded url
    m_highlightedURL = KURL::fromPathOrURL(anchorAt(viewportToContents(mapFromGlobal(QCursor::pos())))).url();

    if (m_highlightedURL.isEmpty())
        viewport()->setCursor(Qt::ArrowCursor);

    if (!block)
        KTextBrowser::contentsContextMenuEvent(ev);
}

bool IRCView::contextMenu(QContextMenuEvent* ce)
{
    if (m_server && m_isOnNick && m_nickPopup->isEnabled())
    {
        updateNickMenuEntries(m_nickPopup, getContextNick());

        m_nickPopup->exec(ce->globalPos());
        m_isOnNick = false;
    }
    else if (m_server && m_isOnChannel && m_channelPopup->isEnabled())
    {
        m_channelPopup->exec(ce->globalPos());
        m_isOnChannel = false;
    }
    else
    {
        KActionCollection* actionCollection = KonversationApplication::instance()->getMainWindow()->actionCollection();
        KToggleAction* toggleMenuBarAction = static_cast<KToggleAction*>(actionCollection->action("options_show_menubar"));

        if (toggleMenuBarAction && !toggleMenuBarAction->isChecked())
        {
            toggleMenuBarAction->plug(m_popup, 0);
            m_popup->setItemVisible(toggleMenuBarSeparator, true);
        }

        m_popup->setItemEnabled(Copy,(hasSelectedText()));

        KAction* channelSettingsAction = 0;

        if (m_chatWin->getType() == ChatWindow::Channel)
        {
            channelSettingsAction = KonversationApplication::instance()->getMainWindow()->actionCollection()->action("channel_settings");
            if (channelSettingsAction) channelSettingsAction->plug(m_popup);
        }

        if (m_chatWin->getType() == ChatWindow::Query)
            updateNickMenuEntries(m_popup, m_chatWin->getName());

        int r = m_popup->exec(ce->globalPos());

        switch (r)
        {
            case -1:
                // dummy. -1 means, no entry selected. we don't want -1to go in default, so
                // we catch it here
                break;
            case Copy:
                copy();
                break;
            case CopyUrl:
            {
                QClipboard *cb = KApplication::kApplication()->clipboard();
                cb->setText(m_urlToCopy,QClipboard::Selection);
                cb->setText(m_urlToCopy,QClipboard::Clipboard);
                break;
            }
            case SelectAll:
                selectAll();
                break;
            case Search:
                search();
                break;
            case SendFile:
                emit sendFile();
                break;
            case Bookmark:
            {
                KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
                KBookmarkGroup bg = bm->addBookmarkDialog(m_urlToCopy, QString::null);
                bm->save();
                bm->emitChanged(bg);
                break;
            }
            default:
                emit extendedPopup(r);
        }

        if (toggleMenuBarAction)
        {
            toggleMenuBarAction->unplug(m_popup);
            m_popup->setItemVisible(toggleMenuBarSeparator, false);
        }

        if (channelSettingsAction) channelSettingsAction->unplug(m_popup);
    }
    return true;
}

void IRCView::setupNickPopupMenu()
{
    m_nickPopup = new KPopupMenu(this,"nicklist_context_menu");
    m_modes = new KPopupMenu(this,"nicklist_modes_context_submenu");
    m_kickban = new KPopupMenu(this,"nicklist_kick_ban_context_submenu");
    m_nickPopupId= m_nickPopup->insertTitle(m_currentNick);

    m_nickPopup->insertItem(i18n("&Whois"),Konversation::Whois);
    m_nickPopup->insertItem(i18n("&Version"),Konversation::Version);
    m_nickPopup->insertItem(i18n("&Ping"),Konversation::Ping);

    m_nickPopup->insertSeparator();

    m_modes->insertItem(i18n("Give Op"),Konversation::GiveOp);
    m_modes->insertItem(i18n("Take Op"),Konversation::TakeOp);
    m_modes->insertItem(i18n("Give Voice"),Konversation::GiveVoice);
    m_modes->insertItem(i18n("Take Voice"),Konversation::TakeVoice);
    m_nickPopup->insertItem(i18n("Modes"),m_modes,Konversation::ModesSub);

    m_kickban->insertItem(i18n("Kick"),Konversation::Kick);
    m_kickban->insertItem(i18n("Kickban"),Konversation::KickBan);
    m_kickban->insertItem(i18n("Ban Nickname"),Konversation::BanNick);
    m_kickban->insertSeparator();
    m_kickban->insertItem(i18n("Ban *!*@*.host"),Konversation::BanHost);
    m_kickban->insertItem(i18n("Ban *!*@domain"),Konversation::BanDomain);
    m_kickban->insertItem(i18n("Ban *!user@*.host"),Konversation::BanUserHost);
    m_kickban->insertItem(i18n("Ban *!user@domain"),Konversation::BanUserDomain);
    m_kickban->insertSeparator();
    m_kickban->insertItem(i18n("Kickban *!*@*.host"),Konversation::KickBanHost);
    m_kickban->insertItem(i18n("Kickban *!*@domain"),Konversation::KickBanDomain);
    m_kickban->insertItem(i18n("Kickban *!user@*.host"),Konversation::KickBanUserHost);
    m_kickban->insertItem(i18n("Kickban *!user@domain"),Konversation::KickBanUserDomain);
    m_nickPopup->insertItem(i18n("Kick / Ban"),m_kickban,Konversation::KickBanSub);

    m_nickPopup->insertItem(i18n("Ignore"), Konversation::IgnoreNick);
    m_nickPopup->insertItem(i18n("Unignore"), Konversation::UnignoreNick);
    m_nickPopup->setItemVisible(Konversation::IgnoreNick, false);
    m_nickPopup->setItemVisible(Konversation::UnignoreNick, false);

    m_nickPopup->insertSeparator();

    m_nickPopup->insertItem(i18n("Open Query"),Konversation::OpenQuery);
    if (kapp->authorize("allow_downloading"))
    {
        m_nickPopup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),Konversation::DccSend);
    }
    m_nickPopup->insertSeparator();

    m_nickPopup->insertItem(i18n("Add to Watched Nicks"), Konversation::AddNotify);

    connect(m_nickPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect(m_modes, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect(m_kickban, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
}

void IRCView::updateNickMenuEntries(QPopupMenu* popup, const QString& nickname)
{
    if (popup)
    {
        if (Preferences::isIgnored(nickname))
        {
            popup->setItemVisible(Konversation::UnignoreNick, true);
            popup->setItemVisible(Konversation::IgnoreNick, false);
        }
        else
        {
            popup->setItemVisible(Konversation::IgnoreNick, true);
            popup->setItemVisible(Konversation::UnignoreNick, false);
        }

        if (!m_server)
            popup->setItemEnabled(Konversation::AddNotify, false);
        else if (!m_server->isConnected())
            popup->setItemEnabled(Konversation::AddNotify, false);
        else if (!Preferences::hasNotifyList(m_server->serverGroupSettings()->id()))
            popup->setItemEnabled(Konversation::AddNotify, false);
        else if (Preferences::isNotify(m_server->serverGroupSettings()->id(), nickname))
            popup->setItemEnabled(Konversation::AddNotify, false);
        else
            popup->setItemEnabled(Konversation::AddNotify, true);
    }
}

void IRCView::setupQueryPopupMenu()
{
    m_nickPopup = new KPopupMenu(this,"query_context_menu");
    m_nickPopupId = m_nickPopup->insertTitle(m_currentNick);
    m_nickPopup->insertItem(i18n("&Whois"),Konversation::Whois);
    m_nickPopup->insertItem(i18n("&Version"),Konversation::Version);
    m_nickPopup->insertItem(i18n("&Ping"),Konversation::Ping);
    m_nickPopup->insertSeparator();

    m_nickPopup->insertItem(i18n("Ignore"), Konversation::IgnoreNick);
    m_nickPopup->insertItem(i18n("Unignore"), Konversation::UnignoreNick);
    m_nickPopup->setItemVisible(Konversation::IgnoreNick, false);
    m_nickPopup->setItemVisible(Konversation::UnignoreNick, false);
    m_nickPopup->insertSeparator();

    if (kapp->authorize("allow_downloading"))
    {
        m_nickPopup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),Konversation::DccSend);
        m_nickPopup->insertSeparator();
    }

    m_nickPopup->insertItem(i18n("Add to Watched Nicks"), Konversation::AddNotify);

    connect(m_nickPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
}

void IRCView::setupChannelPopupMenu()
{
    m_channelPopup = new KPopupMenu(this,"channel_context_menu");
    m_channelPopupId = m_channelPopup->insertTitle(m_currentChannel);
    m_channelPopup->insertItem(i18n("&Join"),Konversation::Join);
    m_channelPopup->insertItem(i18n("Get &user list"),Konversation::Names);
    m_channelPopup->insertItem(i18n("Get &topic"),Konversation::Topic);

    connect(m_channelPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
}

void IRCView::setNickAndChannelContextMenusEnabled(bool enable)
{
    if (m_nickPopup) m_nickPopup->setEnabled(enable);
    if (m_channelPopup) m_channelPopup->setEnabled(enable);
}

void IRCView::search()
{
    emit doSearch();
}

void IRCView::searchAgain()
{
    if (m_pattern.isEmpty())
    {
        emit doSearch();
    }
    else
    {
        // next search must begin one index before / after the last search
        // depending on the search direction.
        if(m_forward)
        {
            ++m_findIndex;
            if(m_findIndex == paragraphLength(m_findParagraph))
            {
                m_findIndex = 0;
                ++m_findParagraph;
            }
        }
        else
        {
            if(m_findIndex)
            {
                --m_findIndex;
            }
            else
            {
                --m_findParagraph;
                m_findIndex = paragraphLength(m_findParagraph);
            }
        }

        if(!find(m_pattern, m_caseSensitive, m_wholeWords, m_forward, &m_findParagraph, &m_findIndex))
        {
            KMessageBox::information(this,i18n("No matches found for \"%1\".").arg(m_pattern),i18n("Information"));
        }

    }
}

bool IRCView::search(const QString& pattern, bool caseSensitive,
bool wholeWords, bool forward, bool fromCursor)
{
    m_pattern       = pattern;
    m_caseSensitive = caseSensitive;
    m_wholeWords    = wholeWords;
    m_forward       = forward;
    m_fromCursor    = fromCursor;

    if (m_pattern.isEmpty())
        return true;

    if (!m_fromCursor)
    {
        if(m_forward)
        {
            m_findParagraph = 1;
            m_findIndex = 1;
        }
        else
        {
            m_findParagraph = paragraphs();
            m_findIndex = paragraphLength(paragraphs());
        }
    }

    return searchNext();
}

bool IRCView::searchNext(bool reversed)
{
    if (m_pattern.isEmpty())
        return true;

    bool fwd = (reversed ? !m_forward : m_forward);
    // next search must begin one index before / after the last search
    // depending on the search direction.
    if (fwd)
    {
        ++m_findIndex;
        if(m_findIndex == paragraphLength(m_findParagraph))
        {
            m_findIndex = 0;
            ++m_findParagraph;
        }
    }
    else
    {
        if (m_findIndex)
        {
            --m_findIndex;
        }
        else
        {
            --m_findParagraph;
            m_findIndex = paragraphLength(m_findParagraph);
        }
    }

    return find(m_pattern, m_caseSensitive, m_wholeWords, fwd,
        &m_findParagraph, &m_findIndex);
}

// other windows can link own menu entries here
QPopupMenu* IRCView::getPopup() const
{
    return m_popup;
}

// for more information about these RTFM
//    http://www.unicode.org/reports/tr9/
//    http://www.w3.org/TR/unicode-xml/
QChar IRCView::LRM = (ushort)0x200e; // Right-to-Left Mark
QChar IRCView::RLM = (ushort)0x200f; // Left-to-Right Mark
QChar IRCView::LRE = (ushort)0x202a; // Left-to-Right Embedding
QChar IRCView::RLE = (ushort)0x202b; // Right-to-Left Embedding
QChar IRCView::RLO = (ushort)0x202e; // Right-to-Left Override
QChar IRCView::LRO = (ushort)0x202d; // Left-to-Right Override
QChar IRCView::PDF = (ushort)0x202c; // Previously Defined Format

QChar::Direction IRCView::basicDirection(const QString &string)
{
#if 1
    // find base direction
    unsigned int pos = 0;
    unsigned int str_len = string.length();
    while ((pos < str_len) &&
        (string.at(pos) != RLE) &&
        (string.at(pos) != LRE) &&
        (string.at(pos) != RLO) &&
        (string.at(pos) != LRO) &&
        (string.at(pos).direction() > 1) &&
                                                  // not R and not L
        (string.at(pos).direction() != QChar::DirAL))
    {
        ++pos;
    }

    if((string.at(pos).direction() == QChar::DirR) ||
        (string.at(pos).direction() == QChar::DirAL) ||
        (string.at(pos) == RLE) ||
        (string.at(pos) == RLO))
    {
        return QChar::DirR;
    }

    return QChar::DirL;
#else

/*
this is an experimental patch which aims at testing a new algorythm for
detecting the direction of the string:
istead of checking the first strong character, this tests for the whole
sentence and counts the appearence of LTR and RTL chars. this also
counts for neutral chars and if the sentence has no real direction, the
next stage will be to keep the direction of the last line.

this brings up a bug in konversation, and lines wich start with a nick (for example)
but are RTL, will look wierd.

for example, user writes:
    other_user, BLA BLA BLA BLA

and that gets rendered like:
                            BLA BLA BLA :<user> other_user
the correct way should be
                            BLA BLA BLA :other_user <user>

the bug has nothing to do with this function, and is somewhere in append().
 
this is on my TODO list. in case this code is not fixed by 1.0.1,
please change the #if in to "1" to revert to the old working code. 
*/
    unsigned int pos = 0;
    unsigned int rtl_chars = 0;
    unsigned int ltr_chars = 0;
    unsigned int other_chars = 0;
    unsigned int str_len = string.length();
    
    for( pos=0; pos<str_len; pos ++ )
    {
        if (string[pos].isNumber() || string[pos].isSymbol() || 
            string[pos].isSpace()  || string[pos].isPunct()  || string[pos].isMark() )
        {
            other_chars++;
        }
        else
        {
            switch( string[pos].direction() )
            {
                case QChar::DirL:
                case QChar::DirLRO:
                case QChar::DirLRE:
                    ltr_chars++;
                    break;
                case QChar::DirR:
                case QChar::DirAL:
                case QChar::DirRLO:
                case QChar::DirRLE:
                    rtl_chars++;
                    break;
                default:
                    break;
            }
        } 
    }
    
    if (rtl_chars > ltr_chars)
        return QChar::DirR;
    else
        if (other_chars == str_len)
            // todo, remember last state
            return QChar::DirL; 
        else
            return QChar::DirL;
#endif
}

void IRCView::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(acceptDrops() && QUriDrag::canDecode(e))
        e->accept();
}

void IRCView::contentsDropEvent(QDropEvent *e)
{
    QStrList s;
    if(QUriDrag::decode(e,s))
        emit filesDropped(s);
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
                timeColor + "\">[%1 %2]</font> ").arg(date.toString(Qt::ISODate), time.toString(timeFormat));
        }

        return timeString;
    }

    return QString::null;
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

void IRCView::keyPressEvent(QKeyEvent* e)
{
    if((e->key() == Qt::Key_V) && (e->state() == Qt::ControlButton))
    {
        emit textPasted(false);
        return;
    }

    KTextBrowser::keyPressEvent(e);
}

void IRCView::resizeEvent(QResizeEvent* e)
{
    KTextBrowser::resizeEvent(e);

    QTimer::singleShot(0, this, SLOT(updateScrollBarPos()));
}

void IRCView::updateScrollBarPos()
{
    ensureVisible(contentsX(), contentsHeight());
    repaintContents(false);
}

#include "ircview.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
