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
    setButtons( KDialog::User1 | KDialog::User2 | KDialog::Ok | KDialog::Cancel );
    setButtonText( KDialog::User1, i18n("Add &Quotation Indicators") );
    setButtonText( KDialog::User2, i18n("Remove Newlines") );
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
    connect( this, SIGNAL( user2Clicked() ), this, SLOT( slotUser2() ) );   
}

MultilineEdit::~MultilineEdit()
{
    Preferences::self()->setMultilineEditSize(size());
}

void MultilineEdit::slotCancel()
{
    returnText.clear();
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

void MultilineEdit::slotUser2()
{
    QString pattern("\n");
    QRegExp searchFor(pattern);
    searchFor.setPatternSyntax(QRegExp::FixedString);
    removeNonprintingChars();
    QString line=textEditor->toPlainText();
    int index=line.indexOf(searchFor);
    int patLen = pattern.length();
    while (index>=0)
    {
        QChar before,after;
        int length,nextLength;
        length=index;
        length+=patLen;
        if (index!=0) before = line.at(index-1);
        if (line.length() > length) after = line.at(length);
    
        if (before.isSpace() || after.isSpace())
        {
            line.replace(index, patLen, QString());
            nextLength = index;
        }
        else
        {
            line.replace(index, patLen, " ");
            nextLength = index+1;
        }
        index=line.indexOf(searchFor,nextLength);
    }
    textEditor->setText(line);
    dislayNonprintingChars();
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
