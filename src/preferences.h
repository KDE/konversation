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
	    
    public:
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
        static void setLogPath(const QString &path);
        static const QString logPath();
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

        static void setDccAddPartner(bool state);
        static const bool dccAddPartner();
        static void setDccCreateFolder(bool state);
        static const bool dccCreateFolder();
        static void setDccAutoGet(bool state);
        static const bool dccAutoGet();
        static void setDccAutoResume(bool state);
        static const bool dccAutoResume();
        static void setDccBufferSize(unsigned long size);
        static const unsigned long dccBufferSize();
        static void setDccPath(const QString &path);
        static const QString dccPath();
        static void setDccMethodToGetOwnIp(int methodId);
        static const int dccMethodToGetOwnIp();
        static void setDccSpecificOwnIp(const QString& ip);
        static const QString dccSpecificOwnIp();
        static void setDccSpecificSendPorts(bool state);
        static const bool dccSpecificSendPorts();
        static void setDccSendPortsFirst(unsigned long port);
        static const unsigned int dccSendPortsFirst();
        static void setDccSendPortsLast(unsigned long port);
        static const unsigned int dccSendPortsLast();
        static void setDccSpecificChatPorts(bool state);
        static const bool dccSpecificChatPorts();
        static void setDccChatPortsFirst(unsigned long port);
        static const unsigned int dccChatPortsFirst();
        static void setDccChatPortsLast(unsigned long port);
        static const unsigned int dccChatPortsLast();
        static void setDccFastSend(bool state);
        static const bool dccFastSend();
        static void setDccSendTimeout(int sec);
        static const int dccSendTimeout();
        static bool iPv4Fallback();
        static void setIPv4Fallback(bool fallback);
        static const QString& iPv4FallbackIface();
        static void setIPv4FallbackIface(const QString& interface);

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
        // more sorting stuff
        static const int adminValue();
        static void setAdminValue(int value);
        static const int ownerValue();
        static void setOwnerValue(int value);
        static const int opValue();
        static void setOpValue(int value);
        static const int halfopValue();
        static void setHalfopValue(int value);
        static const int voiceValue();
        static void setVoiceValue(int value);
        static const int noRightsValue();
        static void setNoRightsValue(int value);

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
        static void setUseNotify(bool use);
        static const QMap<QString, QStringList> notifyList();
        static const QStringList notifyListByGroup(const QString& groupName);
        static const QString notifyStringByGroup(const QString& groupName);
        static void setNotifyList(const QMap<QString, QStringList>& newList);
        static const bool addNotify(const QString& groupName, const QString& newPattern);
        static const bool removeNotify(const QString& groupName, const QString& pattern);

        static const QPtrList<Highlight> highlightList();
        static void setHighlightList(QPtrList<Highlight> newList);
        static void addHighlight(const QString& newHighlight,bool regExp, const QColor &color,const QString& sound,const QString& autoText);
        static void setHighlightSoundEnabled(bool enabled);
        static const bool highlightSoundEnabled();

        static void setHighlightNick(bool state);        // shall we highlight the current nick?
        static const bool highlightNick();

        static void setUseClickableNicks(bool state);
        static const bool useClickableNicks();

        static void setHighlightNickColor(const QString &color);
        static const QColor highlightNickColor();

        static void setHighlightOwnLines(bool state);    // shall we highlight all our own lines?
        static const bool highlightOwnLines();

        static void setHighlightOwnLinesColor(const QString &color);
        static const QColor highlightOwnLinesColor();

        // On Screen Display
        static void setOSDUsage(bool state);
        static const bool oSDUsage();

        static void setOSDShowOwnNick(bool state);
        static const bool oSDShowOwnNick();

        static void setOSDShowChannel(bool state);
        static const bool oSDShowChannel();

        static void setOSDShowQuery(bool state);
        static const bool oSDShowQuery();

        static void setOSDShowChannelEvent(bool state);
        static const bool oSDShowChannelEvent();

        static const QFont oSDFont();
        static void setOSDFont(const QFont &newFont);
        static void setOSDFontRaw(const QString &rawFont);

        static void setOSDUseCustomColors(bool state);
        static const bool oSDUseCustomColors();

        static void setOSDTextColor(const QString &color);
        static const QColor oSDTextColor();

        static void setOSDBackgroundColor(const QString& color);
        static const QColor oSDBackgroundColor();

        static void setOSDDuration(int ms);
        static const int oSDDuration();

        static void setOSDScreen(uint screen);
        static const uint oSDScreen();

        static void setOSDDrawShadow(bool state);
        static const bool oSDDrawShadow();

        static void setOSDOffsetX(int offset);
        static const int oSDOffsetX();

        static void setOSDOffsetY(int offset);
        static const int oSDOffsetY();

        static void setOSDAlignment(int alignment);
        static const int oSDAlignment();

        static const QStringList buttonList();
        static void setButtonList(QStringList newList);

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
//        static IdentityPtr identity;

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
*/
        static const QString nickname(int index);
        static const QStringList nicknameList();
        static void setNickname(int index,const QString &newName);
        static void setNicknameList(const QStringList &newList);
        static const QString color(const QString& name);
        static void setColor(const QString& name,const QString& color);

