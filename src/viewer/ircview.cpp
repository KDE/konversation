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

#include <QStringList>
#include <QRegExp>
#include <QClipboard>
#include <QBrush>
#include <QEvent>
#include <QColor>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QTextObjectInterface>

#include <KUrl>
#include <KBookmarkManager>
#include <kbookmarkdialog.h>
#include <KMenu>
#include <KGlobalSettings>
#include <KFileDialog>
#include <KAuthorized>
#include <KActionCollection>
#include <KToggleAction>
#include <KIO/CopyJob>

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

using namespace Konversation;

class ScrollBarPin
{
        QPointer<QScrollBar> m_bar;
    public:
        ScrollBarPin(QScrollBar *scrollBar) : m_bar(scrollBar)
        {
            if (m_bar)
                m_bar = m_bar->value() == m_bar->maximum()? m_bar : 0;
        }
        ~ScrollBarPin()
        {
            if (m_bar)
                m_bar->setValue(m_bar->maximum());
        }
};

// Scribe bug - if the cursor position or anchor points to the last character in the document,
// the cursor becomes glued to the end of the document instead of retaining the actual position.
// This causes the selection to expand when something is appended to the document.
class SelectionPin
{
    int pos, anc;
    QPointer<IRCView> d;
    public:
        SelectionPin(IRCView *doc) : pos(0), anc(0), d(doc)
        {
            if (d->textCursor().hasSelection())
            {
                int end = d->document()->rootFrame()->lastPosition();
                QTextBlock b = d->document()->lastBlock();
                pos = d->textCursor().position();
                anc = d->textCursor().anchor();
                if (pos != end && anc != end)
                    anc = pos = 0;
            }
        }

        ~SelectionPin()
        {
            if (d && (pos || anc))
            {
                QTextCursor mv(d->textCursor());
                mv.setPosition(anc);
                mv.setPosition(pos, QTextCursor::KeepAnchor);
                d->setTextCursor(mv);
            }
        }
};


IRCView::IRCView(QWidget* parent, Server* newServer) : KTextBrowser(parent), m_nextCullIsMarker(false), m_rememberLinePosition(-1), m_rememberLineDirtyBit(false), markerFormatObject(this)
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


    //// Marker lines
    connect(document(), SIGNAL(contentsChange(int, int, int)), SLOT(cullMarkedLine(int, int, int)));

    //This assert is here because a bad build environment can cause this to fail. There is a note
    // in the Qt source that indicates an error should be output, but there is no such output.
    QTextObjectInterface *iface = qobject_cast<QTextObjectInterface *>(&markerFormatObject);
    if (!iface)
    {
        Q_ASSERT(iface);
    }

    document()->documentLayout()->registerHandler(IRCView::MarkerLine, &markerFormatObject);
    document()->documentLayout()->registerHandler(IRCView::RememberLine, &markerFormatObject);


    //// Other Stuff

    //m_disableEnsureCursorVisible = false;
    //m_wasPainted = false;

    connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect( this, SIGNAL( highlighted ( const QString &) ), this, SLOT( highlightedSlot( const QString &) ) );
    setOpenLinks(false);
    setUndoRedoEnabled(0);
    document()->setDefaultStyleSheet("a.nick:link {text-decoration: none}");
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

    m_popup->addSeparator();

    m_copyUrlClipBoard =m_popup->addAction(KIcon("edit-copy"), i18n("Copy Link Address"), this, SLOT( copyUrl() )) ;
    m_copyUrlClipBoard->setVisible( false );


    m_bookmark = m_popup->addAction(KIcon("bookmark-new"), i18n("Add to Bookmarks"), this, SLOT( slotBookmark() ) );
    m_bookmark->setVisible( false );
    m_saveUrl = m_popup->addAction(KIcon("document-save"), i18n("Save Link As..."), this, SLOT( saveLinkAs() ));
    m_saveUrl->setVisible( false );
    QAction * toggleMenuBarSeparator = m_popup->addSeparator();
    toggleMenuBarSeparator->setVisible(false);
    copyUrlMenuSeparator = m_popup->addSeparator();
    copyUrlMenuSeparator->setVisible( false );
    QAction *act = m_popup->addAction(KIcon("edit-copy"),i18n("&Copy"),this, SLOT( copy()) );
    connect( this, SIGNAL(copyAvailable(bool)),act,SLOT( setEnabled( bool ) ) );
    act->setEnabled( false );
    m_popup->addAction(i18n("Select All"),this, SLOT(selectAll()) );
    m_popup->addAction(KIcon("edit-find"),i18n("Find Text..."),this, SLOT( findText() ) );

    setServer(newServer);

    if (Preferences::self()->useParagraphSpacing()) enableParagraphSpacing();
}

