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

#include <kdeversion.h>

#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

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

class ServerEntry;
class Ignore;
class Highlight;
class Identity;

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
      ServerListPage=0,
      NotifyPage
    };

    Preferences();
    ~Preferences();

    int addServer(const QString& serverString);
    void removeServer(int id);
    QString getServerByIndex(unsigned int);
    QString getServerById(int);
    ServerEntry* getServerEntryById(int id);
    int getServerIdByIndex(unsigned int);
    QValueList<int> getAutoConnectServerIDs();

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

    void clearServerList();
    void changeServerProperty(int id,int property,const QString& value);
    void updateServer(int id,const QString& newDefinition);

    void setLog(bool state);
    bool getLog();
    void setLowerLog(bool state);
    bool getLowerLog();
    void setLogFollowsNick(bool state);
    bool getLogFollowsNick();
    void setLogPath(const QString &path);
    QString getLogPath();

    void setDccAddPartner(bool state);
    bool getDccAddPartner();
    void setDccCreateFolder(bool state);
    bool getDccCreateFolder();
    void setDccAutoGet(bool state);
    bool getDccAutoGet();
    void setDccBufferSize(unsigned long size);
    unsigned long getDccBufferSize();
    void setDccPath(const QString &path);
    QString getDccPath();
    void setDccRollback(unsigned long bytes);
    unsigned long getDccRollback();

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
    QStringList getNotifyList();
    QString getNotifyString();
    void setNotifyList(const QStringList& newList);
    bool addNotify(const QString& newPattern);
    bool removeNotify(const QString& pattern);

    QPtrList<Highlight> getHilightList();
    void setHilightList(QPtrList<Highlight> newList);
    void addHilight(const QString &newHilight, QColor color, const QString& sound);

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

    void setOSDColor(const QString &color);
    QColor getOSDColor();

    QStringList getButtonList();
    void setButtonList(QStringList newList);

    void addIgnore(const QString &newIgnore);
    void clearIgnoreList();
    QPtrList<Ignore> getIgnoreList();
    void setIgnoreList(QPtrList<Ignore> newList);

    void addIdentity(Identity* identity);
    void removeIdentity(Identity* identity);
    void clearIdentityList();
    QPtrList<Identity> getIdentityList();
    Identity *getIdentityByName(const QString& name);
    Identity* identity;

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

    QFont getTextFont();
    QFont getListFont();
    void setTextFont(QFont newFont);
    void setListFont(QFont newFont);
    void setTextFontRaw(const QString &rawFont);
    void setListFontRaw(const QString &newFont);

    void setTimestamping(bool state);
    bool getTimestamping();
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

    //User interface
    bool getShowMenuBar();
    void setShowMenuBar(bool s);
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
    bool getShowToolBar();
    void setShowToolBar(bool s);
#endif
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
    bool getShowStatusBar();
    void setShowStatusBar(bool s);
#endif
#if QT_VERSION >= 0x030200
    bool getShowTabBarCloseButton();
    void setShowTabBarCloseButton(bool s);
#endif

    bool getShowTopic();
    void setShowTopic(bool s);

    bool getHideUnimportantEvents();
    void setHideUnimportantEvents(bool state);
    
    // Web Browser
    bool getWebBrowserUseKdeDefault();
    void setWebBrowserUseKdeDefault(bool state);
    QString getWebBrowserCmd();
    void setWebBrowserCmd(const QString &cmd);

  signals:
    void requestServerConnection(int number);
    void requestSaveOptions();
    void autoUserhostChanged(bool state);
    void updateTrayIcon();

  protected:
    QString getDefaultColor(const QString& name);

    bool log;
    bool lowerLog;
    bool logFollowsNick;
    QString logPath;

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
    bool dccAutoGet;
    unsigned long dccBufferSize;
    unsigned long dccRollback;  // Rollback for Resume
    QString dccPath;

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
    QString timestampFormat;

    bool showQuickButtons;
    bool showModeButtons;
    bool showServerList;
    bool showTrayIcon;
    bool trayNotify;

    QValueList<int> channelSplitter;

    QStringList notifyList;
    QString commandChar;

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

    // On Screen Display
    bool OSDUsage;            // Use OSD?
    bool OSDShowOwnNick;      // Message, if own nick appears
    bool OSDShowChannel;      // Message on any channel acticity
    bool OSDShowQuery;        // Message on query acticity
    bool OSDShowChannelEvent; // Message on channel join/part events
    QFont osdFont;            // Which font to use
    QColor osdColor;

    bool colorInputFields;
    
    QString backgroundImage;

    QStringList buttonList;

    QPtrList<ServerEntry> serverList;
    QPtrList<Ignore> ignoreList;
    QPtrList<Identity> identityList;
    QPtrList<Highlight> hilightList;

    // IRC colors
    QStringList ircColorList;
    // aliases
    QStringList aliasList;

    //Nick completion
    int nickCompletionMode;

    //User interface
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 0)
    bool showToolBar;
#endif
    bool showMenuBar;
#if KDE_VERSION < KDE_MAKE_VERSION(3, 1, 90)
    bool showStatusBar;
#endif
#if QT_VERSION >= 0x030200
    bool showTabBarCloseButton;
#endif

    bool showTopic;
    bool hideUnimportantEvents;
    
    // Web Browser
    bool webBrowserUseKdeDefault;
    QString webBrowserCmd;
};

#endif
