/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Preferences panel for highlight patterns
  begin:     Die Jun 10 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFSPAGEHIGHLIGHT_H
#define PREFSPAGEHIGHLIGHT_H

#include <qptrlist.h>

#include "highlight_preferences.h"
#include "highlight.h"

class Preferences;

class PrefsPageHighlight : public Highlight_Config
{
  Q_OBJECT

  public:
    PrefsPageHighlight(QWidget* newParent,Preferences* newPreferences);
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
    Preferences* preferences;
};

#endif
