/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationapplication.cpp  -  The main application
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qtextcodec.h>
#include <qregexp.h>

#include <kdebug.h>
#include <kconfig.h>
#include <dcopclient.h>

#include "konversationapplication.h"
#include "konvdcop.h"
#include "konversationmainwindow.h"
#include "prefsdialog.h"
#include "highlight.h"
#include "server.h"
#include "serverentry.h"

// include static variables
Preferences KonversationApplication::preferences;

KonversationApplication::KonversationApplication()
{
  kdDebug() << "KonversationApplication::KonversationApplication()" << endl;

  prefsDialog=0;

  preferences.setTextFont(font());
  preferences.setListFont(font());

  readOptions();

#if QT_VERSION >= 0x030100
  // Setup system codec
  // TODO: check if this works now as intended
  QTextCodec::setCodecForCStrings(QTextCodec::codecForLocale());
#endif

  // open main window
  mainWindow=new KonversationMainWindow();
  connect(mainWindow,SIGNAL (openPrefsDialog()),this,SLOT (openPrefsDialog()) );
  connect(mainWindow,SIGNAL (openPrefsDialog(Preferences::Pages)),this,SLOT (openPrefsDialog(Preferences::Pages)) );
  connect(&preferences,SIGNAL (updateTrayIcon()),mainWindow,SLOT (updateTrayIcon()) );

  // handle autoconnect on startup
  QValueList<int> list=preferences.getAutoConnectServerIDs();
  // if there is at least one autoconnect server, start connecting right away
  if(list.count())
  {
    for(unsigned int index=0;index<list.count();index++) connectToServer(list[index]);
    // maybe the user wants to see the prefs dialog anyway
    if(preferences.getShowServerList())
    {
      openPrefsDialog();
    }
  }
  // no autoconnect server, so show the prefs dialog (with exit functionality)
  else
  {
    prefsDialog=new PrefsDialog(&preferences,true);

    connect(prefsDialog,SIGNAL (connectToServer(int)),this,SLOT (connectToServer(int)) );
    connect(prefsDialog,SIGNAL (cancelClicked()),this,SLOT (quitKonversation()) );
    connect(prefsDialog,SIGNAL (prefsChanged()),this,SLOT (saveOptions()) );

    prefsDialog->show();

    connect(&preferences,SIGNAL (requestServerConnection(int)),this,SLOT (connectToAnotherServer(int)) );
    connect(&preferences,SIGNAL (requestSaveOptions()),this,SLOT (saveOptions()) );
  }
  // prepare dcop interface
  dcopObject=new KonvDCOP;
  (void)new KonvIdentDCOP;
  (void)new KonvPrefsDCOP;
  if(dcopObject)
  {
    connect(dcopObject,SIGNAL (dcopSay(const QString&,const QString&,const QString&)),
                    this,SLOT (dcopSay(const QString&,const QString&,const QString&)) );
    connect(dcopObject,SIGNAL (dcopInfo(const QString&)),
                    this,SLOT (dcopInfo(const QString&)) );
  }
}

KonversationApplication::~KonversationApplication()
{
  kdDebug() << "KonversationApplication::~KonversationApplication()" << endl;
  saveOptions(false);

  if(dcopObject) delete dcopObject;
}

void KonversationApplication::dcopSay(const QString& server,const QString& target,const QString& command)
{
  Server* lookServer=serverList.first();
  while(lookServer)
  {
    if(lookServer->getServerName()==server)
    {
      lookServer->dcopSay(target,command);
      break; // leave while loop
    }
    lookServer=serverList.next();
  }
}

void KonversationApplication::dcopInfo(const QString& string)
{
  Server* lookServer=serverList.first();
  if(lookServer) lookServer->dcopInfo(string);
}

void KonversationApplication::connectToServer(int id)
{
  kdDebug() << "KonversationApplication::connectToServer(" << id << ")" << endl;

  connectToAnotherServer(id);
  // to prevent doubleClicked() to crash the dialog
  // FIXME: Seems to have a race, though
  // only close the dialog when we didn't use autoconnect
  if(prefsDialog) prefsDialog->delayedDestruct();
  prefsDialog=0;
}

