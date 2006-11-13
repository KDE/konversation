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

#ifndef AUTOREPLACECONFIG_H
#define AUTOREPLACECONFIG_H

#include "autoreplace_preferencesui.h"
#include "konvisettingspage.h"

/**
  @author Dario Abatianni <eisfuchs@tigress.com>
*/

class Autoreplace_Config : public Autoreplace_ConfigUI, public KonviSettingsPage
{
  Q_OBJECT

  public:
    explicit Autoreplace_Config(QWidget* parent, const char* name=NULL);
    ~Autoreplace_Config();

    virtual void saveSettings();
    virtual void loadSettings();
    virtual void restorePageToDefaults();

    virtual bool hasChanged();

  signals:
    void modified();

  protected slots:
    void entrySelected(QListViewItem* autoreplaceEntry);
    void directionChanged(int newDirection);
    void patternChanged(const QString& newPattern);
    void replacementChanged(const QString& newReplacement);
    void addEntry();
    void removeEntry();
    void sort(int column);
    void disableSort();
    void showRegExpEditor();

  protected:
    void setAutoreplaceListView(const QStringList &autoreplaceList);

    bool m_newItemSelected;

    QStringList m_oldAutoreplaceList;
    QStringList currentAutoreplaceList();
};

#endif