IRCView::~IRCView()
{
    delete m_popup;
}

void IRCView::setServer(Server* newServer)
{
    m_server = newServer;

    if (newServer)
    {
        QAction *action = newServer->getViewContainer()->actionCollection()->action("open_logfile");
        if(action)
        {
                m_popup->addSeparator();
                m_popup->addAction( action );
                action = newServer->getViewContainer()->actionCollection()->action("channel_settings");
                if ( action )
                        m_popup->addAction( action );
        }
    }

}

void IRCView::setChatWin(ChatWindow* chatWin)
{
    m_chatWin = chatWin;

    if(m_chatWin->getType()==ChatWindow::Channel)
        setupNickPopupMenu(false);
    else
        setupNickPopupMenu(true);

    setupChannelPopupMenu();
}

void IRCView::findText()
{
    emit doSearch();
}

void IRCView::findNextText()
{
    emit doSearchNext();
}

void IRCView::findPreviousText()
{
    emit doSearchPrevious();
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
        m_searchFlags &= ~QTextDocument::FindBackward;
    }
    else {
        m_searchFlags |= QTextDocument::FindBackward;
    }
    return find(m_pattern, m_searchFlags);
}

//// Marker lines

#define _S(x) #x << (x)
void dump_doc(QTextDocument* document)
{
    QTextBlock b(document->firstBlock());
    while (b.isValid())
    {
        kDebug()    << _S(b.position())
                    << _S(b.length())
                    << _S(b.userState())
                    ;
                    b=b.next();
    };
}

QDebug operator<<(QDebug dbg, QList<QTextBlock> &l)
{
    dbg.space() << _S(l.count()) << endl;
        for (int i=0; i< l.count(); ++i)
        {
            QTextBlock b=l[i];
            dbg.space() << _S(i) << _S(b.blockNumber()) << _S(b.length()) << _S(b.userState()) << endl;
        }

    return dbg.space();
}

void IrcViewMarkerLine::drawObject(QPainter *painter, const QRectF &r, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(format);

    QTextBlock block=doc->findBlock(posInDocument);
    QPen pen;
    switch (block.userState())
    {
        case IRCView::BlockIsMarker:
            pen.setColor(Preferences::self()->color(Preferences::ActionMessage));
            break;

        case IRCView::BlockIsRemember:
            pen.setColor(Preferences::self()->color(Preferences::CommandMessage));
            // pen.setStyle(Qt::DashDotDotLine);
            break;

        default:
            //nice color, eh?
            pen.setColor(Qt::cyan);
    }

    pen.setWidth(2); // FIXME this is a hardcoded value...
    painter->setPen(pen);

    qreal y = (r.top() + r.height() / 2);
    QLineF line(r.left(), y, r.right(), y);
    painter->drawLine(line);
}

QSizeF IrcViewMarkerLine::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(posInDocument); Q_UNUSED(format);

    QTextFrameFormat f=doc->rootFrame()->frameFormat();
    qreal width = doc->pageSize().width()-(f.leftMargin()+f.rightMargin());
    return QSizeF(width, 6); // FIXME this is a hardcoded value...
}

void IRCView::cullMarkedLine(int where, int rem, int add) //slot
{
    if (where == 0 && add == 0 && rem !=0)
    {
        if (document()->blockCount() == 1 && document()->firstBlock().length() == 1)
        {
            wipeLineParagraphs();
        }
        else
        {
            if (m_nextCullIsMarker)
            {
                //move the remember line up.. if the cull removed it, this will forget its position
                if (m_rememberLinePosition >= 0)
                    --m_rememberLinePosition;
                m_markers.takeFirst();
            }
            int s = document()->firstBlock().userState();
            m_nextCullIsMarker = (s == BlockIsMarker || s == BlockIsRemember);
        }
    }
}

void IRCView::insertMarkerLine() //slot
{
    //if the last line is already a marker of any kind, skip out
    if (lastBlockIsLine())
        return;

    //the code used to preserve the dirty bit status, but that was never affected by appendLine...
    //maybe i missed something
    appendLine(IRCView::MarkerLine);
}

void IRCView::insertRememberLine() //slot
{
    m_rememberLineDirtyBit = true; // means we're going to append a remember line if some text gets inserted

    if (!Preferences::self()->automaticRememberLineOnlyOnTextChange())
        appendRememberLine();
}

