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

class Preferences : public QObject
{
  Q_OBJECT

  public:
    enum TabPlacement
    {
      Top=0,
      Bottom,
      Left,    // not yet supported
      Right    // not yet supported
    };

    enum Pages
    {
      NotifyPage,
      ChatWinAppearancePage
    };

    Preferences();
    ~Preferences();

    const Konversation::ServerGroupList serverGroupList();
    void setServerGroupList(const Konversation::ServerGroupList& list);
    void addServerGroup(Konversation::ServerGroupSettingsPtr serverGroup);
    const Konversation::ServerGroupSettingsPtr serverGroupById(int id);
    void removeServerGroup(int id);

    const bool getAutoReconnect();
    void setAutoReconnect(bool state);
    const unsigned int getReconnectCount();
    void setReconnectCount(unsigned int count);
    const bool getAutoRejoin();
    void setAutoRejoin(bool state);
    const bool getAutojoinOnInvite();
    void setAutojoinOnInvite(bool state);

    const bool getBeep();
    void setBeep(bool state);
    const bool getRawLog();
    void setRawLog(bool state);

    const bool getCustomVersionReplyEnabled();
    void setCustomVersionReplyEnabled(bool state);
    QString getCustomVersionReply();
    void setCustomVersionReply(const QString &reply);

    void setLog(bool state);
    const bool getLog();
    void setLowerLog(bool state);
    const bool getLowerLog();
    void setAddHostnameToLog(bool state);
    const bool getAddHostnameToLog();
    void setLogFollowsNick(bool state);
    const bool getLogFollowsNick();
    void setLogPath(const QString &path);
    const QString getLogPath();
    void setScrollbackMax(int max);
    const int getScrollbackMax();
    
    void setAutoWhoNicksLimit(int limit);
    const int getAutoWhoNicksLimit();
    void setAutoWhoContinuousEnabled(bool state);
    const bool getAutoWhoContinuousEnabled();
    void setAutoWhoContinuousInterval(int interval);
    const int getAutoWhoContinuousInterval();

    void setShowRealNames(bool show);
    const bool getShowRealNames();

    void setDccAddPartner(bool state);
    const bool getDccAddPartner();
    void setDccCreateFolder(bool state);
    const bool getDccCreateFolder();
    void setDccAutoGet(bool state);
    const bool getDccAutoGet();
    void setDccAutoResume(bool state);
    const bool getDccAutoResume();
    void setDccBufferSize(unsigned long size);
    const unsigned long getDccBufferSize();
    void setDccPath(const QString &path);
    const QString getDccPath();
    void setDccMethodToGetOwnIp(int methodId);
    const int getDccMethodToGetOwnIp();
    void setDccSpecificOwnIp(const QString& ip);
    const QString getDccSpecificOwnIp();
    void setDccSpecificSendPorts(bool state);
    const bool getDccSpecificSendPorts();
    void setDccSendPortsFirst(unsigned long port);
    const unsigned int getDccSendPortsFirst();
    void setDccSendPortsLast(unsigned long port);
    const unsigned int getDccSendPortsLast();
    void setDccSpecificChatPorts(bool state);
    const bool getDccSpecificChatPorts();
    void setDccChatPortsFirst(unsigned long port);
    const unsigned int getDccChatPortsFirst();
    void setDccChatPortsLast(unsigned long port);
    const unsigned int getDccChatPortsLast();
    void setDccFastSend(bool state);
    const bool getDccFastSend();
    void setDccSendTimeout(int sec);
    const int getDccSendTimeout();
    bool getIPv4Fallback();
    void setIPv4Fallback(bool fallback);
    const QString& getIPv4FallbackIface();
    void setIPv4FallbackIface(const QString& interface);
    
    const TabPlacement getTabPlacement();
    void setTabPlacement(TabPlacement where);
    void setBlinkingTabs(bool blink);
    const bool getBlinkingTabs();
    void setBringToFront(bool state);
    const bool getBringToFront();
    void setCloseButtonsOnTabs(bool state);
    const bool getCloseButtonsOnTabs();
    void setCloseButtonsAlignRight(bool state);
    const bool getCloseButtonsAlignRight();

