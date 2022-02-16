/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
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
        ~Highlight_Config() override;

    public:
        void saveSettings() override;
        void loadSettings() override;
        void restorePageToDefaults() override;

        bool hasChanged() override;

    Q_SIGNALS:
        void modified();

    private Q_SLOTS:
        void highlightSelected(QTreeWidgetItem* item);
        void patternChanged(const QString& newPattern);
        void notifyModeChanged(bool);
        void colorChanged(const QColor& newColor);
        void soundURLChanged();
        void autoTextChanged(const QString& newText);
        void chatWindowsChanged(const QString& newChatWindows);
        void addHighlight();
        void removeHighlight();
        void playSound();

    private:
        QList<Highlight*> getHighlightList() const; // prefs format
        QStringList currentHighlightList() const;     // hasChanged() format
        void updateButtons();

    private:
        bool newItemSelected;
        QStringList m_oldHighlightList;

        Q_DISABLE_COPY(Highlight_Config)
};

#endif