void IRCView::cancelRememberLine() //slot
{
    m_rememberLineDirtyBit = false;
}

bool IRCView::lastBlockIsLine(int select)
{
    int state = document()->lastBlock().userState();

    if (select == -1)
        return (state == BlockIsRemember || state == BlockIsMarker);

    return state == select;
}

void IRCView::appendRememberLine()
{
    //clear this now, so that doAppend doesn't double insert
    m_rememberLineDirtyBit = false;

    //if the last line is already the remember line, do nothing
    if (lastBlockIsLine(BlockIsRemember))
        return;

    // if we already have a rememberline, remove the previous one
    if (m_rememberLinePosition > -1)
    {
        //get the block that is the remember line
        QTextBlock rem = m_markers[m_rememberLinePosition];
        m_markers.removeAt(m_rememberLinePosition); //probably will be in there only once
        m_rememberLinePosition=-1;
        voidLineBlock(rem);
    }

    //tell the control we did stuff
    //FIXME do we still do something like this?
    //repaintChanged();

    //actually insert a line
    appendLine(IRCView::RememberLine);

    //store the index of the remember line
    m_rememberLinePosition = m_markers.count() - 1;
}

void IRCView::voidLineBlock(QTextBlock rem)
{
    if (rem.blockNumber() == 0)
    {
        Q_ASSERT(m_nextCullIsMarker);
        m_nextCullIsMarker = false;
    }
    QTextCursor c(rem);
    //FIXME make sure this doesn't flicker
    c.select(QTextCursor::BlockUnderCursor);
    c.removeSelectedText();
}

void IRCView::clearLines()
{
    //if we have a remember line, put it in the list
        //its already in the list

    kDebug() << _S(m_nextCullIsMarker) << _S(m_rememberLinePosition) << _S(textCursor().position()) << m_markers;
    dump_doc(document());

    //are there any markers?
    if (hasLines() > 0)
    {
        for (int i=0; i < m_markers.count(); ++i)
            voidLineBlock(m_markers[i]);

        wipeLineParagraphs();

        //FIXME do we have this? //repaintChanged();
    }

}

void IRCView::wipeLineParagraphs()
{
    m_nextCullIsMarker = false;
    m_rememberLinePosition = -1;
    m_markers.clear();
}

bool IRCView::hasLines()
{
    return m_markers.count() > 0;
}

QTextCharFormat IRCView::getFormat(ObjectFormats x)
{
    QTextCharFormat f;
    f.setObjectType(x);
    return f;
}

void IRCView::appendLine(IRCView::ObjectFormats type)
{
    ScrollBarPin barpin(verticalScrollBar());
    SelectionPin selpin(this);

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);

    cursor.insertBlock();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), getFormat(type));
    cursor.block().setUserState(type == MarkerLine? BlockIsMarker : BlockIsRemember);

    m_markers.append(cursor.block());
}


//// Other stuff

void IRCView::enableParagraphSpacing() {}

void IRCView::updateAppearance()
{
    if (Preferences::self()->customTextFont())
        setFont(Preferences::self()->textFont());
    else
        setFont(KGlobalSettings::generalFont());

    setVerticalScrollBarPolicy(Preferences::self()->showIRCViewScrollBar() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

    QPalette p;

    p.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));

    if (Preferences::self()->showBackgroundImage())
    {
        KUrl url = Preferences::self()->backgroundImage();

        if (!url.isEmpty())
        {
            QBrush brush;

            brush.setTexture(QPixmap(url.path()));

            p.setBrush(QPalette::Base, brush);
        }
    }

    setPalette(p);
}

// Data insertion

void IRCView::append(const QString& nick, const QString& message)
{
    QString channelColor = Preferences::self()->color(Preferences::ChannelMessage).name();

    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick, channelColor);

    QString line;
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + channelColor + "\">" + nickLine +" %1" + PDF + RLM + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + channelColor + "\">%1" + nickLine + " %3</font>";
    }

    line = line.arg(timeStamp(), nick, filter(message, channelColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line, rtl);
}

void IRCView::appendRaw(const QString& message, bool suppressTimestamps, bool self)
{
    QColor channelColor=Preferences::self()->color(Preferences::ChannelMessage);
    m_tabNotification = Konversation::tnfNone;

    QString line;
    if (suppressTimestamps)
        line = QString("<font color=\"" + channelColor.name() + "\">" + message + "</font>");
    else
        line = QString(timeStamp() + " <font color=\"" + channelColor.name() + "\">" + message + "</font>");

    doAppend(line, false, self);
}

