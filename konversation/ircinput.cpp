/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircinput.cpp  -  The line input widget with chat enhanced functions
  begin:     Tue Mar 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "konvidebug.h"
#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcompletionbox.h>

#include <qclipboard.h>
#include <qregexp.h>
#include <qdom.h>
#include <qevent.h>
#include <qobject.h>
#include <qregexp.h>
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
  // install eventFilter() function to trap TAB and cursor keys
  installEventFilter(this);
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
  setTextFormat(RichText);
#else
  setTextFormat(PlainText);
#endif


  QObject *p=static_cast<QObject*>(parent);
  //find our parent ChatWindow derivative, if there is one
  while(p && !p->inherits("ChatWindow")) {
    p=p->parent();
  }
  //put an event handler on it to forward text
  if (p) {
    static_cast<ChatWindow*>(p)->getTextView()->installEventFilter(this);
  }
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

// Take care of Tab, Cursor and so on
bool IRCInput::eventFilter(QObject *object,QEvent *event)
{
  if (object==this) {
    switch(event->type())
    {
      case QEvent::KeyPress:
      {
        QKeyEvent* keyEvent=(QKeyEvent*) event;
        switch(keyEvent->key())
        {
          case Key_Tab:
            emit nickCompletion();
            return true;
          break;

          case Key_Up:
            emit history(true);
            return true;
          break;

          case Key_Down:
            emit history(false);
            return true;
          break;
          // page up / down keys are now handled in chatwindow.cpp
          case Key_Enter:
          case Key_Return:
          {
            if(text().length()) addHistory(text());
            if(completionBox->isHidden()) {
              emit submit();
            } else {
              insertCompletion(completionBox->currentText());
              completionBox->hide();
            }
            // prevent widget from adding lines
            return true;
          }
          break;

          case Key_V:
          {
              if ( keyEvent->state() & ControlButton ) {
                  insert( kapp->clipboard()->text( QClipboard::Clipboard ) );
                return true;
              }
          }
          break;

          default:
            // Check if the keystroke actually produced text. If not it was just a qualifier.
            if(!keyEvent->text().isEmpty())
            {
              if(getCompletionMode()!='\0')
              {
                setCompletionMode('\0');
                emit endCompletion();
              }
              completionBox->hide();
            }
            // support ASCII BEL
            if(keyEvent->ascii()==7) insert("%G");
            // support ^U (delete text in input box)
            if(keyEvent->ascii()==21) clear();
            // support ^W (delete word)
            // KDE has CTRL Backspace for that ...
  //          else if(keyEvent->ascii()==23)
  //          {
  //            cursorWordBackward(true);
  //            cut();
  //          }
  //          else kdDebug() << keyEvent->ascii() << endl;
        }
      }
      // To prevent compiler warnings about unhandled case values
      default:
      {
      }
    }
  }
  else if (object->isA("IRCView") || object->isA("NickListView")) {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent* ke = static_cast<QKeyEvent*>(event);
      if (QChar(ke->ascii()).isPrint()) {
        setFocus();
        KonversationApplication::sendEvent(this,event);
        return TRUE;
      }
    }
  }
  return KTextEdit::eventFilter(object,event); //XXX any reason to skip KTextEdit?
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
    // If we aren't at the top of the list, decrement the line counter
    if(lineNum!=0) lineNum--;
  }
  // replace the text in the input field with history
  setText(historyList[lineNum]);
}

void IRCInput::insert(const QString& textToInsert)
{
  QString text(textToInsert);
  // replace \r with \n to make xterm pastes happy
  text.replace("\r","\n");
  // is there a newline in the pasted/inserted text?
  if(text.find('\n')!=-1)
  {
    if(checkPaste(text)) emit textPasted(text); //TODO this should not be synchronous
  }
  // otherwise let KLineEdit handle the new text
  else KTextEdit::insert(text);
}

/**
* Work around the fact that while QTextEdit::paste() is virtual, whether we are
* pasting from middle button or control-V is PRIVATE and NO ACCESSOR is given.
*/
void IRCInput::contentsMouseReleaseEvent( QMouseEvent *ev) {
  if (ev->button() == Qt::MidButton) {
    useSelection=TRUE;
  }
  KTextEdit::contentsMouseReleaseEvent(ev);
  useSelection=FALSE;
}

void IRCInput::paste()
{
  QClipboard *cb = KApplication::kApplication()->clipboard();

  // Copy text from the clipboard (paste)
  QString text;

  if(useSelection)
    text = cb->text(QClipboard::Selection);
  else
    text = cb->text(QClipboard::Clipboard);

  // is there any text in the clipboard?
  if(!text.isEmpty())
  {
    bool signal=false;
    // replace \r with \n to make xterm pastes happy
    text.replace("\r","\n");

    //  remove all trailing newlines
    text.replace(QRegExp("\n+$"),"");

    // does the text contain at least one newline character?
    if(text.find('\n')!=-1)
    {
      // make comparisons easier (avoid signed / unsigned warnings)
      unsigned int pos=text.find('\n');
      unsigned int rpos=text.findRev('\n');

      // emit the signal if there's a line break in the middle of the text
      if(pos>0 && pos!=(text.length()-1)) signal=true;
      // emit the signal if there's more than one line break in the text
      if(pos!=rpos) signal=true;
    }
    else
    {
      insert(text);
      return;
    }

    // should we signal the application due to newlines in the paste?
    if(signal)
    {
      // ask the user on long pastes
      if(checkPaste(text)) emit textPasted(text);
    }
    // otherwise let the KLineEdit handle the pasting
    else KTextEdit::paste();
  }
}

bool IRCInput::checkPaste(QString& text)
{
  int doPaste=KMessageBox::Yes;
  
  // Don't use QString::stripWhiteSpace() because it deletes spaces in the first line too.
  QRegExp reTopSpace("^ *\n");
  while(text.contains(reTopSpace))
    text.remove(reTopSpace);
  QRegExp reBottomSpace("\n *$");
  while(text.contains(reBottomSpace))
    text.remove(reBottomSpace);
  text += "\n";  // convenient to edit
  
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
              KStdGuiItem::yes(),
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
  int pos; // = cursorPosition();
  int oldPos; // = cursorPosition();

  getCursorPosition(&oldPos,&pos);
  oldPos=pos;

  QString line = text();

  while(pos && line[pos-1] != ' ') pos--;

  line.remove(pos, oldPos - pos);

  // did we find the nick in the middle of the line?
  if(pos)
  {
    QString addMiddle(KonversationApplication::preferences.getNickCompleteSuffixMiddle());
    line.insert(pos, nick + addMiddle);
    pos += nick.length() + addMiddle.length();
  }
  // no, it was at the beginning
  else
  {
    QString addStart(KonversationApplication::preferences.getNickCompleteSuffixStart());
    line.insert(pos, nick + addStart);
    pos += nick.length() + addStart.length();
  }

  setText(line);
  setCursorPosition(0,pos);
}

// Accessor methods

void IRCInput::setCompletionMode(char mode) { completionMode=mode; }
char IRCInput::getCompletionMode() { return completionMode; }
void IRCInput::setOldCursorPosition(int pos) { oldPos=pos; }
int IRCInput::getOldCursorPosition() { return oldPos; }

#include "ircinput.moc"
