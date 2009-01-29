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
#include "application.h"

#include <qlayout.h>
//Added by qt3to4:
#include <Q3VBoxLayout>

#include <klocale.h>


QString MultilineEdit::returnText;                // static

MultilineEdit::MultilineEdit(QWidget* parent, const QString& text) :
KDialog(parent )
                            //KGuiItem(i18n("Add &Quotation Indicators")))
{
    setButtons( KDialog::User1 | KDialog::Ok | KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    setCaption( i18n("Edit Multiline Paste") );
    setModal( true );
    // Create the top level widget
    QWidget* page=new QWidget(this);
    setMainWidget(page);
    // Add the layout to the widget
    Q3VBoxLayout* dialogLayout=new Q3VBoxLayout(page);
    dialogLayout->setSpacing(spacingHint());
    // add the text editor
    textEditor=new MultilineTextEdit(page,"multiline_text_editor");
    textEditor->setTextFormat(Qt::PlainText);
    textEditor->setText(text);
    returnText=text;

    dialogLayout->addWidget(textEditor);

    setInitialSize(Preferences::multilineEditSize());
    show();
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
}

MultilineEdit::~MultilineEdit()
{
    Preferences::setMultilineEditSize(size());
}

void MultilineEdit::slotCancel()
{
    returnText=QString();
    reject();
}

void MultilineEdit::slotOk()
{
    returnText=textEditor->text();
    accept();
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
