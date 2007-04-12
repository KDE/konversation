/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The line input widget with chat enhanced functions
  begin:     Tue Mar 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "ircinput.h"
#include "konversationapplication.h"
#include "multilineedit.h"
#include "chatwindow.h"
#include "ircview.h"

#include <private/qrichtext_p.h>
#include <qclipboard.h>
#include <qregexp.h>
#include <qdom.h>
#include <qevent.h>
#include <qobject.h>
#include <qwhatsthis.h>
#include <qpopupmenu.h>

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcompletionbox.h>

#define MAXHISTORY 100
#define RICHTEXT 0


IRCInput::IRCInput(QWidget* parent) : KTextEdit(parent)
{
    m_lastHeight=document()->height();

    //I am not terribly interested in finding out where this value comes from
    //nor in compensating for it if my guess is incorrect. so, cache it.
    m_qtBoxPadding=m_lastHeight-fontMetrics().lineSpacing();

    connect(KApplication::kApplication(), SIGNAL(appearanceChanged()), this, SLOT(updateAppearance()));
    m_multiRow = Preferences::useMultiRowInputBox();

    m_useSelection = false;

    // connect history signal
    connect(this,SIGNAL (history(bool)) ,this,SLOT (getHistory(bool)) );
    // add one empty line to the history (will be overwritten with newest entry)
    historyList.prepend(QString());
    // reset history line counter
    lineNum=0;
    // reset completion mode
    setCompletionMode('\0');
    completionBox = new KCompletionBox(this);
    connect(completionBox, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));

    // widget may not be resized vertically
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));

    //NoWrap coupled with the size policy constrains the line edit to be one row high
    setWordWrap(m_multiRow ? WidgetWidth : NoWrap);

    setHScrollBarMode(AlwaysOff);
    setVScrollBarMode(AlwaysOff);
    #if RICHTEXT == 1
    setAutoFormatting(QTextEdit::AutoNone);
    setTextFormat(RichText);
    #else
    setTextFormat(PlainText);
    #endif

    QWhatsThis::add(this, i18n("<qt>The input line is where you type messages to be sent the channel, query, or server.  A message sent to a channel is seen by everyone on the channel, whereas a message in a query is sent only to the person in the query with you.<p>To automatically complete the nickname you began typing, press Tab. If you have not begun typing, the last successfully completed nickname will be used.<p>You can also send special commands:<br><table><tr><th>/me <i>action</i></th><td>shows up as an action in the channel or query.  For example:  <em>/me sings a song</em> will show up in the channel as 'Nick sings a song'.</td></tr><tr><th>/whois <i>nickname</i></th><td>shows information about this person, including what channels they are in.</td></tr></table><p>For more commands, see the Konversation Handbook.<p>A message cannot contain multiple lines.</qt>"));

    m_disableSpellCheckTimer = new QTimer(this);
    connect(m_disableSpellCheckTimer, SIGNAL(timeout()), this, SLOT(disableSpellChecking()));
}

IRCInput::~IRCInput()
{
}

void IRCInput::showEvent(QShowEvent* /* e */)
{
    m_disableSpellCheckTimer->stop();
    setCheckSpellingEnabled(Preferences::spellChecking());
}

void IRCInput::hideEvent(QHideEvent* /* event */)
{
    Preferences::setSpellChecking(checkSpellingEnabled());

    // If we disable spell-checking here immediately, tab switching will
    // be very slow. If we delay it by five seconds, a user would have to
    // need more than five seconds to switch between all his tabs before
    // the slowdown starts to occur (show event stops the timer, i.e. wrap-
    // around is not an issue). Unless he has unlikely amounts of channels,
    // needing more than five seconds indicates very slow switching speed,
    // which makes the delay a non-issue to begin with. Hence this fixes
    // the problem on the surface. In the KDE 4 version, we want to look
    // into having only one spell-checker instance instead of starting and
    // stopping at all.
    m_disableSpellCheckTimer->start(5000, true);
}

