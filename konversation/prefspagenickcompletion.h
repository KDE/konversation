/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagenickcompletion.h  -  Provides a user interface to customize nickname completion
  begin:     Sat Nov 15 2003
  copyright: (C) 2003 by Peter Simonsson
  email:     psn@linux.se
*/

#ifndef PREFSPAGENICKCOMPLETION_H
#define PREFSPAGENICKCOMPLETION_H

#include <prefspage.h>

class KLineEdit;
class QComboBox;

class PrefsPageNickCompletion : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageNickCompletion(QFrame* newParent, Preferences* newPreferences);
    ~PrefsPageNickCompletion();
    
  public slots:
    void applyPreferences();
  
  protected:
    KLineEdit* suffixStartInput;
    KLineEdit* suffixMiddleInput;
    QComboBox* completionModeCBox;
};

#endif
