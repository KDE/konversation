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

#include <kdebug.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kcompletionbox.h>

#include <qclipboard.h>
#include <qregexp.h>
#include <qdom.h>

#include "ircinput.h"
#include "konversationapplication.h"
#include "multilineedit.h"

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
#if RICHTEXT == 1
  setTextFormat(RichText);
#else
  setTextFormat(PlainText);
#endif
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

        case Key_PageUp:
          emit pageUp();
          return true;
        break;

        case Key_PageDown:
          emit pageDown();
          return true;
        break;

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
  return QTextEdit::eventFilter(object,event);
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
  text.replace(QRegExp("\r"),"\n");
  // is there a newline in the pasted/inserted text?
  if(text.find('\n')!=-1)
  {
    if(checkPaste(text)) emit textPasted(text);
  }
  // otherwise let KLineEdit handle the new text
  else KTextEdit::insert(text);
}

void IRCInput::paste()
{
  QClipboard *cb=KApplication::kApplication()->clipboard();

  // Copy text from the clipboard (paste)
#if QT_VERSION >= 0x030100
  QString text=cb->text(QClipboard::Selection);
#else
  QString text=cb->text();
#endif
  // is there any text in the clipboard?
  if(!text.isEmpty())
  {
    bool signal=false;
    // replace \r with \n to make xterm pastes happy
    text.replace(QRegExp("\r"),"\n");

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
