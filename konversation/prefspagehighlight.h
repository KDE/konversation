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

  $Id$
*/

#ifndef PREFSPAGEHIGHLIGHT_H
#define PREFSPAGEHIGHLIGHT_H

#include <qvaluelist.h>

#include "prefspage.h"

/*
  @author Dario Abatianni
*/

class KListView;
class KColorCombo;
class KLineEdit;
class Highlight;

class PrefsPageHighlight : public PrefsPage
{
  Q_OBJECT

  public:
    PrefsPageHighlight(QFrame* newParent,Preferences* newPreferences);
    ~PrefsPageHighlight();

    QPtrList<Highlight> getHighlightList();

  protected slots:
    void highlightSelected(QListViewItem* item);

    void highlightTextChanged(const QString& newPattern);
    void highlightColorChanged(const QColor& newColor);

    void addHighlight();
    void removeHighlight();

    void currentNickChanged(int state);
    void ownLinesChanged(int state);
    void currentNickColorChanged(const QColor& newColor);
    void ownLinesColorChanged(const QColor& newColor);

  protected:
    KListView* highlightListView;
    KLineEdit* patternInput;
    KColorCombo* patternColor;
};

#endif