    // sorting stuff
    const bool getSortByStatus();
    void setSortByStatus(bool state);
    const bool getSortCaseInsensitive();
    void setSortCaseInsensitive(bool state);
    // more sorting stuff
    const int getAdminValue();
    void setAdminValue(int value);
    const int getOwnerValue();
    void setOwnerValue(int value);
    const int getOpValue();
    void setOpValue(int value);
    const int getHalfopValue();
    void setHalfopValue(int value);
    const int getVoiceValue();
    void setVoiceValue(int value);
    const int getNoRightsValue();
    void setNoRightsValue(int value);

    // Geometry functions
    const QSize getNicksOnlineSize();
    const QSize getNicknameSize();
    const QSize getLogfileReaderSize();
    const QSize getMultilineEditSize();
    
    void setNicksOnlineSize(const QSize &newSize);
    void setNicknameSize(const QSize &newSize);
    void setLogfileReaderSize(const QSize &newSize);
    void setMultilineEditSize(const QSize &newSize);

    const int getLogfileBufferSize();
    void setLogfileBufferSize(int newSize);

    const int getNotifyDelay();
    void setNotifyDelay(int delay);
    const bool getUseNotify();
    void setUseNotify(bool use);
    const QMap<QString, QStringList> getNotifyList();
    const QStringList getNotifyListByGroup(const QString& groupName);
    const QString getNotifyStringByGroup(const QString& groupName);
    void setNotifyList(const QMap<QString, QStringList>& newList);
    const bool addNotify(const QString& groupName, const QString& newPattern);
    const bool removeNotify(const QString& groupName, const QString& pattern);

    const QPtrList<Highlight> getHighlightList();
    void setHighlightList(QPtrList<Highlight> newList);
    void addHighlight(const QString& newHighlight,bool regExp, const QColor &color,const QString& sound,const QString& autoText);
    void setHighlightSoundEnabled(bool enabled);
    const bool getHighlightSoundEnabled();

    void setHighlightNick(bool state);      // shall we highlight the current nick?
    const bool getHighlightNick();

    void setUseClickableNicks(bool state);
    const bool getUseClickableNicks();

    void setHighlightNickColor(const QString &color);
    const QColor getHighlightNickColor();

    void setHighlightOwnLines(bool state);  // shall we highlight all our own lines?
    const bool getHighlightOwnLines();

    void setHighlightOwnLinesColor(const QString &color);
    const QColor getHighlightOwnLinesColor();

    // On Screen Display
    void setOSDUsage(bool state);
    const bool getOSDUsage();

    void setOSDShowOwnNick(bool state);
    const bool getOSDShowOwnNick();

    void setOSDShowChannel(bool state);
    const bool getOSDShowChannel();

    void setOSDShowQuery(bool state);
    const bool getOSDShowQuery();

    void setOSDShowChannelEvent(bool state);
    const bool getOSDShowChannelEvent();

    const QFont getOSDFont();
    void setOSDFont(const QFont &newFont);
    void setOSDFontRaw(const QString &rawFont);

    void setOSDUseCustomColors(bool state);
    const bool getOSDUseCustomColors();
    
    void setOSDTextColor(const QString &color);
    const QColor getOSDTextColor();
    
    void setOSDBackgroundColor(const QString& color);
    const QColor getOSDBackgroundColor();
    
    void setOSDDuration(int ms);
    const int getOSDDuration();
    
    void setOSDScreen(uint screen);
    const uint getOSDScreen();
    
    void setOSDDrawShadow(bool state);
    const bool getOSDDrawShadow();
    
    void setOSDOffsetX(int offset);
    const int getOSDOffsetX();
    
    void setOSDOffsetY(int offset);
    const int getOSDOffsetY();
    
    void setOSDAlignment(int alignment);
    const int getOSDAlignment();

    const QStringList getButtonList();
    void setButtonList(QStringList newList);

    void addIgnore(const QString &newIgnore);
    void clearIgnoreList();
    const QPtrList<Ignore> getIgnoreList();
    void setIgnoreList(QPtrList<Ignore> newList);