/*
        // Colored nicknames
        static const bool useColoredNicks();
        static void setUseColoredNicks(bool usecolor);

        static const QStringList nickColorList();
        static void setNickColorList(const QStringList &cl);

        static const bool useBoldNicks();
        static void setUseBoldNicks(bool boldNicks);

        static const bool useLiteralModes();
        static void setUseLiteralModes(bool literalModes);

        static const bool filterColors();
        static void setFilterColors(bool filter);

        static const bool colorInputFields();
        static void setColorInputFields(bool state);

        static const QString& backgroundImageName();
        static void setBackgroundImageName(const QString& name);

        static void setNickCompleteSuffixStart(const QString &suffix);
        static void setNickCompleteSuffixMiddle(const QString &suffix);
        static const QString nickCompleteSuffixStart();
        static const QString nickCompleteSuffixMiddle();

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

        // Nick completion
        static const int nickCompletionMode();
        static void setNickCompletionMode(int mode);
        static const QString prefixCharacter();
        static void  setPrefixCharacter(const QString &prefix);
        static const bool nickCompletionCaseSensitive();
        static void setNickCompletionCaseSensitive(bool caseSensitive);

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
/*
        // Themes
        static void setIconTheme(const QString& name);
        static const QString iconTheme();

        static void setEmotIconsEnabled(bool enabled) { mEmotIconsEnabled = enabled; }
        static bool emotIconsEnabled() { return mEmotIconsEnabled; }
        static void setEmotIconsTheme(const QString& theme) { mEmotIconsTheme = theme; }
        static QString emotIconsTheme() { return mEmotIconsTheme; }

        static bool showNicknameBox();
        static void setShowNicknameBox(bool show);

        static bool disableNotifyWhileAway() { return mDisableNotifyWhileAway; }
        static void setDisableNotifyWhileAway(bool disable) { mDisableNotifyWhileAway = disable; }

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

        bool mLog;
        bool mLowerLog;
        bool mAddHostnameToLog;
        bool mLogFollowsNick;
        QString mLogPath;
        int mScrollbackMax;

        int mAutoWhoNicksLimit;
        bool mAutoWhoContinuousEnabled;
        int mAutoWhoContinuousInterval;

        bool mShowRealNames;

        TabPlacement mTabPlacement;                // where do the tabs go?
        bool mBlinkingTabs;                        // Do we want the LEDs on the tabs to blink?
        bool mCloseButtonsOnTabs;                  // Do we want close widgets on the tabs?
        bool mCloseButtonsAlignRight;              // Display close widgets on the right side?
        bool mBringToFront;                        // Do we want to see newly created tabs immediately?

        bool mFixedMOTD;
        bool mRawLog;

        bool mCustomVersionReplyEnabled;
        QString mCustomVersionReply;

        bool mDccAddPartner;
        bool mDccCreateFolder;                     // create folders for each DCC partner?
        int mDccMethodToGetOwnIp;
        QString mDccSpecificOwnIp;
        bool mDccSpecificSendPorts;
        unsigned long mDccSendPortsFirst;
        unsigned long mDccSendPortsLast;
        bool mDccSpecificChatPorts;
        unsigned long mDccChatPortsFirst;
        unsigned long mDccChatPortsLast;
        bool mDccAutoGet;
        bool mDccAutoResume;
        unsigned long mDccBufferSize;
        QString mDccPath;
        bool mDccFastSend;
        int mDccSendTimeout;
        bool ipv4Fallback;
        QString ipv4Interface;

        bool mAutoReconnect;
        unsigned int mReconnectCounter;
        bool mAutoRejoin;
        bool mAutojoinOnInvite;

        int mMaximumLag;                           // ask for reconnect

        int mNotifyDelay;
        bool mUseNotify;

        bool mTimestamping;
        bool mShowDate;
        QString mTimestampFormat;

        bool mShowQuickButtons;
        bool mShowModeButtons;
        bool mShowServerList;
        bool mShowTrayIcon;
        bool mSystrayOnly;
        bool mShowBackgroundImage;
        bool mTrayNotify;
        bool mTrayNotifyOnlyOwnNick;

        QValueList<int> mChannelSplitter;
        QValueList<int> mTopicSplitterSizes;

        QMap<QString, QStringList> mNotifyList;
        QString mCommandChar;
        QString mPreShellCommandStr;

        QString mNickCompleteSuffixStart;
        QString mNickCompleteSuffixMiddle;

        QString mChannelDoubleClickAction;
        QString mNotifyDoubleClickAction;

        // Geometries
        QSize mMainWindowSize;
        QSize mNicksOnlineSize;
        QSize mNicknameSize;
        QSize mLogfileReaderSize;
        QSize mColorConfigurationSize;
        QSize mMultilineEditSize;

        int mLogfileBufferSize;

        bool mUseSpacing;
        int mSpacing;
        int mMargin;

        // sorting stuff
        bool mSortByStatus;
        bool mSortCaseInsensitive;

        // more sorting stuff
        int mAdminValue;
        int mOwnerValue;
        int mOpValue;
        int mHalfopValue;
        int mVoiceValue;
        int mAwayValue;
        int mNoRightsValue;

        // flag for hostmasks next to nicknames
        bool mAutoUserhost;

        bool mUseParagraphSpacing;
        int mParagraphSpacing;

        QFont mTextFont;
        QFont mListFont;

        bool mHighlightNick;
        bool mHighlightOwnLines;
        QColor mHighlightNickColor;
        QColor mHighlightOwnLinesColor;
        bool mHighlightSoundEnabled;

        bool mClickableNicks;

        // On Screen Display
        bool mOSDUsage;                            // Use OSD?
        bool mOSDShowOwnNick;                      // Message, if own nick appears
        bool mOSDShowChannel;                      // Message on any channel acticity
        bool mOSDShowQuery;                        // Message on query acticity
        bool mOSDShowChannelEvent;                 // Message on channel join/part events
        QFont mOsdFont;                            // Which font to use
        bool mUseOSDCustomColors;
        bool mOSDDrawShadow;
        int mOSDDuration;
        uint mOSDScreen;
        QColor mOsdTextColor;
        QColor mOsdBackgroundColor;
        int mOSDOffsetX;
        int mOSDOffsetY;
        int mOSDAlignment;                         // 0: Left, 1: Middle, 2: Center, 3: Right

        bool mColorInputFields;

        QString mBackgroundImage;

        QStringList mButtonList;

        Konversation::ServerGroupList mServerGroupList;
        QPtrList<Ignore> mIgnoreList;
        QValueList<IdentityPtr> mIdentityList;
        QPtrList<Highlight> mHighlightList;
        QMap< QString,QMap<QString,QString> > mChannelEncodingsMap;

        bool mDisableNotifyWhileAway;

        // IRC colors
        QStringList mIrcColorList;
        QStringList mNickColorList;
        bool mFilterColors;
        bool mUseColoredNicks;
        bool mUseBoldNicks;
        bool mUseLiteralModes;

        // aliases
        QStringList mAliasList;

        //Nick completion
        int mNickCompletionMode;
        QString mPrefixCharacter;
        bool mNickCompletionCaseSensitive;

        //User interface
        bool mShowMenuBar;
        bool mShowTabBarCloseButton;
        bool mShowNicknameBox;

        bool mShowTopic;
        bool mShowRememberLineInAllWindows;
        bool mFocusNewQueries;
        bool mHideUnimportantEvents;
        bool mDisableExpansion;

        // Web Browser
        bool mWebBrowserUseKdeDefault;
        QString mWebBrowserCmd;

        bool mOpenWatchedNicksAtStartup;

        // Themes
        QString mIconTheme;
        bool mEmotIconsEnabled;
        QString mEmotIconsTheme;

        QString mWikiUrl;
        bool mExpandWikiUrl;*/
};
#endif
