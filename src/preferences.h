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
        enum TabPlacement
        {
            Top=0,
            Bottom,
            Left,                                 // not yet supported
            Right                                 // not yet supported
        };

        enum Pages
        {
            NotifyPage,
            ChatWinAppearancePage
        };
/*
        static const Konversation::ServerGroupList serverGroupList();
        static void setServerGroupList(const Konversation::ServerGroupList& list);
        static void addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup);
        static const Konversation::ServerGroupSettingsPtr serverGroupById(int id);
        static const Konversation::ServerGroupSettingsPtr serverGroupByServer(const QString& server);
        static int serverGroupIdByName(const QString& serverGroup);
	*/
        static bool isServerGroup(const QString& server);
	/*
        static void removeServerGroup(int id);

        static const bool autoReconnect();
        static void setAutoReconnect(bool state);
        static const unsigned int reconnectCount();
        static void setReconnectCount(unsigned int count);
        static const bool autoRejoin();
        static void setAutoRejoin(bool state);
        static const bool autojoinOnInvite();
        static void setAutojoinOnInvite(bool state);

        static const bool rawLog();
        static void setRawLog(bool state);

        static const bool customVersionReplyEnabled();
        static void setCustomVersionReplyEnabled(bool state);
        static QString customVersionReply();
        static void setCustomVersionReply(const QString &reply);

        static void setLog(bool state);
        static const bool log();
        static void setLowerLog(bool state);
        static const bool lowerLog();
        static void setAddHostnameToLog(bool state);
        static const bool addHostnameToLog();
        static void setLogFollowsNick(bool state);
        static const bool logFollowsNick();
        static void setScrollbackMax(int max);
        static const int scrollbackMax();

        static void setAutoWhoNicksLimit(int limit);
        static const int autoWhoNicksLimit();
        static void setAutoWhoContinuousEnabled(bool state);
        static const bool autoWhoContinuousEnabled();
        static void setAutoWhoContinuousInterval(int interval);
        static const int autoWhoContinuousInterval();

        static void setShowRealNames(bool show);
        static const bool showRealNames();
*/

	/* Map the nicer names to the crappy abbreviated kconfigxt names.
	 * To fix this properly, rename the kconfigxt to the nicer names,
	 * remove these mappings, then make a kconf_update script to convert
	 */
        static bool iPv4Fallback();
        static void setIPv4Fallback(bool fallback);
        static const QString& iPv4FallbackIface();
        static void setIPv4FallbackIface(const QString& interface);
/*
        static const TabPlacement tabPlacement();
        static void setTabPlacement(TabPlacement where);
        static void setBlinkingTabs(bool blink);
        static const bool blinkingTabs();
        static void setBringToFront(bool state);
        static const bool bringToFront();
        static void setCloseButtonsOnTabs(bool state);
        static const bool closeButtonsOnTabs();
        static void setCloseButtonsAlignRight(bool state);
        static const bool closeButtonsAlignRight();

        // sorting stuff
        static const bool sortByStatus();
        static void setSortByStatus(bool state);
        static const bool sortCaseInsensitive();
        static void setSortCaseInsensitive(bool state);
        
        // Geometry functions
        static const QSize nicksOnlineSize();
        static const QSize nicknameSize();
        static const QSize logfileReaderSize();
        static const QSize multilineEditSize();

        static void setNicksOnlineSize(const QSize &newSize);
        static void setNicknameSize(const QSize &newSize);
        static void setLogfileReaderSize(const QSize &newSize);
        static void setMultilineEditSize(const QSize &newSize);

        static const int logfileBufferSize();
        static void setLogfileBufferSize(int newSize);

        static const int notifyDelay();
        static void setNotifyDelay(int delay);
        static const bool useNotify();
        static void setUseNotify(bool use);*/
        static const QMap<QString, QStringList> notifyList();
        static const QStringList notifyListByGroup(const QString& groupName);
        static const QString notifyStringByGroup(const QString& groupName);
        static void setNotifyList(const QMap<QString, QStringList>& newList);
        static const bool addNotify(const QString& groupName, const QString& newPattern);
        static const bool removeNotify(const QString& groupName, const QString& pattern);

        static const QPtrList<Highlight> highlightList();
        static void setHighlightList(QPtrList<Highlight> newList);
        static void addHighlight(const QString& newHighlight,bool regExp, const QColor &color,const QString& sound,const QString& autoText);
