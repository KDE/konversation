/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefspagehighlight.h  -  Preferences panel for highlight patterns
  begin:     Die Jun 10 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEHIGHLIGHT_H
#define PREFSPAGEHIGHLIGHT_H

#include <qptrlist.h>

#include "prefspage.h"
#include "highlight.h"

/*
  @author Dario Abatianni
*/

class QCheckBox;
class QPushButton;

class KListView;
class KColorCombo;
class KLineEdit;
class KURLRequester;

class PrefsPageHighlight : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageHighlight();

    QPtrList<Highlight> getHighlightList();

  public slots:
    void applyPreferences();

  protected slots:
    void highlightSelected(QListViewItem* item);

    void highlightTextChanged(const QString& newPattern);
    void highlightColorChanged(const QColor& newColor);
    void soundURLChanged(const QString& newURL);
    void autoTextChanged(const QString& newURL);

    void addHighlight();
    void removeHighlight();

    void currentNickChanged(int state);
    void ownLinesChanged(int state);
    
    void playSound();

  protected:
    KListView* highlightListView;
    QLabel* patternLabel;
    KLineEdit* patternInput;
    KColorCombo* patternColor;
    QCheckBox* currentNickCheck;
    KColorCombo* currentNickColor;
    QCheckBox* ownLinesCheck;
    KColorCombo* ownLinesColor;
    KURLRequester* soundURL;
    QLabel* soundLabel;
    QPushButton* soundPlayBtn;
    QCheckBox* enableSoundCheck;
    QLabel* autoTextLabel;
    KLineEdit* autoTextInput;
};

#endif
