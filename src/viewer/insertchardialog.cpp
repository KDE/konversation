/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#include "insertchardialog.h"

#include <KCharSelect>
#include <KGuiItem>
#include <KLocalizedString>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>


namespace Konversation
{

    InsertCharDialog::InsertCharDialog(const QString& font, QWidget *parent)
        : QDialog(parent)
    {
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Close);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &InsertCharDialog::slotAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &InsertCharDialog::reject);
        buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
        setModal( false );
        setWindowTitle(  i18n("Insert Character") );
        KGuiItem::assign(okButton, KGuiItem(i18n("&Insert"), "dialog-ok", i18n("Insert a character")));

        m_charTable = new KCharSelect(this,0, KCharSelect::CharacterTable|KCharSelect::FontCombo|KCharSelect::BlockCombos|KCharSelect::SearchLine);

        m_charTable->setCurrentFont( QFont( font ) );
        mainLayout->addWidget(m_charTable);
        mainLayout->addWidget(buttonBox);
        connect(m_charTable, &KCharSelect::charSelected, this, &InsertCharDialog::charSelected);
    }

    InsertCharDialog::~InsertCharDialog()
    {
    }

    void InsertCharDialog::setFont(const QFont &font)
    {
        m_charTable->setFont(font.family());
    }

    QChar InsertCharDialog::chr()
    {
        return m_charTable->currentChar();
    }

    void InsertCharDialog::charSelected()
    {
        emit insertChar(m_charTable->currentChar());
    }
    void InsertCharDialog::slotAccepted()
    {
        charSelected();
        accept();
  
    }
}


