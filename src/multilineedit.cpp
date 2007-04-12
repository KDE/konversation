/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "multilineedit.h"
#include "multilinetextedit.h"
#include "konversationapplication.h"

#include <qlayout.h>

#include <klocale.h>


QString MultilineEdit::returnText;                // static

MultilineEdit::MultilineEdit(QWidget* parent, const QString& text) :
KDialogBase(parent,"multiline_edit_dialog",true,i18n("Edit Multiline Paste"),
KDialogBase::User1 | KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true,
KGuiItem(i18n("Add &Quotation Indicators")))
{
    // Create the top level widget
    QWidget* page=new QWidget(this);
    setMainWidget(page);
    // Add the layout to the widget
    QVBoxLayout* dialogLayout=new QVBoxLayout(page);
    dialogLayout->setSpacing(spacingHint());
    // add the text editor
    textEditor=new MultilineTextEdit(page,"multiline_text_editor");
    textEditor->setTextFormat(PlainText);
    textEditor->setText(text);
    returnText=text;

    dialogLayout->addWidget(textEditor);

    setInitialSize(Preferences::multilineEditSize());
    show();
}

MultilineEdit::~MultilineEdit()
{
    Preferences::setMultilineEditSize(size());
}

void MultilineEdit::slotCancel()
{
    returnText=QString();
    KDialogBase::slotCancel();
}

void MultilineEdit::slotOk()
{
    returnText=textEditor->text();
    KDialogBase::slotOk();
}

void MultilineEdit::slotUser1()
{
    QStringList lines=QStringList::split("\n",textEditor->text(),true);
    for( QStringList::iterator it=lines.begin() ; it!=lines.end() ; ++it )
        (*it) = "> " + (*it);
    textEditor->setText(lines.join("\n"));
}

QString MultilineEdit::edit(QWidget* parent, const QString& text)
{
    MultilineEdit dlg(parent,text);
    dlg.exec();

    return returnText;
}

#include "multilineedit.moc"
