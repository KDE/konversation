/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef WATCHEDNICKNAMES_CONFIG_H
#define WATCHEDNICKNAMES_CONFIG_H

#include "settingspage.h" ////// header renamed
#include "servergroupsettings.h"
#include "ui_watchednicknames_preferencesui.h"

class QTreeWidgetItem;
class QTreeWidget;

class WatchedNicknames_Config : public QWidget, public KonviSettingsPage, private Ui::WatchedNicknames_ConfigUI
{
    Q_OBJECT

    public:
        explicit WatchedNicknames_Config(QWidget *parent = 0, const char *name = 0);
        ~WatchedNicknames_Config();

        virtual void saveSettings();
        virtual void loadSettings();
        virtual void restorePageToDefaults();

        virtual bool hasChanged();

    signals:
        void modified();

    protected slots:
        void checkIfEmptyListview(bool state);
        void newNotify();
        void removeNotify();
        void entrySelected(QTreeWidgetItem* notifyEntry);
        void networkChanged(const QString& newNetwork);
        void nicknameChanged(const QString& newNickname);
        void updateNetworkNames();

    protected:
        void enableEditWidgets(bool enabled);
        QStringList currentNotifyList();       // for hasChanged()
        void addNetworkBranch(Konversation::ServerGroupSettingsPtr group);
        QTreeWidgetItem* getItemById(QTreeWidget* listView,int id);

        bool newItemSelected;
        QStringList m_oldNotifyList;
};

#endif /* WATCHEDNICKNAMES_CONFIG_H */
