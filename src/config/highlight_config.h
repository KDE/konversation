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

#ifndef HIGHLIGHT_CONFIG_H
#define HIGHLIGHT_CONFIG_H

#include "ui_highlight_configui.h"
#include "settingspage.h"

class Highlight_Config;
class Highlight;

class Highlight_Config : public QWidget, public KonviSettingsPage, private Ui::Highlight_ConfigUI
{
    Q_OBJECT

    public:
        explicit Highlight_Config(QWidget *parent = nullptr, const QString& name = QString());
        ~Highlight_Config();

    public:
        void saveSettings() override;
        void loadSettings() override;
        void restorePageToDefaults() override;

        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    protected Q_SLOTS:
        void highlightSelected(QTreeWidgetItem* item);
        void patternChanged(const QString& newPattern);
        void notifyModeChanged(bool);
        void colorChanged(const QColor& newColor);
        void soundURLChanged(const QString& newURL);
        void autoTextChanged(const QString& newText);
        void chatWindowsChanged(const QString& newChatWindows);
        void addHighlight();
        void removeHighlight();
        void playSound();
        QList<Highlight*> getHighlightList(); // prefs format
        QStringList currentHighlightList();     // hasChanged() format
    protected:
        void updateButtons();

        bool newItemSelected;
        QStringList m_oldHighlightList;
};

#endif