    void addIdentity(IdentityPtr identity);
    void removeIdentity(IdentityPtr identity);
    void clearIdentityList();
    const QValueList<IdentityPtr> getIdentityList();
    void setIdentityList(const QValueList<IdentityPtr>& list);
    const IdentityPtr getIdentityByName(const QString& name);
    const IdentityPtr getIdentityById(int id);
    IdentityPtr identity;

    const QString getIdent();
    void setIdent(const QString &ident);

    const QString getRealName();
    void setRealName(const QString &name);

    const QString getPartReason();
    void setPartReason(const QString &newReason);

    const QString getKickReason();
    void setKickReason(const QString &newReason);

    void setShowAwayMessage(bool state);
    const bool getShowAwayMessage();
    const QString getAwayMessage();
    void setAwayMessage(const QString &newMessage);
    const QString getUnAwayMessage();
    void setUnAwayMessage(const QString &newMessage);

    const QString getNickname(int index);
    const QStringList getNicknameList();
    void setNickname(int index,const QString &newName);
    void setNicknameList(const QStringList &newList);

    const QString getColor(const QString& name);
    void setColor(const QString& name,const QString& color);
    

    // Colored nicknames
    const bool getUseColoredNicks();
    void setUseColoredNicks(bool usecolor);

    const QStringList getNickColorList();
    void setNickColorList(const QStringList &cl);

    const bool getUseBoldNicks();
    void setUseBoldNicks(bool boldNicks);

    const bool getUseLiteralModes();
    void setUseLiteralModes(bool literalModes);
    
    const bool getFilterColors();
    void setFilterColors(bool filter);

    
    const bool getColorInputFields();
    void setColorInputFields(bool state);

    const QString& getBackgroundImageName();
    void setBackgroundImageName(const QString& name);

    void setNickCompleteSuffixStart(const QString &suffix);
    void setNickCompleteSuffixMiddle(const QString &suffix);
    const QString getNickCompleteSuffixStart();
    const QString getNickCompleteSuffixMiddle();

    const bool getFixedMOTD();
    void setFixedMOTD(bool fixed);

    void setCommandChar(const QString &newCommandChar);
    const QString getCommandChar();

    const QFont getTextFont();
    const QFont getListFont();
    void setTextFont(const QFont &newFont);
    void setListFont(const QFont &newFont);
    void setTextFontRaw(const QString &rawFont);
    void setListFontRaw(const QString &newFont);

    void setTimestamping(bool state);
    const bool getTimestamping();
    void setShowDate(bool state);
    const bool getShowDate();
    void setTimestampFormat(const QString& newFormat);
    const QString getTimestampFormat();

    void setShowQuickButtons(bool state);
    const bool getShowQuickButtons();

    void setShowModeButtons(bool state);
    const bool getShowModeButtons();

    void setShowServerList(bool state);
    const bool getShowServerList();

    void setShowTrayIcon(bool state);
    const bool getShowTrayIcon();
    
    void setSystrayOnly(bool state);
    const bool getSystrayOnly();

    void setShowBackgroundImage(bool state);
    const bool getShowBackgroundImage();
    
    void setTrayNotify(bool state);
    const bool getTrayNotify() const;
    void setTrayNotifyOnlyOwnNick(bool onlyOwnNick);
    bool trayNotifyOnlyOwnNick() const;

    void setChannelSplitterSizes(QValueList<int> sizes);
    const QValueList<int> channelSplitterSizes();

    void setTopicSplitterSizes(QValueList<int> sizes);
    const QValueList<int> topicSplitterSizes();

    void setChannelDoubleClickAction(const QString &action);
    const QString getChannelDoubleClickAction();

    void setNotifyDoubleClickAction(const QString &action);
    const QString getNotifyDoubleClickAction();

    void setUseSpacing(bool state);
    const bool getUseSpacing();
    void setSpacing(int newSpacing);
    const int getSpacing();
    void setMargin(int newMargin);
    const int getMargin();

    void setUseParagraphSpacing(bool state);
    const bool getUseParagraphSpacing();
    void setParagraphSpacing(int newSpacing);
    const int getParagraphSpacing();

    void setAutoUserhost(bool state);
    const bool getAutoUserhost();

    const bool getDialogFlag(const QString& flagName);
    void setDialogFlag(const QString& flagName,bool state);

