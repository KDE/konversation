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

  $Id$
*/

#include <kdebug.h>
#include <kapplication.h>

#include <qclipboard.h>

#include "ircinput.h"

#define MAXHISTORY 100

IRCInput::IRCInput(QWidget* parent) : QLineEdit(parent)
{
  /* install eventFilter() function to trap TAB and cursor keys */
  installEventFilter(this);
  /* connect history signal */
  connect(this,SIGNAL (history(bool)) ,this,SLOT (getHistory(bool)) );
  /* add one empty line to the history (will be overwritten with newest entry) */
  historyList.prepend("");
  /* reset history line counter */
  lineNum=0;
  /* reset completion mode */
  setCompletionMode('\0');
}

IRCInput::~IRCInput()
{
}

/* Take care of Tab, Cursor and so on */
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
          if(text().length()) addHistory(text());
        break;

        default:
          // Check if the keystroke actually produced text. If not it was just a qualifier.
          if(keyEvent->text()!="") setCompletionMode('\0');
      }
    }
    // To prevent compiler warnings about unhandled case values
    default:
    {
    }
  }
  // reset completion mode
  return QLineEdit::eventFilter(object,event);
}

void IRCInput::addHistory(const QString& line)
{
  // Only add line if it's not the same as the last was
  if(historyList[1]!=line)
  {
    // Replace empty first entry with line
    historyList[0]=line;
    // Add new empty entry to history
    historyList.prepend("");
    // Remove oldest line in history, if the list grows beyond MAXHISTORY
    if(historyList.count()>MAXHISTORY) historyList.remove(historyList.last());
  }
  // Reset history counter
  lineNum=0;
}

void IRCInput::getHistory(bool up)
{
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

void IRCInput::paste()
{
  // TODO: prompt user on large / multiline pastes
  QClipboard *cb=KApplication::kApplication()->clipboard();
  QString text;

  // Copy text from the clipboard (paste)
  text=cb->text();
  // is there any text in the clipboard?
  if(text)
  {
    bool signal=false;
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

    if(signal)
      emit textPasted(text);
    else
      QLineEdit::paste();
  }
}

// Accessor methods

void IRCInput::setCompletionMode(char mode) { completionMode=mode; }
char IRCInput::getCompletionMode() { return completionMode; }
void IRCInput::setOldCursorPosition(int pos) { oldPos=pos; }
int IRCInput::getOldCursorPosition() { return oldPos; }

#include "ircinput.moc"