void KonversationApplication::connectToAnotherServer(int id)
{
  kdDebug() << "KonversationApplication::connectToAnotherServer(" << id << ")" << endl;

  mainWindow->show();

  ServerEntry* chosenServer=preferences.getServerEntryById(id);

  // Check if a server window with same name and port is already open
  Server* newServer=serverList.first();
  while(newServer)
  {
    if(chosenServer->getServerName()==newServer->getServerName() &&
       chosenServer->getPort()==newServer->getPort() &&
       chosenServer->getIdentity()==newServer->getIdentity()->getName())
    {
      QString autoJoinChannel=chosenServer->getChannelName();

      if(newServer->isConnected())
      {
        if(!autoJoinChannel.isEmpty())
          newServer->queue("JOIN "+autoJoinChannel+" "+chosenServer->getChannelKey());
      }
      else
      {
        if(!autoJoinChannel.isEmpty())
        {
          newServer->setAutoJoin(true);
          newServer->setAutoJoinChannel(newServer->getAutoJoinChannel()+" "+autoJoinChannel);
          newServer->setAutoJoinChannelKey(newServer->getAutoJoinChannelKey()+" "+chosenServer->getChannelKey());
        }
        else newServer->setAutoJoin(false);

        newServer->connectToIRCServer();
      }
      return;
    }

    newServer=serverList.next();
  } // endwhile
  // We came this far, so generate a new server

  newServer=new Server(mainWindow,id);

  connect(mainWindow,SIGNAL (startNotifyTimer(int)),newServer,SLOT (startNotifyTimer(int)) );
  connect(mainWindow,SIGNAL (quitServer()),newServer,SLOT (quitServer()) );

  connect(newServer,SIGNAL (nicksNowOnline(Server*,const QStringList&)),mainWindow,SLOT (setOnlineList(Server*,const QStringList&)) );

  connect(newServer,SIGNAL (deleted(Server*)),this,SLOT (removeServer(Server*)) );

  serverList.append(newServer);
}

Server* KonversationApplication::getServerByName(const QString& name)
{
  Server* lookServer=serverList.first();
  while(lookServer)
  {
    if(lookServer->getServerName()==name) return lookServer;
    lookServer=serverList.next();
  }
  return 0;
}

void KonversationApplication::removeServer(Server* server)
{
  kdDebug() << "KonversationApplication::removeServer()" << endl;

  serverList.setAutoDelete(false);     // don't delete items when they are removed
  if(!serverList.remove(server))
    kdDebug() << "Could not remove " << server->getServerName() << endl;
}

void KonversationApplication::quitKonversation()
{
  kdDebug() << "KonversationApplication::quitKonversation()" << endl;

  if(prefsDialog) delete prefsDialog;
  prefsDialog=0;

  this->exit();
}

