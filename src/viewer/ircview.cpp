// -*- mode: c++; c-file-style: "bsd"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-

/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005-2016 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2006-2010 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2004-2011 Eli Mackenzie <argonel@gmail.com>
*/

#include "ircview.h"

#include "channel.h"
#include "dcc/chatcontainer.h"
#include "application.h"
#include "highlight.h"
#include "sound.h"
#include "notificationhandler.h"
#include "konversation_log.h"

#include <KStandardShortcut>
#include <KUrlMimeData>
#include <QLocale>

#include <QDrag>
#include <QScrollBar>
#include <QTextBlock>
#include <QPainter>
#include <QTextDocumentFragment>
#include <QMimeData>

using namespace Konversation;

class ScrollBarPin
{
        QPointer<QScrollBar> m_bar;

        Q_DISABLE_COPY(ScrollBarPin)

    public:
        ScrollBarPin(QScrollBar *scrollBar) : m_bar(scrollBar)
        {
            if (m_bar)
                m_bar = m_bar->value() == m_bar->maximum()? m_bar : nullptr;
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

    Q_DISABLE_COPY(SelectionPin)

    public:
        SelectionPin(IRCView *doc) : pos(0), anc(0), d(doc)
        {
            if (d->textCursor().hasSelection())
            {
                int end = d->document()->rootFrame()->lastPosition();

                //WARNING if selection pins don't work in some build environments, we need to keep the result
                d->document()->lastBlock();

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


IRCView::IRCView(QWidget* parent) : QTextBrowser(parent), m_rememberLine(nullptr), m_lastMarkerLine(nullptr),
    m_rememberLineDirtyBit(false), markerFormatObject(this), m_prevTimestamp(QDateTime::currentDateTime())
{
    m_mousePressedOnUrl = false;
    m_isOnNick = false;
    m_isOnChannel = false;
    m_chatWin = nullptr;
    m_server = nullptr;
    m_fontSizeDelta = 0;
    m_showDate = false;

    setAcceptDrops(false);

    // Marker lines
    connect(document(), &QTextDocument::contentsChange, this, &IRCView::cullMarkedLine);

    //This assert is here because a bad build environment can cause this to fail. There is a note
    // in the Qt source that indicates an error should be output, but there is no such output.
    QTextObjectInterface *iface = qobject_cast<QTextObjectInterface *>(&markerFormatObject);
    if (!iface)
    {
        Q_ASSERT(iface);
    }

    document()->documentLayout()->registerHandler(IRCView::MarkerLine, &markerFormatObject);
    document()->documentLayout()->registerHandler(IRCView::RememberLine, &markerFormatObject);
    document()->documentLayout()->registerHandler(IRCView::DateLine, &markerFormatObject);


    connect(this, &IRCView::anchorClicked, this, &IRCView::handleAnchorClicked);
    connect(this, QOverload<const QUrl&>::of(&IRCView::highlighted), this, &IRCView::highlightedSlot);
    setOpenLinks(false);
    setUndoRedoEnabled(false);
    document()->setDefaultStyleSheet(QStringLiteral("a.nick:link {text-decoration: none}"));
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setFocusPolicy(Qt::ClickFocus);
    setReadOnly(true);
    viewport()->setCursor(Qt::ArrowCursor);
    setTextInteractionFlags(Qt::TextBrowserInteraction);
    viewport()->setMouseTracking(true);

    //HACK to workaround an issue with the QTextDocument
    //doing a relayout/scrollbar over and over resulting in 100%
    //proc usage. See bug 215256
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContextMenuOptions(IrcContextMenus::ShowTitle | IrcContextMenus::ShowFindAction, true);
}

IRCView::~IRCView()
{
}

void IRCView::increaseFontSize()
{
    QFont newFont(Preferences::self()->customTextFont()
        ? Preferences::self()->textFont() : QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    ++m_fontSizeDelta;
    newFont.setPointSize(newFont.pointSize() + m_fontSizeDelta);

    setFont(newFont);
}

void IRCView::decreaseFontSize()
{
    QFont newFont(Preferences::self()->customTextFont()
        ? Preferences::self()->textFont() : QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    --m_fontSizeDelta;
    newFont.setPointSize(newFont.pointSize() + m_fontSizeDelta);

    setFont(newFont);
}

void IRCView::resetFontSize()
{
    QFont newFont(Preferences::self()->customTextFont()
        ? Preferences::self()->textFont() : QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    m_fontSizeDelta = 0;

    setFont(newFont);
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
    Q_EMIT doSearch();
}

void IRCView::findNextText()
{
    Q_EMIT doSearchNext();
}

void IRCView::findPreviousText()
{
    Q_EMIT doSearchPrevious();
}

bool IRCView::search(const QString& pattern, QTextDocument::FindFlags flags, bool fromCursor)
{
    if (pattern.isEmpty())
        return true;

    m_pattern       = pattern;
    m_searchFlags = flags;

    if (!fromCursor)
        moveCursor(QTextCursor::End);
    else
        moveCursor(QTextCursor::StartOfWord); // Do this to that if possible the same position is kept when changing search options

    return searchNext();
}

bool IRCView::searchNext(bool reversed)
{
    QTextDocument::FindFlags flags = m_searchFlags;

    if(!reversed)
        flags |= QTextDocument::FindBackward;

    return find(m_pattern, flags);
}

class IrcViewMimeData : public QMimeData
{
    Q_OBJECT

public:
    IrcViewMimeData(const QTextDocumentFragment& _fragment): fragment(_fragment) {}
    QStringList formats() const override;

protected:
    QVariant retrieveData(const QString &mimeType, QMetaType type) const override;

private:
    mutable QTextDocumentFragment fragment;
};

QStringList IrcViewMimeData::formats() const
{
    if (!fragment.isEmpty())
        return QStringList{ QStringLiteral("text/plain") };
    else
        return QMimeData::formats();
}
QVariant IrcViewMimeData::retrieveData(const QString &mimeType, QMetaType type) const
{
    if (!fragment.isEmpty())
    {
        auto *that = const_cast<IrcViewMimeData *>(this);

        //Copy the text, skipping any QChar::ObjectReplacementCharacter
        const QRegularExpression needle(QStringLiteral("\\x{FFFC}\\n?"));

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
        Q_EMIT urlsDropped(KUrlMimeData::urlsFromMimeData(e->mimeData(), KUrlMimeData::PreferLocalUrls));
}

// Marker lines

#define _S(x) #x << (x)

QDebug operator<<(QDebug dbg, QTextBlockUserData *bd);
QDebug operator<<(QDebug d, QTextFrame* feed);
QDebug operator<<(QDebug d, QTextDocument* document);
QDebug operator<<(QDebug d, const QTextBlock &b);

// This object gets stuffed into the userData field of a text block.
// Qt does not give us a way to track blocks, so we have to
// rely on the destructor of this object to notify us that a
// block we care about was removed from the document. This does not
// prevent the first block bug from deleting the wrong block's data,
// however that should not result in a crash.
struct Burr: public QTextBlockUserData
{
    Burr(IRCView* o, Burr* prev, const QTextBlock &b, int objFormat)
        : m_block(b), m_format(objFormat), m_prev(prev), m_next(nullptr),
        m_owner(o)
    {
        if (m_prev)
            m_prev->m_next = this;
    }

    ~Burr() override
    {
        m_owner->blockDeleted(this);
        unlink();
    }

    void unlink()
    {
        if (m_prev)
            m_prev->m_next = m_next;
        if (m_next)
            m_next->m_prev = m_prev;
    }

    QTextBlock m_block;
    int m_format;
    Burr* m_prev, *m_next;
    IRCView* m_owner;

    Q_DISABLE_COPY(Burr)
};

void IrcViewMarkerLine::drawObject(QPainter *painter, const QRectF &r, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(format)

    QTextBlock block=doc->findBlock(posInDocument);
    QPen pen;
    Burr* b = dynamic_cast<Burr*>(block.userData());
    Q_ASSERT(b); // remember kids, only YOU can makes this document support two user data types
    switch (b->m_format)
    {
        case IRCView::BlockIsMarker:
            pen.setColor(Preferences::self()->color(Preferences::ActionMessage));
            break;

        case IRCView::BlockIsRemember:
            pen.setColor(Preferences::self()->color(Preferences::CommandMessage));
            // pen.setStyle(Qt::DashDotDotLine);
            break;

        case IRCView::BlockIsDateMarker:
            pen.setColor(Preferences::self()->color(Preferences::Time));
            pen.setStyle(Qt::DashLine);
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
    Q_UNUSED(posInDocument)
    Q_UNUSED(format)

    QTextFrameFormat f=doc->rootFrame()->frameFormat();
    qreal width = doc->pageSize().width()-(f.leftMargin()+f.rightMargin());
    return QSizeF(width, 6); // FIXME this is a hardcoded value...
}

QTextCharFormat IRCView::getFormat(ObjectFormats x)
{
    QTextCharFormat f;
    f.setObjectType(x);
    return f;
}

void IRCView::blockDeleted(Burr* b) //slot
{
    Q_ASSERT(b); // this method only to be called from a ~Burr();

    //tracking only the tail
    if (b == m_lastMarkerLine)
        m_lastMarkerLine = b->m_prev;

    if (b == m_rememberLine)
        m_rememberLine = nullptr;
}

void IRCView::cullMarkedLine(int, int, int) //slot
{
    QTextBlock prime = document()->firstBlock();

    if (prime.length() == 1 && document()->blockCount() == 1) //the entire document was wiped. was a signal such a burden? apparently..
        wipeLineParagraphs();
}

void IRCView::insertMarkerLine() //slot
{
    //if the last line is already a marker of any kind, skip out
    if (lastBlockIsLine(BlockIsMarker))
        return;

    //the code used to preserve the dirty bit status, but that was never affected by appendLine...
    //maybe i missed something
    appendLine(IRCView::MarkerLine);
}

void IRCView::insertRememberLine() //slot
{
    m_rememberLineDirtyBit = true; // means we're going to append a remember line if some text gets inserted

    if (!Preferences::self()->automaticRememberLineOnlyOnTextChange())
    {
        appendRememberLine();
    }
}

void IRCView::cancelRememberLine() //slot
{
    m_rememberLineDirtyBit = false;
}

bool IRCView::lastBlockIsLine(int select)
{
    Burr *b = dynamic_cast<Burr*>(document()->lastBlock().userData());

    int state = -1;

    if (b)
        state = b->m_format;

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

    if (m_rememberLine)
    {
        QTextBlock rem = m_rememberLine->m_block;
        voidLineBlock(rem);
        if (m_rememberLine != nullptr)
        {
            // this probably means we had a block containing only 0x2029, so Scribe merged the userData/userState into the next
            m_rememberLine = nullptr;
        }
    }

    m_rememberLine = appendLine(IRCView::RememberLine);
}

void IRCView::voidLineBlock(const QTextBlock &rem)
{
    QTextCursor c(rem);

    c.select(QTextCursor::BlockUnderCursor);
    c.removeSelectedText();
}

void IRCView::clearLines()
{
    while (hasLines())
    {
        //IRCView::blockDeleted takes care of the pointers
        voidLineBlock(m_lastMarkerLine->m_block);
    };
}

void IRCView::wipeLineParagraphs()
{
    m_rememberLine = m_lastMarkerLine = nullptr;
}

bool IRCView::hasLines()
{
    return m_lastMarkerLine != nullptr;
}

Burr* IRCView::appendLine(IRCView::ObjectFormats type)
{
    ScrollBarPin barpin(verticalScrollBar());
    SelectionPin selpin(this);

    QTextCursor cursor(document());
    cursor.movePosition(QTextCursor::End);

    if (cursor.block().length() > 1) // this will be a 0x2029
        cursor.insertBlock();
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), getFormat(type));

    QTextBlock block = cursor.block();
    Burr *prevBurr = m_lastMarkerLine;
    if(type == DateLine)
        prevBurr = nullptr;
    Burr *b = new Burr(this, prevBurr, block, objectFormatToBlockState(type));
    block.setUserData(b);

    if(type != DateLine)
        m_lastMarkerLine = b;

    //TODO figure out what this is for
    cursor.setPosition(block.position());

    return b;
}

IRCView::BlockStates IRCView::objectFormatToBlockState(ObjectFormats format)
{
    BlockStates state;

    switch(format)
    {
    case MarkerLine:
        state = BlockIsMarker;
        break;
    case RememberLine:
        state = BlockIsRemember;
        break;
    case DateLine:
        state = BlockIsDateMarker;
        break;
    default:
        state = None;
        break;
    }

    return state;
}

// Other stuff

void IRCView::updateAppearance()
{
    QFont newFont(Preferences::self()->customTextFont()
        ? Preferences::self()->textFont() : QFontDatabase::systemFont(QFontDatabase::GeneralFont));

    newFont.setPointSize(newFont.pointSize() + m_fontSizeDelta);

    setFont(newFont);

    setVerticalScrollBarPolicy(Preferences::self()->showIRCViewScrollBar() ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAlwaysOff);

    if (Preferences::self()->showBackgroundImage())
    {
        QUrl url = Preferences::self()->backgroundImage();

        if (url.isValid())
        {
            viewport()->setStyleSheet(QLatin1String("QWidget { background-image: url(") + url.path() + QLatin1String("); background-attachment:fixed; }"));

            return;
        }
    }

    if (!viewport()->styleSheet().isEmpty())
        viewport()->setStyleSheet(QString());

    QPalette p;
    p.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
    viewport()->setPalette(p);
}

bool IRCView::dateRtlDirection()
{
    // Keep format synced with IRCView::timeStamp
    return QLocale().toString(QDate::currentDate(), QLocale::ShortFormat).isRightToLeft();
}

// To minimize the use of bidi marks, for cases below, some bidi marks are
// needed.
// * left aligned lines in LTR locales, and
// * right aligned lines in RTL locales
//
// First, check if the direction of the message is the same as the
// direction of the timestamp, if not, then add a mark depending on
// message's direction, so that timestamp don't be first strong character.
//
// If we have a LTR label, and the message is right-aligned, we prepend
// it with LRM to look correct (check nickname case below), and then append
// it with LRM also and then a RLM to preserve the direction of the
// right-aligned line.
//
// Later, if the message is RTL, nicknames like "_nick]" will appear
// as "[nick_".
// First, add a LRM mark to make underscore on the left, next add the
// nickname, and then another LRM mark. Since we use RTL/LTR count, the
// message may start with a LTR word, and appear to the right of the
// nickname. That's why we add a RLM mark before the nick to force it
// appearing on left.
QString IRCView::formatFinalLine(bool rtl, const QString &lineColor, const QString &label, const QString &nickLine, const QString &nickStar, const QString &text)
{
    // Nick correctly displayed: <_nick]>
    QString line;
    // It's right-aligned under LTR locale, or left-aligned under RTL locale
    if (!rtl == dateRtlDirection())
        line += (rtl ? RLM : LRM);
    line += QLatin1String("<font color=\"") + lineColor + QLatin1String("\">%1");
    if (!label.isEmpty()) { // Label correctly displayed: [_label.]
        if (rtl) {
          line += LRM; // [.label_] -> [._label]
          line += QLatin1String(" <font color=\"") + lineColor + QLatin1String("\"><b>[</b>%4<b>]</b></font>");
        }
        else {
          line += QLatin1String("<font color=\"") + lineColor + QLatin1String("\"><b>[</b>%4<b>]</b></font> ");
        }
        if (!label.isRightToLeft() == rtl)
            line += LRM + RLM; // [._label] -> [_label.]
    }
    if (!nickStar.isEmpty()) // Used for [timeStamp] * nick action
        line += nickStar;
    if (rtl) line += LRM; // <[nick_> -> <[_nick]>
    line += nickLine;
    if (rtl)
    {
        line += LRM; // <[_nick]> -> <_nick]>
        // It might start with an English word, but it's RTL because of counting
        if (!text.isEmpty() && !text.isRightToLeft())
            line += RLM; // ARABIC_TEXT <_nick]> Hi -> ARABIC_TEXT Hi <_nick]>
    }
    if (text.isEmpty()) line += QLatin1String("</font>");
    else line += QLatin1String(" %3</font>");

    return line;
}

// Data insertion

void IRCView::append(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags, const QString& label)
{
    QString channelColor = Preferences::self()->color(Preferences::ChannelMessage).name();

    m_tabNotification = Konversation::tnfNormal;

    QString nickLine = createNickLine(nick, channelColor);

    QChar::Direction dir;
    QString text(filter(message, channelColor, nick, true, true, false, &dir));
    QString line;
    bool rtl = (dir == QChar::DirR);

    // Normal chat lines
    // [timestamp] <nickname> chat message
    line = formatFinalLine(rtl, channelColor, label, nickLine, QString(), text);
    line = line.arg(timeStamp(messageTags, rtl), nick, text);

    if (!label.isEmpty())
    {
        line = line.arg(label);
    }

    Q_EMIT textToLog(QStringLiteral("<%1>\t%2").arg(nick, message));

    doAppend(line, rtl);
}

void IRCView::appendRaw(const QString& message, bool self)
{
    QColor color = self ? Preferences::self()->color(Preferences::ChannelMessage)
        : Preferences::self()->color(Preferences::ServerMessage);
    m_tabNotification = Konversation::tnfNone;

    // Raw log is always left-aligned
    // [timestamp] << server line
    // If the timedate string is RTL, prepend a LTR mark to force the direction
    // to be LTR, as the datetime string is already returned as it's for a
    // left-aligned line.

    QString line;
    if (dateRtlDirection()) line += LRM;
    line += (timeStamp(QHash<QString, QString>(), false) + QLatin1String(" <font color=\"") + color.name() + QLatin1String("\">") + message + QLatin1String("</font>"));

    doAppend(line, false, self);
}

void IRCView::appendLog(const QString & message)
{
    QColor channelColor = Preferences::self()->color(Preferences::ChannelMessage);
    m_tabNotification = Konversation::tnfNone;

    // Log view is plain log files.
    // Direction will be depending on the logfile line direction.

    QString line(QLatin1String("<font color=\"") + channelColor.name() + QLatin1String("\">") + message + QLatin1String("</font>"));

    doRawAppend(line, message.isRightToLeft());
}

void IRCView::appendQuery(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags, bool inChannel)
{
    QString queryColor=Preferences::self()->color(Preferences::QueryMessage).name();

    m_tabNotification = Konversation::tnfPrivate;

    QString nickLine = createNickLine(nick, queryColor, true, inChannel);

    QString line;
    QChar::Direction dir;
    QString text(filter(message, queryColor, nick, true, true, false, &dir));
    bool rtl = (dir == QChar::DirR);

    // Private chat lines
    // [timestamp] <nickname> chat message
    line = formatFinalLine(rtl, queryColor, QString(), nickLine, QString(), text);
    line = line.arg(timeStamp(messageTags, rtl), nick, text);

    if (inChannel) {
        Q_EMIT textToLog(QStringLiteral("<-> %1>\t%2").arg(nick, message));
    } else {
        Q_EMIT textToLog(QStringLiteral("<%1>\t%2").arg(nick, message));
    }

    doAppend(line, rtl);
}

void IRCView::appendChannelAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags)
{
    m_tabNotification = Konversation::tnfNormal;
    appendAction(nick, message, messageTags);
}

void IRCView::appendQueryAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags)
{
    m_tabNotification = Konversation::tnfPrivate;
    appendAction(nick, message, messageTags);
}

void IRCView::appendAction(const QString& nick, const QString& message, const QHash<QString, QString> &messageTags)
{
    QString actionColor = Preferences::self()->color(Preferences::ActionMessage).name();

    QString line;

    QString nickLine = createNickLine(nick, actionColor, false);

    if (message.isEmpty())
    {
        // No text to check direction. Better to check last line, if it's RTL,
        // treat it as that.
        QTextCursor formatCursor(document()->lastBlock());
        bool rtl = (formatCursor.blockFormat().alignment().testFlag(Qt::AlignRight));

        line = formatFinalLine(rtl, actionColor, QString(), nickLine, QStringLiteral(" * "), QString());
        line = line.arg(timeStamp(messageTags, rtl), nick);

        Q_EMIT textToLog(QStringLiteral("\t * %1").arg(nick));

        doAppend(line, rtl);
    }
    else
    {
        QChar::Direction dir;
        QString text(filter(message, actionColor, nick, true,true, false, &dir));
        bool rtl = (dir == QChar::DirR);

        // Actions line
        // [timestamp] * nickname action
        line = formatFinalLine(rtl, actionColor, QString(), nickLine, QStringLiteral(" * "), text);
        line = line.arg(timeStamp(messageTags, rtl), nick, text);

        Q_EMIT textToLog(QStringLiteral("\t * %1 %2").arg(nick, message));

        doAppend(line, rtl);
    }
}

void IRCView::appendServerMessage(const QString& type, const QString& message, const QHash<QString, QString> &messageTags, bool parseURL)
{
    QString serverColor = Preferences::self()->color(Preferences::ServerMessage).name();
    m_tabNotification = Konversation::tnfControl;

    // Fixed width font option for MOTD
    QString fixed;
    if(Preferences::self()->fixedMOTD() && !m_fontDataBase.isFixedPitch(font().family()))
    {
        if(type == i18n("MOTD"))
            fixed = QLatin1String(" face=\"") + QFontDatabase::systemFont(QFontDatabase::FixedFont).family() + QLatin1Char('\"');
    }

    QString line;
    QChar::Direction dir;
    QString text(filter(message, serverColor, QString(), true, parseURL, false, &dir));
    // Server text may be translated strings. It's not user input: treat with first strong.
    bool rtl = text.isRightToLeft();

    // It's right-aligned under LTR locale, or left-aligned under RTL locale
    if (!rtl == dateRtlDirection())
        line += (rtl ? RLM : LRM);
    line += QLatin1String("<font color=\"") + serverColor + QLatin1Char('\"') + fixed + QLatin1String(">%1 <b>[</b>%2<b>]</b>");
    if (!rtl == type.isRightToLeft())
        line += (rtl ? RLM : LRM); // [50 [ARABIC_TEXT users -> [ARABIC_TEXT] 50 users
    line += QLatin1String(" %3</font>");
    line = line.arg(timeStamp(messageTags, rtl), type, text);

    Q_EMIT textToLog(QStringLiteral("%1\t%2").arg(type, message));

    doAppend(line, rtl);
}

void IRCView::appendCommandMessage(const QString& type, const QString& message, const QHash<QString, QString> &messageTags, bool parseURL, bool self)
{
    QString commandColor = Preferences::self()->color(Preferences::CommandMessage).name();
    QString prefix=QStringLiteral("***");
    m_tabNotification = Konversation::tnfControl;

    if(type == i18nc("Message type", "Join"))
    {
        prefix=QStringLiteral("-->");
        parseURL=false;
    }
    else if(type == i18nc("Message type", "Part") || type == i18nc("Message type", "Quit"))
    {
        prefix=QStringLiteral("<--");
    }

    prefix=prefix.toHtmlEscaped();

    QString line;
    QChar::Direction dir;
    QString text(filter(message, commandColor, QString(), true, parseURL, self, &dir));
    // Commands are translated and contain LTR IP addresses. Treat with first strong.
    bool rtl = text.isRightToLeft();

    // It's right-aligned under LTR locale, or left-aligned under RTL locale
    if (!rtl == dateRtlDirection())
        line += (rtl ? RLM : LRM);
    line += QLatin1String("<font color=\"") + commandColor + QLatin1String("\">%1 %2 %3</font>");
    line = line.arg(timeStamp(messageTags, rtl), prefix, text);

    Q_EMIT textToLog(QStringLiteral("%1\t%2").arg(type, message));

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
    const int eot = nick.lastIndexOf(QLatin1Char(' '));
    time = nick.left(eot);
    nick = nick.mid(eot+1);

    if (!nick.isEmpty() && !nick.startsWith(QLatin1Char('<')) && !nick.startsWith(QLatin1Char('*'))) {
        nick = QLatin1Char('|') + nick + QLatin1Char('|');
    }

    // Nicks are in "<nick>" format so replace the "<>"
    nick.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    nick.replace(QLatin1Char('>'), QLatin1String("&gt;"));

    QString line;
    QChar::Direction dir;
    QString text(filter(message, backlogColor, QString(), false, false, false, &dir));
    bool rtl = nick.startsWith(QLatin1Char('|')) ? text.isRightToLeft() : (dir == QChar::DirR);

    // It's right-aligned under LTR locale, or left-aligned under RTL locale
    if (!rtl == time.isRightToLeft())
        line += (rtl ? RLM : LRM);

    line += QLatin1String("<font color=\"") + backlogColor + QLatin1String("\">");
    // Prepend and append timestamp's correct bidi mark if the time and text
    // directions are different.
    if (rtl == time.isRightToLeft())
        line += QLatin1String("%1");
    else
        line += (time.isRightToLeft() ? RLM + QLatin1String("%1") + RLM : LRM + QLatin1String("%1") + LRM);

    // Partially copied from IRCView::formatFinalLine
    if (rtl)
    {
        // Return back to the normal direction after setting mark
        if (!rtl == time.isRightToLeft()) line += (!time.isRightToLeft() ? RLM : LRM);
        line += LRM; // <[nick_> -> <[_nick]>
    }
    line += QLatin1String("%2");
    if (rtl)
    {
        line += LRM; // <[_nick]> -> <_nick]>
        if (!text.isRightToLeft())
            line += RLM; // ARABIC_TEXT <_nick]> Hi -> ARABIC_TEXT Hi <_nick]>
    }

    line +=  QLatin1String(" %3</font>");
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

    if (m_showDate)
    {
        QString timeColor = Preferences::self()->color(Preferences::Time).name();
        doRawAppend(QStringLiteral("<font color=\"%1\">%2</font>").arg(timeColor, QLocale().toString(m_prevTimestamp.date(), QLocale::ShortFormat)), rtl);
        appendLine(DateLine);
        m_showDate = false;
    }

    doRawAppend(newLine, rtl);

    //FIXME: Disable auto-text for DCC Chats since we don't have a server to parse wildcards.
    if (!m_autoTextToSend.isEmpty() && m_server)
    {
        // replace placeholders in autoText
        QString sendText = m_server->parseWildcards(m_autoTextToSend,m_server->getNickname(), QString(), QString(), QString(), QString());
        // avoid recursion due to signalling
        m_autoTextToSend.clear();
        // send signal only now
        Q_EMIT autoText(sendText);
    }
    else
    {
        m_autoTextToSend.clear();
    }

    if (!m_lastStatusText.isEmpty())
        Q_EMIT clearStatusBarTempText();
}

void IRCView::doRawAppend(const QString& newLine, bool rtl)
{
    SelectionPin selpin(this); // HACK stop selection at end from growing
    QString line(newLine);

    line.remove(QLatin1Char('\n'));

    QTextBrowser::append(line);

    QTextCursor formatCursor(document()->lastBlock());
    QTextBlockFormat format = formatCursor.blockFormat();

    format.setAlignment(Qt::AlignAbsolute|(rtl ? Qt::AlignRight : Qt::AlignLeft));
    formatCursor.setBlockFormat(format);
}

QString IRCView::timeStamp(QHash<QString, QString> messageTags, bool rtl)
{
    if(Preferences::self()->timestamping())
    {
        QDateTime serverTime;

        if (messageTags.contains(QStringLiteral("time"))) // If it exists use the supplied server time.
            serverTime = QDateTime::fromString(messageTags[QStringLiteral("time")], Qt::ISODate).toLocalTime();

        QDateTime dateTime = serverTime.isValid() ? serverTime : QDateTime::currentDateTime();
        QString timeColor = Preferences::self()->color(Preferences::Time).name();
        QString timeFormat = Preferences::self()->timestampFormat();
        QString timeString;

        bool dateRtl = dateRtlDirection();

        if(!Preferences::self()->showDate())
        {
            timeString = QString(QLatin1String("<font color=\"") + timeColor + QLatin1String("\">[%1]</font> ")).arg(dateTime.time().toString(timeFormat));
            m_showDate = Preferences::self()->showDateLine() && dateTime.date() != m_prevTimestamp.date();
        }
        else
        {
            timeString = QString(QLatin1String("<font color=\"") +
                timeColor + QLatin1String("\">[%1%2 %3%4]</font> "))
                    .arg((dateRtl==rtl) ? QString() : (dateRtl ? RLM : LRM),
                        QLocale().toString(dateTime.date(), QLocale::ShortFormat),
                        dateTime.time().toString(timeFormat),
                         (dateRtl==rtl) ? QString() : (!dateRtl ? RLM : LRM));
        }

        m_prevTimestamp = dateTime;
        return timeString;
    }

    return QString();
}

QString IRCView::createNickLine(const QString& nick, const QString& defaultColor, bool encapsulateNick, bool privMsg)
{
    QString nickLine =QStringLiteral("%2");
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
            const QString ownNick = static_cast<DCC::ChatContainer*>(m_chatWin)->ownNick();

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
        nickLine = QLatin1String(R"(<a class="nick" href="#%1">%2</a>)").arg(QString::fromLatin1(QUrl::toPercentEncoding(nick)), nickLine);

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

    while ((pos = line.indexOf(QLatin1Char(decoration))) != -1) {
        line.replace(pos,1,(decorated) ? QStringLiteral("</%1>").arg(replacement) : QStringLiteral("<%1>").arg(replacement));
        decorated = !decorated;
    }
}

QString IRCView::filter(const QString& line, const QString& defaultColor, const QString& whoSent,
    bool doHighlight, bool parseURL, bool self, QChar::Direction* direction)
{
    QString filteredLine(line);
    Application* konvApp = Application::instance();

    //Since we can't turn off whitespace simplification withouteliminating text wrapping,
    //  if the line starts with a space turn it into a non-breaking space.
    //    (which magically turns back into a space on copy)

    if (filteredLine[0] == QLatin1Char(' ')) {
        filteredLine[0] = QLatin1Char('\xA0');
    }

    // TODO: Use QStyleSheet::escape() here
    // Replace all < with &lt;
    filteredLine.replace(QLatin1Char('<'), QLatin1String("\x0blt;"));
    // Replace all > with &gt;
    filteredLine.replace(QLatin1Char('>'), QLatin1String("\x0bgt;"));

    if (filteredLine.contains(QLatin1Char('\x07'))) {
        if (Preferences::self()->beep())
        {
            qApp->beep();
        }
        //remove char after beep
        filteredLine.remove(QLatin1Char('\x07'));
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
            line.toLower().contains(QRegularExpression(QLatin1String("(^|[^\\d\\w])")
                                                       + QRegularExpression::escape(ownNick.toLower())
                                                       + QLatin1String("([^\\d\\w]|$)"))))
        {
            // highlight current nickname
            highlightColor = Preferences::self()->highlightNickColor().name();
            m_tabNotification = Konversation::tnfNick;
        }
        else
        {
            const QList<Highlight*> highlightList = Preferences::highlightList();
            Highlight* matchedHighlight = nullptr;
            QStringList highlightChatWindowList;

            QStringList captures;
            for (Highlight* highlight : highlightList) {
                highlightChatWindowList = highlight->getChatWindowList();
                if (highlightChatWindowList.isEmpty() ||
                    highlightChatWindowList.contains(m_chatWin->getName(), Qt::CaseInsensitive))
                {
                    bool patternFound = false;
                    if (highlight->getRegExp())
                    {
                        const QRegularExpression needleReg(highlight->getPattern(),
                                                           QRegularExpression::CaseInsensitiveOption);
                        QRegularExpressionMatch rmatch;

                        patternFound = line.contains(needleReg, &rmatch)        // highlight regexp in text
                                       || whoSent.contains(needleReg, &rmatch); // highlight regexp in nickname

                        // remember captured patterns for later
                        captures = rmatch.capturedTexts();

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
                        matchedHighlight = highlight;
                        break;
                    }
                }
            }

            if (matchedHighlight) {
                highlightColor = matchedHighlight->getColor().name();
                m_highlightColor = highlightColor;

                if (matchedHighlight->getNotify()) {
                    m_tabNotification = Konversation::tnfHighlight;

                    if (Preferences::self()->highlightSoundsEnabled() && m_chatWin->notificationsEnabled())
                    {
                        konvApp->sound()->play(matchedHighlight->getSoundURL());
                    }

                    konvApp->notificationHandler()->highlight(m_chatWin, whoSent, line);
                }
                m_autoTextToSend = matchedHighlight->getAutoText();

                // replace %0 - %9 in regex groups
                for (int capture = 0; capture < captures.count(); capture++)
                {
                    m_autoTextToSend.replace(QStringLiteral("%%1").arg(capture), captures[capture]);
                }
                m_autoTextToSend.remove(QRegularExpression(QStringLiteral("%[0-9]")));
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
        htmlText.replace(QLatin1Char('&'), QLatin1String("&amp;"));
        htmlText.replace(QLatin1Char('\x0b'), QLatin1String("&"));
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

                QString link(QLatin1String(R"(%1<a href="#%2" style="color:)") + linkColor + QLatin1String("\">%3</a>%4%5"));

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
                colorCodes = removeDuplicateCodes(colorCodes, &data, allowColors);

                QString link(QLatin1String(R"(%1<a href="%2" style="color:)") + linkColor + QLatin1String("\">%3</a>%4%5"));

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


        switch (htmlText.at(pos).toLatin1())
        {
            case '\x02': //bold
                offset = defaultHtmlReplace(htmlText, &data, pos, QStringLiteral("b"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x1d': //italic
                offset = defaultHtmlReplace(htmlText, &data, pos, QStringLiteral("i"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x15': //mirc underline
            case '\x1f': //kvirc underline
                offset = defaultHtmlReplace(htmlText, &data, pos, QStringLiteral("u"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x13': //historic strikethru
            case '\x1e': //modern strikethru
                offset = defaultHtmlReplace(htmlText, &data, pos, QStringLiteral("s"));
                pos += offset -1;
                linkOffset += offset -1;
                break;
            case '\x11': //monospace
                offset = defaultHtmlReplace(htmlText, &data, pos, QStringLiteral("tt"));
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
                            if (data.openHtmlTags.contains(QStringLiteral("font")) &&
                                data.openHtmlTags.contains(QStringLiteral("span")))
                            {
                                colorString += closeToTagString(&data, QStringLiteral("span"));
                                data.lastBgColor.clear();
                                colorString += closeToTagString(&data, QStringLiteral("font"));
                                data.lastFgColor.clear();
                            }
                            else if (data.openHtmlTags.contains(QStringLiteral("font")))
                            {
                                colorString += closeToTagString(&data, QStringLiteral("font"));
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
                        if (data.openHtmlTags.contains(QStringLiteral("font")) &&
                            data.openHtmlTags.contains(QStringLiteral("span")))
                        {
                            colorString += closeToTagString(&data, QStringLiteral("span"));
                            colorString += closeToTagString(&data, QStringLiteral("font"));
                        }
                        else if (data.openHtmlTags.contains(QStringLiteral("font")))
                        {
                            colorString += closeToTagString(&data, QStringLiteral("font"));
                        }
                        data.lastFgColor = fgColor;
                        if (!bgColor.isEmpty())
                            data.lastBgColor = bgColor;

                        if (!data.lastFgColor.isEmpty())
                        {
                            colorString += fontColorOpenTag(data.lastFgColor);
                            data.openHtmlTags.append(QStringLiteral("font"));
                            if (!data.lastBgColor.isEmpty())
                            {
                                colorString += spanColorOpenTag(data.lastBgColor);
                                data.openHtmlTags.append(QStringLiteral("span"));
                            }
                        }
                    }
                    htmlText.replace(pos, colorMatch.length(), colorString);

                    pos += colorString.length() - 1;
                    linkOffset += colorString.length() -colorMatch.length();
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
                        if (data.openHtmlTags.contains(QStringLiteral("span")))
                        {
                            colorString += closeToTagString(&data, QStringLiteral("span"));
                        }
                        if (data.openHtmlTags.contains(QStringLiteral("font")))
                        {
                            colorString += closeToTagString(&data, QStringLiteral("font"));
                        }
                        data.reverse = true;
                        colorString += fontColorOpenTag(Preferences::self()->color(Preferences::TextViewBackground).name());
                        data.openHtmlTags.append(QStringLiteral("font"));
                        colorString += spanColorOpenTag(defaultColor);
                        data.openHtmlTags.append(QStringLiteral("span"));
                    }
                    else
                    {
                        // if reset reverse, close reverse and set old fore- and
                        // back-groundcolor if set in data
                        colorString += closeToTagString(&data, QStringLiteral("span"));
                        colorString += closeToTagString(&data, QStringLiteral("font"));
                        data.reverse = false;
                        if (!data.lastFgColor.isEmpty())
                        {
                            colorString += fontColorOpenTag(data.lastFgColor);
                            data.openHtmlTags.append(QStringLiteral("font"));
                            if (!data.lastBgColor.isEmpty())
                            {
                                colorString += spanColorOpenTag(data.lastBgColor);
                                data.openHtmlTags.append(QStringLiteral("span"));
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
                    if (dirChar == QLatin1Char(' ') &&
                        !lastChar.isNull() && lastChar == QLatin1Char(' ')) {
                        htmlText[pos] = QLatin1Char('\xA0');
                        lastChar = QLatin1Char('\xA0');
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
        htmlText.replace(QLatin1Char('&'), QLatin1String("&amp;"));
        htmlText.replace(QLatin1Char('\x0b'), QLatin1String("&"));
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
    if (i > -1)
        ret += openTags(data, i);

    return ret;
}

QString IRCView::openTags(TextHtmlData* data, int from)
{
    QString ret, tag;
    int i = from > -1 ? from : 0;

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

QString IRCView::removeDuplicateCodes(const QString& codes, TextHtmlData* data, bool allowColors)
{
    int pos = 0;
    QString ret;
    while (pos < codes.length())
    {
        switch (codes.at(pos).toLatin1())
        {
            case '\x02': //bold
                defaultRemoveDuplicateHandling(data, QStringLiteral("b"));
                ++pos;
                break;
            case '\x1d': //italic
                defaultRemoveDuplicateHandling(data, QStringLiteral("i"));
                ++pos;
                break;
            case '\x15': //mirc underline
            case '\x1f': //kvirc underline
                defaultRemoveDuplicateHandling(data, QStringLiteral("u"));
                ++pos;
                break;
            case '\x13': //historic strikethru
            case '\x1e': //modern strikethru
                defaultRemoveDuplicateHandling(data, QStringLiteral("s"));
                ++pos;
                break;
            case '\x11': //monospace
                defaultRemoveDuplicateHandling(data, QStringLiteral("tt"));
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
                if (!allowColors)
                {
                    pos += 1;
                    continue;
                }

                if (data->reverse)
                {
                    data->openHtmlTags.removeOne(QStringLiteral("span"));
                    data->openHtmlTags.removeOne(QStringLiteral("font"));
                    data->reverse = false;
                    if (!data->lastFgColor.isEmpty())
                    {
                        data->openHtmlTags.append(QStringLiteral("font"));
                        if (!data->lastBgColor.isEmpty())
                        {
                            data->openHtmlTags.append(QStringLiteral("span"));
                        }
                    }
                }
                else
                {
                    data->openHtmlTags.removeOne(QStringLiteral("span"));
                    data->openHtmlTags.removeOne(QStringLiteral("font"));
                    data->reverse = true;
                    data->openHtmlTags.append(QStringLiteral("font"));
                    data->openHtmlTags.append(QStringLiteral("span"));
                }
                ++pos;
                break;
            case '\x03': //color
                {
                    QString fgColor, bgColor;
                    bool fgOK = true, bgOK = true;
                    QString colorMatch(getColors(codes, pos, fgColor, bgColor, &fgOK, &bgOK));

                    if (!allowColors)
                    {
                        pos += colorMatch.length();
                        continue;
                    }

                    // check for color reset conditions
                    //TODO check if \x11 \017 is really valid here
                    if (colorMatch == QLatin1String("\x03") || colorMatch == QLatin1String("\x11") ||
                        (fgColor.isEmpty() && bgColor.isEmpty()) || (!fgOK && !bgOK))
                    {
                        if (!data->lastBgColor.isEmpty())
                        {
                            data->lastBgColor.clear();
                            data->openHtmlTags.removeOne(QStringLiteral("span"));
                        }
                        if (!data->lastFgColor.isEmpty())
                        {
                            data->lastFgColor.clear();
                            data->openHtmlTags.removeOne(QStringLiteral("font"));
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
                        data->openHtmlTags.append(QStringLiteral("font"));
                        if (!bgColor.isEmpty())
                        {
                            data->lastBgColor = bgColor;
                            data->openHtmlTags.append(QStringLiteral("span"));
                        }
                    }

                    pos += colorMatch.length();
                }
                break;
            default:
//                 qCDebug(KONVERSATION_LOG) << "unsupported duplicate code:" << QString::number(codes.at(pos).toLatin1(), 16);
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
    Q_UNUSED(fixedUrls)

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
            else
            {
                QRegularExpressionMatch match = colorRegExp.match(richtext, i, QRegularExpression::NormalMatch, QRegularExpression::AnchorAtOffsetMatchOption);
                if (match.hasMatch())
                {
                    // forward to the last matched position
                    i += match.capturedLength() - 1;
                }
                else
                {
                    j = 0;
                }
            }
        }
    }
}

QString IRCView::getColors(const QString& text, int start, QString& _fgColor, QString& _bgColor, bool* fgValueOK, bool* bgValueOK)
{
    static const QRegularExpression ircColorRegExp(QStringLiteral("(\003([0-9]{2}|[0-9]|)(,([0-9]{2}|[0-9]|)|,|)|\017)"));

    const QRegularExpressionMatch match = ircColorRegExp.match(text, start);
    if (!match.hasMatch()) {
        return QString{};
    }

    const QString ret(match.captured(0));

    const QString fgColor(match.captured(2));
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

    const QString bgColor(match.captured(4));
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
    QTextBrowser::resizeEvent(event);
}

void IRCView::mouseMoveEvent(QMouseEvent* ev)
{
    if (m_mousePressedOnUrl && (m_mousePressPosition - ev->pos()).manhattanLength() > QApplication::startDragDistance())
    {
        m_mousePressedOnUrl = false;

        QTextCursor textCursor = this->textCursor();
        textCursor.clearSelection();
        setTextCursor(textCursor);


        QPointer<QDrag> drag = new QDrag(this);
        auto* mimeData = new QMimeData;

        QUrl url(m_dragUrl);

        mimeData->setUrls(QList<QUrl> { url });

        drag->setMimeData(mimeData);

        const QString iconName = KIO::iconNameForUrl(url);
        const QPixmap pixmap = QIcon::fromTheme(iconName, QIcon::fromTheme(QStringLiteral("application-octet-stream"))).pixmap(32);
        drag->setPixmap(pixmap);

        drag->exec();

        return;
    }
    else
    {
        // Store the url here instead of in highlightedSlot as the link given there is decoded.
        m_urlToCopy = anchorAt(ev->pos());
    }

    QTextBrowser::mouseMoveEvent(ev);
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

    QTextBrowser::mousePressEvent(ev);
}

void IRCView::wheelEvent(QWheelEvent *ev)
{
    if(ev->modifiers()==Qt::ControlModifier)
    {
        if(ev->angleDelta().y() < 0) decreaseFontSize();
        if(ev->angleDelta().y() > 0) increaseFontSize();
    }

    QTextBrowser::wheelEvent(ev);
}

void IRCView::mouseReleaseEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
    {
        m_mousePressedOnUrl = false;
    }
    else if (ev->button() == Qt::MiddleButton)
    {
        if (m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
        {
            // The QUrl magic is what QTextBrowser's anchorClicked() does internally;
            // we copy it here for consistent behavior between left and middle clicks.
            openLink(QUrl::fromEncoded(m_urlToCopy.toUtf8())); // krazy:exclude=qclasses
            return;
        }
        else
        {
            Q_EMIT textPasted(true);
            return;
        }
    }

    QTextBrowser::mouseReleaseEvent(ev);
}

void IRCView::keyPressEvent(QKeyEvent* ev)
{
    const int key = ev->key() | ev->modifiers();

    if (KStandardShortcut::paste().contains(key))
    {
        Q_EMIT textPasted(false);
        ev->accept();
        return;
    }

    QTextBrowser::keyPressEvent(ev);
}

void IRCView::handleAnchorClicked(const QUrl& url)
{
    openLink(url);
}

void IRCView::openLink(const QUrl& url)
{
    if (!url.scheme().isEmpty())
    {
        Application::openUrl(QString::fromUtf8(url.toEncoded()));
    }
    else if (!url.hasQuery() && url.path().isEmpty() && url.hasFragment())
    {
        //FIXME: Don't do channel or user links in DCC Chats to begin with since they don't have a server.
        if (!m_server || !m_server->isConnected())
            return;

        QString fragment = url.fragment(QUrl::FullyDecoded);

        if (m_server->isAChannel(fragment))
        {
            m_server->sendJoinCommand(fragment);
        }
        else
        {
            NickInfoPtr nickInfo = m_server->obtainNickInfo(fragment);
            m_server->addQuery(nickInfo, true /*we initiated*/);
        }
    }
}

void IRCView::highlightedSlot(const QUrl& /*_link*/)
{
    QUrl link(QUrl::fromEncoded(m_urlToCopy.toUtf8()));

    //we just saw this a second ago.  no need to reemit.
    if (link.toString() == m_lastStatusText && !link.isEmpty())
        return;

    if (link.isEmpty())
    {
        if (!m_lastStatusText.isEmpty())
        {
            Q_EMIT clearStatusBarTempText();
            m_lastStatusText.clear();
        }
    }
    else
    {
        m_lastStatusText = link.toString();
    }

    if (link.isEmpty() || !link.scheme().isEmpty())
    {
        m_isOnNick = false;
        m_isOnChannel = false;

        if (!link.isEmpty()) {
            //link therefore != m_lastStatusText  so emit with this new text
            Q_EMIT setStatusBarTempText(m_lastStatusText);
        }

        if (link.isEmpty() && m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            setContextMenuOptions(IrcContextMenus::ShowLinkActions, false);
        else if (!link.isEmpty() && !m_contextMenuOptions.testFlag(IrcContextMenus::ShowLinkActions))
            setContextMenuOptions(IrcContextMenus::ShowLinkActions, true);
    }
    else if (!link.hasQuery() && link.path().isEmpty() && link.hasFragment())
    {
        if (!m_server || !m_server->isConnected())
            return;

        QString decoded = link.fragment(QUrl::FullyDecoded);

        if (!m_server->isAChannel(decoded))
        {
            m_currentNick = decoded;
            m_isOnNick = true;

            Q_EMIT setStatusBarTempText(i18n("Open a query with %1", m_currentNick));
        }
        else
        {
            m_currentChannel = decoded;
            m_isOnChannel = true;

            Q_EMIT setStatusBarTempText(i18n("Join the channel %1", m_currentChannel));
        }
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
        IrcContextMenus::nickMenu(ev->globalPos(), m_contextMenuOptions, m_server, QStringList() << m_currentNick, m_chatWin->getName());

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
                IrcContextMenus::processNickAction(contextMenuActionId, m_server, QStringList { m_chatWin->getName() },
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


#define dS d.space()
#define dN d.nospace()

QDebug operator<<(QDebug d, QTextBlockUserData *bd)
{
    Burr* b = dynamic_cast<Burr*>(bd);
    if (b)
    {
        dN;
        d   << "(";
        d   << (void*)(b) << ", format=" << b->m_format << ", blockNumber=" << b->m_block.blockNumber() << " p,n=" << (void*)b->m_prev << ", " << (void*)b->m_next;
        d   << ")";
    }
    else if (bd)
        dN << "(UNKNOWN! " << (void*)bd << ")";
    else
        d << "(none)";

    return d.space();
}

QDebug operator<<(QDebug d, QTextFrame* feed)
{
    if (feed)
    {
        d  << "\nDumping frame...";
        dN << Qt::hex << (void*)feed << Qt::dec;
        QTextFrame::iterator it = feed->begin();
        if (it.currentFrame() == feed)
            dS << "loop!" << Qt::endl;
        dS << "position" << feed->firstPosition() << feed->lastPosition();
        dN << "parentFrame=" << (void*)feed->parentFrame();
        dS;
        while (!it.atEnd())
        {
            //d << "spin";
            QTextFrame *frame = it.currentFrame();
            if (!frame) // this is a block
            {
                //d<<"dumping blocks:";
                QTextBlock b = it.currentBlock();
                //d << "block" << b.position() << b.length();
                d << Qt::endl << b;
            }
            else if (frame != feed)
            {
                d << frame;
            }
            ++it;
        };
        d << "\n...done.\n";
    }
    else
        d << "No frame to dump.";
    return d;
}

QDebug operator<<(QDebug d, QTextDocument* document)
{
    d << "=====================================================================================================================================================================";
    if (document)
        d << document->rootFrame();
    return d;
}

QDebug operator<<(QDebug d, const QTextBlock &b)
{
    QTextBlock::Iterator it = b.begin();

    int fragCount = 0;
    d   << "blockNumber"    << b.blockNumber();
    d   << "position"       << b.position();
    d   << "length"         << b.length();
    dN  << "firstChar 0x"   << Qt::hex << b.document()->characterAt(b.position()).unicode() << Qt::dec;
    if (b.length() == 2)
        dN << " second 0x"   << Qt::hex << b.document()->characterAt(b.position()+1).unicode() << Qt::dec;
    dS  << "userState"      << b.userState();
    dN  << "userData "      << (void*)b.userData();
    //dS  << "text"           << b.text();
    dS  << '\n';

    if (b.userData())
        d << b.userData();

    for (it = b.begin(); !(it.atEnd()); ++it)
    {
        QTextFragment f = it.fragment();
        if (f.isValid())
        {
            fragCount++;
            //d << "frag" << fragCount << _S(f.position()) << _S(f.length());
        }
    }
    d << _S(fragCount);
    return d;
}

#include "ircview.moc"
#include "moc_ircview.cpp"
