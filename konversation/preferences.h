/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  preferences.h  -  Class for application wide preferences
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

    Konversation::ServerGroupList serverGroupList();
    void setServerGroupList(const Konversation::ServerGroupList& list);
    void addServerGroup(const Konversation::ServerGroupSettings& serverGroup);
    Konversation::ServerGroupSettings serverGroupById(int id);
    void removeServerGroup(int id);

    bool getAutoReconnect();
    void setAutoReconnect(bool state);
    bool getAutoRejoin();
    void setAutoRejoin(bool state);
    bool getAutojoinOnInvite();
    void setAutojoinOnInvite(bool state);

    bool getBeep();
    void setBeep(bool state);
    bool getRawLog();
    void setRawLog(bool state);

    QString getVersionReply();
    void    setVersionReply(QString reply);

    void setLog(bool state);
    bool getLog();
    void setLowerLog(bool state);
    bool getLowerLog();
    void setLogFollowsNick(bool state);
    bool getLogFollowsNick();
    void setLogPath(const QString &path);
    QString getLogPath();
    void setScrollbackMax(int max);
    int getScrollbackMax();
    
    void setAutoWhoNicksLimit(int limit);
    int getAutoWhoNicksLimit();
    void setAutoWhoContinuousEnabled(bool state);
    bool getAutoWhoContinuousEnabled();
    void setAutoWhoContinuousInterval(int interval);
    int getAutoWhoContinuousInterval();

    void setDccAddPartner(bool state);
    bool getDccAddPartner();
    void setDccCreateFolder(bool state);
    bool getDccCreateFolder();
    void setDccAutoGet(bool state);
    bool getDccAutoGet();
    void setDccAutoResume(bool state);
    bool getDccAutoResume();
    void setDccBufferSize(unsigned long size);
    unsigned long getDccBufferSize();
    void setDccPath(const QString &path);
    QString getDccPath();
    void setDccMethodToGetOwnIp(int methodId);
    int getDccMethodToGetOwnIp();
    void setDccSpecificOwnIp(const QString& ip);
    QString getDccSpecificOwnIp();
    void setDccSpecificSendPorts(bool state);
    bool getDccSpecificSendPorts();
    void setDccSendPortsFirst(unsigned long port);
    unsigned int getDccSendPortsFirst();
    void setDccSendPortsLast(unsigned long port);
    unsigned int getDccSendPortsLast();
    void setDccSpecificChatPorts(bool state);
    bool getDccSpecificChatPorts();
    void setDccChatPortsFirst(unsigned long port);
    unsigned int getDccChatPortsFirst();
    void setDccChatPortsLast(unsigned long port);
    unsigned int getDccChatPortsLast();
    void setDccFastSend(bool state);
    bool getDccFastSend();
    void setDccSendTimeout(int sec);
    int getDccSendTimeout();
    
    TabPlacement getTabPlacement();
    void setTabPlacement(TabPlacement where);
    void setBlinkingTabs(bool blink);
    bool getBlinkingTabs();
    void setBringToFront(bool state);
    bool getBringToFront();
    void setCloseButtonsOnTabs(bool state);
    bool getCloseButtonsOnTabs();
    void setCloseButtonsAlignRight(bool state);
    bool getCloseButtonsAlignRight();

    // sorting stuff
    bool getSortByStatus();
    void setSortByStatus(bool state);
    bool getSortCaseInsensitive();
    void setSortCaseInsensitive(bool state);
    // more sorting stuff
    int getAdminValue();
    void setAdminValue(int value);
    int getOwnerValue();
    void setOwnerValue(int value);
    int getOpValue();
    void setOpValue(int value);
    int getHalfopValue();
    void setHalfopValue(int value);
    int getVoiceValue();
    void setVoiceValue(int value);
    int getNoRightsValue();
    void setNoRightsValue(int value);

    // Geometry functions
    QSize getNicksOnlineSize();
    QSize getNicknameSize();
    QSize getLogfileReaderSize();
    QSize getMultilineEditSize();
    
    void setNicksOnlineSize(QSize newSize);
    void setNicknameSize(QSize newSize);
    void setLogfileReaderSize(QSize newSize);
    void setMultilineEditSize(QSize newSize);

    int getLogfileBufferSize();
    void setLogfileBufferSize(int newSize);

    int getNotifyDelay();
    void setNotifyDelay(int delay);
    bool getUseNotify();
    void setUseNotify(bool use);
    QMap<QString, QStringList> getNotifyList();
    QStringList getNotifyListByGroup(const QString& groupName);
    QString getNotifyStringByGroup(const QString& groupName);
    void setNotifyList(const QMap<QString, QStringList>& newList);
    bool addNotify(const QString& groupName, const QString& newPattern);
    bool removeNotify(const QString& groupName, const QString& pattern);

    QPtrList<Highlight> getHilightList();
    void setHilightList(QPtrList<Highlight> newList);
    void addHilight(const QString& newHilight,bool regExp,QColor color,const QString& sound,const QString& autoText);
    void setHilightSoundEnabled(bool enabled);
    bool getHilightSoundEnabled();

    void setHilightNick(bool state);      // shall we hilight the current nick?
    bool getHilightNick();

    void setHilightNickColor(const QString &color);
    QColor getHilightNickColor();

    void setHilightOwnLines(bool state);  // shall we hilight all our own lines?
    bool getHilightOwnLines();

    void setHilightOwnLinesColor(const QString &color);
    QColor getHilightOwnLinesColor();

    // On Screen Display
    void setOSDUsage(bool state);
    bool getOSDUsage();

    void setOSDShowOwnNick(bool state);
    bool getOSDShowOwnNick();

    void setOSDShowChannel(bool state);
    bool getOSDShowChannel();

    void setOSDShowQuery(bool state);
    bool getOSDShowQuery();

    void setOSDShowChannelEvent(bool state);
    bool getOSDShowChannelEvent();

    QFont getOSDFont();
    void setOSDFont(QFont newFont);
    void setOSDFontRaw(const QString &rawFont);

    void setOSDUseCustomColors(bool state);
    bool getOSDUseCustomColors();
    
    void setOSDTextColor(const QString &color);
    QColor getOSDTextColor();
    
    void setOSDBackgroundColor(const QString& color);
    QColor getOSDBackgroundColor();
    
    void setOSDDuration(int ms);
    int getOSDDuration();
    
    void setOSDScreen(uint screen);
    uint getOSDScreen();
    
    void setOSDDrawShadow(bool state);
    bool getOSDDrawShadow();
    
    void setOSDOffsetX(int offset);
    int getOSDOffsetX();
    
    void setOSDOffsetY(int offset);
    int getOSDOffsetY();
    
    void setOSDAlignment(int alignment);
    int getOSDAlignment();

    QStringList getButtonList();
    void setButtonList(QStringList newList);

    void addIgnore(const QString &newIgnore);
    void clearIgnoreList();
    QPtrList<Ignore> getIgnoreList();
    void setIgnoreList(QPtrList<Ignore> newList);

    void addIdentity(IdentityPtr identity);
    void removeIdentity(IdentityPtr identity);
    void clearIdentityList();
    QValueList<IdentityPtr> getIdentityList();
    void setIdentityList(const QValueList<IdentityPtr>& list);
    IdentityPtr getIdentityByName(const QString& name);
    IdentityPtr getIdentityById(int id);
    IdentityPtr identity;

    QString getIdent();
    void setIdent(const QString &ident);

    QString getRealName();
    void setRealName(const QString &name);

    QString getPartReason();
    void setPartReason(const QString &newReason);

    QString getKickReason();
    void setKickReason(const QString &newReason);

    void setShowAwayMessage(bool state);
    bool getShowAwayMessage();
    QString getAwayMessage();
    void setAwayMessage(const QString &newMessage);
    QString getUnAwayMessage();
    void setUnAwayMessage(const QString &newMessage);

    QString getNickname(int index);
    QStringList getNicknameList();
    void setNickname(int index,const QString &newName);
    void setNicknameList(const QStringList &newList);

    QString getColor(const QString& name);
    void setColor(const QString& name,const QString& color);
    
    bool getFilterColors();
    void setFilterColors(bool filter);

    
    bool getColorInputFields();
    void setColorInputFields(bool state);

    const QString& getBackgroundImageName();
    void setBackgroundImageName(const QString& name);

    void setNickCompleteSuffixStart(const QString &suffix);
    void setNickCompleteSuffixMiddle(const QString &suffix);
    QString getNickCompleteSuffixStart();
    QString getNickCompleteSuffixMiddle();

    int getOpLedColor();
    int getVoiceLedColor();
    int getNoRightsLedColor();
    void setOpLedColor(int color);
    void setVoiceLedColor(int color);
    void setNoRightsLedColor(int color);

    bool getFixedMOTD();
    void setFixedMOTD(bool fixed);

    void setCommandChar(const QString &newCommandChar);
    QString getCommandChar();
    void setPreShellCommand(const QString &command);
    QString getPreShellCommand();

    QFont getTextFont();
    QFont getListFont();
    void setTextFont(QFont newFont);
    void setListFont(QFont newFont);
    void setTextFontRaw(const QString &rawFont);
    void setListFontRaw(const QString &newFont);

    void setTimestamping(bool state);
    bool getTimestamping();
    void setShowDate(bool state);
    bool getShowDate();
    void setTimestampFormat(const QString& newFormat);
    QString getTimestampFormat();

    void setShowQuickButtons(bool state);
    bool getShowQuickButtons();

    void setShowModeButtons(bool state);
    bool getShowModeButtons();

    void setShowServerList(bool state);
    bool getShowServerList();

    void setShowTrayIcon(bool state);
    bool getShowTrayIcon();
    
    void setSystrayOnly(bool state);
    bool getSystrayOnly();

    void setShowBackgroundImage(bool state);
    bool getShowBackgroundImage();
    
    void setTrayNotify(bool state);
    bool getTrayNotify();

    void setChannelSplitter(QValueList<int> sizes);
    QValueList<int> getChannelSplitter();

    void setChannelDoubleClickAction(const QString &action);
    QString getChannelDoubleClickAction();

    void setNotifyDoubleClickAction(const QString &action);
    QString getNotifyDoubleClickAction();

    void setUseSpacing(bool state);
    bool getUseSpacing();
    void setSpacing(int newSpacing);
    int getSpacing();
    void setMargin(int newMargin);
    int getMargin();

    void setUseParagraphSpacing(bool state);
    bool getUseParagraphSpacing();
    void setParagraphSpacing(int newSpacing);
    int getParagraphSpacing();

    void setAutoUserhost(bool state);
    bool getAutoUserhost();

    bool getDialogFlag(const QString& flagName);
    void setDialogFlag(const QString& flagName,bool state);

    int getMaximumLagTime();
    void setMaximumLagTime(int maxLag);

    // IRC colors
    QStringList getIRCColorList();
    void setIRCColorList(QStringList cl);

    // aliases
    QStringList getAliasList();
    void setAliasList(QStringList aliasList);

    // Nick completion
    int getNickCompletionMode();
    void setNickCompletionMode(int mode);
    QString getPrefixCharacter();
    void  setPrefixCharacter(QString prefix);

    //User interface
    bool getShowMenuBar();
    void setShowMenuBar(bool s);
    bool getShowTabBarCloseButton();
    void setShowTabBarCloseButton(bool s);

    bool getShowTopic();
    void setShowTopic(bool s);
    
    bool getShowRememberLineInAllWindows();
    void setShowRememberLineInAllWindows(bool s);

    bool getFocusNewQueries();
    void setFocusNewQueries(bool s);
    
    bool getHideUnimportantEvents();
    void setHideUnimportantEvents(bool state);
    
    bool getDisableExpansion();
    void setDisableExpansion(bool state);
    
    // Web Browser
    bool getWebBrowserUseKdeDefault();
    void setWebBrowserUseKdeDefault(bool state);
    QString getWebBrowserCmd();
    void setWebBrowserCmd(const QString &cmd);
    
    bool getRedirectToStatusPane();
    void setRedirectToStatusPane(bool redirect);
    
    bool getOpenWatchedNicksAtStartup();
    void setOpenWatchedNicksAtStartup(bool open);
    
    QString getChannelEncoding(const QString& server,const QString& channel);
    void setChannelEncoding(const QString& server,const QString& channel,const QString& encoding);
    QStringList getChannelEncodingsServerList();
    QStringList getChannelEncodingsChannelList(const QString& server);

  signals:
    void requestServerConnection(int number);
    void requestSaveOptions();
    void autoUserhostChanged(bool state);
    void autoContinuousWhoChanged();
    void updateTrayIcon();

  protected:
    QString getDefaultColor(const QString& name);

    bool log;
    bool lowerLog;
    bool logFollowsNick;
    QString logPath;
    int scrollbackMax;
    
    int autoWhoNicksLimit;
    bool autoWhoContinuousEnabled;
    int autoWhoContinuousInterval;

    TabPlacement tabPlacement;   // where do the tabs go?
    bool blinkingTabs;           // Do we want the LEDs on the tabs to blink?
    bool closeButtonsOnTabs;     // Do we want close widgets on the tabs?
    bool closeButtonsAlignRight; // Display close widgets on the right side?
    bool bringToFront;           // Do we want to see newly created tabs immediately?

    bool fixedMOTD;
    bool beep;
    bool rawLog;

    QString versionReply;

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

    bool autoReconnect;
    bool autoRejoin;
    bool autojoinOnInvite;

    int maximumLag;     // ask for reconnect

    int notifyDelay;
    int opLedColor;
    int voiceLedColor;
    int noRightsLedColor;
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

    QValueList<int> channelSplitter;

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
    int noRightsValue;

    // flag for hostmasks next to nicknames
    bool autoUserhost;

    bool useParagraphSpacing;
    int paragraphSpacing;

    QFont textFont;
    QFont listFont;

    bool hilightNick;
    bool hilightOwnLines;
    QColor hilightNickColor;
    QColor hilightOwnLinesColor;
    bool hilightSoundEnabled;

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
    QPtrList<Highlight> hilightList;
    QMap< QString,QMap<QString,QString> > channelEncodingsMap;

    // IRC colors
    QStringList ircColorList;
    bool filterColors;
    // aliases
    QStringList aliasList;

    //Nick completion
    int nickCompletionMode;
    QString prefixCharacter;

    //User interface
    bool showMenuBar;
    bool showTabBarCloseButton;

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
};

#endif
