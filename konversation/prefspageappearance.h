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
*/

#ifndef PREFSPAGEAPPEARANCE_H
#define PREFSPAGEAPPEARANCE_H

#include <qfont.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QLabel;
class QCheckBox;
class QSpinBox;
class QHGroupBox;

class KListView;

class PrefsPageAppearance : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageAppearance(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageAppearance();

  public slots:
    void applyPreferences();

  protected slots:
    void textFontClicked();
    void listFontClicked();
    void timestampingChanged(int state);

    void useSpacingChanged(int state);

    void useParagraphSpacingChanged(int state);

    void sortByStatusChanged(int state);

    void moveUp();
    void moveDown();

  protected:
    void updateFonts();

    QLabel* textPreviewLabel;
    QLabel* listPreviewLabel;

    QFont textFont;
    QFont listFont;

    QCheckBox* doTimestamping;
    QCheckBox* showQuickButtons;
    QCheckBox* showModeButtons;
    QCheckBox* showTopic;

    QLabel* formatLabel;

    QComboBox* timestampFormat;
    QComboBox* codecList;

    QCheckBox* autoUserhostCheck;
    QCheckBox* useSpacingCheck;

    QLabel* spacingLabel;
    QLabel* marginLabel;

    QSpinBox* spacingSpin;
    QSpinBox* marginSpin;

    QCheckBox* useParagraphSpacingCheck;
    QSpinBox* paragraphSpacingSpin;

    QCheckBox* sortByStatusCheck;
    QCheckBox* sortCaseInsensitiveCheck;

    QHGroupBox* sortOrderGroup;
    KListView* sortingOrder;
};

#endif