void KonversationApplication::readOptions()
{
  kdDebug() << "KonversationApplication::readOptions()" << endl;

  // get standard config file
  KConfig* config=kapp->config();

  // Read configuration and provide the default values
  config->setGroup("General Options");

  // Command char settings
  preferences.setCommandChar(config->readEntry("CommandChar",preferences.getCommandChar()));

  // Tool bar position settings
  preferences.mainWindowToolBarPos     =config->readNumEntry("ServerWindowToolBarPos",KToolBar::Top);
  preferences.mainWindowToolBarStatus  =config->readNumEntry("ServerWindowToolBarStatus",KToolBar::Show);
  preferences.mainWindowToolBarIconText=config->readNumEntry("ServerWindowToolBarIconText",KToolBar::IconTextBottom);
  preferences.mainWindowToolBarIconSize=config->readNumEntry("ServerWindowToolBarIconSize",0);

  // Status bar settings
  preferences.mainWindowStatusBarStatus=config->readBoolEntry("ServerWindowStatusBarStatus",true);

  // Menu bar settings
  preferences.mainWindowMenuBarStatus=config->readBoolEntry("ServerWindowMenuBarStatus",true);

  // Tray icon settings
  preferences.setShowTrayIcon(config->readBoolEntry("ShowTrayIcon",preferences.getShowTrayIcon()));

  // Window geometries
  preferences.setMainWindowSize(config->readSizeEntry("Geometry"));
  preferences.setHilightSize(config->readSizeEntry("HilightGeometry"));
  preferences.setButtonsSize(config->readSizeEntry("ButtonsGeometry"));
  preferences.setIgnoreSize(config->readSizeEntry("IgnoreGeometry"));
  preferences.setNotifySize(config->readSizeEntry("NotifyGeometry"));
  preferences.setNicksOnlineSize(config->readSizeEntry("NicksOnlineGeometry"));
  preferences.setNicknameSize(config->readSizeEntry("NicknameGeometry"));

  // Double click actions
  preferences.setChannelDoubleClickAction(config->readEntry("ChannelDoubleClickAction",preferences.getChannelDoubleClickAction()));
  preferences.setNotifyDoubleClickAction(config->readEntry("NotifyDoubleClickAction",preferences.getNotifyDoubleClickAction()));

  // Beep
  preferences.setBeep(config->readBoolEntry("Beep",preferences.getBeep()));

  // Raw log window
  preferences.setRawLog(config->readBoolEntry("RawLog",preferences.getRawLog()));

  // Reconnection timeout
  preferences.setMaximumLagTime(config->readNumEntry("MaximumLag",preferences.getMaximumLagTime()));

  // Appearance
  config->setGroup("Appearance");
  // Fonts
  preferences.setTextFontRaw(config->readEntry("TextFont",preferences.getTextFont().rawName()));
  preferences.setListFontRaw(config->readEntry("ListFont",preferences.getListFont().rawName()));
  preferences.setTimestamping(config->readBoolEntry("Timestamping",preferences.getTimestamping()));
  preferences.setTimestampFormat(config->readEntry("TimestampFormat",preferences.getTimestampFormat()));
  preferences.setShowQuickButtons(config->readBoolEntry("ShowQuickButtons",preferences.getShowQuickButtons()));
  preferences.setShowModeButtons(config->readBoolEntry("ShowModeButtons",preferences.getShowModeButtons()));
  preferences.setCloseButtonsOnTabs(config->readBoolEntry("CloseButtonsOnTabs",preferences.getCloseButtonsOnTabs()));

  preferences.setAutoUserhost(config->readBoolEntry("AutoUserhost",preferences.getAutoUserhost()));

  preferences.setUseSpacing(config->readBoolEntry("UseSpacing",preferences.getUseSpacing()));
  preferences.setSpacing(config->readNumEntry("Spacing",preferences.getSpacing()));
  preferences.setMargin(config->readNumEntry("Margin",preferences.getMargin()));

  preferences.setUseParagraphSpacing(config->readBoolEntry("UseParagraphSpacing",preferences.getUseParagraphSpacing()));
  preferences.setParagraphSpacing(config->readNumEntry("ParagraphSpacing",preferences.getParagraphSpacing()));

  QValueList<int> sizes;
  QString sizesString=config->readEntry("ChannelSplitter","10,1");
  sizes.append(sizesString.section(',',0,0).toInt());
  sizes.append(sizesString.section(',',1,1).toInt());
  preferences.setChannelSplitter(sizes);

  preferences.setBackgroundImageName(config->readEntry("BackgroundImage",preferences.getBackgroundImageName()));
  QStringList ircColorList = preferences.getIRCColorList();
  preferences.setIRCColorList(config->readListEntry("IRCColors"));

  if(preferences.getIRCColorList().empty()) {
    preferences.setIRCColorList(ircColorList);
  }

  // Colors are now handled in preferences

  // Led Colors
  config->setGroup("Led Colors");
  preferences.setOpLedColor(config->readNumEntry("OperatorColor",preferences.getOpLedColor()));
  preferences.setVoiceLedColor(config->readNumEntry("VoiceColor",preferences.getVoiceLedColor()));
  preferences.setNoRightsLedColor(config->readNumEntry("NoRightsColor",preferences.getNoRightsLedColor()));

  // Sorting
  config->setGroup("Sort Nicknames");
  preferences.setOpValue(config->readNumEntry("OperatorValue",preferences.getOpValue()));
  preferences.setVoiceValue(config->readNumEntry("VoiceValue",preferences.getVoiceValue()));
  preferences.setNoRightsValue(config->readNumEntry("NoRightsValue",preferences.getNoRightsValue()));
  preferences.setSortByStatus(config->readBoolEntry("SortByStatus",preferences.getSortByStatus()));
  preferences.setSortCaseInsensitive(config->readBoolEntry("SortCaseInsensitive",preferences.getSortCaseInsensitive()));

  // Identity list
  QStringList identityList=config->groupList().grep(QRegExp("Identity [0-9]+"));
  if(identityList.count())
  {
    preferences.clearIdentityList();

    for(unsigned int index=0;index<identityList.count();index++)
    {
//      kdDebug() << "Reading identity " << identityList[index] << endl;
      config->setGroup(identityList[index]);

      Identity* newIdentity=new Identity();
      QString n=config->readEntry("Name");

      newIdentity->setName(config->readEntry("Name"));

      newIdentity->setIdent(config->readEntry("Ident"));
      newIdentity->setRealName(config->readEntry("Realname"));

      QString nickList=config->readEntry("Nicknames");
      newIdentity->setNicknameList(QStringList::split(",",nickList));

      newIdentity->setBot(config->readEntry("Bot"));
      newIdentity->setPassword(config->readEntry("Password"));

      newIdentity->setShowAwayMessage(config->readBoolEntry("ShowAwayMessage"));
      newIdentity->setAwayMessage(config->readEntry("AwayMessage"));
      newIdentity->setReturnMessage(config->readEntry("ReturnMessage"));

      newIdentity->setPartReason(config->readEntry("PartReason"));
      newIdentity->setKickReason(config->readEntry("KickReason"));

      newIdentity->setCodec(config->readEntry("Codec"));

      preferences.addIdentity(newIdentity);
    } // endfor
  }
  else
  {
    kdDebug() << "Importing pre 0.10 identity settings ..." << endl;

    // Default user identity for pre 0.10 preferences files
    config->setGroup("User Identity");
    preferences.setIdent(config->readEntry("Ident",preferences.getIdent()));
    preferences.setRealName(config->readEntry("Realname",preferences.getRealName()));

    QString nickList=config->readEntry("Nicknames",preferences.getNicknameList().join(","));
    preferences.setNicknameList(QStringList::split(",",nickList));

    preferences.setShowAwayMessage(config->readBoolEntry("ShowAwayMessage",preferences.getShowAwayMessage()));
    preferences.setAwayMessage(config->readEntry("AwayMessage",preferences.getAwayMessage()));
    preferences.setUnAwayMessage(config->readEntry("UnAwayMessage",preferences.getUnAwayMessage()));

    config->deleteGroup("User Identity");
  }
  // Notify Settings and list
  config->setGroup("Notify List");
  preferences.setNotifyDelay(config->readNumEntry("NotifyDelay",20));
  preferences.setUseNotify(config->readBoolEntry("UseNotify",true));
  QString notifyList=config->readEntry("NotifyList",QString::null);
  preferences.setNotifyList(QStringList::split(' ',notifyList));

  // Server List
  config->setGroup("Server List");

  int index=0;
  // Remove all default entries if there is at least one Server in the preferences file
  if(config->hasKey("Server0")) preferences.clearServerList();
  // Read all servers
  while(config->hasKey(QString("Server%1").arg(index)))
  {
    preferences.addServer(config->readEntry(QString("Server%1").arg(index++)));
  }

  // Quick Buttons List
  config->setGroup("Button List");
  // Read all buttons and overwrite default entries
  QStringList buttonList(preferences.getButtonList());
  for(index=0;index<8;index++)
  {
    QString buttonKey(QString("Button%1").arg(index));
    if(config->hasKey(buttonKey)) buttonList[index]=config->readEntry(buttonKey);
  }
  // Put back the changed button list
  preferences.setButtonList(buttonList);

  // Hilight List
  config->setGroup("Hilight List");
  QString hilight=config->readEntry("Hilight");
  QStringList hiList=QStringList::split(' ',hilight);

  unsigned int hiIndex;
  for(hiIndex=0;hiIndex<hiList.count();hiIndex+=2)
  {
    preferences.addHilight(hiList[hiIndex],"#"+hiList[hiIndex+1]);
  }

  preferences.setHilightNick(config->readBoolEntry("HilightNick",preferences.getHilightNick()));
  hilight=config->readEntry("HilightNickColor");
  if(hilight.isEmpty())
    preferences.setHilightNickColor(preferences.getHilightNickColor().name());
  else
    preferences.setHilightNickColor("#"+hilight);

  preferences.setHilightOwnLines(config->readBoolEntry("HilightOwnLines",preferences.getHilightOwnLines()));
  hilight=config->readEntry("HilightOwnLinesColor");
  if(hilight.isEmpty())
    preferences.setHilightOwnLinesColor(preferences.getHilightOwnLinesColor().name());
  else
    preferences.setHilightOwnLinesColor("#"+hilight);

  // Ignore List
  config->setGroup("Ignore List");
  // Remove all default entries if there is at least one Ignore in the preferences file
  if(config->hasKey("Ignore0")) preferences.clearIgnoreList();
  // Read all ignores
  index=0;
  while(config->hasKey(QString("Ignore%1").arg(index)))
  {
    preferences.addIgnore(config->readEntry(QString("Ignore%1").arg(index++)));
  }

  // Aliases
  config->setGroup("Aliases");
  QStringList newList=config->readListEntry("AliasList");
  if(!newList.isEmpty()) preferences.setAliasList(newList);

  // Nick Completion
  config->setGroup("Nick Completion");
  preferences.setNickCompleteSuffixStart(config->readEntry("SuffixStart",preferences.getNickCompleteSuffixStart()));
  preferences.setNickCompleteSuffixMiddle(config->readEntry("SuffixMiddle",preferences.getNickCompleteSuffixMiddle()));

  // DCC Settings
  config->setGroup("DCC Settings");
  preferences.setDccBufferSize(config->readNumEntry("BufferSize",preferences.getDccBufferSize()));
  preferences.setDccRollback(config->readNumEntry("Rollback",preferences.getDccRollback()));
  preferences.setDccAddPartner(config->readBoolEntry("AddPartner",preferences.getDccAddPartner()));
  preferences.setDccCreateFolder(config->readBoolEntry("CreateFolder",preferences.getDccCreateFolder()));
  preferences.setDccAutoGet(config->readBoolEntry("AutoGet",preferences.getDccAutoGet()));

  // Path settings
  config->setGroup("Path Settings");
  preferences.setLogPath(config->readEntry("LogfilePath",preferences.getLogPath()));
  preferences.setDccPath(config->readEntry("DccPath",preferences.getDccPath()));

  // Miscellaneous Flags
  config->setGroup("Flags");

  preferences.setLog(config->readBoolEntry("Log",preferences.getLog()));
  preferences.setLowerLog(config->readBoolEntry("LowerLog",preferences.getLowerLog()));
  preferences.setLogFollowsNick(config->readBoolEntry("LogFollowsNick",preferences.getLogFollowsNick()));

  preferences.setTabPlacement(static_cast<Preferences::TabPlacement>(config->readNumEntry("TabPlacement",static_cast<int>(preferences.getTabPlacement()))));
  preferences.setBlinkingTabs(config->readBoolEntry("BlinkingTabs",preferences.getBlinkingTabs()));
  preferences.setBringToFront(config->readBoolEntry("BringToFront",preferences.getBringToFront()));

  preferences.setAutoReconnect(config->readBoolEntry("AutoReconnect",preferences.getAutoReconnect()));
  preferences.setAutoRejoin(config->readBoolEntry("AutoRejoin",preferences.getAutoRejoin()));
  preferences.setAutojoinOnInvite(config->readBoolEntry("AutojoinOnInvite",preferences.getAutojoinOnInvite()));

  preferences.setFixedMOTD(config->readBoolEntry("FixedMOTD",preferences.getFixedMOTD()));
  preferences.setShowServerList(config->readBoolEntry("ShowServerList",preferences.getShowServerList()));
}

