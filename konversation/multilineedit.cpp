//
// C++ Implementation: multilineedit
//
// Description: Multiline edit widget for pasting into line edit
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qlayout.h>

#include <qtextedit.h>
#include <klocale.h>

#include "multilineedit.h"
#include "konversationapplication.h"

QString MultilineEdit::returnText; // static

MultilineEdit::MultilineEdit(QWidget* parent,QString text) : 
               KDialogBase(parent,"multiline_edit_dialog",true,i18n("Edit multiline paste"),
                           KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true)
{
  // Create the top level widget
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  // Add the layout to the widget
  QVBoxLayout* dialogLayout=new QVBoxLayout(page);
  dialogLayout->setSpacing(spacingHint());
  // add the text editor
  textEditor=new QTextEdit(page,"multiline_text_editor");
  textEditor->setTextFormat(PlainText);
  textEditor->setText(text);
  returnText=text;

  dialogLayout->addWidget(textEditor);

  setInitialSize(KonversationApplication::preferences.getMultilineEditSize());
  show();
}

MultilineEdit::~MultilineEdit()
{
  KonversationApplication::preferences.setMultilineEditSize(size());
}

void MultilineEdit::slotCancel()
{
  returnText=QString::null;
  KDialogBase::slotCancel();
}

void MultilineEdit::slotOk()
{
  returnText=textEditor->text();
  KDialogBase::slotOk();
}

QString MultilineEdit::edit(QWidget* parent,QString text)
{
  MultilineEdit dlg(parent,text);
  dlg.exec();

  return returnText.stripWhiteSpace();
}

#include "multilineedit.moc"
