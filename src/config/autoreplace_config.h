/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef AUTOREPLACE_CONFIG_H
#define AUTOREPLACE_CONFIG_H

#include "ui_autoreplace_configui.h"
#include "settingspage.h"


class Autoreplace_Config : public QWidget, public KonviSettingsPage, private Ui::Autoreplace_ConfigUI
{
  Q_OBJECT

  public:
    explicit Autoreplace_Config(QWidget* parent, const char* name=NULL);
    ~Autoreplace_Config() override;

    void saveSettings() override;
    void loadSettings() override;
    void restorePageToDefaults() override;

    bool hasChanged() override;

  Q_SIGNALS:
    void modified();

  protected Q_SLOTS:
    void entrySelected(QTreeWidgetItem* autoreplaceEntry);
    void directionChanged(int newDirection);
    void patternChanged(const QString& newPattern);
    void replacementChanged(const QString& newReplacement);
    void addEntry();
    void removeEntry();

  protected:
    void setAutoreplaceListView(const QList<QStringList> &autoreplaceList);

    bool m_newItemSelected;

    QList<QStringList> m_oldAutoreplaceList;
    QList<QStringList> currentAutoreplaceList();
};

#endif
