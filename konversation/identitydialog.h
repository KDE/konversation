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
#ifndef KONVERSATIONIDENTITYDIALOG_H
#define KONVERSATIONIDENTITYDIALOG_H

#include <kdialogbase.h>

#include "identity.h"

class KComboBox;
class KLineEdit;

namespace Konversation {

class IdentityDialog : public KDialogBase
{
  Q_OBJECT
  public:
    IdentityDialog(QWidget *parent = 0, const char *name = 0);
    ~IdentityDialog();
  
  private:
    KComboBox* m_identityCBox;
    QValueList<IdentityPtr> m_identityList;
    KLineEdit* m_realNameEdit;
    KLineEdit* m_loginEdit;
    KComboBox* m_codecCBox;
};

};

#endif