void IRCInput::disableSpellChecking()
{
    setCheckSpellingEnabled(false);
}
void IRCInput::slotSpellCheckDone(const QString& s)
{
    // NOTE: kdelibs 3.5's KSpell stupidly adds newlines to its
    // buffer at some point for god-knows-what-reason, and for-
    // gets to remove them again before handing the result back.
    // There's a FIXME to the effect in KSpell::check. This is
    // a workaround.

    if (s == text() || s == (text() + '\n'+'\n'))
        return;

    setText(s.simplifyWhiteSpace());
}

void IRCInput::updateAppearance()
{
    m_multiRow = Preferences::useMultiRowInputBox();
    setWordWrap(m_multiRow ? WidgetWidth : NoWrap);
    m_lastHeight=heightForWidth(sizeHint().width());
    ensureCursorVisible(); //appears to trigger updateGeometry
}

void IRCInput::resizeContents( int w, int h )
{
    if (document()->height() != m_lastHeight) {
        m_lastHeight=document()->height();
        updateGeometry();
    }
    KTextEdit::resizeContents(w,h);
}

// widget must be only one line high - luckily QT will enforce this via wrappping policy
QSize IRCInput::sizeHint() const
{
    constPolish();

    int ObscurePadding = 4;
    int f=2*frameWidth();
    int w=12 * (kMax(fontMetrics().lineSpacing(),14) + f + ObscurePadding);
    int h=m_lastHeight - m_qtBoxPadding + f + ObscurePadding;
    return QSize(w,h);
}

QPopupMenu *IRCInput::createPopupMenu( const QPoint &pos )
{
    QPopupMenu *menu=KTextEdit::createPopupMenu(pos);
    menu->removeItemAt(menu->count()-1);
    menu->removeItemAt(menu->count()-1);
    return menu;
}

QString IRCInput::text() const
{
    #if RICHTEXT == 1
    QString content=KTextEdit::text();

    QDomDocument document;

    document.setContent(content,false);
    QDomNodeList nodes=document.elementsByTagName("p");
    if(nodes.count())
    {
        QDomElement node=nodes.item(0).toElement();
        return node.text();
    }
    return QString();

    #else
    return KTextEdit::text();
    #endif
}

void IRCInput::setText(const QString& text)
{
    // reimplemented to  set cursor at the end of the new text
    KTextEdit::setText(text);
    setCursorPosition(0,text.length()+1);
}

// FIXME - find a better way to do this. eventfilters introduce nebulous behaviour
//take text events from IRCView and TopicLabel
bool IRCInput::eventFilter(QObject *object,QEvent *event)
{
    if (object->isA("IRCView") || object->isA("Konversation::TopicLabel"))
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);

            // Allow tab to be handled naturally by the widget.
            // Once it runs out of links it goes to the next control.
            if (ke->key() == Key_Tab && (ke->state() == 0 || ke->state() == Qt::ShiftButton))
                return false;

            if (!ke->text().isEmpty() && ((ke->state() & (Qt::ShiftButton|Qt::Keypad)) || ke->state() == 0))
            {
                setFocus();
                KonversationApplication::sendEvent(this,event);
                return true;
            }
        }
    }

    return KTextEdit::eventFilter(object,event);
}

