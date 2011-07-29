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
  Copyright (C) 2006-2010 Eike Hein <hein@kde.org>
  Copyright (C) 2004-2009 Eli Mackenzie <argonel@gmail.com>
*/

#include "ircview.h"
#include "channel.h"
#include "dcc/chatcontainer.h"
#include "application.h"
#include "highlight.h"
#include "sound.h"
#include "emoticons.h"
#include "notificationhandler.h"

#include <QScrollBar>
#include <QTextBlock>
#include <QPainter>
#include <QTextDocumentFragment>

#include <KStandardShortcut>

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


IRCView::IRCView(QWidget* parent) : KTextBrowser(parent), m_nextCullIsMarker(false), m_rememberLinePosition(-1), m_rememberLineDirtyBit(false), markerFormatObject(this)
{
    m_resetScrollbar = true;
    m_mousePressedOnUrl = false;
    m_isOnNick = false;
    m_isOnChannel = false;
    m_chatWin = 0;
    m_server = 0;

    setAcceptDrops(false);

    // Marker lines
    connect(document(), SIGNAL(contentsChange(int,int,int)), SLOT(cullMarkedLine(int,int,int)));

    //This assert is here because a bad build environment can cause this to fail. There is a note
    // in the Qt source that indicates an error should be output, but there is no such output.
    QTextObjectInterface *iface = qobject_cast<QTextObjectInterface *>(&markerFormatObject);
    if (!iface)
    {
        Q_ASSERT(iface);
    }

    document()->documentLayout()->registerHandler(IRCView::MarkerLine, &markerFormatObject);
    document()->documentLayout()->registerHandler(IRCView::RememberLine, &markerFormatObject);


    connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(anchorClicked(QUrl)));
    connect( this, SIGNAL(highlighted(QString)), this, SLOT(highlightedSlot(QString)) );
    setOpenLinks(false);
    setUndoRedoEnabled(0);
    document()->setDefaultStyleSheet("a.nick:link {text-decoration: none}");
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setFocusPolicy(Qt::ClickFocus);
    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    viewport()->setMouseTracking(true);

    if (Preferences::self()->useParagraphSpacing()) enableParagraphSpacing();

    //HACK to workaround an issue with the QTextDocument
    //doing a relayout/scrollbar over and over resulting in 100%
    //proc usage. See bug 215256
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContextMenuOptions(IrcContextMenus::ShowTitle | IrcContextMenus::ShowFindAction, true);
}

IRCView::~IRCView()
{
}

void IRCView::setServer(Server* newServer)
{
    if (m_server == newServer)
        return;

    m_server = newServer;
}

void IRCView::setChatWin(ChatWindow* chatWin)
{
    m_chatWin = chatWin;
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

// Marker lines

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

class IrcViewMimeData : public QMimeData
{
public:
    IrcViewMimeData(const QTextDocumentFragment& _fragment): fragment(_fragment) {}
    virtual QStringList formats() const;

protected:
    virtual QVariant retrieveData(const QString &mimeType, QVariant::Type type) const;

private:
    mutable QTextDocumentFragment fragment;
};

QStringList IrcViewMimeData::formats() const
{
    if (!fragment.isEmpty())
        return QStringList() << QString::fromLatin1("text/plain");
    else
        return QMimeData::formats();
}

QVariant IrcViewMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    if (!fragment.isEmpty())
    {
        IrcViewMimeData *that = const_cast<IrcViewMimeData *>(this);

        //Copy the text, skipping any QChar::ObjectReplacementCharacter
        QRegExp needle(QString("\\xFFFC\\n?"));

        that->setText(fragment.toPlainText().remove(needle));
        fragment = QTextDocumentFragment();
    }
    return QMimeData::retrieveData(mimeType, type);
}

QMimeData *IRCView::createMimeDataFromSelection() const
{
    const QTextDocumentFragment fragment(textCursor());
    return new IrcViewMimeData(fragment);
}

void IRCView::dragEnterEvent(QDragEnterEvent* e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
    else
        e->ignore();
}

void IRCView::dragMoveEvent(QDragMoveEvent* e)
{
    if (e->mimeData()->hasUrls())
        e->accept();
    else
        e->ignore();
}

