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
#include <KLocale>
#include <kdeversion.h>


namespace Konversation
{

    InsertCharDialog::InsertCharDialog(const QString& font, QWidget *parent)
        : KDialog(parent)
    {
        setButtons( KDialog::Ok | KDialog::Close );
        setDefaultButton( KDialog::Ok );
        setModal( false );
        setCaption(  i18n("Insert Character") );
        setButtonGuiItem(KDialog::Ok, KGuiItem(i18n("&Insert"), "dialog-ok", i18n("Insert a character")));

#if KDE_IS_VERSION(4, 1, 90)
        m_charTable = new KCharSelect(this,0, KCharSelect::CharacterTable|KCharSelect::FontCombo|KCharSelect::BlockCombos);
#else
        m_charTable = new KCharSelect(this, KCharSelect::CharacterTable|KCharSelect::FontCombo|KCharSelect::BlockCombos);
#endif
        m_charTable->setCurrentFont( QFont( font ) );
        setMainWidget(m_charTable);
        connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
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

    void InsertCharDialog::slotOk()
    {
        emit insertChar(m_charTable->currentChar());
    }

}

#include "insertchardialog.moc"
