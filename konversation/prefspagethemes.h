/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004 by İsmail Dönmez
*/
#ifndef PREFSPAGETHEMES_H
#define PREFSPAGETHEMES_H

#include <qstringlist.h>

#include "prefspage.h"

class QLabel;
class QPushButton;
class KListBox;

class PrefsPageThemes : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageThemes(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageThemes();

 public slots:
    void applyPreferences();

 protected slots:
    void updatePreview(int id);
    void updateButtons();
    void installTheme();
    void removeTheme();

 private:

    /* Icon labels */
    QLabel* label[6];

    QStringList dirs;
    QPushButton* removeButton;
    KListBox *themeList;

    void updateList();

};

#endif
