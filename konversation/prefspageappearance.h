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

#ifndef PREFSPAGEAPPEARANCE_H
#define PREFSPAGEAPPEARANCE_H

#include <qlabel.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <klistview.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class QHGroupBox;

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
    void showQuickButtonsChanged(int state);
    void showModeButtonsChanged(int state);
    void showCloseButtonsChanged(int state);
    void formatChanged(const QString& newFormat);
    void encodingChanged(int newEncodingIndex);

    void autoUserhostChanged(int state);

    void useSpacingChanged(int state);
    void spacingChanged(int newSpacing);
    void marginChanged(int newMargin);

    void useParagraphSpacingChanged(int state);
    void paragraphSpacingChanged(int newSpacing);

    void sortByStatusChanged(int state);
    void sortCaseInsensitiveChanged(int state);
    void sortingOrderChanged();

    void moveUp();
    void moveDown();

  protected:
    void updateFonts();

    QLabel* textPreviewLabel;
    QLabel* listPreviewLabel;

    QCheckBox* doTimestamping;
    QCheckBox* showQuickButtons;
    QCheckBox* showModeButtons;
    QLabel* formatLabel;
    QComboBox* timestampFormat;
    QComboBox* codecList;

    QCheckBox* useSpacingCheck;

    QLabel* spacingLabel;
    QLabel* marginLabel;

    QSpinBox* spacing;
    QSpinBox* margin;

    QCheckBox* useParagraphSpacingCheck;
    QSpinBox* paragraphSpacingSpin;

    QHGroupBox* sortOrderGroup;
    KListView* sortingOrder;
};

#endif
