/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspage.cpp  -  This is the base class used for settings user interfaces
  begin:     Don Aug 29 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdialog.h>

#include "prefspage.h"

PrefsPage::PrefsPage(QFrame* newParent,Preferences* newPreferences)
{
  preferences=newPreferences;
  parentFrame=newParent;
}

PrefsPage::~PrefsPage()
{
}

int PrefsPage::marginHint()
{
  return KDialog::marginHint();
}

int PrefsPage::spacingHint()
{
  return KDialog::spacingHint();
}

#include "prefspage.moc"
