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
#include "identitydialog.h"

#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvaluelist.h>

#include <kcombobox.h>
#include <klocale.h>
#include <klineedit.h>

#include "konversationapplication.h"

namespace Konversation {

IdentityDialog::IdentityDialog(QWidget *parent, const char *name)
  : KDialogBase(Plain, i18n("Identities"), Ok|Cancel, Ok, parent, name)
{
  QFrame* mainWidget = plainPage();
  QGridLayout* mainLayout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());

  QLabel* identityLabel = new QLabel(i18n("&Identity:"), mainWidget);
  m_identityCBox = new KComboBox(mainWidget, "identity_combo");
  m_identityCBox->setEditable(false);
  identityLabel->setBuddy(m_identityCBox);

  m_identityList = KonversationApplication::preferences.getIdentityList();

  for(QValueList<IdentityPtr>::iterator it = m_identityList.begin(); it != m_identityList.end(); ++it) {
    m_identityCBox->insertItem((*it)->getName());
  }

  QLabel* realNameLabel = new QLabel(i18n("&Real name:"), mainWidget);
  m_realNameEdit = new KLineEdit(mainWidget);
  realNameLabel->setBuddy(m_realNameEdit);

  QLabel* loginLabel = new QLabel(i18n("I&dent:"), mainWidget);
  m_loginEdit = new KLineEdit(mainWidget);
  loginLabel->setBuddy(m_loginEdit);

  // encoding combo box
  QLabel* codecLabel = new QLabel(i18n("&Encoding:"), mainWidget);
  m_codecCBox = new KComboBox(mainWidget,"codec_combo_box");
  codecLabel->setBuddy(m_codecCBox);
}

IdentityDialog::~IdentityDialog()
{
}

};
#include "identitydialog.moc"