void IRCView::dropEvent(QDropEvent* e)
{
    if (e->mimeData() && e->mimeData()->hasUrls())
        emit urlsDropped(KUrl::List::fromMimeData(e->mimeData(), KUrl::List::PreferLocalUrls));
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
    if (hasLines())
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


// Other stuff

void IRCView::enableParagraphSpacing() {}

void IRCView::updateAppearance()
{
    if (Preferences::self()->customTextFont())
        setFont(Preferences::self()->textFont());
    else
        setFont(KGlobalSettings::generalFont());

    setVerticalScrollBarPolicy(Preferences::self()->showIRCViewScrollBar() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

    if (Preferences::self()->showBackgroundImage())
    {
        KUrl url = Preferences::self()->backgroundImage();

        if (url.hasPath())
        {
            viewport()->setStyleSheet("QWidget { background-image: url("+url.path()+"); background-attachment:fixed; }");

            return;
        }
    }

    if (!viewport()->styleSheet().isEmpty())
        viewport()->setStyleSheet("");

    QPalette p;
    p.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
    viewport()->setPalette(p);
}


// Data insertion

void IRCView::append(const QString& nick, const QString& message)
{
    QString channelColor = Preferences::self()->color(Preferences::ChannelMessage).name();

    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick, channelColor);

    QChar::Direction dir;
    QString text(filter(message, channelColor, nick, true, true, false, &dir));
    QString line;
    bool rtl = (dir == QChar::DirR);

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

    line = line.arg(timeStamp(), nick, text);

    emit textToLog(QString("<%1>\t%2").arg(nick, message));

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
    QChar::Direction dir;
    QString text(filter(message, queryColor, nick, true, true, false, &dir));
    bool rtl = (dir == QChar::DirR);

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

    line = line.arg(timeStamp(), nick, text);

    emit textToLog(QString("<%1>\t%2").arg(nick, message));

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
    QString actionColor = Preferences::self()->color(Preferences::ActionMessage).name();

    QString line;

    QString nickLine = createNickLine(nick, actionColor, false);

    if (message.isEmpty())
    {
        if (!QApplication::isLeftToRight())
            line += LRE;

        line += "<font color=\"" + actionColor + "\">%1 * " + nickLine + "</font>";

        line = line.arg(timeStamp(), nick);

        emit textToLog(QString("\t * %1").arg(nick));

        doAppend(line, false);
    }
    else
    {
        QChar::Direction dir;
        QString text(filter(message, actionColor, nick, true,true, false, &dir));
        bool rtl = (dir == QChar::DirR);

        if (rtl)
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

        line = line.arg(timeStamp(), nick, text);

        emit textToLog(QString("\t * %1 %2").arg(nick, message));

        doAppend(line, rtl);
    }
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
    QChar::Direction dir;
    QString text(filter(message, serverColor, 0 , true, parseURL, false, &dir));
    bool rtl = (dir == QChar::DirR);

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

    line = line.arg(timeStamp(), type, text);

    emit textToLog(QString("%1\t%2").arg(type, message));

    doAppend(line, rtl);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool parseURL, bool self)
{
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
    QChar::Direction dir;
    QString text(filter(message, commandColor, 0, true, parseURL, self, &dir));
    bool rtl = (dir == QChar::DirR);

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

    line = line.arg(timeStamp(), prefix, text);

    emit textToLog(QString("%1\t%2").arg(type, message));

    doAppend(line, rtl, self);
}

void IRCView::appendBacklogMessage(const QString& firstColumn,const QString& rawMessage)
{
    QString time;
    QString message = rawMessage;
    QString nick = firstColumn;
    QString backlogColor = Preferences::self()->color(Preferences::BacklogMessage).name();
    m_tabNotification = Konversation::tnfNone;

    //The format in Chatwindow::logText is not configurable, so as long as nobody allows square brackets in a date/time format....
    int eot = nick.lastIndexOf(' ');
    time = nick.left(eot);
    nick = nick.mid(eot+1);

    if(!nick.isEmpty() && !nick.startsWith('<') && !nick.startsWith('*'))
    {
        nick = '|' + nick + '|';
    }

    // Nicks are in "<nick>" format so replace the "<>"
    nick.replace('<',"&lt;");
    nick.replace('>',"&gt;");

    QString line;
    QChar::Direction dir;
    QString text(filter(message, backlogColor, NULL, false, false, false, &dir));
    bool rtl = (dir == QChar::DirR);

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

    line = line.arg(time, nick, text);

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
    }

    doRawAppend(newLine, rtl);

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
            timeString = QString(QLatin1String("<font color=\"") + timeColor + QLatin1String("\">[%1]</font> ")).arg(time.toString(timeFormat));
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
            QString ownNick = static_cast<DCC::ChatContainer*>(m_chatWin)->ownNick();

            if (nick != ownNick)
                nickColor = Preferences::self()->nickColor(Konversation::colorForNick(ownNick)).name();
            else
                nickColor = Preferences::self()->nickColor(8).name();
        }
    }
    else
        nickColor = defaultColor;

    nickLine = QLatin1String("<font color=\"") + nickColor + QLatin1String("\">") + nickLine + QLatin1String("</font>");

    if (Preferences::self()->useClickableNicks())
        nickLine = "<a class=\"nick\" href=\"#" + nick + "\">" + nickLine + "</a>";

    if (privMsg)
        nickLine.prepend(QLatin1String("-&gt; "));

    if(encapsulateNick)
        nickLine = QLatin1String("&lt;") + nickLine + QLatin1String("&gt;");

    if(Preferences::self()->useBoldNicks())
        nickLine = QLatin1String("<b>") + nickLine + QLatin1String("</b>");

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
    bool doHighlight, bool parseURL, bool self, QChar::Direction* direction)
{
    QString filteredLine(line);
    Application* konvApp = static_cast<Application*>(kapp);

    //Since we can't turn off whitespace simplification withouteliminating text wrapping,
    //  if the line starts with a space turn it into a non-breaking space.
    //    (which magically turns back into a space on copy)

    if (filteredLine[0] == ' ')
    {
        filteredLine[0] = '\xA0';
    }

    // TODO: Use QStyleSheet::escape() here
    // Replace all < with &lt;
    filteredLine.replace('<', "\x0blt;");
    // Replace all > with &gt;
    filteredLine.replace('>', "\x0bgt;");

    if (filteredLine.contains('\x07'))
    {
        if (Preferences::self()->beep())
        {
            kapp->beep();
        }
        //remove char after beep
        filteredLine.remove('\x07');
    }

    filteredLine = ircTextToHtml(filteredLine,  parseURL, defaultColor, whoSent, true, direction);

    // Highlight
    QString ownNick;

    if (m_server)
    {
        ownNick = m_server->getNickname();
    }
    else if (m_chatWin->getType() == ChatWindow::DccChat)
    {
        ownNick = static_cast<DCC::ChatContainer*>(m_chatWin)->ownNick();
    }

    if(doHighlight && (whoSent != ownNick) && !self)
    {
        QString highlightColor;

        if (Preferences::self()->highlightNick() &&
            line.toLower().contains(QRegExp("(^|[^\\d\\w])" +
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
            QStringList highlightChatWindowList;
            bool patternFound = false;

            QStringList captures;
            while (it.hasNext())
            {
                highlight = it.next();
                QStringList highlightChatWindowList = highlight->getChatWindowList();
                if (highlightChatWindowList.isEmpty() ||
                    highlightChatWindowList.contains(m_chatWin->getName(), Qt::CaseInsensitive))
                {
                    if (highlight->getRegExp())
                    {
                        QRegExp needleReg(highlight->getPattern());
                        needleReg.setCaseSensitivity(Qt::CaseInsensitive);
                                                      // highlight regexp in text
                        patternFound = ((line.contains(needleReg)) ||
                                                      // highlight regexp in nickname
                            (whoSent.contains(needleReg)));

                        // remember captured patterns for later
                        captures = needleReg.capturedTexts();

                    }
                    else
                    {
                        QString needle = highlight->getPattern();
                                                      // highlight patterns in text
                        patternFound = ((line.contains(needle, Qt::CaseInsensitive)) ||
                                                      // highlight patterns in nickname
                            (whoSent.contains(needle, Qt::CaseInsensitive)));
                    }

                    if (patternFound)
                    {
                        break;
                    }
                }
            }

            if (patternFound)
            {
                highlightColor = highlight->getColor().name();
                m_highlightColor = highlightColor;
                m_tabNotification = Konversation::tnfHighlight;

                if (Preferences::self()->highlightSoundsEnabled() && m_chatWin->notificationsEnabled())
                {
                    konvApp->sound()->play(highlight->getSoundURL());
                }

                konvApp->notificationHandler()->highlight(m_chatWin, whoSent, line);
                m_autoTextToSend = highlight->getAutoText();

                // replace %0 - %9 in regex groups
                for (int capture = 0; capture < captures.count(); capture++)
                {
                    m_autoTextToSend.replace(QString("%%1").arg(capture), captures[capture]);
                }
                m_autoTextToSend.remove(QRegExp("%[0-9]"));
            }
        }

        // apply found highlight color to line
        if (!highlightColor.isEmpty())
        {
            filteredLine = QLatin1String("<font color=\"") + highlightColor + QLatin1String("\">") + filteredLine +
                QLatin1String("</font>");
        }
    }
    else if (doHighlight && (whoSent == ownNick) && Preferences::self()->highlightOwnLines())
    {
        // highlight own lines
        filteredLine = QLatin1String("<font color=\"") + Preferences::self()->highlightOwnLinesColor().name() +
            QLatin1String("\">") + filteredLine + QLatin1String("</font>");
    }

    filteredLine = Konversation::Emoticons::parseEmoticons(filteredLine);

    return filteredLine;
}

QString IRCView::ircTextToHtml(const QString& text, bool parseURL, const QString& defaultColor,
                               const QString& whoSent, bool closeAllTags, QChar::Direction* direction)
{
    TextHtmlData data;
    data.defaultColor = defaultColor;
    QString htmlText(text);

    bool allowColors = Preferences::self()->allowColorCodes();
    QString linkColor = Preferences::self()->color(Preferences::Hyperlink).name();

    unsigned int rtl_chars = 0;
    unsigned int ltr_chars = 0;

    QString fromNick;
    TextUrlData urlData;
    TextChannelData channelData;
    if (parseURL)
    {
        QString strippedText(removeIrcMarkup(htmlText));
        urlData = extractUrlData(strippedText);
        if (!urlData.urlRanges.isEmpty())
        {
            // we detected the urls on a clean richtext-char-less text
            // to make 100% sure we get the correct urls, but as a result
            // we have to map them back to the original url
            adjustUrlRanges(urlData.urlRanges, urlData.fixedUrls, htmlText, strippedText);

            //Only set fromNick if we actually have a url,
            //yes this is a ultra-minor-optimization
            if (whoSent.isEmpty())
                fromNick = m_chatWin->getName();
            else
                fromNick = whoSent;
        }

        channelData = extractChannelData(strippedText);
        adjustUrlRanges(channelData.channelRanges, channelData.fixedChannels , htmlText, strippedText);
    }
    else
    {
        // Change & to &amp; to prevent html entities to do strange things to the text
        htmlText.replace('&', "&amp;");
        htmlText.replace("\x0b", "&");
    }

    int linkPos = -1;
    int linkOffset = 0;
    bool doChannel = false;
    if (parseURL)
    {
        //get next recent channel or link pos
        if (!urlData.urlRanges.isEmpty() && !channelData.channelRanges.isEmpty())
        {
            if (urlData.urlRanges.first() < channelData.channelRanges.first())
            {
                doChannel = false;
                linkPos = urlData.urlRanges.first().first;
            }
            else
            {
                doChannel = true;
                linkPos = channelData.channelRanges.first().first;
            }
        }
        else if (!urlData.urlRanges.isEmpty() && channelData.channelRanges.isEmpty())
        {
            doChannel = false;
            linkPos = urlData.urlRanges.first().first;
        }
        else if (urlData.urlRanges.isEmpty() && !channelData.channelRanges.isEmpty())
        {
            doChannel = true;
            linkPos = channelData.channelRanges.first().first;
        }
        else
        {
            linkPos = -1;
        }
    }

    // Remember last char for pair of spaces situation, see default in switch (htmlText.at(pos)...
    QChar lastChar;
    int offset;
    for (int pos = 0; pos < htmlText.length(); ++pos)
    {
        offset = 0;

        //check for next relevant url or channel link to insert
        if (parseURL && pos == linkPos+linkOffset)
        {
            if (doChannel)
            {
                QString fixedChannel = channelData.fixedChannels.takeFirst();
                const QPair<int, int>& range = channelData.channelRanges.takeFirst();

                QString oldChannel = htmlText.mid(pos, range.second);
                QString strippedChannel = removeIrcMarkup(oldChannel);
                QString colorCodes = extractColorCodes(oldChannel);

                QString link("%1<a href=\"#%2\" style=\"color:" + linkColor + "\">%3</a>%4%5");

                link = link.arg(closeTags(&data), fixedChannel, strippedChannel, openTags(&data, 0), colorCodes);
                htmlText.replace(pos, oldChannel.length(), link);

                pos += link.length() - colorCodes.length() - 1;
                linkOffset += link.length() - oldChannel.length();
            }
            else
            {
                QString fixedUrl = urlData.fixedUrls.takeFirst();
                const QPair<int, int>& range = urlData.urlRanges.takeFirst();

                QString oldUrl = htmlText.mid(pos, range.second);
                QString strippedUrl = removeIrcMarkup(oldUrl);

                QString closeTagsString(closeTags(&data));
                QString colorCodes = extractColorCodes(oldUrl);
                colorCodes = removeDuplicateCodes(colorCodes, &data);

                QString link("%1<a href=\"%2\" style=\"color:" + linkColor + "\">%3</a>%4%5");

                link = link.arg(closeTagsString, fixedUrl, strippedUrl, openTags(&data, 0), colorCodes);
                htmlText.replace(pos, oldUrl.length(), link);

                //url catcher
                QMetaObject::invokeMethod(Application::instance(), "storeUrl", Qt::QueuedConnection,
                                          Q_ARG(QString, fromNick), Q_ARG(QString, fixedUrl), Q_ARG(QDateTime, QDateTime::currentDateTime()));

                pos += link.length() - colorCodes.length() - 1;
                linkOffset += link.length() - oldUrl.length();
            }

            bool invalidNextLink = false;
            do
            {

                if (!urlData.urlRanges.isEmpty() && !channelData.channelRanges.isEmpty())
                {
                    if (urlData.urlRanges.first() < channelData.channelRanges.first())
                    {
                        doChannel = false;
                        linkPos = urlData.urlRanges.first().first;
                    }
                    else
                    {
                        doChannel = true;
                        linkPos = channelData.channelRanges.first().first;
                    }
                }
                else if (!urlData.urlRanges.isEmpty() && channelData.channelRanges.isEmpty())
                {
                    doChannel = false;
                    linkPos = urlData.urlRanges.first().first;
                }
                else if (urlData.urlRanges.isEmpty() && !channelData.channelRanges.isEmpty())
                {
                    doChannel = true;
                    linkPos = channelData.channelRanges.first().first;
                }
                else
                {
                    linkPos = -1;
                }

                //for cases like "#www.some.url" we get first channel
                //and also url, the channel->clickable-channel replace we are
                //already after the url, so just forget it, as a clickable
                //channel is correct in this case
                if (linkPos > -1 && linkPos+linkOffset < pos)
                {
                    invalidNextLink = true;
                    if (doChannel)
                    {
                        channelData.channelRanges.removeFirst();
                        channelData.fixedChannels.removeFirst();
                    }
                    else
                    {
                        urlData.urlRanges.removeFirst();
                        urlData.fixedUrls.removeFirst();
                    }
                }
                else
                {
                    invalidNextLink = false;
                }
            } while (invalidNextLink);

            continue;
        }


        switch (htmlText.at(pos).toAscii())
        {
            case '\x02': //bold
                offset = defaultHtmlReplace(htmlText, &data, pos, QLatin1String("b"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x1d': //italic
                offset = defaultHtmlReplace(htmlText, &data, pos, QLatin1String("i"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x15': //mirc underline
            case '\x1f': //kvirc underline
                offset = defaultHtmlReplace(htmlText, &data, pos, QLatin1String("u"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x13': //strikethru
                offset = defaultHtmlReplace(htmlText, &data, pos, QLatin1String("s"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x03': //color
                {
                    QString fgColor, bgColor;
                    bool fgOK = true, bgOK = true;
                    QString colorMatch(getColors(htmlText, pos, fgColor, bgColor, &fgOK, &bgOK));

                    if (!allowColors)
                    {
                        htmlText.remove(pos, colorMatch.length());
                        pos -= 1;
                        linkOffset -= colorMatch.length();
                        break;
                    }
                    QString colorString;
                    // check for color reset conditions
                    //TODO check if \x11 \017 is really valid here
                    if (colorMatch == QLatin1String("\x03") || colorMatch == QLatin1String("\x11") ||
                        (fgColor.isEmpty() && bgColor.isEmpty()) || (!fgOK && !bgOK))
                    {
                        //in reverse mode, just reset both colors
                        //color tags are already closed before the reverse start
                        if (data.reverse)
                        {
                            data.lastFgColor.clear();
                            data.lastBgColor.clear();
                        }
                        else
                        {
                            if (data.openHtmlTags.contains(QLatin1String("font")) &&
                                data.openHtmlTags.contains(QLatin1String("span")))
                            {
                                colorString += closeToTagString(&data, QLatin1String("span"));
                                data.lastBgColor.clear();
                                colorString += closeToTagString(&data, QLatin1String("font"));
                                data.lastFgColor.clear();
                            }
                            else if (data.openHtmlTags.contains("font"))
                            {
                                colorString += closeToTagString(&data, QLatin1String("font"));
                                data.lastFgColor.clear();
                            }
                        }
                        htmlText.replace(pos, colorMatch.length(), colorString);

                        pos += colorString.length() - 1;
                        linkOffset += colorString.length() -colorMatch.length();
                        break;
                    }

                    if (!fgOK)
                    {
                        fgColor = defaultColor;
                    }
                    if (!bgOK)
                    {
                        bgColor = fontColorOpenTag(Preferences::self()->color(Preferences::TextViewBackground).name());
                    }

                    // if we are in reverse mode, just remember the new colors
                    if (data.reverse)
                    {
                        if (!fgColor.isEmpty())
                        {
                            data.lastFgColor = fgColor;
                            if (!bgColor.isEmpty())
                            {
                                data.lastBgColor = bgColor;
                            }
                        }
                    }
                    // do we have a new fgColor?
                    // NOTE: there is no new bgColor is there is no fgColor
                    else if (!fgColor.isEmpty())
                    {
                        if (data.openHtmlTags.contains(QLatin1String("font")) &&
                            data.openHtmlTags.contains(QLatin1String("span")))
                        {
                            colorString += closeToTagString(&data, QLatin1String("span"));
                            colorString += closeToTagString(&data, QLatin1String("font"));
                        }
                        else if (data.openHtmlTags.contains(QLatin1String("font")))
                        {
                            colorString += closeToTagString(&data, QLatin1String("font"));
                        }
                        data.lastFgColor = fgColor;
                        if (!bgColor.isEmpty())
                            data.lastBgColor = bgColor;

                        if (!data.lastFgColor.isEmpty())
                        {
                            colorString += fontColorOpenTag(data.lastFgColor);
                            data.openHtmlTags.append(QLatin1String("font"));
                            if (!data.lastBgColor.isEmpty())
                            {
                                colorString += spanColorOpenTag(data.lastBgColor);
                                data.openHtmlTags.append(QLatin1String("span"));
                            }
                        }
                    }
                    htmlText.replace(pos, colorMatch.length(), colorString);

                    pos += colorString.length() - 1;
                    linkOffset += colorString.length() -colorMatch.length();
                    break;
                }
                break;
            case '\x0f': //reset to default
                {
                    QString closeText;
                    while (!data.openHtmlTags.isEmpty())
                    {
                        closeText += QLatin1String("</") + data.openHtmlTags.takeLast() + QLatin1Char('>');
                    }
                    data.lastBgColor.clear();
                    data.lastFgColor.clear();
                    data.reverse = false;
                    htmlText.replace(pos, 1, closeText);

                    pos += closeText.length() - 1;
                    linkOffset += closeText.length() - 1;
                }
                break;
            case '\x16': //reverse
                {
                    // treat inverse as color and block it if colors are not allowed
                    if (!allowColors)
                    {
                        htmlText.remove(pos, 1);
                        pos -= 1;
                        linkOffset -= 1;
                        break;
                    }

                    QString colorString;

                    // close current color strings and open reverse tags
                    if (!data.reverse)
                    {
                        if (data.openHtmlTags.contains(QLatin1String("span")))
                        {
                            colorString += closeToTagString(&data, QLatin1String("span"));
                        }
                        if (data.openHtmlTags.contains(QLatin1String("font")))
                        {
                            colorString += closeToTagString(&data, QLatin1String("font"));
                        }
                        data.reverse = true;
                        colorString += fontColorOpenTag(Preferences::self()->color(Preferences::TextViewBackground).name());
                        data.openHtmlTags.append(QLatin1String("font"));
                        colorString += spanColorOpenTag(defaultColor);
                        data.openHtmlTags.append(QLatin1String("span"));
                    }
                    else
                    {
                        // if reset reverse, close reverse and set old fore- and
                        // back-groundcolor if set in data
                        colorString += closeToTagString(&data, QLatin1String("span"));
                        colorString += closeToTagString(&data, QLatin1String("font"));
                        data.reverse = false;
                        if (!data.lastFgColor.isEmpty())
                        {
                            colorString += fontColorOpenTag(data.lastFgColor);
                            data.openHtmlTags.append(QLatin1String("font"));
                            if (!data.lastBgColor.isEmpty())
                            {
                                colorString += spanColorOpenTag(data.lastBgColor);
                                data.openHtmlTags.append(QLatin1String("span"));
                            }
                        }
                    }
                    htmlText.replace(pos, 1, colorString);
                    pos += colorString.length() -1;
                    linkOffset += colorString.length() -1;
                }
                break;
            default:
                {
                    const QChar& dirChar = htmlText.at(pos);

                    // Replace pairs of spaces with "<space>&nbsp;" to preserve some semblance of text wrapping
                    //filteredLine.replace("  ", " \xA0");
                    // This used to work like above. But just for normal text like "test    test"
                    // It got replaced as "test \xA0 \xA0test" and QTextEdit showed 4 spaces.
                    // In case of color/italic/bold codes we don't necessary get a real pair of spaces
                    // just "test<html> <html> <html> <html> test" and QTextEdit shows it as 1 space.
                    // Now if we remember the last char, to ignore html tags, and check if current and last ones are spaces
                    // we replace the current one with \xA0 (a forced space) and get
                    // "test<html> <html>\xA0<html> <html>\xA0test", which QTextEdit correctly shows as 4 spaces.
                    //NOTE: replacing all spaces with forced spaces will break text wrapping
                    if (dirChar == ' ' &&
                        !lastChar.isNull() && lastChar == ' ')
                    {
                        htmlText[pos] = '\xA0';
                        lastChar = '\xA0';
                    }
                    else
                    {
                        lastChar = dirChar;
                    }

                    if (!(dirChar.isNumber() || dirChar.isSymbol() ||
                        dirChar.isSpace()  || dirChar.isPunct()  ||
                        dirChar.isMark()))
                    {
                        switch(dirChar.direction())
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
        }
    }

    if (direction)
    {
        // in case we found no right or left direction chars both
        // values are 0, but rtl_chars > ltr_chars is still false and QChar::DirL
        // is returned as default.
        if (rtl_chars > ltr_chars)
            *direction = QChar::DirR;
        else
            *direction = QChar::DirL;
    }

    if (parseURL)
    {
        // Change & to &amp; to prevent html entities to do strange things to the text
        htmlText.replace('&', "&amp;");
        htmlText.replace("\x0b", "&");
    }

    if (closeAllTags)
    {
        htmlText += closeTags(&data);
    }

    return htmlText;
}

int IRCView::defaultHtmlReplace(QString& htmlText, TextHtmlData* data, int pos, const QString& tag)
{
    QString replace;
    if (data->openHtmlTags.contains(tag))
    {
        replace = closeToTagString(data, tag);
    }
    else
    {
        data->openHtmlTags.append(tag);
        replace = QLatin1Char('<') + tag + QLatin1Char('>');
    }
    htmlText.replace(pos, 1, replace);
    return replace.length();
}

QString IRCView::closeToTagString(TextHtmlData* data, const QString& _tag)
{
    QString ret;
    QString tag;
    int i = data->openHtmlTags.count() - 1;
    //close all tags to _tag
    for ( ; i >= 0 ; --i)
    {
        tag = data->openHtmlTags.at(i);
        ret += QLatin1String("</") + tag + QLatin1Char('>');
        if (tag == _tag)
        {
            data->openHtmlTags.removeAt(i);
            break;
        }
    }

    // reopen relevant tags
    ret += openTags(data, i);

    return ret;
}

QString IRCView::openTags(TextHtmlData* data, int from)
{
    QString ret, tag;
    int i = from;
    for ( ;  i < data->openHtmlTags.count(); ++i)
    {
        tag = data->openHtmlTags.at(i);
        if (tag == QLatin1String("font"))
        {
            if (data->reverse)
            {
                ret += fontColorOpenTag(Preferences::self()->color(Preferences::TextViewBackground).name());
            }
            else
            {
                ret += fontColorOpenTag(data->lastFgColor);
            }
        }
        else if (tag == QLatin1String("span"))
        {
            if (data->reverse)
            {
                ret += spanColorOpenTag(data->defaultColor);
            }
            else
            {
                ret += spanColorOpenTag(data->lastBgColor);
            }
        }
        else
        {
            ret += QLatin1Char('<') + tag + QLatin1Char('>');
        }
    }
    return ret;
}

QString IRCView::closeTags(TextHtmlData* data)
{
    QString ret;
    QListIterator< QString > i(data->openHtmlTags);
    i.toBack();
    while (i.hasPrevious())
    {
        ret += QLatin1String("</") + i.previous() + QLatin1Char('>');
    }
    return ret;
}

QString IRCView::fontColorOpenTag(const QString& fgColor)
{
    return QLatin1String("<font color=\"") + fgColor + QLatin1String("\">");
}

QString IRCView::spanColorOpenTag(const QString& bgColor)
{
    return QLatin1String("<span style=\"background-color:") + bgColor + QLatin1String("\">");
}

QString IRCView::removeDuplicateCodes(const QString& codes, TextHtmlData* data)
{
    int pos = 0;
    QString ret;
    while (pos < codes.length())
    {
        switch (codes.at(pos).toAscii())
        {
            case '\x02': //bold
                defaultRemoveDuplicateHandling(data, QLatin1String("b"));
                ++pos;
                break;
            case '\x1d': //italic
                defaultRemoveDuplicateHandling(data, QLatin1String("i"));
                ++pos;
                break;
            case '\x15': //mirc underline
            case '\x1f': //kvirc underline
                defaultRemoveDuplicateHandling(data, QLatin1String("u"));
                ++pos;
                break;
            case '\x13': //strikethru
                defaultRemoveDuplicateHandling(data, QLatin1String("s"));
                ++pos;
                break;
            case '\x0f': //reset to default
                data->openHtmlTags.clear();
                data->lastBgColor.clear();
                data->lastFgColor.clear();
                data->reverse = false;
                ++pos;
                break;

            case '\x16': //reverse
                if (data->reverse)
                {
                    data->openHtmlTags.removeOne(QLatin1String("span"));
                    data->openHtmlTags.removeOne(QLatin1String("font"));
                    data->reverse = false;
                    if (!data->lastFgColor.isEmpty())
                    {
                        data->openHtmlTags.append(QLatin1String("font"));
                        if (!data->lastBgColor.isEmpty())
                        {
                            data->openHtmlTags.append(QLatin1String("span"));
                        }
                    }
                }
                else
                {
                    data->openHtmlTags.removeOne(QLatin1String("span"));
                    data->openHtmlTags.removeOne(QLatin1String("font"));
                    data->reverse = true;
                    data->openHtmlTags.append(QLatin1String("font"));
                    data->openHtmlTags.append(QLatin1String("span"));
                }
                ++pos;
                break;
            case '\x03': //color
                {
                    QString fgColor, bgColor;
                    bool fgOK = true, bgOK = true;
                    QString colorMatch(getColors(codes, pos, fgColor, bgColor, &fgOK, &bgOK));

                    // check for color reset conditions
                    //TODO check if \x11 \017 is really valid here
                    if (colorMatch == QLatin1String("\x03") || colorMatch == QLatin1String("\x11") ||
                        (fgColor.isEmpty() && bgColor.isEmpty()) || (!fgOK && !bgOK))
                    {
                        if (!data->lastBgColor.isEmpty())
                        {
                            data->lastBgColor.clear();
                            data->openHtmlTags.removeOne(QLatin1String("span"));
                        }
                        if (!data->lastFgColor.isEmpty())
                        {
                            data->lastFgColor.clear();
                            data->openHtmlTags.removeOne(QLatin1String("font"));
                        }
                        pos += colorMatch.length();
                        break;
                    }

                    if (!fgOK)
                    {
                        fgColor = data->defaultColor;
                    }
                    if (!bgOK)
                    {
                        bgColor = fontColorOpenTag(Preferences::self()->color(Preferences::TextViewBackground).name());
                    }

                    if (!fgColor.isEmpty())
                    {
                        data->lastFgColor = fgColor;
                        data->openHtmlTags.append(QLatin1String("font"));
                        if (!bgColor.isEmpty())
                        {
                            data->lastBgColor = bgColor;
                            data->openHtmlTags.append(QLatin1String("span"));
                        }
                    }

                    pos += colorMatch.length();
                }
                break;
            default:
//                 kDebug() << "unsupported duplicate code:" << QString::number(codes.at(pos).toAscii(), 16);
                ret += codes.at(pos);
                ++pos;
        }
    }

    return ret;
}

void IRCView::defaultRemoveDuplicateHandling(TextHtmlData* data, const QString& tag)
{
    if (data->openHtmlTags.contains(tag))
    {
        data->openHtmlTags.removeOne(tag);
    }
    else
    {
        data->openHtmlTags.append(tag);
    }
}

void IRCView::adjustUrlRanges(QList< QPair<int, int> >& urlRanges, const QStringList& fixedUrls,  QString& richtext, const QString& strippedText)
{
    Q_UNUSED(fixedUrls);
    QRegExp ircRichtextRegExp(colorRegExp);
    int start = 0, j;
    int i = 0;
    QString url;
    int htmlTextLength = richtext.length(), urlCount = urlRanges.count();
    for (int x = 0; x < urlCount; ++x)
    {
        if (x == 0)
            i = urlRanges.first().first;

        j = 0;
        const QPair<int, int>& range = urlRanges.at(x);
        url = strippedText.mid(range.first, range.second);
        for ( ; i < htmlTextLength; ++i)
        {
            if (richtext.at(i) == url.at(j))
            {
                if (j == 0)
                    start = i;
                ++j;
                if (j == url.length())
                {
                    urlRanges[x].first = start;
                    urlRanges[x].second = i - start + 1;
                    break;
                }
            }
            else if (ircRichtextRegExp.exactMatch(richtext.at(i)))
            {
                ircRichtextRegExp.indexIn(richtext, i);
                i += ircRichtextRegExp.matchedLength() - 1;
            }
            else
            {
                j = 0;
            }
        }
    }
}

QString IRCView::getColors(const QString& text, int start, QString& _fgColor, QString& _bgColor, bool* fgValueOK, bool* bgValueOK)
{
    QRegExp ircColorRegExp("(\003([0-9][0-9]|[0-9]|)(,([0-9][0-9]|[0-9]|)|,|)|\017)");
    if (ircColorRegExp.indexIn(text,start) == -1)
        return QString();

    QString ret(ircColorRegExp.cap(0));

    QString fgColor(ircColorRegExp.cap(2)), bgColor(ircColorRegExp.cap(4));
    if (!fgColor.isEmpty())
    {
        int foregroundColor = fgColor.toInt();
        if (foregroundColor > -1 && foregroundColor < 16)
        {
            _fgColor = Preferences::self()->ircColorCode(foregroundColor).name();
            if (fgValueOK)
                *fgValueOK = true;
        }
        else
        {
            if (fgValueOK)
                *fgValueOK = false;
        }
    }
    else
    {
        if (fgValueOK)
            *fgValueOK = true;
    }

    if (!bgColor.isEmpty())
    {
        int backgroundColor = bgColor.toInt();
        if (backgroundColor > -1 && backgroundColor < 16)
        {
            _bgColor = Preferences::self()->ircColorCode(backgroundColor).name();
            if (bgValueOK)
                *bgValueOK = true;
        }
        else
        {
            if (bgValueOK)
                *bgValueOK = false;
        }
    }
    else
    {
        if (bgValueOK)
            *bgValueOK = true;
    }

    return ret;
}

void IRCView::resizeEvent(QResizeEvent *event)
{
    ScrollBarPin b(verticalScrollBar());
    KTextBrowser::resizeEvent(event);
}

void IRCView::mouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressedOnUrl && (m_mousePressPosition - ev->pos()).manhattanLength() > KApplication::startDragDistance())
    {
        m_mousePressedOnUrl = false;

        QTextCursor textCursor = this->textCursor();
        textCursor.clearSelection();
        setTextCursor(textCursor);


        QPointer<QDrag> drag = new QDrag(this);
        QMimeData* mimeData = new QMimeData;

        KUrl url(m_dragUrl);
        url.populateMimeData(mimeData);

        drag->setMimeData(mimeData);

        QPixmap pixmap = KIO::pixmapForUrl(url, 0, KIconLoader::Desktop, KIconLoader::SizeMedium);
        drag->setPixmap(pixmap);

        drag->exec();

        return;
    }
    else
    {
        // Store the url here instead of in highlightedSlot as the link given there is decoded.
        m_urlToCopy = anchorAt(ev->pos());
    }

    KTextBrowser::mouseMoveEvent(ev);
}

void IRCView::mousePressEvent(QMouseEvent* ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_dragUrl = anchorAt(ev->pos());

        if (!m_dragUrl.isEmpty() && Konversation::isUrl(m_dragUrl))
        {
            m_mousePressedOnUrl = true;
            m_mousePressPosition = ev->pos();
        }
    }

    KTextBrowser::mousePressEvent(ev);
}

void IRCView::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_mousePressedOnUrl = false;
    }
    else if (ev->button() == Qt::MidButton)
    {
        if (m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
        {
            openLink(QUrl (m_urlToCopy));
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

void IRCView::keyPressEvent(QKeyEvent* ev)
{
    const int key = ev->key() | ev->modifiers();

    if (KStandardShortcut::paste().contains(key))
    {
        emit textPasted(false);
        ev->accept();
        return;
    }

    KTextBrowser::keyPressEvent(ev);
}

void IRCView::anchorClicked(const QUrl& url)
{
    openLink(url);
}

void IRCView::openLink(const QUrl& url)
{
    QString link(url.toString());
    // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
    link = link.replace (' ', "%20");

    if (!link.isEmpty() && !link.startsWith('#'))
        Application::openUrl(url.toEncoded());
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

void IRCView::highlightedSlot(const QString& /*_link*/)
{
    QString link = m_urlToCopy;
    // HACK Replace " " with %20 for channelnames, NOTE there can't be 2 channelnames in one link
    link = link.replace (' ', "%20");

    //we just saw this a second ago.  no need to reemit.
    if (link == m_lastStatusText && !link.isEmpty())
        return;

    if (link.isEmpty())
    {
        if (!m_lastStatusText.isEmpty())
        {
            emit clearStatusBarTempText();
            m_lastStatusText.clear();
        }
    }
    else
    {
        m_lastStatusText = link;
    }

    if (!link.startsWith(QLatin1Char('#')))
    {
        m_isOnNick = false;
        m_isOnChannel = false;

        if (!link.isEmpty()) {
            //link therefore != m_lastStatusText  so emit with this new text
            emit setStatusBarTempText(link);
        }

        if (link.isEmpty() && m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            setContextMenuOptions(IrcContextMenus::ShowLinkActions, false);
        else if (!link.isEmpty() && !m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            setContextMenuOptions(IrcContextMenus::ShowLinkActions, true);
    }
    else if (link.startsWith(QLatin1Char('#')) && !link.startsWith(QLatin1String("##")))
    {
        m_currentNick = link.mid(1);

        m_isOnNick = true;

        emit setStatusBarTempText(i18n("Open a query with %1", m_currentNick));
    }
    else
    {
        // link.startsWith("##")
        m_currentChannel = link.mid(1);

        m_isOnChannel = true;

        emit setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
    }
}

void IRCView::setContextMenuOptions(IrcContextMenus::MenuOptions options, bool on)
{
    if (on)
        m_contextMenuOptions |= options;
    else
        m_contextMenuOptions &= ~options;
}

void IRCView::contextMenuEvent(QContextMenuEvent* ev)
{
    // Consider the following scenario: (1) context menu opened, (2) mouse
    // pointer moved, (3) mouse button clicked to dismiss menu, (4) mouse
    // button clicked to reopen context menu. In this scenario, if there is
    // no mouse movement between steps (3) and (4), highlighted() is never
    // emitted, and the data we use here to display the correct context menu
    // is outdated. Thus what we're going to do here is post a fake mouse
    // move event using the context menu event coordinate, forcing an update
    // just before we display the context menu.
    QMouseEvent fake(QEvent::MouseMove, ev->pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    mouseMoveEvent(&fake);

    if (m_isOnChannel && m_server)
    {
        IrcContextMenus::channelMenu(ev->globalPos(), m_server, m_currentChannel);

        m_isOnChannel = false;

        return;
    }

    if (m_isOnNick && m_server)
    {
        IrcContextMenus::nickMenu(ev->globalPos(), m_contextMenuOptions, m_server, QStringList() << m_currentNick,
            m_contextMenuOptions.testFlag(IrcContextMenus::ShowChannelActions) ? m_chatWin->getName() : QString());

        m_currentNick.clear();

        m_isOnNick = false;

        return;
    }

    int contextMenuActionId = IrcContextMenus::textMenu(ev->globalPos(), m_contextMenuOptions, m_server,
        textCursor().selectedText(), m_urlToCopy,
        m_contextMenuOptions.testFlag(IrcContextMenus::ShowNickActions) ? m_chatWin->getName() : QString());

    switch (contextMenuActionId)
    {
        case -1:
            break;
        case IrcContextMenus::TextCopy:
            copy();
            break;
        case IrcContextMenus::TextSelectAll:
            selectAll();
            break;
        default:
            if (m_contextMenuOptions.testFlag(IrcContextMenus::ShowNickActions))
            {
                IrcContextMenus::processNickAction(contextMenuActionId, m_server, QStringList() << m_chatWin->getName(),
                    m_contextMenuOptions.testFlag(IrcContextMenus::ShowChannelActions) ? m_chatWin->getName() : QString());
            }
            break;
    }
}

// For more information about these RTFM
// http://www.unicode.org/reports/tr9/
// http://www.w3.org/TR/unicode-xml/
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
