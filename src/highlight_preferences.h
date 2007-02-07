/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
*/

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
        explicit Highlight_Config(QWidget *parent = 0, const char *name = 0);
        ~Highlight_Config();

    public:
        virtual void saveSettings();
        virtual void loadSettings();
        virtual void restorePageToDefaults();

        virtual bool hasChanged();

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
        QPtrList<Highlight> getHighlightList(); // prefs format
        QStringList currentHighlightList();     // hasChanged() format
    protected:
        void updateButtons();

        bool newItemSelected;
        QStringList m_oldHighlightList;
};

#endif