    const int getMaximumLagTime();
    void setMaximumLagTime(int maxLag);

    // IRC colors
    const QStringList getIRCColorList();
    void setIRCColorList(const QStringList &cl);

    // aliases
    const QStringList getAliasList();
    void setAliasList(const QStringList &aliasList);

    // Nick completion
    const int getNickCompletionMode();
    void setNickCompletionMode(int mode);
    const QString getPrefixCharacter();
    void  setPrefixCharacter(const QString &prefix);
    const bool nickCompletionCaseSensitive() const;
    void setNickCompletionCaseSensitive(bool caseSensitive);

    //User interface
    const bool getShowMenuBar();
    void setShowMenuBar(bool s);
    const bool getShowTabBarCloseButton();
    void setShowTabBarCloseButton(bool s);

    const bool getShowTopic();
    void setShowTopic(bool s);
    
    const bool getShowRememberLineInAllWindows();
    void setShowRememberLineInAllWindows(bool s);

    const bool getFocusNewQueries();
    void setFocusNewQueries(bool s);
    
    const bool getHideUnimportantEvents();
    void setHideUnimportantEvents(bool state);
    
    const bool getDisableExpansion();
    void setDisableExpansion(bool state);
    
    // Web Browser
    const bool getWebBrowserUseKdeDefault();
    void setWebBrowserUseKdeDefault(bool state);
    const QString getWebBrowserCmd();
    void setWebBrowserCmd(const QString &cmd);
    
    const bool getRedirectToStatusPane();
    void setRedirectToStatusPane(bool redirect);
    
    const bool getOpenWatchedNicksAtStartup();
    void setOpenWatchedNicksAtStartup(bool open);
    
    const QString getChannelEncoding(const QString& server,const QString& channel);
    void setChannelEncoding(const QString& server,const QString& channel,const QString& encoding);
    const QStringList getChannelEncodingsServerList();
    const QStringList getChannelEncodingsChannelList(const QString& server);

    // Themes
    void setIconTheme(const QString& name);
    const QString getIconTheme();

    void setEmotIconsEnabled(bool enabled) { m_emotIconsEnabled = enabled; }
    bool emotIconsEnabled() const { return m_emotIconsEnabled; }
    void setEmotIconsTheme(const QString& theme) { m_emotIconsTheme = theme; }
    QString emotIconsTheme() const { return m_emotIconsTheme; }

    bool showNicknameBox() const;
    void setShowNicknameBox(bool show);
    
    bool disableNotifyWhileAway() const { return m_disableNotifyWhileAway; }
    void setDisableNotifyWhileAway(bool disable) { m_disableNotifyWhileAway = disable; }

  signals:
    void requestServerConnection(int number);
    void requestSaveOptions();
    void autoUserhostChanged(bool state);
    void autoContinuousWhoChanged();
    void updateTrayIcon();

  protected:
    const QString getDefaultColor(const QString& name);

    bool log;
    bool lowerLog;
    bool addHostnameToLog;
    bool logFollowsNick;
    QString logPath;
    int scrollbackMax;
    
    int autoWhoNicksLimit;
    bool autoWhoContinuousEnabled;
    int autoWhoContinuousInterval;

    bool showRealNames;

    TabPlacement tabPlacement;   // where do the tabs go?
    bool blinkingTabs;           // Do we want the LEDs on the tabs to blink?
    bool closeButtonsOnTabs;     // Do we want close widgets on the tabs?
    bool closeButtonsAlignRight; // Display close widgets on the right side?
    bool bringToFront;           // Do we want to see newly created tabs immediately?

    bool fixedMOTD;
    bool beep;
    bool rawLog;

    bool customVersionReplyEnabled;
    QString customVersionReply;

    bool dccAddPartner;
    bool dccCreateFolder;   // create folders for each DCC partner?
    int dccMethodToGetOwnIp;
    QString dccSpecificOwnIp;
    bool dccSpecificSendPorts;
    unsigned long dccSendPortsFirst;
    unsigned long dccSendPortsLast;
    bool dccSpecificChatPorts;
    unsigned long dccChatPortsFirst;
    unsigned long dccChatPortsLast;
    bool dccAutoGet;
    bool dccAutoResume;
    unsigned long dccBufferSize;
    QString dccPath;
    bool dccFastSend;
    int dccSendTimeout;
    bool ipv4Fallback;
    QString ipv4Interface;

