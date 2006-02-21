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

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcompletionbox.h>

#include <qclipboard.h>
#include <qregexp.h>
#include <qdom.h>
#include <qevent.h>
#include <qobject.h>
#include <qwhatsthis.h>

#include "ircinput.h"
#include "konversationapplication.h"
#include "multilineedit.h"
#include "chatwindow.h"
#include "ircview.h"

#define MAXHISTORY 100
#define RICHTEXT 0

IRCInput::IRCInput(QWidget* parent) : KTextEdit(parent)
{
    m_useSelection = false;

    // connect history signal
    connect(this,SIGNAL (history(bool)) ,this,SLOT (getHistory(bool)) );
    // add one empty line to the history (will be overwritten with newest entry)
    historyList.prepend(QString::null);
    // reset history line counter
    lineNum=0;
    // reset completion mode
    setCompletionMode('\0');
    completionBox = new KCompletionBox(this);
    connect(completionBox, SIGNAL(activated(const QString&)), this, SLOT(insertCompletion(const QString&)));

    // widget may not resize vertically
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed));
    setWordWrap(NoWrap);
    setHScrollBarMode(AlwaysOff);
    setVScrollBarMode(AlwaysOff);
    #if RICHTEXT == 1
    setAutoFormatting(QTextEdit::AutoNone);
    setTextFormat(RichText);
    #else
    setTextFormat(PlainText);
    #endif

    QWhatsThis::add(this, i18n("<qt>The input line is where you type messages to be sent the channel, query, or server.  A message sent to a channel is seen by everyone on the channel, whereas a message in a query is sent only to the person in the query with you.<p>You can also send special commands:<br><table><tr><th>/me <i>action</i></th><td>shows up as an action in the channel or query.  For example:  <em>/me sings a song</em> will show up in the channel as 'Nick sings a song'.</td></tr><tr><th>/whois <i>nickname</i></th><td>shows information about this person, including what channels they are in.</td></tr></table><p>For more commands, see the Konversation Handbook.<p>A message can be a maximum of 512 letters long, and cannot contain multiple lines.</qt>"));
}

IRCInput::~IRCInput()
{
}

// widget must be only one line high
QSize IRCInput::sizeHint() const
{
    constPolish();

    int f=2*frameWidth();
    int h=QMAX(fontMetrics().lineSpacing(),14)+f+4;

    return QSize(12*h,h);
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
    return QString::null;

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

bool IRCInput::eventFilter(QObject *object,QEvent *event)
{
    if (object->isA("IRCView"))
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);

            if (!ke->text().isEmpty())
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
            emit history(true);
            return;
            break;

        case Key_Down:
            emit history(false);
            return;
            break;

        case Key_Enter:
        case Key_Return:
        {
            if(text().length()) addHistory(text());
            if(completionBox->isHidden())
            {
                // Ctrl+Enter is a special case in which commands should be send as normal messages
                if ( e->state() & ControlButton )
                {
                    emit envelopeCommand();
                }
                else
                {
                    setText(doAutoreplace(text()));
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
            if(!e->text().isEmpty())
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

QString IRCInput::doAutoreplace(const QString& text)
{
  QStringList autoreplaceList=Preferences::autoreplaceList();
  QString line=text;

  for(unsigned int index=0;index<autoreplaceList.count();index++)
  {
    QString definition=autoreplaceList[index];
    QString pattern=definition.section(',',1,1);
    QString replacement=definition.section(',',2);

    if(definition.section(',',0,0)=="1")
    {
      QRegExp needleReg=pattern;
      needleReg.setCaseSensitive(true);
      if(line.find(needleReg)!=-1)
      {
        QStringList captures;
        // remember captured patterns
        captures=needleReg.capturedTexts();

        // replace %0 - %9 in regex groups
        for(unsigned int capture=0;capture<captures.count();capture++)
        {
          replacement.replace(QString("%%1").arg(capture),captures[capture]);
        } // for
        replacement.replace(QRegExp("%[0-9]"),QString::null);

        line.replace(needleReg,replacement);
      }
    }
    else
    {
      line.replace(pattern,replacement);
    }
  } // for

  return line;
}

void IRCInput::addHistory(const QString& line)
{
    // Only add line if it's not the same as the last was
    if(historyList[1]!=line)
    {
        // Replace empty first entry with line
        historyList[0]=line;
        // Add new empty entry to history
        historyList.prepend(QString::null);
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

    kdDebug() << "paste()" << endl;
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
        bool signal=false;
        // replace \r with \n to make xterm pastes happy
        pasteText.replace("\r","\n");

        //  remove all trailing newlines
        pasteText.replace(QRegExp("\n+$"),"");

        // does the text contain at least one newline character?
        if(pasteText.find('\n')!=-1)
        {
            // make comparisons easier (avoid signed / unsigned warnings)
            unsigned int pos=pasteText.find('\n');
            unsigned int rpos=pasteText.findRev('\n');

            // emit the signal if there's a line break in the middle of the text
            if(pos>0 && pos!=(pasteText.length()-1)) signal=true;
            // emit the signal if there's more than one line break in the text
            if(pos!=rpos) signal=true;
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
              pasteText=text()+"\n"+pasteText;
            }
            // ask the user on long pastes
            if(checkPaste(pasteText))
            {
              // signal pasted text
              emit textPasted(pasteText);
              // remember old line, in case the user does not paste eventually
              addHistory(text());
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

    // Don't use QString::stripWhiteSpace() because it deletes spaces in the first line too.
    // remove waste spaces (including LF)
    QRegExp reTopSpace("^ *\n");
    while(text.contains(reTopSpace))
        text.remove(reTopSpace);
    QRegExp reBottomSpace("\n *$");
    while(text.contains(reBottomSpace))
        text.remove(reBottomSpace);
    // remove blank lines
    while(text.contains("\n\n"))
        text.replace("\n\n","\n");

    int lines=text.contains('\n');

    if(text.length()>256 || lines)
    {
        doPaste=KMessageBox::warningYesNoCancel
            (
            0,
            i18n("<qt>You are attempting to paste a large portion of text (%1 bytes or %2 lines) into "
            "the chat. This can cause connection resets or flood kills. "
            "Do you really want to continue?</qt>").arg(text.length()).arg(lines+1),
            i18n("Large Paste Warning"),
            i18n("Paste"),
            i18n("&Edit..."),
            "LargePaste"
            );
    }

    if(doPaste==KMessageBox::No)
    {
        text=MultilineEdit::edit(this,text);
        return true;
    }

    return(doPaste==KMessageBox::Yes);
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
