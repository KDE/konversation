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
#include "application.h"
#include "chatwindow.h"
#include "ircview.h"
#include "pasteeditor.h"
#include "viewcontainer.h"

#include <QClipboard>
#include <QKeyEvent>

#include <KMessageBox>
#include <KCompletionBox>
#include <KStandardShortcut>
#include <KActionCollection>

#define MAXHISTORY 100


IRCInput::IRCInput(QWidget* parent) : KTextEdit(parent)
{
    enableFindReplace(false);
    setAcceptRichText(false);
    //I am not terribly interested in finding out where this value comes from
    //nor in compensating for it if my guess is incorrect. so, cache it.
    m_qtBoxPadding = document()->size().toSize().height() - fontMetrics().lineSpacing();

    connect(KApplication::kApplication(), SIGNAL(appearanceChanged()), this, SLOT(updateAppearance()));
    m_multiRow = Preferences::self()->useMultiRowInputBox();

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

    updateAppearance();

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setWhatsThis(i18n("<qt><p>The input line is where you type messages to be sent the channel, query, or server.  A message sent to a channel is seen by everyone on the channel, whereas a message in a query is sent only to the person in the query with you.</p><p>To automatically complete the nickname you began typing, press Tab. If you have not begun typing, the last successfully completed nickname will be used.</p><p>You can also send special commands:</p><table><tr><th>/me <i>action</i></th><td>shows up as an action in the channel or query.  For example:  <em>/me sings a song</em> will show up in the channel as 'Nick sings a song'.</td></tr><tr><th>/whois <i>nickname</i></th><td>shows information about this person, including what channels they are in.</td></tr></table><p>For more commands, see the Konversation Handbook.</p><p>A message cannot contain multiple lines.</p></qt>"));

    m_disableSpellCheckTimer = new QTimer(this);
    connect(m_disableSpellCheckTimer, SIGNAL(timeout()), this, SLOT(disableSpellChecking()));

    document()->adjustSize();

    document()->setDocumentMargin(2);
}

IRCInput::~IRCInput()
{
}

QSize IRCInput::sizeHint() const
{
    QFontMetrics fm(font());

    int h = document()->size().toSize().height() - fm.descent() + 2 * frameWidth();

    QStyleOptionFrameV2 opt;
    opt.initFrom(this);
    opt.rect = QRect(0, 0, 100, h);
    opt.lineWidth = lineWidth();
    opt.midLineWidth = 0;
    opt.state |= QStyle::State_Sunken;

    QSize s = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(100, h).expandedTo(QApplication::globalStrut()), this);

    return s;
}

QSize IRCInput::minimumSizeHint() const
{
   return sizeHint();
}

void IRCInput::maybeResize()
{
    updateGeometry();
}

void IRCInput::showEvent(QShowEvent* /* e */)
{
    m_disableSpellCheckTimer->stop();
    setCheckSpellingEnabled(Preferences::self()->spellChecking());
    connect(this, SIGNAL(checkSpellingChanged(bool)), this, SLOT(setSpellChecking(bool)));
}

void IRCInput::hideEvent(QHideEvent* /* event */)
{
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

    //TODO FIXME when we get to require 4.2 the above is possible with
    //KTextEditSpellInterface and is actually quite easy to do.

    disconnect(SIGNAL(checkSpellingChanged(bool)));
    m_disableSpellCheckTimer->setSingleShot(true);
    m_disableSpellCheckTimer->start(5000);
}

void IRCInput::disableSpellChecking()
{
    setCheckSpellingEnabled(false);
}

void IRCInput::setSpellChecking(bool set)
{
    Preferences::self()->setSpellChecking(set);
}

void IRCInput::slotSpellCheckDone(const QString& s)
{
    // NOTE: kdelibs 3.5's KSpell stupidly adds newlines to its
    // buffer at some point for god-knows-what-reason, and for-
    // gets to remove them again before handing the result back.
    // There's a FIXME to the effect in KSpell::check. This is
    // a workaround.

    if (s == toPlainText() || s == (toPlainText() + '\n'+'\n'))
        return;

    setText(s.simplified());
}

void IRCInput::updateAppearance()
{
    m_multiRow = Preferences::self()->useMultiRowInputBox();
    setLineWrapMode(m_multiRow ? WidgetWidth : NoWrap);

    if (m_multiRow)
        connect(this, SIGNAL(textChanged()), this, SLOT(maybeResize()));
    else
        disconnect(this, SIGNAL(textChanged()), this, SLOT(maybeResize()));

    maybeResize();
    ensureCursorVisible(); //appears to trigger updateGeometry
}