void KonversationApplication::saveOptions(bool updateGUI)
{
  kdDebug() << "KonversationApplication::saveOptions()" << endl;

  KConfig* config=kapp->config();

  config->setGroup("General Options");

  config->writeEntry("CommandChar",preferences.getCommandChar());

  config->writeEntry("Geometry",preferences.getMainWindowSize());
  config->writeEntry("HilightGeometry",preferences.getHilightSize());
  config->writeEntry("ButtonsGeometry",preferences.getButtonsSize());
  config->writeEntry("IgnoreGeometry",preferences.getIgnoreSize());
  config->writeEntry("NotifyGeometry",preferences.getNotifySize());
  config->writeEntry("NicksOnlineGeometry",preferences.getNicksOnlineSize());
  config->writeEntry("NicknameGeometry",preferences.getNicknameSize());

  config->writeEntry("ServerWindowToolBarPos",preferences.mainWindowToolBarPos);
  config->writeEntry("ServerWindowToolBarStatus",preferences.mainWindowToolBarStatus);
  config->writeEntry("ServerWindowToolBarIconText",preferences.mainWindowToolBarIconText);
  config->writeEntry("ServerWindowToolBarIconSize",preferences.mainWindowToolBarIconSize);

  config->writeEntry("ServerWindowStatusBarStatus",preferences.mainWindowStatusBarStatus);

  config->writeEntry("ServerWindowMenuBarStatus",preferences.mainWindowMenuBarStatus);
  config->writeEntry("ShowTrayIcon",preferences.getShowTrayIcon());

  config->writeEntry("ChannelDoubleClickAction",preferences.getChannelDoubleClickAction());
  config->writeEntry("NotifyDoubleClickAction",preferences.getNotifyDoubleClickAction());

  config->writeEntry("Beep",preferences.getBeep());
  config->writeEntry("RawLog",preferences.getRawLog());

  config->writeEntry("MaximumLag",preferences.getMaximumLagTime());

  config->setGroup("Appearance");

  config->writeEntry("TextFont",preferences.getTextFont().toString());
  config->writeEntry("ListFont",preferences.getListFont().toString());
  config->writeEntry("Timestamping",preferences.getTimestamping());
  config->writeEntry("TimestampFormat",preferences.getTimestampFormat());
  config->writeEntry("ShowQuickButtons",preferences.getShowQuickButtons());
  config->writeEntry("ShowModeButtons",preferences.getShowModeButtons());
  config->writeEntry("CloseButtonsOnTabs",preferences.getCloseButtonsOnTabs());

  config->writeEntry("AutoUserhost",preferences.getAutoUserhost());

  config->writeEntry("UseSpacing",preferences.getUseSpacing());
  config->writeEntry("Spacing",preferences.getSpacing());
  config->writeEntry("Margin",preferences.getMargin());

  config->writeEntry("UseParagraphSpacing",preferences.getUseParagraphSpacing());
  config->writeEntry("ParagraphSpacing",preferences.getParagraphSpacing());

  QString sizesString(QString::number(preferences.getChannelSplitter()[0])+","+QString::number(preferences.getChannelSplitter()[1]));
  config->writeEntry("ChannelSplitter",sizesString);
  config->writeEntry("BackgroundImage",preferences.getBackgroundImageName());
  config->writeEntry("IRCColors", preferences.getIRCColorList());

  // Colors are now handled in preferences

  config->setGroup("Sort Nicknames");
  config->writeEntry("OperatorValue",preferences.getOpValue());
  config->writeEntry("VoiceValue",preferences.getVoiceValue());
  config->writeEntry("NoRightsValue",preferences.getNoRightsValue());
  config->writeEntry("SortByStatus",preferences.getSortByStatus());
  config->writeEntry("SortCaseInsensitive",preferences.getSortCaseInsensitive());

  config->setGroup("Led Colors");
  config->writeEntry("OperatorColor", preferences.getOpLedColor());
  config->writeEntry("VoiceColor", preferences.getVoiceLedColor());
  config->writeEntry("NoRightsColor", preferences.getNoRightsLedColor());

  // Clean up identity list
  QStringList identities=config->groupList().grep(QRegExp("Identity [0-9]+"));
  if(identities.count())
  {
    // remove old identity list from preferences file to keep numbering under control
    for(unsigned int index=0;index<identities.count();index++)
      config->deleteGroup(identities[index]);
  }

  QPtrList<Identity> identityList=preferences.getIdentityList();
  for(unsigned int index=0;index<identityList.count();index++)
  {
//    kdDebug() << "Writing identity " << index << endl;

    Identity* identity=identityList.at(index);
    config->setGroup(QString("Identity %1").arg(index));

    config->writeEntry("Name",identity->getName());
    config->writeEntry("Ident",identity->getIdent());
    config->writeEntry("Realname",identity->getRealName());
    config->writeEntry("Nicknames",identity->getNicknameList());
    config->writeEntry("Bot",identity->getBot());
    config->writeEntry("Password",identity->getPassword());
    config->writeEntry("ShowAwayMessage",identity->getShowAwayMessage());
    config->writeEntry("AwayMessage",identity->getAwayMessage());
    config->writeEntry("ReturnMessage",identity->getReturnMessage());
    config->writeEntry("PartReason",identity->getPartReason());
    config->writeEntry("KickReason",identity->getKickReason());
    config->writeEntry("Codec",identity->getCodec().stripWhiteSpace());
  } // endfor

  config->setGroup("Notify List");

  config->writeEntry("NotifyDelay",preferences.getNotifyDelay());
  config->writeEntry("UseNotify",preferences.getUseNotify());
  config->writeEntry("NotifyList",preferences.getNotifyString());

  config->deleteGroup("Server List");
  config->setGroup("Server List");

  int index=0;
  QString serverEntry=preferences.getServerByIndex(0);

  while(!serverEntry.isEmpty())
  {
    config->writeEntry(QString("Server%1").arg(index),serverEntry);
    serverEntry=preferences.getServerByIndex(++index);
  }

  config->setGroup("Button List");

  for(index=0;index<8;index++)
  {
    QStringList buttonList(preferences.getButtonList());
    config->writeEntry(QString("Button%1").arg(index),buttonList[index]);
  }

  // Write all hilight entries
  config->setGroup("Hilight List");

  QPtrList<Highlight> hiList=preferences.getHilightList();

  // Put all hilight patterns and colors after another, separated with a space
  QString hilight;
  for(unsigned int index=0;index<hiList.count();index++)
    hilight+=hiList.at(index)->getText()+" "+hiList.at(index)->getColor().name().mid(1)+" ";

  // remove extra spaces
  hilight=hilight.stripWhiteSpace();
  // write hilight string
  config->writeEntry("Hilight",hilight);

  config->writeEntry("HilightNick",preferences.getHilightNick());
  config->writeEntry("HilightNickColor",preferences.getHilightNickColor().name().mid(1));
  config->writeEntry("HilightOwnLines",preferences.getHilightOwnLines());
  config->writeEntry("HilightOwnLinesColor",preferences.getHilightOwnLinesColor().name().mid(1));

  // Ignore List
  config->deleteGroup("Ignore List");
  config->setGroup("Ignore List");
  QPtrList<Ignore> ignoreList=preferences.getIgnoreList();
  Ignore* item=ignoreList.first();
  index=0;
  while(item)
  {
    config->writeEntry(QString("Ignore%1").arg(index),QString("%1,%2").arg(item->getName()).arg(item->getFlags()));
    item=ignoreList.next();
    index++;
  }

  // Aliases
  config->setGroup("Aliases");
  config->writeEntry("AliasList",preferences.getAliasList());

  // Nick Completion
  config->setGroup("Nick Completion");
  config->writeEntry("SuffixStart",preferences.getNickCompleteSuffixStart());
  config->writeEntry("SuffixMiddle",preferences.getNickCompleteSuffixMiddle());

  // DCC Settings
  config->setGroup("DCC Settings");
  config->writeEntry("AddPartner",preferences.getDccAddPartner());
  config->writeEntry("CreateFolder",preferences.getDccCreateFolder());
  config->writeEntry("BufferSize",preferences.getDccBufferSize());
  config->writeEntry("Rollback",preferences.getDccRollback());
  config->writeEntry("AutoGet",preferences.getDccAutoGet());

 // Path Settings
  config->setGroup("Path Settings");
  config->writeEntry("DccPath",preferences.getDccPath());
  config->writeEntry("LogfilePath",preferences.getLogPath());

  // Flags
  config->setGroup("Flags");

  config->writeEntry("Log",preferences.getLog());
  config->writeEntry("LowerLog",preferences.getLowerLog());
  config->writeEntry("LogFollowsNick",preferences.getLogFollowsNick());

  config->writeEntry("TabPlacement",static_cast<int>(preferences.getTabPlacement()));
  config->writeEntry("BlinkingTabs",preferences.getBlinkingTabs());
  config->writeEntry("BringToFront",preferences.getBringToFront());

  config->writeEntry("AutoReconnect",preferences.getAutoReconnect());
  config->writeEntry("AutoRejoin",preferences.getAutoRejoin());
  config->writeEntry("AutojoinOnInvite",preferences.getAutojoinOnInvite());

  config->writeEntry("FixedMOTD",preferences.getFixedMOTD());
  config->writeEntry("ShowServerList",preferences.getShowServerList());

  config->sync();

  if(updateGUI)
  {
    Server* lookServer=serverList.first();
    while(lookServer)
    {
      // TODO: updateFonts() also updates the background color and more stuff! We must finally
      // find a way to do all this with signals / slots!
      lookServer->updateFonts();
      lookServer->updateChannelQuickButtons();

      lookServer->setShowQuickButtons(preferences.getShowQuickButtons());
      lookServer->setShowModeButtons(preferences.getShowModeButtons());
      lookServer=serverList.next();
    }

    mainWindow->updateTabPlacement();
  }
}