/*        static void setHighlightSoundEnabled(bool enabled);
        static const bool highlightSoundEnabled();

        static void setHighlightNick(bool state);        // shall we highlight the current nick?
        static const bool highlightNick();

        static void setUseClickableNicks(bool state);
        static const bool useClickableNicks();

        static void setHighlightNickColor(const QString &color);

        static void setHighlightOwnLines(bool state);    // shall we highlight all our own lines?
        static const bool highlightOwnLines();

        static void setHighlightOwnLinesColor(const QString &color);
        static const QColor highlightOwnLinesColor();

        // On Screen Display
        static void setOSDUsage(bool state);
        static const bool OSDUsage();

        static void setOSDShowOwnNick(bool state);
        static const bool OSDShowOwnNick();

        static void setOSDShowChannel(bool state);
        static const bool OSDShowChannel();

        static void setOSDShowQuery(bool state);
        static const bool OSDShowQuery();

        static void setOSDShowChannelEvent(bool state);
        static const bool OSDShowChannelEvent();

        static const QFont OSDFont();
        static void setOSDFont(const QFont &newFont);
        static void setOSDFontRaw(const QString &rawFont);

        static void setOSDUseCustomColors(bool state);
        static const bool OSDUseCustomColors();

        static void setOSDTextColor(const QString &color);
        static const QColor OSDTextColor();

        static void setOSDBackgroundColor(const QString& color);
        static const QColor OSDBackgroundColor();

        static void setOSDDuration(int ms);
        static const int OSDDuration();

        static void setOSDScreen(uint screen);
        static const uint OSDScreen();

        static void setOSDDrawShadow(bool state);
        static const bool OSDDrawShadow();

        static void setOSDOffsetX(int offset);
        static const int OSDOffsetX();

        static void setOSDOffsetY(int offset);
        static const int OSDOffsetY();

        static void setOSDAlignment(int alignment);
        static const int OSDAlignment();

        static const QStringList buttonList();
        static void setButtonList(QStringList newList);
*/
        
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

	
/*
        // Colored nicknames
        static const bool useLiteralModes();
        static void setUseLiteralModes(bool literalModes);

        static const bool filterColors();
        static void setFilterColors(bool filter);

        static const QString& backgroundImageName();
        static void setBackgroundImageName(const QString& name);

        static const bool fixedMOTD();
        static void setFixedMOTD(bool fixed);

        static void setCommandChar(const QString &newCommandChar);
        static const QString commandChar();

        static const QFont textFont();
        static const QFont listFont();
        static void setTextFont(const QFont &newFont);
        static void setListFont(const QFont &newFont);
        static void setTextFontRaw(const QString &rawFont);
        static void setListFontRaw(const QString &newFont);

        static void setTimestamping(bool state);
        static const bool timestamping();
        static void setShowDate(bool state);
        static const bool showDate();
        static void setTimestampFormat(const QString& newFormat);
        static const QString timestampFormat();

        static void setShowQuickButtons(bool state);
        static const bool showQuickButtons();

        static void setShowModeButtons(bool state);
        static const bool showModeButtons();

        static void setShowServerList(bool state);
        static const bool showServerList();

        static void setShowTrayIcon(bool state);
        static const bool showTrayIcon();

        static void setSystrayOnly(bool state);
        static const bool systrayOnly();

        static void setShowBackgroundImage(bool state);
        static const bool showBackgroundImage();

        static void setTrayNotify(bool state);
        static const bool trayNotify();
        static void setTrayNotifyOnlyOwnNick(bool onlyOwnNick);
        static bool trayNotifyOnlyOwnNick();

        static void setChannelSplitterSizes(QValueList<int> sizes);
        static const QValueList<int> channelSplitterSizes();

        static void setTopicSplitterSizes(QValueList<int> sizes);
        static const QValueList<int> topicSplitterSizes();

        static void setChannelDoubleClickAction(const QString &action);
        static const QString channelDoubleClickAction();

        static void setNotifyDoubleClickAction(const QString &action);
        static const QString notifyDoubleClickAction();

        static void setUseSpacing(bool state);
        static const bool useSpacing();
        static void setSpacing(int newSpacing);
        static const int spacing();
        static void setMargin(int newMargin);
        static const int margin();

        static void setUseParagraphSpacing(bool state);
        static const bool useParagraphSpacing();
        static void setParagraphSpacing(int newSpacing);
        static const int paragraphSpacing();

        static void setAutoUserhost(bool state);
        static const bool autoUserhost();

        static const bool dialogFlag(const QString& flagName);
        static void setDialogFlag(const QString& flagName,bool state);

        static const int maximumLagTime();
        static void setMaximumLagTime(int maxLag);

        // IRC colors
        static const QStringList IRCColorList();
        static void setIRCColorList(const QStringList &cl);

        // aliases
        static const QStringList aliasList();
        static void setAliasList(const QStringList &aliasList);

        //User interface
        static const bool showMenuBar();
        static void setShowMenuBar(bool s);
        static const bool showTabBarCloseButton();
        static void setShowTabBarCloseButton(bool s);

        static const bool showTopic();
        static void setShowTopic(bool s);

        static const bool showRememberLineInAllWindows();
        static void setShowRememberLineInAllWindows(bool s);

        static const bool focusNewQueries();
        static void setFocusNewQueries(bool s);

        static const bool hideUnimportantEvents();
        static void setHideUnimportantEvents(bool state);

        static const bool disableExpansion();
        static void setDisableExpansion(bool state);

        // Web Browser
        static const bool webBrowserUseKdeDefault();
        static void setWebBrowserUseKdeDefault(bool state);
        static const QString webBrowserCmd();
        static void setWebBrowserCmd(const QString &cmd);


        static const bool openWatchedNicksAtStartup();
        static void setOpenWatchedNicksAtStartup(bool open);
*/
        static const QString channelEncoding(const QString& server,const QString& channel);
        static void setChannelEncoding(const QString& server,const QString& channel,const QString& encoding);
        static const QStringList channelEncodingsServerList();
        static const QStringList channelEncodingsChannelList(const QString& server);

        static QString wikiUrl();
        static void setWikiUrl(const QString& url);

        static bool expandWikiUrl();
        static void setExpandWikiUrl(bool expandUrl);

        signals:
        void requestServerConnection(int number);
        void requestSaveOptions();
        void autoUserhostChanged(bool state);
        void autoContinuousWhoChanged();
        void updateTrayIcon();

    protected:
        const QString defaultColor(const QString& name);

        bool mCustomVersionReplyEnabled;
        QString mCustomVersionReply;
/*
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