void IRCView::appendLog(const QString & message)
{
    QColor channelColor = Preferences::self()->color(Preferences::ChannelMessage);
    m_tabNotification = Konversation::tnfNone;

    QString line("<font color=\"" + channelColor.name() + "\">" + message + "</font>");

    doRawAppend(line, !QApplication::isLeftToRight());
}

void IRCView::appendQuery(const QString& nick, const QString& message, bool inChannel)
{
    QString queryColor=Preferences::self()->color(Preferences::QueryMessage).name();

    m_tabNotification = Konversation::tnfPrivate;

    QString nickLine = createNickLine(nick, queryColor, true, inChannel);

    QString line;
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + queryColor + "\">" + nickLine + " %1" + PDF + RLM + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + queryColor + "\">%1 " + nickLine + " %3</font>";
    }

    line = line.arg(timeStamp(), nick, filter(message, queryColor, nick, true));

    emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

    doAppend(line, rtl);
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

    QString nickLine = createNickLine(nick, actionColor, false);

    QString line;
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + actionColor + "\">" + nickLine + " * %1" + PDF + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + actionColor + "\">%1 * " + nickLine + " %3</font>";
    }

    line = line.arg(timeStamp(), nick, filter(message, actionColor, nick, true));

    emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

    doAppend(line, rtl);
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
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + serverColor + "\"" + fixed + "><b>[</b>%2<b>]</b> %1" + PDF + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + serverColor + "\"" + fixed + ">%1 <b>[</b>%2<b>]</b> %3</font>";
    }

    if(type != i18n("Notify"))
        line = line.arg(timeStamp(), type, filter(message, serverColor, 0 , true, parseURL));
    else
        line = "<font color=\"" + serverColor + "\">"+line.arg(timeStamp(), type, message)+"</font>";

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line, rtl);
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
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + commandColor + "\">%2 %1" + PDF + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + commandColor + "\">%1 %2 %3</font>";
    }

    line = line.arg(timeStamp(), prefix, filter(message, commandColor, 0, true, parseURL, self));

    emit textToLog(QString("%1\t%2").arg(type).arg(message));

    doAppend(line, rtl, self);
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

    if(!nick.isEmpty() && !nick.startsWith('<') && !nick.startsWith('*'))
    {
        nick = '|' + nick + '|';
    }

    // Nicks are in "<nick>" format so replace the "<>"
    nick.replace('<',"&lt;");
    nick.replace('>',"&gt;");

    QString line;
    bool rtl = (basicDirection(message) == QChar::DirR);

    if(rtl)
    {
        line = RLE;
        line += LRE;
        line += "<font color=\"" + backlogColor + "\">%2 %1" + PDF + " %3</font>";
    }
    else
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + backlogColor + "\">%1 %2 %3</font>";
    }

    line = line.arg(time, nick, filter(message, backlogColor, NULL, false, false));

    doAppend(line, rtl);
}

