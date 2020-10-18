/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
*/

#ifndef QUICKBUTTONS_CONFIG_H
#define QUICKBUTTONS_CONFIG_H

#include "ui_quickbuttons_configui.h"
#include "settingspage.h"


class QuickButtons_Config : public QWidget, public KonviSettingsPage, private Ui::QuickButtons_ConfigUI
{
  Q_OBJECT

  public:
    explicit QuickButtons_Config(QWidget* parent, const char* name=nullptr);
    ~QuickButtons_Config() override;

    void saveSettings() override;
    void loadSettings() override;
    void restorePageToDefaults() override;

    bool hasChanged() override;

  Q_SIGNALS:
    void modified();

  protected Q_SLOTS:
    void entrySelected(QTreeWidgetItem* quickButtonEntry);
    void nameChanged(const QString& newName);
    void actionChanged(const QString& newAction);
    void addEntry();
    void removeEntry();

  private:
    void setButtonsListView(const QStringList &buttonList);

  private:
    bool m_newItemSelected;

    QStringList m_oldButtonList;
    QStringList currentButtonList();

    Q_DISABLE_COPY(QuickButtons_Config)
};

#endif
