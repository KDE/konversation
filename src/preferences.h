/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Class for application wide preferences
  begin:     Tue Feb 5 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <qobject.h>
#include <qptrlist.h>
#include <qsize.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qfont.h>
#include <qcolor.h>
#include <qmap.h>

#include <kdeversion.h>

#include "servergroupsettings.h"
#include "identity.h"
#include "preferences_base.h"

/*
  @author Dario Abatianni
*/

/*
Options still to be GUIfied:

Operator LEDs (int)
OperatorColor (int)
VoiceColor (int)
NoRightsColor (int)
*/

class Ignore;
class Highlight;

class Preferences : public QObject, public PreferencesBase 
{
    Q_OBJECT

    protected:
        Preferences();
        static Preferences *mSelf;
	    
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

	//notifylist is in kconfigxt - FIXME
        static const QMap<QString, QStringList> notifyList();
        static const QStringList notifyListByGroup(const QString& groupName);
        static const QString notifyStringByGroup(const QString& groupName);
        static void setNotifyList(const QMap<QString, QStringList>& newList);
        static const bool addNotify(const QString& groupName, const QString& newPattern);
        static const bool removeNotify(const QString& groupName, const QString& pattern);

        static const QPtrList<Highlight> highlightList();
        static void setHighlightList(QPtrList<Highlight> newList);
        static void addHighlight(const QString& newHighlight,bool regExp, const QColor &color,const QString& sound,const QString& autoText);

	/* All of the below work on the first (default) identity in your identity list*/
	static void addIgnore(const QString &newIgnore);
        static void clearIgnoreList();
        static const QPtrList<Ignore> ignoreList();
        static void setIgnoreList(QPtrList<Ignore> newList);
        static void addIdentity(IdentityPtr identity);
        static void removeIdentity(IdentityPtr identity);
        static void clearIdentityList();
        static const QValueList<IdentityPtr> identityList();
        static void setIdentityList(const QValueList<IdentityPtr>& list);
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
        static const bool showAwayMessage();
        static const QString awayMessage();
        static void setAwayMessage(const QString &newMessage);
        static const QString unAwayMessage();
        static void setUnAwayMessage(const QString &newMessage);
        static const QString nickname(int index);
        static const QStringList nicknameList();
        static void setNickname(int index,const QString &newName);
        static void setNicknameList(const QStringList &newList);

        static const bool dialogFlag(const QString& flagName);
        static void setDialogFlag(const QString& flagName,bool state);

        static const QString channelEncoding(const QString& server,const QString& channel);
        static void setChannelEncoding(const QString& server,const QString& channel,const QString& encoding);
        static const QStringList channelEncodingsServerList();
        static const QStringList channelEncodingsChannelList(const QString& server);

	static void setShowTrayIcon(bool state);
	static void setSystrayOnly(bool state);
	static void setTrayNotify(bool state);
	static void setAutoUserhost(bool state);


    signals:
        void requestServerConnection(int number);
        void requestSaveOptions();
        void autoUserhostChanged(bool state);
        void autoContinuousWhoChanged();
        void updateTrayIcon();

    protected:
/*
        bool mCustomVersionReplyEnabled;
        QString mCustomVersionReply;
        // Geometries
        QSize mMainWindowSize;
        QSize mNicksOnlineSize;
        QSize mNicknameSize;
        QSize mColorConfigurationSize;
        QSize mMultilineEditSize;



        // sorting stuff

        // more sorting stuff
        int mOpValue;

        // flag for hostmasks next to nicknames
        bool mClickableNicks;

*/
        IdentityPtr mIdentity;
        Konversation::ServerGroupList mServerGroupList;
        QPtrList<Ignore> mIgnoreList;
        QValueList<IdentityPtr> mIdentityList;
        QPtrList<Highlight> mHighlightList;
	QMap<QString, QStringList> mNotifyList;
        QMap< QString,QMap<QString,QString> > mChannelEncodingsMap;
/*
        // IRC colors
        QStringList mIrcColorList;
        bool mFilterColors;

        // aliases

        //Nick completion
        int mNickCompletionMode;
        bool mNickCompletionCaseSensitive;

        //User interface
        bool mShowMenuBar;

        // Web Browser
        bool mWebBrowserUseKdeDefault;

        bool mOpenWatchedNicksAtStartup;

        // Themes
        bool mEmotIconsEnabled;
        QString mEmotIconsTheme;

        QString mWikiUrl;
        bool mExpandWikiUrl;*/
};
#endif