void IRCView::doAppend(const QString& newLine, bool rtl, bool self)
{
    if (m_rememberLineDirtyBit)
        appendRememberLine();

    if (!self && m_chatWin)
        m_chatWin->activateTabNotification(m_tabNotification);

    int scrollMax = Preferences::self()->scrollbackMax();
    if (scrollMax != 0)
    {
        //don't remove lines if the user has scrolled up to read old lines
        bool atBottom = (verticalScrollBar()->value() == verticalScrollBar()->maximum());
        document()->setMaximumBlockCount(atBottom ? scrollMax : document()->maximumBlockCount() + 1);
        //setMaximumBlockCount(atBottom ? scrollMax : maximumBlockCount() + 1);
    }

    doRawAppend(newLine, rtl);

    //appendHtml(line);

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

void IRCView::doRawAppend(const QString& newLine, bool rtl)
{
    SelectionPin selpin(this); // HACK stop selection at end from growing
    QString line(newLine);

    line.remove('\n');

    KTextBrowser::append(line);

    QTextCursor formatCursor(document()->lastBlock());
    QTextBlockFormat format = formatCursor.blockFormat();

    if (!QApplication::isLeftToRight())
        rtl = !rtl;

    format.setAlignment(rtl ? Qt::AlignRight : Qt::AlignLeft);
    formatCursor.setBlockFormat(format);
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

QString IRCView::createNickLine(const QString& nick, const QString& defaultColor, bool encapsulateNick, bool privMsg)
{
    QString nickLine = "%2";
    QString nickColor;

    if (Preferences::self()->useColoredNicks())
    {
        if (m_server)
        {
            if (nick != m_server->getNickname())
                nickColor = Preferences::self()->nickColor(m_server->obtainNickInfo(nick)->getNickColor()).name();
            else
                nickColor =  Preferences::self()->nickColor(8).name();
        }
        else if (m_chatWin->getType() == ChatWindow::DccChat)
        {
            QString ownNick = static_cast<DCC::Chat*>(m_chatWin)->getOwnNick();

            if (nick != ownNick)
                nickColor = Preferences::self()->nickColor(Konversation::colorForNick(ownNick)).name();
            else
                nickColor = Preferences::self()->nickColor(8).name();
        }
    }
    else
        nickColor = defaultColor;

    nickLine = "<font color=\"" + nickColor + "\">"+nickLine+"</font>";

    if (Preferences::self()->useClickableNicks())
        nickLine = "<a class=\"nick\" href=\"#" + nick + "\">" + nickLine + "</a>";

    if (privMsg)
        nickLine.prepend ("-&gt; ");

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
    Application* konvApp = static_cast<Application*>(kapp);

    //Since we can't turn off whitespace simplification withouteliminating text wrapping,
    //  if the line starts with a space turn it into a non-breaking space.
    //    (which magically turns back into a space on copy)

    if (filteredLine[0]==' ')
        filteredLine[0]='\xA0';

    // TODO: Use QStyleSheet::escape() here
    // Replace all < with &lt;
    filteredLine.replace('<',"\x0blt;");
    // Replace all > with &gt;
    filteredLine.replace('>', "\x0bgt;");

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
            colorString.clear();
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
    // TODO: \017 should reset all text decorations to plain text
    replaceDecoration(filteredLine,'\x02','b');
    replaceDecoration(filteredLine,'\x09','i');
    replaceDecoration(filteredLine,'\x13','s');
    replaceDecoration(filteredLine,'\x15','u');
    replaceDecoration(filteredLine,'\x16','b');   // should be inverse
    replaceDecoration(filteredLine,'\x1f','u');

    if(parseURL)
    {
        if(whoSent.isEmpty())
            filteredLine = Konversation::tagUrls(filteredLine, m_chatWin->getName());
        else
            filteredLine = Konversation::tagUrls(filteredLine, whoSent);
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
        ownNick = static_cast<DCC::Chat*>(m_chatWin)->getOwnNick();
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
            QList<Highlight*> highlightList = Preferences::highlightList();
            QListIterator<Highlight*> it(highlightList);
            Highlight* highlight;
            bool patternFound = false;

            QStringList captures;
            while (it.hasNext())
            {
                highlight = it.next();
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

                if (patternFound)
                    break;
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
                m_autoTextToSend.remove(QRegExp("%[0-9]"));
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
    m_currentNick.clear();
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

void IRCView::setupNickPopupMenu(bool isQuery)
{
    m_nickPopup = new KMenu(this);
    m_nickPopup->setObjectName("nicklist_context_menu");
    m_nickPopup->setTitle(m_currentNick);

    QAction* action = m_nickPopup->addAction(i18n("&Whois"), this, SLOT(handleContextActions()));
    action->setData(Konversation::Whois);
    action = m_nickPopup->addAction(i18n("&Version"), this, SLOT(handleContextActions()));
    action->setData(Konversation::Version);
    action = m_nickPopup->addAction(i18n("&Ping"), this, SLOT(handleContextActions()));
    action->setData(Konversation::Ping);

    m_nickPopup->addSeparator();

    if(!isQuery)
    {
        QMenu* modes = m_nickPopup->addMenu(i18n("Modes"));
        action = modes->addAction(i18n("Give Op"), this, SLOT(handleContextActions()));
        action->setData(Konversation::GiveOp);
#if KDE_IS_VERSION(4, 2, 85)
        action->setIcon(KIcon("irc-operator"));
#endif
        action = modes->addAction(i18n("Take Op"), this, SLOT(handleContextActions()));
        action->setData(Konversation::TakeOp);
#if KDE_IS_VERSION(4, 2, 85)
        action->setIcon(KIcon("irc-remove-operator"));
#endif
        action = modes->addAction(i18n("Give Voice"), this, SLOT(handleContextActions()));
        action->setData(Konversation::GiveVoice);
#if KDE_IS_VERSION(4, 2, 85)
        action->setIcon(KIcon("irc-voice"));
#endif
        action = modes->addAction(i18n("Take Voice"), this, SLOT(handleContextActions()));
        action->setData(Konversation::TakeVoice);
#if KDE_IS_VERSION(4, 2, 85)
        action->setIcon(KIcon("irc-unvoice"));
#endif

        QMenu* kickban = m_nickPopup->addMenu(i18n("Kick / Ban"));
        action = kickban->addAction(i18n("Kick"), this, SLOT(handleContextActions()));
        action->setData(Konversation::Kick);
        action = kickban->addAction(i18n("Kickban"), this, SLOT(handleContextActions()));
        action->setData(Konversation::KickBan);
        action = kickban->addAction(i18n("Ban Nickname"), this, SLOT(handleContextActions()));
        action->setData(Konversation::BanNick);
        kickban->addSeparator();
        action = kickban->addAction(i18n("Ban *!*@*.host"), this, SLOT(handleContextActions()));
        action->setData(Konversation::BanHost);
        action = kickban->addAction(i18n("Ban *!*@domain"), this, SLOT(handleContextActions()));
        action->setData(Konversation::BanDomain);
        action = kickban->addAction(i18n("Ban *!user@*.host"), this, SLOT(handleContextActions()));
        action->setData(Konversation::BanUserHost);
        action = kickban->addAction(i18n("Ban *!user@domain"), this, SLOT(handleContextActions()));
        action->setData(Konversation::BanUserDomain);
        kickban->addSeparator();
        action = kickban->addAction(i18n("Kickban *!*@*.host"), this, SLOT(handleContextActions()));
        action->setData(Konversation::KickBanHost);
        action = kickban->addAction(i18n("Kickban *!*@domain"), this, SLOT(handleContextActions()));
        action->setData(Konversation::KickBanDomain);
        action = kickban->addAction(i18n("Kickban *!user@*.host"), this, SLOT(handleContextActions()));
        action->setData(Konversation::KickBanUserHost);
        action = kickban->addAction(i18n("Kickban *!user@domain"), this, SLOT(handleContextActions()));
        action->setData(Konversation::KickBanUserDomain);
    }

    m_ignoreAction = new KToggleAction(i18n("Ignore"), this);
    m_ignoreAction->setCheckedState(KGuiItem(i18n("Unignore")));
    m_ignoreAction->setData(Konversation::IgnoreNick);
    m_nickPopup->addAction(m_ignoreAction);
    connect(m_ignoreAction, SIGNAL(triggered()), this, SLOT(handleContextActions()));

    m_nickPopup->addSeparator();

    action = m_nickPopup->addAction(i18n("Open Query"), this, SLOT(handleContextActions()));
    action->setData(Konversation::OpenQuery);

    KConfigGroup config = KGlobal::config()->group("KDE Action Restrictions");

    if(config.readEntry<bool>("allow_downloading", true))
    {
        action = m_nickPopup->addAction(SmallIcon("arrow-right-double"),i18n("Send &File..."), this, SLOT(handleContextActions()));
        action->setData(Konversation::DccSend);
    }

    m_nickPopup->addSeparator();

    m_addNotifyAction = m_nickPopup->addAction(i18n("Add to Watched Nicks"), this, SLOT(handleContextActions()));
    m_addNotifyAction->setData(Konversation::AddNotify);
}

void IRCView::updateNickMenuEntries(const QString& nickname)
{
    if (Preferences::isIgnored(nickname))
    {
        m_ignoreAction->setChecked(true);
        m_ignoreAction->setData(Konversation::UnignoreNick);
    }
    else
    {
        m_ignoreAction->setChecked(false);
        m_ignoreAction->setData(Konversation::IgnoreNick);
    }

    if (!m_server || !m_server->getServerGroup() || !m_server->isConnected() || !Preferences::hasNotifyList(m_server->getServerGroup()->id())
        || Preferences::isNotify(m_server->getServerGroup()->id(), nickname))
    {
        m_addNotifyAction->setEnabled(false);
    }
    else
    {
        m_addNotifyAction->setEnabled(true);
    }
}

void IRCView::setupChannelPopupMenu()
{
    m_channelPopup = new KMenu(this);
    m_channelPopup->setObjectName("channel_context_menu");
    m_channelPopup->setTitle(m_currentChannel);

    QAction* action = m_channelPopup->addAction(i18n("&Join Channel..."), this, SLOT(handleContextActions()));
    action->setData(Konversation::Join);
    #if KDE_IS_VERSION(4,2,85)
    action->setIcon(KIcon("irc-join-channel"));
    #else
    action->setIcon(KIcon("list-add"));
    #endif
    action = m_channelPopup->addAction(i18n("Get &user list"), this, SLOT(handleContextActions()));
    action->setData(Konversation::Names);
    action = m_channelPopup->addAction(i18n("Get &topic"), this, SLOT(handleContextActions()));
    action->setData(Konversation::Topic);
}

void IRCView::resizeEvent(QResizeEvent *event)
{
    ScrollBarPin b(verticalScrollBar());
    KTextBrowser::resizeEvent(event);
}

void IRCView::mouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressed && (m_pressPosition - ev->pos()).manhattanLength() > KApplication::startDragDistance())
    {
        m_mousePressed = false;

        QTextCursor textCursor = this->textCursor();
        textCursor.clearSelection();
        setTextCursor(textCursor);


        QPointer<QDrag> drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;

        KUrl url(m_urlToDrag);
        url.populateMimeData(mimeData);

        drag->setMimeData(mimeData);

        QPixmap pixmap = KIO::pixmapForUrl(url, 0, KIconLoader::Desktop, KIconLoader::SizeMedium);
        drag->setPixmap(pixmap);

        drag->exec();

        return;
    }

    KTextBrowser::mouseMoveEvent(ev);
}

void IRCView::mousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_urlToDrag = anchorAt(ev->pos());

        if (!m_urlToDrag.isEmpty() && Konversation::isUrl(m_urlToDrag))
        {
            m_mousePressed = true;
            m_pressPosition = ev->pos();
        }
    }

    KTextBrowser::mousePressEvent(ev);
}

void IRCView::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_mousePressed = false;
    }
    else if (ev->button() == Qt::MidButton)
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

    KTextBrowser::mouseReleaseEvent(ev);
}

