// -*- mode: c++; c-file-style: "stroustrup"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

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
#include <qfont.h>
#include <qscrollbar.h>

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

#include "channel.h"
#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"
#include "searchdialog.h"
#include "konversationsound.h"
#include "chatwindow.h"
#include "common.h"
#include "images.h"
#include "emoticon.h"
#include "notificationhandler.h"

IRCView::IRCView(QWidget* parent, Server* newServer) : KTextBrowser(parent) {
    m_copyUrlMenu = false;
    m_resetScrollbar = true;
    m_offset = 0;
    m_mousePressed = false;
    m_currentNick = QString::null;
    m_isOnNick = false;
    m_chatWin = 0;
    m_findParagraph=0;
    m_findIndex=0;

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

    setServer(newServer);
    setFont(KonversationApplication::preferences.getTextFont());
    setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),QString::null);

    m_popup = new QPopupMenu(this,"ircview_context_menu");
    m_popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
    m_popup->insertItem(i18n("Select All"),SelectAll);
    m_popup->insertSeparator();
    m_popup->insertItem(SmallIcon("find"),i18n("Find Text..."),Search);
    m_popup->insertSeparator();

    if(newServer) { // ## This is not working --cartman
        KAction *action = newServer->getMainWindow()->actionCollection()->action("open_logfile");
        action->plug(m_popup);
    }

    setupNickPopupMenu();

    connect(this, SIGNAL(highlighted(const QString&)), this, SLOT(highlightedSlot(const QString&)));
    connect(this, SIGNAL(linkClicked(const QString&)), this, SLOT(urlClickSlot(const QString&)));
}

IRCView::~IRCView() {
    delete m_popup;
}

void IRCView::updateStyleSheet() {
    // set style sheet for <p> to define paragraph spacing
    QStyleSheet* sheet = styleSheet();

    if(!sheet)
        return;

    int paragraphSpacing;

    if(KonversationApplication::preferences.getUseParagraphSpacing())
        paragraphSpacing=KonversationApplication::preferences.getParagraphSpacing();
    else
        paragraphSpacing = 0;

    QStyleSheetItem* style=sheet->item("p");

    if(!sheet) {
        kdDebug() << "IRCView::updateStyleSheet(): style == 0!" << endl;
        return;
    }

    style->setDisplayMode(QStyleSheetItem::DisplayBlock);
    style->setMargin(QStyleSheetItem::MarginVertical,paragraphSpacing);
    style->setSelfNesting(false);
}

void IRCView::setViewBackground(const QString& color, const QString& pixmapName) {
    QColor backgroundColor("#"+color);
    QPixmap backgroundPixmap;
    backgroundPixmap.load(pixmapName);

    if(backgroundPixmap.isNull()) {
        setPaper(backgroundColor);
    } else {
        QBrush backgroundBrush;
        backgroundBrush.setColor(backgroundColor);
        backgroundBrush.setPixmap(backgroundPixmap);
        setPaper(backgroundBrush);
    }
}

void IRCView::setServer(Server* newServer) {
    m_server = newServer;
}

const QString& IRCView::getContextNick() const {
    return m_currentNick;
}

void IRCView::clearContextNick() {
    m_currentNick = QString::null;
}

void IRCView::clear() {
    m_buffer = QString::null;
    KTextBrowser::clear();
}

void IRCView::highlightedSlot(const QString& link) {
    if(!link.startsWith("#")) {
        m_isOnNick = false;
        if(link.isEmpty() && m_copyUrlMenu) {
            m_popup->removeItem(CopyUrl);
            m_popup->removeItem(Bookmark);
            m_copyUrlMenu = false;
        } else if(!link.isEmpty() && !m_copyUrlMenu) {
            m_popup->insertItem(i18n("Copy URL to Clipboard"),CopyUrl,1);
            m_popup->insertItem(i18n("Add to Bookmarks"),Bookmark,2);
            m_copyUrlMenu = true;
            m_urlToCopy = link;
        }
    } else if(link.startsWith("#") && !link.startsWith("##")) {
        m_currentNick = link;
        m_currentNick.remove("#");
        m_nickPopup->changeTitle(m_popupId,m_currentNick);
        m_isOnNick = true;
    }
}

