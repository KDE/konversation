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

  $Id$
*/

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <qobject.h>
#include <qlist.h>
#include <qsize.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qfont.h>

#include "serverentry.h"
#include "ignore.h"
#include "highlight.h"
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

    Preferences();
    ~Preferences();

    int mainWindowToolBarPos;
    int mainWindowToolBarStatus;
    int mainWindowToolBarIconText;
    int mainWindowToolBarIconSize;
    bool mainWindowStatusBarStatus;
    bool mainWindowMenuBarStatus;

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

    // sorting stuff
    bool getSortByStatus();
    void setSortByStatus(bool state);
    bool getSortCaseInsensitive();
    void setSortCaseInsensitive(bool state);
    // more sorting stuff
    int getOpValue();
    void setOpValue(int value);
    int getVoiceValue();
    void setVoiceValue(int value);
    int getNoRightsValue();
    void setNoRightsValue(int value);

    // Geometry functions
    QSize getMainWindowSize();
    QSize getHilightSize();
    QSize getButtonsSize();
    QSize getIgnoreSize();
    QSize getNotifySize();
    QSize getNicksOnlineSize();
    QSize getNicknameSize();
    void setMainWindowSize(QSize newSize);
    void setHilightSize(QSize newSize);
    void setButtonsSize(QSize newSize);
    void setIgnoreSize(QSize newSize);
    void setNotifySize(QSize newSize);
    void setNicksOnlineSize(QSize newSize);
    void setNicknameSize(QSize newSize);

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
    void addHilight(const QString &newHilight, QColor color);

    void setHilightNick(bool state);      // shall we hilight the current nick?
    bool getHilightNick();

    void setHilightNickColor(const QString &color);
    QColor getHilightNickColor();

    void setHilightOwnLines(bool state);  // shall we hilight all our own lines?
    bool getHilightOwnLines();

    void setHilightOwnLinesColor(const QString &color);
    QColor getHilightOwnLinesColor();

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
    const Identity *getIdentityByName(const QString& name);

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

    void setCodec(const QString &newCodec);
    QString getCodec();

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

  signals:
    void requestServerConnection(int number);
    void requestSaveOptions();
    void autoUserhostChanged(bool state);

  protected:
    QString getDefaultColor(const QString& name);

    bool log;
    bool lowerLog;
    bool logFollowsNick;
    QString logPath;

    TabPlacement tabPlacement; // where do the tabs go?
    bool blinkingTabs;         // Do we want the LEDs on the tabs to blink?
    bool closeButtonsOnTabs;   // Do we want close widgets on the tabs?
    bool bringToFront;         // Do we want to see newly created tabs immediately?

    bool fixedMOTD;
    bool beep;
    bool rawLog;

    bool dccAddPartner;
    bool dccCreateFolder;   // create folders for each DCC partner?
    bool dccAutoGet;
    unsigned long dccBufferSize;
    unsigned long dccRollback;  // Rollback for Resume
    QString dccPath;

    bool autoReconnect;
    bool autoRejoin;
    bool autojoinOnInvite;

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

    QValueList<int> channelSplitter;

    QStringList notifyList;
    QString commandChar;

    QString nickCompleteSuffixStart;
    QString nickCompleteSuffixMiddle;

    QString channelDoubleClickAction;
    QString notifyDoubleClickAction;

    // Geometries
    QSize mainWindowSize;
    QSize buttonsSize;
    QSize hilightSize;
    QSize ignoreSize;
    QSize notifySize;
    QSize nicksOnlineSize;
    QSize nicknameSize;
    QSize colorConfigurationSize;

    bool useSpacing;
    int spacing;
    int margin;

    // sorting stuff
    bool sortByStatus;
    bool sortCaseInsensitive;

    // more sorting stuff
    int opValue;
    int voiceValue;
    int noRightsValue;

    // flag for hostmasks next to nicknames
    bool autoUserhost;

    bool useParagraphSpacing;
    int paragraphSpacing;

    QFont textFont;
    QFont listFont;

    QPtrList<ServerEntry> serverList;
    QPtrList<Highlight> hilightList;

    bool hilightNick;
    bool hilightOwnLines;
    QColor hilightNickColor;
    QColor hilightOwnLinesColor;

    QString backgroundImage;

    QStringList buttonList;

    QPtrList<Ignore> ignoreList;

    QPtrList<Identity> identityList;

    QString codec;
};

#endif