void IRCView::anchorClicked(const QUrl& url)
{
    openLink(url.toString());
}

// FIXME do we still care about newtab? looks like konqi has lots of config now..
void IRCView::openLink(const QString& url, bool)
{
    QString link(url);
    // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
    link = link.replace (' ', "%20");

    if (!link.isEmpty() && !link.startsWith('#'))
    {
        if (link.startsWith(QLatin1String("irc://")))
        {
            Application* konvApp = Application::instance();
            konvApp->getConnectionManager()->connectTo(Konversation::SilentlyReuseConnection, link);
        }
        else
            Application::openUrl(link);
    }
    //FIXME: Don't do channel links in DCC Chats to begin with since they don't have a server.
    else if (link.startsWith(QLatin1String("##")) && m_server && m_server->isConnected())
    {
        QString channel(link);
        channel.replace("##", "#");
        m_server->sendJoinCommand(channel);
    }
    //FIXME: Don't do user links in DCC Chats to begin with since they don't have a server.
    else if (link.startsWith('#') && m_server && m_server->isConnected())
    {
        QString recipient(link);
        recipient.remove('#');
        NickInfoPtr nickInfo = m_server->obtainNickInfo(recipient);
        m_server->addQuery(nickInfo, true /*we initiated*/);
    }
}

