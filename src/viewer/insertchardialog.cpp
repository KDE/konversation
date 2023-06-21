/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#include "insertchardialog.h"

#include <KCharSelect>
#include <KGuiItem>
#include <KLocalizedString>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>


namespace Konversation
{

    InsertCharDialog::InsertCharDialog(const QString& font, QWidget *parent)
        : QDialog(parent)
    {
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Close);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &InsertCharDialog::slotAccepted);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &InsertCharDialog::reject);
        buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
        setModal( false );
        setWindowTitle(  i18n("Insert Character") );
        KGuiItem::assign(okButton, KGuiItem(i18n("&Insert"), QStringLiteral("dialog-ok"), i18n("Insert a character")));

        m_charTable = new KCharSelect(this,nullptr, KCharSelect::CharacterTable|KCharSelect::FontCombo|KCharSelect::BlockCombos|KCharSelect::SearchLine);

        m_charTable->setAllPlanesEnabled(true);
        m_charTable->setCurrentFont( QFont( font ) );
        mainLayout->addWidget(m_charTable);
        mainLayout->addWidget(buttonBox);
        connect(m_charTable, &KCharSelect::codePointSelected, this, &InsertCharDialog::charSelected);
    }

    InsertCharDialog::~InsertCharDialog()
    {
    }

    void InsertCharDialog::setFont(const QFont &font)
    {
        m_charTable->setFont(font.family());
    }

    uint InsertCharDialog::chr() const
    {
        return m_charTable->currentCodePoint();
    }

    void InsertCharDialog::charSelected()
    {
        Q_EMIT insertChar(m_charTable->currentCodePoint());
    }
    void InsertCharDialog::slotAccepted()
    {
        charSelected();
        accept();
  
    }
}

#include "moc_insertchardialog.cpp"
