//
// C++ Interface: highlight_Config
//
// Description: 
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2003, 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef KONVERSATIONHIGHLIGHT_CONFIG_H
#define KONVERSATIONHIGHLIGHT_CONFIG_H

#include <qobject.h>
#include "highlight_preferencesui.h"
#include "konvisettingspage.h"

class Highlight_Config;

/**
  @author Dario Abatianni
*/

class Highlight_Config : public Highlight_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT
  
  public:
    Highlight_Config(QWidget *parent = 0, const char *name = 0);
    ~Highlight_Config();

  public:
    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

  signals:
    void modified();
  
  protected slots:
    void highlightSelected(QListViewItem* item);
    void highlightTextChanged(const QString& newPattern);
    void highlightTextEditButtonClicked();
    void highlightColorChanged(const QColor& newColor);
    void soundURLChanged(const QString& newURL);
    void autoTextChanged(const QString& newText);
    void addHighlight();
    void removeHighlight();
    void playSound();
    QPtrList<Highlight> getHighlightList();
  protected:
    void updateButtons();
  
  protected:
    bool newItemSelected;
};

#endif