// Take care of Tab, Cursor and so on
void IRCInput::keyPressEvent(QKeyEvent* e)
{
    switch(e->key())
    {
        case Key_Tab:
            emit nickCompletion();
            return;
            break;

        case Key_Up:
            if (m_multiRow && (e->state() != (Qt::ShiftButton|Qt::ControlButton)))
                break;
            emit history(true);
            return;
            break;

        case Key_Down:
            if (m_multiRow && (e->state() != (Qt::ShiftButton|Qt::ControlButton)))
                break;
            emit history(false);
            return;
            break;

        case Key_Enter:
        case Key_Return:
        {
            if(text().length()) addHistory(text());
            if(completionBox->isHidden())
            {
                // Reset completion mode
                setCompletionMode('\0');

                // Ctrl+Enter is a special case in which commands should be send as normal messages
                if ( e->state() & ControlButton )
                {
                    emit envelopeCommand();
                }
                else
                {
                    setText(static_cast<KonversationApplication*>(kapp)->doAutoreplace(text(),true));
                    emit submit();
                }
            }
            else
            {
                insertCompletion(completionBox->currentText());
                completionBox->hide();
            }
            // prevent widget from adding lines
            return;
        }
        break;

        default:
            // Check if the keystroke actually produced text. If not it was just a qualifier.
            if(!e->text().isEmpty() || ((e->key() >= Qt::Key_Home) && (e->key() <= Qt::Key_Down)))
            {
                if(getCompletionMode()!='\0')
                {
                    setCompletionMode('\0');
                    emit endCompletion();
                }

                completionBox->hide();
            }

            // support ASCII BEL
            if(e->ascii() == 7)
                insert("%G");
            // support ^U (delete text in input box)
            else if(e->ascii() == 21)
                clear();
    }

    KTextEdit::keyPressEvent(e);
}

void IRCInput::addHistory(const QString& line)
{
    // Only add line if it's not the same as the last was
    if(historyList[1]!=line)
    {
        // Replace empty first entry with line
        historyList[0]=line;
        // Add new empty entry to history
        historyList.prepend(QString());
        // Remove oldest line in history, if the list grows beyond MAXHISTORY
        if(historyList.count()>MAXHISTORY) historyList.remove(historyList.last());
    }
    // Reset history counter
    lineNum=0;
}

void IRCInput::getHistory(bool up)
{
    // preserve text
    historyList[lineNum]=text();
    // Did the user press cursor up?
    if(up)
    {
        // increment the line counter
        lineNum++;
        // if we are past the end of the list, go to the last entry
        if(lineNum==historyList.count()) lineNum--;
    }
    // no, it was cursor down
    else
    {
        // If we are at the top of the lest, arrow-down shall add the text to the history and clear the field for new input
        if(lineNum==0)
        {
            if(text().length()) addHistory(text());
            clear();
        }
        // If we aren't at the top of the list, decrement the line counter
        else
        {
            lineNum--;
        }
    }
    // replace the text in the input field with history
    setText(historyList[lineNum]);
}

/**
 * Work around the fact that while QTextEdit::paste() is virtual, whether we are
 * pasting from middle button or control-V is PRIVATE and NO ACCESSOR is given.
 */
void IRCInput::contentsMouseReleaseEvent( QMouseEvent *ev)
{
    if (ev->button() == Qt::MidButton)
    {
        m_useSelection=true;
    }

    // Reset completion
    setCompletionMode('\0');
    emit endCompletion();

    KTextEdit::contentsMouseReleaseEvent(ev);
    m_useSelection=false;
}

void IRCInput::paste(bool useSelection)
{
    m_useSelection = useSelection;
    paste();
    m_useSelection = false;
}

