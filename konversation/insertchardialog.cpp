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

#include <kcharselect.h>
#include <klocale.h>
#include <kguiitem.h>

namespace Konversation {

InsertCharDialog::InsertCharDialog(const QString& font, QWidget *parent, const char *name)
  : KDialogBase(parent, name, false, i18n("Insert Character"), 
  KDialogBase::Ok | KDialogBase::Close,
  KDialogBase::Ok, false)
{
  setButtonOK(KGuiItem(i18n("&Insert"), "ok", i18n("Insert a character")));
  
  m_charTable = new KCharSelect(this, "charTable", font);
  m_charTable->enableFontCombo(false);
  setMainWidget(m_charTable);
}

InsertCharDialog::~InsertCharDialog()
{
}

QChar InsertCharDialog::chr()
{
  return m_charTable->chr();
}

void InsertCharDialog::slotOk()
{
  emit insertChar(m_charTable->chr());
}

}

#include "insertchardialog.moc"