// FIXME: use KURL maybe?
void KonversationApplication::storeUrl(const QString& who,const QString& newUrl)
{
  QString url(newUrl);
  // clean up URL to help KRun() in URL catcher interface
  if(url.startsWith("www.")) url="http://"+url;
  else if(url.startsWith("ftp.")) url="ftp://"+url;

  url=url.replace(QRegExp("&amp;"),"&");

  // check that we don't add the same URL twice
  deleteUrl(who,url);
  urlList.append(who+" "+url);
  emit catchUrl(who,url);
}

const QStringList& KonversationApplication::getUrlList()
{
  return urlList;
}

void KonversationApplication::deleteUrl(const QString& who,const QString& url)
{
  urlList.remove(who+" "+url);
}

void KonversationApplication::clearUrlList()
{
  urlList.clear();
}

void KonversationApplication::openPrefsDialog()
{
  if(prefsDialog==0)
  {
    prefsDialog=new PrefsDialog(&preferences,false);

    connect(prefsDialog,SIGNAL (connectToServer(int)),this,SLOT (connectToAnotherServer(int)) );
    connect(prefsDialog,SIGNAL (cancelClicked()),this,SLOT (closePrefsDialog()) );
    connect(prefsDialog,SIGNAL (prefsChanged()),this,SLOT (saveOptions()) );

    prefsDialog->show();
  }
  else
  {
    prefsDialog->show();
    prefsDialog->raise();
    prefsDialog->setActiveWindow();
  }
}