void IRCInput::paste()
{
    QClipboard *cb = KApplication::kApplication()->clipboard();
    setFocus();

    // Copy text from the clipboard (paste)
    QString pasteText;
    if(m_useSelection)
    {
        pasteText = cb->text( QClipboard::Selection);
    }
    else
    {
        pasteText = cb->text( QClipboard::Clipboard);
    }

    // is there any text in the clipboard?
    if(!pasteText.isEmpty())
    {
        //End completion on paste
        setCompletionMode('\0');
        emit endCompletion();

        bool signal=false;

        // replace \r with \n to make xterm pastes happy
        pasteText.replace("\r","\n");
        // remove blank lines
        while(pasteText.contains("\n\n"))
            pasteText.replace("\n\n","\n");

        QRegExp reTopSpace("^ *\n");
        while(pasteText.contains(reTopSpace))
            pasteText.remove(reTopSpace);

        QRegExp reBottomSpace("\n *$");
        while(pasteText.contains(reBottomSpace))
            pasteText.remove(reBottomSpace);

        // does the text contain at least one newline character?
        if(pasteText.find('\n')!=-1)
        {
            // make comparisons easier (avoid signed / unsigned warnings)
            unsigned int pos=pasteText.find('\n');
            unsigned int rpos=pasteText.findRev('\n');

            // emit the signal if there's a line break in the middle of the text
            if(pos>0 && pos!=(pasteText.length()-1))
                signal=true;
            // emit the signal if there's more than one line break in the text
            if(pos!=rpos)
                signal=true;

            // Remove the \n from end of the line if there's only one \n
            if(!signal)
                pasteText.remove('\n');
        }
        else
        {
            insert(pasteText);
            return;
        }

        // should we signal the application due to newlines in the paste?
        if(signal)
        {
            // if there is text in the input line
            if(!text().isEmpty())
            {
              // prepend text to the paste
              pasteText=text()+'\n'+pasteText;
            }
            // ask the user on long pastes
            if(checkPaste(pasteText))
            {
              // signal pasted text
              emit textPasted(pasteText);
              // remember old line, in case the user does not paste eventually
              addHistory(pasteText);
              // delete input text
              clear();
            }
        }
        // otherwise let the KLineEdit handle the pasting
        else KTextEdit::paste();
    }
}

bool IRCInput::checkPaste(QString& text)
{
    int doPaste=KMessageBox::Yes;

    //text is now preconditioned when you get here
    int lines=text.contains('\n');

    if(text.length()>256 || lines)
    {
        doPaste=KMessageBox::warningYesNoCancel
            (this,
            i18n("<qt>You are attempting to paste a large portion of text (%1 bytes or %2 lines) into "
            "the chat. This can cause connection resets or flood kills. "
            "Do you really want to continue?</qt>").arg(text.length()).arg(lines+1),
            i18n("Large Paste Warning"),
            i18n("Paste"),
            i18n("&Edit..."),
            "LargePaste");
    }

    if (doPaste==KMessageBox::No)
    {
        QString ret(MultilineEdit::edit(this,text));
        if (ret.isEmpty())
            return false;
        text=ret;
        return true;
    }

    return (doPaste==KMessageBox::Yes);
}

void IRCInput::showCompletionList(const QStringList& nicks)
{
    completionBox->setItems(nicks);
    completionBox->popup();
}

void IRCInput::insertCompletion(const QString& nick)
{
    int pos;                                      // = cursorPosition();
    int oldPos;                                   // = cursorPosition();

    getCursorPosition(&oldPos,&pos);
    oldPos=pos;

    QString line = text();

    while(pos && line[pos-1] != ' ') pos--;

    line.remove(pos, oldPos - pos);

    // did we find the nick in the middle of the line?
    if(pos)
    {
        QString addMiddle(Preferences::nickCompleteSuffixMiddle());
        line.insert(pos, nick + addMiddle);
        pos += nick.length() + addMiddle.length();
    }
    // no, it was at the beginning
    else
    {
        setLastCompletion(nick);
        QString addStart(Preferences::nickCompleteSuffixStart());
        line.insert(pos, nick + addStart);
        pos += nick.length() + addStart.length();
    }

    setText(line);
    setCursorPosition(0,pos);
}

void IRCInput::setLastCompletion(const QString& completion)
{
    m_lastCompletion = completion;
}

// Accessor methods

void IRCInput::setCompletionMode(char mode) { completionMode=mode; }
char IRCInput::getCompletionMode() { return completionMode; }
void IRCInput::setOldCursorPosition(int pos) { oldPos=pos; }
int IRCInput::getOldCursorPosition() { return oldPos; }

#include "ircinput.moc"