void IRCView::urlClickSlot(const QString &url) {
    urlClickSlot(url,false);
}

void IRCView::urlClickSlot(const QString &url, bool newTab) {
    if (!url.isEmpty() && !url.startsWith("#")) {
        // Always use KDE default mailer.
        if (KonversationApplication::preferences.getWebBrowserUseKdeDefault() || url.startsWith("mailto:")) {
            if(newTab && !url.startsWith("mailto:")) {
                QCString foundApp, foundObj;
                QByteArray data;
                QDataStream str(data, IO_WriteOnly);
                if( KApplication::dcopClient()->findObject("konqueror*", "konqueror-mainwindow*",
                        "windowCanBeUsedForTab()", data, foundApp, foundObj, false, 3000)) {
                    DCOPRef ref(foundApp, foundObj);
                    ref.call("newTab", url);
                }
            } else
                new KRun(KURL(url));
        } else {
            QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
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
    } else if(url.startsWith("##")) // Channel
    {
        QString channel(url);
        channel.replace("##", "#");
        m_server->sendJoinCommand(channel);
    } else if(url.startsWith("#")) // Nick
    {
        QString recepient(url);
        recepient.remove("#");
        NickInfoPtr nickInfo = m_server->obtainNickInfo(recepient);
        m_server->addQuery(nickInfo, true /*we initiated*/);
    }
}

void IRCView::replaceDecoration(QString& line, char decoration, char replacement) {
    int pos;
    bool decorated = false;

    while((pos=line.find(decoration))!=-1) {
        line.replace(pos,1,(decorated) ? QString("</%1>").arg(replacement) : QString("<%1>").arg(replacement));
        decorated = !decorated;
    }
}

QString IRCView::filter(const QString& line, const QString& defaultColor, const QString& whoSent,
                        bool doHighlight, bool parseURL, bool self) {
    QString filteredLine(line);
    KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

    // TODO: Use QStyleSheet::escape() here
    filteredLine.replace("&","&amp;");
    // Replace all < with &lt;
    filteredLine.replace("<","&lt;");
    // Replace all > with &gt;
    filteredLine.replace(">","&gt;");

#if 0
    if(!KonversationApplication::preferences.getDisableExpansion()) {
        QRegExp boldRe("\\*\\*([a-zA-Z0-9]+)\\*\\*");
        QRegExp underRe("\\_\\_([a-zA-Z0-9]+)\\_\\_");
        int position = 0;
        QString replacement;

        while( position >= 0) {
            position = boldRe.search(filteredLine, position);
            if( position > -1) {
                replacement = boldRe.cap(1);
                replacement = "\x02"+replacement+"\x02";
                filteredLine.replace(position,replacement.length()+2,replacement);
            }
            position += boldRe.matchedLength();
        }

        position = 0;
        while( position >= 0) {
            position = underRe.search(filteredLine, position);
            if( position > -1) {
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

    bool inverted = false; // TODO this flag should be stored somewhere
    {
        QColor fg("#"+KonversationApplication::preferences.getColor("ChannelMessage"));
        QColor  bg("#"+KonversationApplication::preferences.getColor("TextViewBackground"));

        int h = 0, s = 0,fv = 0,bv = 0;
        fg.getHsv(&h,&s,&fv);
        bg.getHsv(&h,&s,&bv);

        if (bv <= fv) {
            inverted = false;
        }
    }

    if(inverted) {
        filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0030,1\\1");
    } else {
        filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0031,0\\1");
    }

    // Hack to allow for whois info hostmask info to not be parsed as email
    filteredLine.replace("&amp;#64;","&#64;");

    if(filteredLine.find("\x07") != -1) {
        if(KonversationApplication::preferences.getBeep()) {
            kapp->beep();
        }
    }

    // replace \003 and \017 codes with rich text color codes
    // captures          1    2                   23 4                   4 3     1
    QRegExp colorRegExp("(\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)");

    int pos;
    bool filterColors = KonversationApplication::preferences.getFilterColors();
    bool firstColor = true;
    QString colorString;
    QStringList colorCodes = KonversationApplication::preferences.getIRCColorList();

    while((pos=colorRegExp.search(filteredLine))!=-1) {
        if(filterColors) {
            colorString = QString::null;
        } else {
            colorString = (firstColor) ? QString::null : QString("</font>");

            // reset colors on \017 to default value
            if(colorRegExp.cap(1) == "\017")
                colorString += "<font color=\"#"+defaultColor+"\">";
            else {
                int foregroundColor = colorRegExp.cap(2).toInt();
                colorString += "<font color=\"" + colorCodes[foregroundColor] + "\">";
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
    replaceDecoration(filteredLine,'\x16','b'); // should be inverse
    replaceDecoration(filteredLine,'\x1f','u');

    if(parseURL) {
        filteredLine = Konversation::tagURLs(filteredLine, whoSent);
    }

    filteredLine = Konversation::EmotIcon::filter(filteredLine, fontMetrics());

    // Highlight
    QString ownNick;

    if(m_server) {
        ownNick = m_server->getNickname();
    }

    if(doHighlight && m_server && (whoSent != ownNick) && !self) {
        QString highlightColor;

        // FIXME: We got to get rid of m_server dependance here
        if(KonversationApplication::preferences.getHighlightNick() &&
                filteredLine.lower().find(QRegExp("(^|[^\\d\\w])" +
                                                  QRegExp::escape(ownNick.lower()) +
                                                  "([^\\d\\w]|$)")) != -1) {
            // highlight current nickname
            highlightColor = KonversationApplication::preferences.getHighlightNickColor().name();
            m_tabNotification = Konversation::tnfNick;
        } else {
            QPtrList<Highlight> highlightList = KonversationApplication::preferences.getHighlightList();
            QPtrListIterator<Highlight> it(highlightList);
            Highlight* highlight = it.current();
            bool patternFound = false;
            int index = 0;

            while(highlight) {
                if(highlight->getRegExp()) {
                    QRegExp needle(highlight->getPattern().lower());
                    patternFound = ((filteredLine.lower().find(needle) != -1) ||   // highlight regexp in text
                                    (whoSent.lower().find(needle) != -1));            // highlight regexp in nickname
                } else {
                    QString needle(highlight->getPattern().lower());
                    patternFound = ((filteredLine.lower().find(needle) != -1) ||   // highlight patterns in text
                                    (whoSent.lower().find(needle) != -1));            // highlight patterns in nickname
                }

                if(!patternFound) {
                    ++it;
                    highlight = it.current();
                    ++index;
                } else {
                    break;
                }
            }

            if(patternFound) {
                highlightColor = highlight->getColor().name();
                m_tabNotification = Konversation::tnfHighlight;

                if(KonversationApplication::preferences.getHighlightSoundEnabled()) {
                    konvApp->sound()->play(highlight->getSoundURL());
                }

                konvApp->notificationHandler()->highlight(m_chatWin, whoSent, line);
                m_autoTextToSend = highlight->getAutoText();
            }
        }

        // apply found highlight color to line
        if(!highlightColor.isEmpty()) {
            filteredLine = "<font color=\"" + highlightColor + "\">" + filteredLine + "</font>";
        }
    } else if(doHighlight && (whoSent == ownNick) && KonversationApplication::preferences.getHighlightOwnLines()) {
        // highlight own lines
        filteredLine = "<font color=\"" + KonversationApplication::preferences.getHighlightOwnLinesColor().name() +
                       "\">" + filteredLine + "</font>";
    }

    // Replace multiple Spaces with "<space>&nbsp;"
    while((pos = filteredLine.find("  ")) != -1) {
        filteredLine.replace(pos + (pos == 0 ? 0 : 1), 1, "&nbsp;");
    }

    return filteredLine;
}

void IRCView::append(const QString& nick,const QString& message) {
    QString channelColor = KonversationApplication::preferences.getColor("ChannelMessage");
    QString line;
    QString nickLine = "%2";
    QString color;
    m_tabNotification = Konversation::tnfNormal;

    if(nick != m_server->getNickname()) {
        bool linkNicks = KonversationApplication::preferences.getUseClickableNicks();
        if(linkNicks)
            nickLine = "<a href=\"#" + nick + "\">%2</a>";
        KonversationApplication::instance()->increaseKarma(nick,1);
    }

    if(KonversationApplication::preferences.getUseBoldNicks())
        nickLine = "<b>"+nickLine+"</b>";

    if(KonversationApplication::preferences.getUseColoredNicks()) {

        if(nick != m_server->getNickname())
            color = m_server->obtainNickInfo(nick)->getNickColor();
        else
            color = KonversationApplication::preferences.getNickColorList()[8];

        nickLine = "<font color=\"" + color + "\">"+nickLine+"</font>";

        if(color == "#000000") {
            color = "#000001"; // HACK Working around QTextBrowser's auto link coloring
        }
    }
    else {
        if(channelColor  == "000000") {
            channelColor = "000001"; // HACK Working around QTextBrowser's auto link coloring
        }
    }

    if(basicDirection(message) == QChar::DirR) {
        line = RLO;
        line += LRE;
        line += "<p><font color=\"#" + channelColor + "\"><b>&lt;</b>" + nickLine + "<b>&gt;</b> %1" + PDF + " %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + channelColor + "\">%1 <b>&lt;</b>" + nickLine + "<b>&gt;</b> %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, channelColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendRaw(const QString& message, bool suppressTimestamps, bool self) {
    QString channelColor=KonversationApplication::preferences.getColor("ChannelMessage");
    QString line;
    m_tabNotification = Konversation::tnfNone;

    if(suppressTimestamps) {
        line = QString("<p><font color=\"#" + channelColor + "\">" + message + "</font></p>\n");
    } else {
        line = QString("<p>" + timeStamp() + " <font color=\"#" + channelColor + "\">" + message + "</font></p>\n");
    }

    doAppend(line, true, self);
}

void IRCView::appendQuery(const QString& nick,const QString& message) {
    QString queryColor=KonversationApplication::preferences.getColor("QueryMessage");
    QString line;
    QString nickLine = "%2";
    QString color;
    m_tabNotification = Konversation::tnfNormal;

    if(nick != m_server->getNickname()) {
        bool linkNicks = KonversationApplication::preferences.getUseClickableNicks();
        if(linkNicks)
            nickLine = "<a href=\"#" + nick + "\">%2</a>";
        KonversationApplication::instance()->increaseKarma(nick,2);
    }

    if(KonversationApplication::preferences.getUseBoldNicks())
        nickLine = "<b>"+nickLine+"</b>";

    if(KonversationApplication::preferences.getUseColoredNicks()) {

        if(nick != m_server->getNickname())
            color = m_server->obtainNickInfo(nick)->getNickColor();
        else
            color = KonversationApplication::preferences.getNickColorList()[8];

        nickLine = "<font color=\"" + color + "\">"+nickLine+"</font>";

        if(color == "#000000") {
            color = "#000001"; // HACK Working around QTextBrowser's auto link coloring
        }
    }
    else {
        if(queryColor  == "000000") {
            queryColor = "000001"; // HACK Working around QTextBrowser's auto link coloring
        }
    }


    if(basicDirection(message) == QChar::DirR) {
        line = RLO;
        line += LRE;
        line += "<p><font color=\"#" + queryColor + "\"><b>&lt;</b>" + nickLine + "<b>&gt;</b> %1" + PDF + " %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + queryColor + "\">%1 <b>&lt;</b>" + nickLine + "<b>&gt;</b> %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, queryColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendAction(const QString& nick,const QString& message) {
    QString actionColor=KonversationApplication::preferences.getColor("ActionMessage");
    QString line;
    QString nickLine = "%2";
    QString color;
    m_tabNotification = Konversation::tnfNormal;

    if(nick != m_server->getNickname()) {
        bool linkNicks = KonversationApplication::preferences.getUseClickableNicks();
        if(linkNicks)
            nickLine = "<a href=\"#" + nick + "\">%2</a>";
        KonversationApplication::instance()->increaseKarma(nick,1);
    }

    if(KonversationApplication::preferences.getUseBoldNicks())
        nickLine = "<b>"+nickLine+"</b>";

    if(KonversationApplication::preferences.getUseColoredNicks()) {

        if(nick != m_server->getNickname())
            color = m_server->obtainNickInfo(nick)->getNickColor();
        else
            color = KonversationApplication::preferences.getNickColorList()[8];

        if(color == "#000000") {
            color = "#000001"; // HACK Working around QTextBrowser's auto link coloring
        }

        nickLine = "<font color=\"" + color + "\">"+nickLine+"</font>";
    }
    else {
        if(actionColor  == "000000") {
            actionColor = "000001"; // HACK Working around QTextBrowser's auto link coloring
        }
    }

    if(basicDirection(message) == QChar::DirR) {
        line = RLO;
        line += LRE;
        line += "<p><font color=\"#" + actionColor + "\">" + nickLine + " * %1" + PDF + " %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + actionColor + "\">%1 * " + nickLine + " %3</font></p>\n";
    }

    line = line.arg(timeStamp(), nick, filter(message, actionColor, nick, true));

    emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

    doAppend(line);
}

void IRCView::appendServerMessage(const QString& type, const QString& message) {
    QString m_serverColor = KonversationApplication::preferences.getColor("ServerMessage");
    m_tabNotification = Konversation::tnfControl;

    // Fixed width font option for MOTD
    QString fixed;
    if(KonversationApplication::preferences.getFixedMOTD()) {
        if(type == "MOTD")
            fixed=" face=\"" + KGlobalSettings::fixedFont().family() + "\"";
    }

    QString line;

    if(basicDirection(message) == QChar::DirR) {
        line = RLO;
        line += LRE;
        line += "<p><font color=\"#" + m_serverColor + "\"" + fixed + "><b>[</b>%2<b>]</b> %1" + PDF + " %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + m_serverColor + "\"" + fixed + ">%1 <b>[</b>%2<b>]</b> %3</font></p>\n";
    }

    if(type != "Notify")
        line = line.arg(timeStamp(), type, filter(message,m_serverColor));
    else
        line = "<font color=\"#" + m_serverColor + "\">"+line.arg(timeStamp(), type, message)+"</font>";

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool important, bool parseURL, bool self) {
    QString commandColor = KonversationApplication::preferences.getColor("CommandMessage");
    QString line;
    QString prefix="***";
    m_tabNotification = Konversation::tnfControl;

    if(type == i18n("Join")) {
        prefix="-->";
        parseURL=false;
    } else if(type == i18n("Part") || type == i18n("Quit")) {
        prefix="<--";
    }

    prefix=QStyleSheet::escape(prefix);

    if(basicDirection(message) == QChar::DirR) {
        line = RLO;
        line += LRE;
        line += "<p><font color=\"#" + commandColor + "\">%2 %1" + PDF + " %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + commandColor + "\">%1 %2 %3</font></p>\n";
    }

    line = line.arg(timeStamp(), prefix, filter(message, commandColor, 0, true, parseURL, self));

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line, important, self);
}

void IRCView::appendBacklogMessage(const QString& firstColumn,const QString& rawMessage) {
    QString time;
    QString message = rawMessage;
    QString nick = firstColumn;
    QString backlogColor = KonversationApplication::preferences.getColor("BacklogMessage");
    m_tabNotification = Konversation::tnfNone;

    time = nick.section(' ', 0, 4);
    nick = nick.section(' ', 5);

    if(!nick.isEmpty() && !nick.startsWith("<") && !nick.startsWith("*")) {
        nick = "|" + nick + "|";
    }

    // Nicks are in "<nick>" format so replace the "<>"
    nick.replace("<","&lt;");
    nick.replace(">","&gt;");

    QString line;

    if(basicDirection(message) == QChar::DirR) {
        line = "<p><font color=\"#" + backlogColor + "\">%2 %1 %3</font></p>\n";
    } else {
        line = "<p><font color=\"#" + backlogColor + "\">%1 %2 %3</font></p>\n";
    }

    line = line.arg(time, nick, filter(message, backlogColor, NULL, false));

    doAppend(line);
}

//without any display update stuff that freaks out the scrollview
void IRCView::removeSelectedText( int selNum ) {
    QTextDocument* doc=document();

    for ( int i = 0; i < (int)doc->numSelections(); ++i ) {
        if ( i == selNum )
            continue;
        doc->removeSelection( i );
    }
    // ...snip...

    doc->removeSelectedText( selNum, QTextEdit::textCursor() );

    // ...snip...
}

void IRCView::scrollToBottom() {
    // QTextEdit::scrollToBottom does sync() too, but we don't want it because its slow
    setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
}

void IRCView::doAppend(const QString& newLine, bool important, bool self) {
    // Add line to buffer
    QString line(newLine);

    if(important || !KonversationApplication::preferences.getHideUnimportantEvents()) {
        if(!self) {
            emit updateTabNotification(m_tabNotification);
        }

        // scroll view only if the scroll bar is already at the bottom
        bool doScroll = ( KTextBrowser::verticalScrollBar()->value() == KTextBrowser::verticalScrollBar()->maxValue());

        line.remove('\n');// TODO why have newlines? we get <p>, so the \n are unnecessary...

        bool up = KTextBrowser::viewport()->isUpdatesEnabled();

        KTextBrowser::viewport()->setUpdatesEnabled(false);
        KTextBrowser::append(line);

        document()->lastParagraph()->format();
        resizeContents(contentsWidth(), document()->height());

        //Explanation: the scrolling mechanism cannot handle the buffer changing when the scrollbar is not
        // at an end, so the scrollbar wets its pants and forgets who it is for ten minutes
        // TODO: make this eat multiple lines at once when the preference is changed so it doesn't take so long
        if (doScroll) {
            int sbm = KonversationApplication::preferences.getScrollbackMax();
            if (sbm) {
                //loop for two reasons: 1) preference changed 2) lines added while scrolled up
                for(sbm = paragraphs() - sbm; sbm > 0; --sbm)
                    removeParagraph(0);
                resizeContents(contentsWidth(), document()->height());
            }
        }

        KTextBrowser::viewport()->setUpdatesEnabled(up);

        if(doScroll) {
            setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
            repaintContents(false);
        }
    }

    if(!m_autoTextToSend.isEmpty()) {
        // replace placeholders in autoText
        QString sendText = m_server->parseWildcards(m_autoTextToSend,m_server->getNickname(),
                           QString::null, QString::null, QString::null, QString::null);
        // avoid recursion due to signalling
        m_autoTextToSend = QString::null;
        // send signal only now
        emit autoText(sendText);
    }
}

// remember if scrollbar was positioned at the end of the text or not
void IRCView::hideEvent(QHideEvent* /* event */) {
    m_resetScrollbar = ((contentsHeight()-visibleHeight()) == contentsY());
}

// Workaround to scroll to the end of the TextView when it's shown
void IRCView::showEvent(QShowEvent* /* event */) {
    // did the user scroll the view to the end of the text before hiding?
    if(m_resetScrollbar) {
        moveCursor(MoveEnd,false);
        ensureVisible(0,contentsHeight());
    }
}

void IRCView::contentsMouseReleaseEvent(QMouseEvent *ev) {
    if (ev->button() == Qt::MidButton) {
        if(m_copyUrlMenu) {
            urlClickSlot(m_urlToCopy,true);
            return;
        } else {
            emit textPasted(true);
            return;
        }
    }

    if (ev->button() == QMouseEvent::LeftButton) {
        if (m_mousePressed) {
            urlClickSlot(m_urlToDrag);
            m_mousePressed = false;
            return;
        }

        m_mousePressed = false;
    }

    KTextBrowser::contentsMouseReleaseEvent(ev);
}

void IRCView::contentsMousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == QMouseEvent::LeftButton) {
        m_urlToDrag = anchorAt(viewportToContents(ev->pos()));

        if (!m_urlToDrag.isNull()) {
            m_mousePressed = true;
            m_pressPosition = ev->pos();
            return;
        }
    }

    KTextBrowser::contentsMousePressEvent(ev);
}

void IRCView::contentsMouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressed && (m_pressPosition - ev->pos()).manhattanLength() > QApplication::startDragDistance()) {
        m_mousePressed = false;
        removeSelection();
        KURL ux(m_urlToDrag);

        if (m_urlToDrag.startsWith("##")) {
            ux = QString("irc://%1:%2/%3").arg(m_server->getServerName()).arg(m_server->getPort()).arg(m_urlToDrag.mid(2));
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

    if(!block) {
        KTextBrowser::contentsContextMenuEvent(ev);
    }
}

bool IRCView::contextMenu(QContextMenuEvent* ce) {
    if(m_isOnNick) {
        m_nickPopup->exec(ce->globalPos());
    } else {
        m_popup->setItemEnabled(Copy,(hasSelectedText()));

        int r = m_popup->exec(ce->globalPos());

        switch(r) {
                case -1:
                // dummy. -1 means, no entry selected. we don't want -1to go in default, so
                // we catch it here
                break;
                case Copy:
                copy();
                break;
                case CopyUrl: {
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
                case Bookmark: {
                    KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
                    KBookmarkGroup bg = bm->addBookmarkDialog(m_urlToCopy, QString::null);
                    bm->save();
                    bm->emitChanged(bg);
                    break;
                }
                default:
                emit extendedPopup(r);
        }
    }
    return true;
}

void IRCView::setupNickPopupMenu() {
    m_nickPopup = new KPopupMenu(this,"nicklist_context_menu");
    m_modes = new KPopupMenu(this,"nicklist_modes_context_submenu");
    m_kickban = new KPopupMenu(this,"nicklist_kick_ban_context_submenu");
    m_popupId= m_nickPopup->insertTitle(m_currentNick);
    m_modes->insertItem(i18n("Give Op"),Konversation::GiveOp);
    m_modes->insertItem(i18n("Take Op"),Konversation::TakeOp);
    m_modes->insertItem(i18n("Give Voice"),Konversation::GiveVoice);
    m_modes->insertItem(i18n("Take Voice"),Konversation::TakeVoice);
    m_nickPopup->insertItem(i18n("Modes"),m_modes,Konversation::ModesSub);
    m_nickPopup->insertSeparator();
    m_nickPopup->insertItem(i18n("&Whois"),Konversation::Whois);
    m_nickPopup->insertItem(i18n("&Version"),Konversation::Version);
    m_nickPopup->insertItem(i18n("&Ping"),Konversation::Ping);
    m_nickPopup->insertSeparator();
    m_nickPopup->insertItem(i18n("Open Query"),Konversation::OpenQuery);
    if (kapp->authorize("allow_downloading")) {
      m_nickPopup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),Konversation::DccSend);
    }
    m_nickPopup->insertSeparator();
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
    m_nickPopup->insertItem(i18n("Ignore"),Konversation::IgnoreNick);

    connect(m_nickPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect(m_modes, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
    connect(m_kickban, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
}

void IRCView::search() {
/*    m_caseSensitive = false;
    m_wholeWords = false;
    m_forward = false;
    m_fromCursor = false;

    m_pattern = SearchDialog::search(this, &m_caseSensitive, &m_wholeWords, &m_forward, &m_fromCursor);

    if(!m_fromCursor) {
        if(m_forward) {
            m_findParagraph = 1;
            m_findIndex = 1;
        } else {
            m_findParagraph = paragraphs();
            m_findIndex = paragraphLength(paragraphs());
        }
    }

    searchAgain(); */

    emit doSearch();
}

void IRCView::searchAgain() {

    if(!m_pattern.isEmpty()) {
        // next search must begin one index before / after the last search
        // depending on the search direction.
        if(m_forward) {
            ++m_findIndex;
            if(m_findIndex == paragraphLength(m_findParagraph)) {
                m_findIndex = 0;
                ++m_findParagraph;
            }
        } else {
            if(m_findIndex) {
                --m_findIndex;
            } else {
                --m_findParagraph;
                m_findIndex = paragraphLength(m_findParagraph);
            }
        }

        if(!find(m_pattern, m_caseSensitive, m_wholeWords, m_forward, &m_findParagraph, &m_findIndex)) {
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
        } else {
            m_findParagraph = paragraphs();
            m_findIndex = paragraphLength(paragraphs());
        }
    }

    return searchNext();
}

bool IRCView::searchNext()
{
    if (m_pattern.isEmpty())
        return true;

    // next search must begin one index before / after the last search
    // depending on the search direction.
    if (m_forward)
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

    return find(m_pattern, m_caseSensitive, m_wholeWords, m_forward,
                &m_findParagraph, &m_findIndex);
}


// other windows can link own menu entries here
QPopupMenu* IRCView::getPopup() const {
    return m_popup;
}

QChar IRCView::LRM = (ushort)0x200e;
QChar IRCView::RLM = (ushort)0x200f;
QChar IRCView::LRE = (ushort)0x202a;
QChar IRCView::RLE = (ushort)0x202b;
QChar IRCView::RLO = (ushort)0x202e;
QChar IRCView::LRO = (ushort)0x202d;
QChar IRCView::PDF = (ushort)0x202c;

QChar::Direction IRCView::basicDirection(const QString &string) {
    // find base direction
    unsigned int pos = 0;
    while ((pos < string.length()) &&
            (string.at(pos) != RLE) &&
            (string.at(pos) != LRE) &&
            (string.at(pos) != RLO) &&
            (string.at(pos) != LRO) &&
            (string.at(pos).direction() > 1) &&
            (string.at(pos).direction() != QChar::DirAL)) // not R and not L
    {
        ++pos;
    }

    if((string.at(pos).direction() == QChar::DirR) ||
            (string.at(pos).direction() == QChar::DirAL) ||
            (string.at(pos) == RLE) ||
            (string.at(pos) == RLO)) {
        return QChar::DirR;
    }

    return QChar::DirL;
}

void IRCView::contentsDragMoveEvent(QDragMoveEvent *e) {
    if(acceptDrops() && QUriDrag::canDecode(e))
        e->accept();
}

void IRCView::contentsDropEvent(QDropEvent *e) {
    QStrList s;
    if(QUriDrag::decode(e,s))
        emit filesDropped(s);
}

QString IRCView::timeStamp() {
    if(KonversationApplication::preferences.getTimestamping()) {
        QTime time = QTime::currentTime();
        QString timeColor = KonversationApplication::preferences.getColor("Time");
        QString timeFormat = KonversationApplication::preferences.getTimestampFormat();
        QString timeString;

        if(!KonversationApplication::preferences.getShowDate()) {
            timeString = QString("<font color=\"#" + timeColor + "\">[%1]</font> ").arg(time.toString(timeFormat));
        } else {
            QDate date = QDate::currentDate();
            timeString = QString("<font color=\"#" +
                                 timeColor + "\">[%1 %2]</font> ").arg(date.toString(Qt::ISODate), time.toString(timeFormat));
        }

        return timeString;
    }

    return QString::null;
}

void IRCView::setChatWin(ChatWindow* chatWin) {
    m_chatWin = chatWin;
}

void IRCView::keyPressEvent(QKeyEvent* e)
{
    if((e->key() == Qt::Key_V) && (e->state() == Qt::ControlButton)) {
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