void KonversationApplication::openPrefsDialog(Preferences::Pages page)
{
  openPrefsDialog();
  prefsDialog->openPage(page);
}

void KonversationApplication::syncPrefs()
{
  kapp->config()->sync();
}

void KonversationApplication::closePrefsDialog()
{
  delete prefsDialog;
  prefsDialog=0;
}

bool KonversationApplication::emitDCOPSig(const QString &appId, const QString &objId, const QString &signal, QByteArray &data)
{
  kdDebug() << "emitDCOPSig (" << signal << ")" << endl;
  //dcopObject->emitDCOPSignal(signal, data);
  QByteArray replyData;
  QCString replyType;
  if (!KApplication::dcopClient()->call(appId.ascii(), objId.ascii(), signal.ascii() /*must have prototype*/,
					data, replyType, replyData)) {
    qDebug("there was some error using DCOP.");
    return true; // Keep processing filters
  } else {
    QDataStream reply(replyData, IO_ReadOnly);
    if (replyType == "bool") {
      bool result;
      reply >> result;
      return result;
    } else {
      qDebug("doIt returned an unexpected type of reply!");
      return true; // Keep processing
    }
  }
}

QPtrList<IRCEvent> KonversationApplication::retreiveHooks (EVENT_TYPE a_type)
{
  QPtrList<IRCEvent> ret_value;
  IRCEvent *e;

  for (e = dcopObject->registered_events.first(); e; e = dcopObject->registered_events.next()) {
    if (e->type == a_type) {
      ret_value.append(e);
      }
    }
  return ret_value;
}

#include "konversationapplication.moc"