// TODO FIXME - ok, wtf are we removing here? this is exactly the kind of shit i don't want to see any more
/*
Q3PopupMenu *IRCInput::createPopupMenu( const QPoint &pos )
{
    Q3PopupMenu *menu=KTextEdit::createPopupMenu(pos);
    menu->removeItemAt(menu->count()-1);
    menu->removeItemAt(menu->count()-1);
    return menu;
}
*/

/*
QString IRCInput::text() const
{
    return KTextEdit::text();
}
*/
void IRCInput::setText(const QString& text)
{
    // reimplemented to  set cursor at the end of the new text
    KTextEdit::setPlainText(text);
    moveCursor(QTextCursor::End);
}

// FIXME - find a better way to do this. eventfilters introduce nebulous behaviour
//take text events from IRCView and TopicLabel
bool IRCInput::eventFilter(QObject *object,QEvent *event)
{
    if (object->metaObject()->className() == QString("IRCView") || object->metaObject()->className() == QString("Konversation::TopicLabel"))
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);

            // Allow tab to be handled naturally by the widget.
            // Once it runs out of links it goes to the next control.
            if (ke->key() == Qt::Key_Tab && (ke->modifiers() == 0 || ke->modifiers() == Qt::ShiftModifier))
                return false;

            if (!ke->text().isEmpty() && ((ke->modifiers() & (Qt::ShiftModifier|Qt::KeypadModifier|Qt::GroupSwitchModifier)) ||
                                          ke->modifiers() == 0))
            {
                setFocus();
                Application::sendEvent(this,event);
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
        case Qt::Key_Tab:
            emit nickCompletion();
            return;
            break;

        case Qt::Key_Up:
            if (m_multiRow && textCursor().movePosition(QTextCursor::Up))
                break;
            emit history(true);
            return;
            break;

        case Qt::Key_Down:
            if (m_multiRow && textCursor().movePosition(QTextCursor::Down))
                break;
            emit history(false);
            return;
            break;

        case Qt::Key_Enter:
        case Qt::Key_Return:
        {
            if(!toPlainText().isEmpty()) addHistory(toPlainText());
            if(completionBox->isHidden())
            {
                // Reset completion mode
                setCompletionMode('\0');

                // Ctrl+Enter is a special case in which commands should be send as normal messages
                if ( e->modifiers() & Qt::ControlModifier )
                {
                    emit envelopeCommand();
                }
                else
                {
                    setText(static_cast<Application*>(kapp)->doAutoreplace(toPlainText(),true));
                    emit submit();
                }
            }
            else
            {
                insertCompletion(completionBox->currentItem() ? completionBox->currentItem()->text() : completionBox->item(0)->text());
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
            if(e->text().unicode()->toLatin1() == 7)
                insertPlainText("%G");
            // support ^U (delete text in input box)
            else if(e->text().unicode()->toLatin1() == 21)
                setText("");
    }

    KTextEdit::keyPressEvent(e);
}

bool IRCInput::event(QEvent* e)
{
    if (e->type() == QEvent::ShortcutOverride)
    {
        // Make sure KTextEdit doesn't eat actionCollection shortcuts
        QKeyEvent* event = static_cast<QKeyEvent*>(e);
        const int key = event->key() | event->modifiers();

        foreach(QAction* action, Application::instance()->getMainWindow()->actionCollection()->actions())
        {
            KAction* kAction = qobject_cast<KAction*>(action);

            if(kAction->shortcut().contains(key))
            {
                event->ignore();
                return false;
            }
        }
    }

    return KTextEdit::event(e);
}


void IRCInput::wheelEvent(QWheelEvent* e)
{
    if (e->delta() > 0)
    {
        emit history(true);
    }
    else if (e->delta() < 0)
    {
        emit history(false);
    }

    KTextEdit::wheelEvent(e);
}

void IRCInput::addHistory(const QString& line)
{
    // Only add line if it's not the same as the last was
    if(historyList.value(1)!=line)
    {
        // Replace empty first entry with line
        historyList[0]=line;
        // Add new empty entry to history
        historyList.prepend(QString());
        // Remove oldest line in history, if the list grows beyond MAXHISTORY
        if(historyList.count()>MAXHISTORY) historyList.removeLast();
    }
    // Reset history counter
    lineNum=0;
}

void IRCInput::getHistory(bool up)
{
    // preserve text
    historyList[lineNum]=toPlainText();
    // Did the user press cursor up?
    if(up)
    {
        // increment the line counter
        lineNum++;
        // if we are past the end of the list, go to the last entry
        if (lineNum==historyList.count()) {
            lineNum--;
            return;
        }
    }
    // no, it was cursor down
    else
    {
        // If we are at the top of the lest, arrow-down shall add the text to the history and clear the field for new input
        if(lineNum==0)
        {
            if(!toPlainText().isEmpty()) addHistory(toPlainText());
            setText("");
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

void IRCInput::paste(bool useSelection)
{
    insertFromMimeData(KApplication::clipboard()->mimeData(useSelection ? QClipboard::Selection : QClipboard::Clipboard));
}

void IRCInput::insertFromMimeData(const QMimeData * source)
{
    if(!source)
        return;

    setFocus();

    // Copy text from the clipboard (paste)
    QString pasteText = source->text();

    // is there any text in the clipboard?
    if(!pasteText.isEmpty())
    {
        //End completion on paste
        setCompletionMode('\0');
        emit endCompletion();

        bool signal=false;

        //filter out crashy crap
        Konversation::sterilizeUnicode(pasteText);

        // replace \r with \n to make xterm pastes happy
        pasteText.replace('\r','\n');
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
        if(pasteText.contains('\n'))
        {
            // make comparisons easier (avoid signed / unsigned warnings)
            int pos=pasteText.indexOf('\n');
            int rpos=pasteText.lastIndexOf('\n');

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
            insertPlainText(pasteText);
            ensureCursorVisible();
            return;
        }

        // should we signal the application due to newlines in the paste?
        if(signal)
        {
            // if there is text in the input line
            if(!toPlainText().isEmpty())
            {
              // prepend text to the paste
              pasteText=toPlainText()+'\n'+pasteText;
            }
            // ask the user on long pastes
            if(checkPaste(pasteText))
            {
              Konversation::sterilizeUnicode(pasteText);
              // signal pasted text
              emit textPasted(pasteText);
              // remember old line, in case the user does not paste eventually
              addHistory(pasteText);
              // delete input text
              setText("");
            }
        }
        // otherwise let the KLineEdit handle the pasting
        else KTextEdit::insertFromMimeData(source);
    }
}

bool IRCInput::checkPaste(QString& text)
{
    int doPaste=KMessageBox::Yes;

    //text is now preconditioned when you get here
    int lines=text.count('\n');

    if(text.length()>256 || lines)
    {
        QString bytesString = i18np("1 byte", "%1 bytes", text.length());
        QString linesString = i18np("1 line", "%1 lines", lines+1);

        doPaste=KMessageBox::warningYesNoCancel
            (this,
            i18nc(
            "%1 is, for instance, '200 bytes'.  %2 is, for instance, '7 lines'.  Both are localised (see the two previous messages).",
            "<qt>You are attempting to paste a large portion of text (%1 or %2) into "
            "the chat. This can cause connection resets or flood kills. "
            "Do you really want to continue?</qt>", bytesString, linesString),
            i18n("Large Paste Warning"),
            KGuiItem(i18n("Paste")),
            KGuiItem(i18n("&Edit...")),
            KStandardGuiItem::cancel(),
            QString("LargePaste"),
            KMessageBox::Dangerous);
    }

    if (doPaste==KMessageBox::No)
    {
        QString ret(PasteEditor::edit(this,text));
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

    //getCursorPosition(&oldPos,&pos);
    oldPos=pos=textCursor().position();

    QString line = toPlainText();

    while(pos && line[pos-1] != ' ') pos--;

    line.remove(pos, oldPos - pos);

    // did we find the nick in the middle of the line?
    if(pos)
    {
        QString addMiddle(Preferences::self()->nickCompleteSuffixMiddle());
        line.insert(pos, nick + addMiddle);
        pos += nick.length() + addMiddle.length();
    }
    // no, it was at the beginning
    else
    {
        setLastCompletion(nick);
        QString addStart(Preferences::self()->nickCompleteSuffixStart());
        line.insert(pos, nick + addStart);
        pos += nick.length() + addStart.length();
    }

    setText(line);
    textCursor().setPosition(pos);
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