void IRCView::saveLinkAs()
{
    if(m_urlToCopy.isEmpty())
        return;

    KUrl srcUrl (m_urlToCopy);
    KUrl saveUrl = KFileDialog::getSaveUrl(srcUrl.fileName(KUrl::ObeyTrailingSlash), QString(), this, i18n("Save link as"));

    if (saveUrl.isEmpty() || !saveUrl.isValid())
        return;

    KIO::copy(srcUrl, saveUrl);
}

void IRCView::highlightedSlot(const QString& _link)
{
    QString link = _link;
    // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
    link = link.replace (' ', "%20");

    //Hack to handle the fact that we get a decoded url
    //FIXME someone who knows what it looks like when we get a decoded url can reenable this if necessary...
    //link = KUrl(link).url();

    //we just saw this a second ago.  no need to reemit.
    if (link == m_lastStatusText && !link.isEmpty())
        return;

    // remember current URL to overcome link clicking problems in KTextBrowser
    //m_highlightedURL = link;

    if (link.isEmpty())
    {
        if (!m_lastStatusText.isEmpty())
        {
            emit clearStatusBarTempText();
            m_lastStatusText.clear();
        }
    } else
    {
        m_lastStatusText = link;
    }

    if(!link.startsWith('#'))
    {
        m_isOnNick = false;
        m_isOnChannel = false;

        if (!link.isEmpty()) {
            //link therefore != m_lastStatusText  so emit with this new text
            emit setStatusBarTempText(link);
        }
        if (link.isEmpty() && m_copyUrlMenu)
        {
                m_copyUrlClipBoard->setVisible( false );
                m_bookmark->setVisible( false );
                m_saveUrl->setVisible( false );
            copyUrlMenuSeparator->setVisible( false );
            m_copyUrlMenu = false;

        }
        else if (!link.isEmpty() && !m_copyUrlMenu)
        {
                copyUrlMenuSeparator->setVisible( true );
                m_copyUrlClipBoard->setVisible( true );
                m_bookmark->setVisible( true );
                m_saveUrl->setVisible( true );
            m_copyUrlMenu = true;
            m_urlToCopy = link;
        }
    }
    else if (link.startsWith('#') && !link.startsWith(QLatin1String("##")))
    {
        m_currentNick = link.mid(1);

        if(m_nickPopup)
        {
            m_nickPopup->setTitle(m_currentNick);
        }

        m_isOnNick = true;
        emit setStatusBarTempText(i18n("Open a query with %1", m_currentNick));
    }
    else
    {
        // link.startsWith("##")
        m_currentChannel = link.mid(1);

        if(m_channelPopup)
        {
            QString prettyId = m_currentChannel;

            if (prettyId.length()>15)
            {
                prettyId.truncate(15);
                prettyId.append("...");
            }

            m_channelPopup->setTitle(prettyId);
        }

        m_isOnChannel = true;
        emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
    }
}

