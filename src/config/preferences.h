/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2005 Peter Simonsson <psn@linux.se>
  Copyright (C) 2005 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "servergroupsettings.h"
#include "identity.h"
#include "preferences_base.h"

#include <qobject.h>
#include <qsize.h>
#include <qstringlist.h>
#include <qfont.h>
#include <qcolor.h>
#include <qmap.h>

#include <kdeversion.h>


/*
Options still to be GUIfied:

Operator LEDs (int)
OperatorColor (int)
VoiceColor (int)
NoRightsColor (int)
*/

class Ignore;
class Highlight;
struct PreferencesSingleton;

class Preferences : public PreferencesBase
{
    Q_OBJECT

    friend struct PreferencesSingleton;

    protected:
        Preferences();

    public:

        static Preferences *self();
        ~Preferences();
        enum Pages
        {
            NotifyPage,
            ChatWinAppearancePage
        };
        static const Konversation::ServerGroupList serverGroupList();
        static void setServerGroupList(const Konversation::ServerGroupList& list);
        static void addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup);
        static const Konversation::ServerGroupSettingsPtr serverGroupById(int id);
        static const Konversation::ServerGroupSettingsPtr serverGroupByServer(const QString& server);
        static int serverGroupIdByName(const QString& serverGroup);
        static bool isServerGroup(const QString& server);
        static void removeServerGroup(int id);

        /** Returns a list of alias set up by default.  This is a set of aliases for the scripts found. */
        static QStringList defaultAliasList();

        //notifylist is in kconfigxt - FIXME
        static const QMap<int, QStringList> notifyList();
        static const QStringList notifyListByGroupName(const QString& groupName);
        static const QString notifyStringByGroupName(const QString& groupName);
        static void setNotifyList(const QMap<int, QStringList>& newList);
        static bool addNotify(int serverGroupId, const QString& newPattern);
        static bool removeNotify(const QString& groupName, const QString& pattern);
        static bool isNotify(int serverGroupId, const QString& pattern);
        static bool hasNotifyList(int serverGroupId);

        static const QList<Highlight*> highlightList();
        static void setHighlightList(QList<Highlight*> newList);
        static void addHighlight(const QString& newHighlight,bool regExp, const QColor &color,const QString& sound,const QString& autoText);

        /* All of the below work on the first (default) identity in your identity list*/
        static void addIgnore(const QString &newIgnore);
        static bool removeIgnore(const QString &oldIgnore);
        static bool isIgnored(const QString &nickname);
        static void clearIgnoreList();
        static const QList<Ignore*> ignoreList();
        static void setIgnoreList(QList<Ignore*> newList);

        static const QStringList quickButtonList();
        static const QStringList defaultQuickButtonList();
        static void setQuickButtonList(const QStringList newList);
        static void clearQuickButtonList();

        static const QList<QStringList> autoreplaceList();
        static const QList<QStringList> defaultAutoreplaceList();
        static void setAutoreplaceList(const QList<QStringList> newList);
        static void clearAutoreplaceList();

        static void addIdentity(IdentityPtr identity);
        static void removeIdentity(IdentityPtr identity);
        static void clearIdentityList();
        static const IdentityList identityList();
        static void setIdentityList(const IdentityList& list);
        static const IdentityPtr identityByName(const QString& name);
        static const IdentityPtr identityById(int id);
        static const QString ident();
        static void setIdent(const QString &ident);
        static const QString realName();
        static void setRealName(const QString &name);
        static const QString partReason();
        static void setPartReason(const QString &newReason);
        static const QString kickReason();
        static void setKickReason(const QString &newReason);
        static void setShowAwayMessage(bool state);
        static /* const */ bool showAwayMessage();
        static const QString awayMessage();
        static void setAwayMessage(const QString &newMessage);
        static const QString unAwayMessage();
        static void setUnAwayMessage(const QString &newMessage);
        static const QString defaultNicknameSortingOrder();
        static const QString nickname(int index);
        static const QStringList nicknameList();
        static void setNickname(int index,const QString &newName);
        static void setNicknameList(const QStringList &newList);

        static bool dialogFlag(const QString& flagName);
        static void setDialogFlag(const QString& flagName,bool state);

        static const QString channelEncoding(const QString& server,const QString& channel);
        static const QString channelEncoding(int serverGroupId,const QString& channel);
        static void setChannelEncoding(const QString& server,const QString& channel,const QString& encoding);
        static void setChannelEncoding(int serverGroupId,const QString& channel,const QString& encoding);
        static const QList<int> channelEncodingsServerGroupIdList();
        static const QStringList channelEncodingsChannelList(int serverGroupId);

        static void setShowTrayIcon(bool state);
        static void setTrayNotify(bool state);
        static void setAutoUserhost(bool state);

        static QString webBrowserCmd();

    signals:
        void requestServerConnection(int number);
        void requestSaveOptions();
        void autoContinuousWhoChanged();
        void updateTrayIcon();

    protected:
        IdentityPtr mIdentity;
        Konversation::ServerGroupList mServerGroupList;
        QList<Ignore*> mIgnoreList;
        QList<IdentityPtr> mIdentityList;
        QList<Highlight*> mHighlightList;
        QMap<int, QStringList> mNotifyList;  // network id, list of nicks
        QMap< int,QMap<QString,QString> > mChannelEncodingsMap;  // mChannelEncodingsMap[serverGroupdId][channelName]
        QStringList mQuickButtonList;
        QList<QStringList> mAutoreplaceList;
        QString mSortingOrder;
};
#endif