    bool autoReconnect;
    unsigned int reconnectCounter;
    bool autoRejoin;
    bool autojoinOnInvite;

    int maximumLag;     // ask for reconnect

    int notifyDelay;
    bool useNotify;

    bool timestamping;
    bool showDate;
    QString timestampFormat;

    bool showQuickButtons;
    bool showModeButtons;
    bool showServerList;
    bool showTrayIcon;
    bool systrayOnly;
    bool showBackgroundImage;
    bool trayNotify;
    bool m_trayNotifyOnlyOwnNick;

    QValueList<int> channelSplitter;
    QValueList<int> m_topicSplitterSizes;

    QMap<QString, QStringList> notifyList;
    QString commandChar;
    QString preShellCommandStr;

    QString nickCompleteSuffixStart;
    QString nickCompleteSuffixMiddle;

    QString channelDoubleClickAction;
    QString notifyDoubleClickAction;

    // Geometries
    QSize mainWindowSize;
    QSize nicksOnlineSize;
    QSize nicknameSize;
    QSize logfileReaderSize;
    QSize colorConfigurationSize;
    QSize multilineEditSize;

    int logfileBufferSize;
    
    bool useSpacing;
    int spacing;
    int margin;

    // sorting stuff
    bool sortByStatus;
    bool sortCaseInsensitive;

    // more sorting stuff
    int adminValue;
    int ownerValue;
    int opValue;
    int halfopValue;
    int voiceValue;
    int awayValue;
    int noRightsValue;

    // flag for hostmasks next to nicknames
    bool autoUserhost;

    bool useParagraphSpacing;
    int paragraphSpacing;

    QFont textFont;
    QFont listFont;

    bool highlightNick;
    bool highlightOwnLines;
    QColor highlightNickColor;
    QColor highlightOwnLinesColor;
    bool highlightSoundEnabled;

    bool clickableNicks;

    // On Screen Display
    bool OSDUsage;            // Use OSD?
    bool OSDShowOwnNick;      // Message, if own nick appears
    bool OSDShowChannel;      // Message on any channel acticity
    bool OSDShowQuery;        // Message on query acticity
    bool OSDShowChannelEvent; // Message on channel join/part events
    QFont osdFont;            // Which font to use
    bool useOSDCustomColors;
    bool OSDDrawShadow;
    int OSDDuration;
    uint OSDScreen;
    QColor osdTextColor;
    QColor osdBackgroundColor;
    int OSDOffsetX;
    int OSDOffsetY;
    int OSDAlignment;         // 0: Left, 1: Middle, 2: Center, 3: Right

    bool colorInputFields;
    
    QString backgroundImage;

    QStringList buttonList;

    Konversation::ServerGroupList m_serverGroupList;
    QPtrList<Ignore> ignoreList;
    QValueList<IdentityPtr> identityList;
    QPtrList<Highlight> highlightList;
    QMap< QString,QMap<QString,QString> > channelEncodingsMap;
    
    bool m_disableNotifyWhileAway;

    // IRC colors
    QStringList ircColorList;
    QStringList nickColorList;
    bool filterColors;
    bool useColoredNicks;
    bool useBoldNicks;
    bool useLiteralModes;

    // aliases
    QStringList aliasList;

    //Nick completion
    int nickCompletionMode;
    QString prefixCharacter;
    bool m_nickCompletionCaseSensitive;

    //User interface
    bool showMenuBar;
    bool showTabBarCloseButton;
    bool m_showNicknameBox;

    bool showTopic;
    bool showRememberLineInAllWindows;
    bool focusNewQueries;
    bool hideUnimportantEvents;
    bool disableExpansion;
    
    // Web Browser
    bool webBrowserUseKdeDefault;
    QString webBrowserCmd;
    
    bool redirectToStatusPane;
    
    bool m_openWatchedNicksAtStartup;

    // Themes
    QString iconTheme;
    bool m_emotIconsEnabled;
    QString m_emotIconsTheme;
};

#endif