void IRCView::copyUrl()
{
        if ( !m_urlToCopy.isEmpty() )
        {
                QClipboard *cb = qApp->clipboard();
                cb->setText(m_urlToCopy,QClipboard::Selection);
                cb->setText(m_urlToCopy,QClipboard::Clipboard);
        }

}

void IRCView::slotBookmark()
{
    if (m_urlToCopy.isEmpty())
        return;

    KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
    KBookmarkDialog* dialog = new KBookmarkDialog(bm, this);
    dialog->addBookmark(m_urlToCopy, m_urlToCopy);
    delete dialog;
}

void IRCView::contextMenuEvent(QContextMenuEvent* ev)
{
    if (m_nickPopup && m_server && m_isOnNick && m_nickPopup->isEnabled())
    {
        updateNickMenuEntries(getContextNick());

        if(m_nickPopup->exec(ev->globalPos()) == 0)
            clearContextNick();

        m_isOnNick = false;
    }
    else if(m_channelPopup && m_server && m_isOnChannel && m_channelPopup->isEnabled())
    {
        m_channelPopup->exec(ev->globalPos());
        m_isOnChannel = false;
    }
    else
    {
        KActionCollection* actionCollection = Application::instance()->getMainWindow()->actionCollection();
        KToggleAction* toggleMenuBarAction = static_cast<KToggleAction*>(actionCollection->action("options_show_menubar"));
        QAction* separator = NULL;

        if(toggleMenuBarAction && !toggleMenuBarAction->isChecked())
        {
            m_popup->insertAction(m_copyUrlClipBoard, toggleMenuBarAction);
            separator = m_popup->insertSeparator(m_copyUrlClipBoard);
        }

        m_popup->exec(ev->globalPos());

        if(separator)
        {
            m_popup->removeAction(toggleMenuBarAction);
            m_popup->removeAction(separator);
        }
    }
}

void IRCView::handleContextActions()
{
    QAction* action = qobject_cast<QAction*>(sender());

    emit popupCommand(action->data().toInt());
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

QChar::Direction IRCView::basicDirection(const QString& string)
{
    // The following code decides between LTR or RTL direction for
    // a line based on the amount of each type of characters pre-
    // sent. It does so by counting, but stops when one of the two
    // counters becomes higher than half of the string length to
    // avoid unnecessary work.

    unsigned int pos = 0;
    unsigned int rtl_chars = 0;
    unsigned int ltr_chars = 0;
    unsigned int str_len = string.length();
    unsigned int str_half_len = str_len/2;

    for(pos=0; pos < str_len; ++pos)
    {
        if (!(string[pos].isNumber() || string[pos].isSymbol() ||
            string[pos].isSpace()  || string[pos].isPunct()  ||
            string[pos].isMark()))
        {
            switch(string[pos].direction())
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

        if (ltr_chars > str_half_len)
            return QChar::DirL;
        else if (rtl_chars > str_half_len)
            return QChar::DirR;
    }

    if (rtl_chars > ltr_chars)
        return QChar::DirR;
    else
        return QChar::DirL;
}

// **WARNING** the selectionChange signal comes BEFORE the selection has actually been changed, hook cursorPositionChanged too

//void IRCView::mouseDoubleClickEvent(QEvent *e, Qt::MouseButton button, const QPointF &pos)

