/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspageappearance.h  -  The preferences panel that holds the appearance settings
  begin:     Son Dez 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>

#ifndef PREFSPAGEAPPEARANCE_H
#define PREFSPAGEAPPEARANCE_H

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class PrefsPageAppearance : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageAppearance(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageAppearance();

  protected slots:
    void textFontClicked();
    void listFontClicked();
    void timestampingChanged(int state);
    void formatChanged(const QString& newFormat);

  protected:
    void updateFonts();

    QLabel* textPreviewLabel;
    QLabel* listPreviewLabel;

    QCheckBox* doTimestamping;
    QLabel* formatLabel;
    QComboBox* timestampFormat;
};

#endif
