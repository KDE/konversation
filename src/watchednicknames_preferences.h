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

#include "konvisettingspage.h"
#include "servergroupsettings.h"
#include "watchednicknames_preferencesui.h"


class Q3ListView;
class Q3ListViewItem;

class WatchedNicknames_Config : public WatchedNicknames_ConfigUI, public KonviSettingsPage
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
        void entrySelected(Q3ListViewItem* notifyEntry);
        void networkChanged(const QString& newNetwork);
        void nicknameChanged(const QString& newNickname);
        void updateNetworkNames();

    protected:
        void enableEditWidgets(bool enabled);
        QStringList currentNotifyList();       // for hasChanged()
        void addNetworkBranch(Konversation::ServerGroupSettingsPtr group);
        Q3ListViewItem* getItemById(Q3ListView* listView,int id);

        bool newItemSelected;
        QStringList m_oldNotifyList;
};

#endif
