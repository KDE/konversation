/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
*/
#ifndef PREFSPAGECOLORSAPPEARANCE_H
#define PREFSPAGECOLORSAPPEARANCE_H

#include <qstringlist.h>
#include <qptrlist.h>

#include <kcolorbutton.h>

#include "prefspage.h"

class QCheckBox;
class KURLRequester;

class PrefsPageColorsAppearance : public PrefsPage
{
  Q_OBJECT
  public:
    PrefsPageColorsAppearance(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageColorsAppearance();

  public slots:
    void applyPreferences();

  protected:
    QStringList colorList;
    KURLRequester* backgroundURL;
    QPtrList<KColorButton> colorBtnList;
    QPtrList<KColorButton> ircColorBtnList;
    QCheckBox* colorInputFieldsCheck;
    QCheckBox* parseIrcColorsCheck;
};

#endif
