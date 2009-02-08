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
#include "application.h"

#include <KTextEdit>

#include <QVBoxLayout>


QString MultilineEdit::returnText;                // static

MultilineEdit::MultilineEdit(QWidget* parent, const QString& text) :
KDialog(parent )
{
    setButtons( KDialog::User1 | KDialog::Ok | KDialog::Cancel );
    setButtonText( KDialog::User1, i18n("Add &Quotation Indicators") );
    setDefaultButton( KDialog::Ok );
    setCaption( i18n("Edit Multiline Paste") );
    setModal( true );

    QVBoxLayout* dialogLayout=new QVBoxLayout(mainWidget());
    dialogLayout->setSpacing(spacingHint());
    // add the text editor
    textEditor=new KTextEdit(mainWidget());
    connect(textEditor, SIGNAL(textChanged()), this, SLOT(dislayNonprintingChars()));
    textEditor->setPlainText(text);

    returnText=text;

    dialogLayout->addWidget(textEditor);

    setInitialSize(Preferences::self()->multilineEditSize());
    show();
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( slotCancel() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotUser1() ) );
}

MultilineEdit::~MultilineEdit()
{
    Preferences::self()->setMultilineEditSize(size());
}

void MultilineEdit::slotCancel()
{
    returnText=QString();
    reject();
}

void MultilineEdit::slotOk()
{
    removeNonprintingChars();
    returnText=textEditor->toPlainText();
    accept();
}

void MultilineEdit::slotUser1()
{
    QStringList lines=textEditor->toPlainText().split('\n', QString::KeepEmptyParts);
    for( QStringList::iterator it=lines.begin() ; it!=lines.end() ; ++it )
        (*it) = "> " + (*it);
    textEditor->setText(lines.join("\n"));
}

QString MultilineEdit::edit(QWidget* parent, const QString& text)
{
    MultilineEdit dlg(parent,text);
    return dlg.exec() ? returnText : QString();
}

void MultilineEdit::dislayNonprintingChars()
{
    textEditor->blockSignals(true);

    const int position = textEditor->textCursor().position();

    removeNonprintingChars();

    const QString html = QString("<html>" + Qt::escape(textEditor->toPlainText()) + "</html>")
        .replace(' ', QString::fromUtf8("<span style=\"color:blue\">·</span>"))
        .replace('\n', QString::fromUtf8("<span style=\"color:blue\">¶</span><br/>"))
        .replace('\t', QString::fromUtf8("<span style=\"color:blue\">»</span>"))
        ;
    textEditor->setHtml(html);

    QTextCursor cursor = textEditor->textCursor();
    cursor.setPosition(position);
    textEditor->setTextCursor(cursor);

    textEditor->blockSignals(false);
}

void MultilineEdit::removeNonprintingChars()
{
    const bool blockSignals = textEditor->signalsBlocked();
    textEditor->blockSignals(true);
    QString text = textEditor->toPlainText();
    text
        .remove(QString::fromUtf8("¶"))
        .replace(QString::fromUtf8("»"), "\t")
        .replace(QString::fromUtf8("·"), " ")
        ;
    textEditor->setPlainText(text);
    textEditor->blockSignals(blockSignals);
}

#include "multilineedit.moc"
