/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#ifndef AUTOREPLACE_CONFIG_H
#define AUTOREPLACE_CONFIG_H

#include "ui_autoreplace_configui.h"
#include "settingspage.h"


class Autoreplace_Config : public QWidget, public KonviSettingsPage, private Ui::Autoreplace_ConfigUI
{
  Q_OBJECT

  public:
    explicit Autoreplace_Config(QWidget* parent, const char* name = nullptr);
    ~Autoreplace_Config() override;

    void saveSettings() override;
    void loadSettings() override;
    void restorePageToDefaults() override;

    bool hasChanged() override;

  Q_SIGNALS:
    void modified();

  private Q_SLOTS:
    void entrySelected(QTreeWidgetItem* autoreplaceEntry);
    void directionChanged(int newDirection);
    void patternChanged(const QString& newPattern);
    void replacementChanged(const QString& newReplacement);
    void addEntry();
    void removeEntry();

  private:
    void setAutoreplaceListView(const QList<QStringList> &autoreplaceList);
    QList<QStringList> currentAutoreplaceList() const;

  private:
    bool m_newItemSelected;
    QList<QStringList> m_oldAutoreplaceList;

    Q_DISABLE_COPY(Autoreplace_Config)
};

#endif
