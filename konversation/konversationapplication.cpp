/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationapplication.cpp  -  description
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kdebug.h>

#include "konversationapplication.h"
#include "serverwindow.h"

/* include static variables */
Preferences KonversationApplication::preferences;
QStringList KonversationApplication::urlList;

KonversationApplication::KonversationApplication()
{
  kdDebug() << "KonversationApplication::KonversationApplication()" << endl;

  preferences.setTextFont(font());
  preferences.setListFont(font());

  config=new KSimpleConfig("konversationrc");
  readOptions();
  prefsDialog=new PrefsDialog(&preferences,true);

  connect(prefsDialog,SIGNAL (connectToServer(int)),this,SLOT (connectToServer(int)) );
  connect(prefsDialog,SIGNAL (cancelClicked()),this,SLOT (quitKonversation()) );
  connect(prefsDialog,SIGNAL (prefsChanged()),this,SLOT (saveOptions()) );

  /* TODO: Check if this serverList is needed anyway */
  serverList.setAutoDelete(true);     // delete items when they are removed

  prefsDialog->show();

  connect(&preferences,SIGNAL (requestServerConnection(int)),this,SLOT (connectToAnotherServer(int)) );
  connect(&preferences,SIGNAL (requestSaveOptions()),this,SLOT (saveOptions()) );
}

KonversationApplication::~KonversationApplication()
{
  kdDebug() << "KonversationApplication::~KonversationApplication()" << endl;
  saveOptions();

/*
  kdDebug() << "Deleting Server list ..." << endl;
  while(serverList.count())
  {
    kdDebug() << "deleted" << endl;
    delete serverList.first();
  }
*/
}

void KonversationApplication::connectToServer(int id)
{
  kdDebug() << "KonversationApplication::connectToServer(" << id << ")" << endl;

  connectToAnotherServer(id);
  /* to prevent doubleClicked() to crash the dialog */
  /* FIXME: Seems to have a race, though */
  prefsDialog->delayedDestruct();
  prefsDialog=0;
}

void KonversationApplication::connectToAnotherServer(int id)
{
  kdDebug() << "KonversationApplication::connectToAnotherServer(" << id << ")" << endl;

  ServerEntry* chosenServer=preferences.getServerEntryById(id);

  /* Check if a server window with same name and port is already open */
  Server* newServer=serverList.first();
  while(newServer)
  {
    if(chosenServer->getServerName()==newServer->getServerName() &&
       chosenServer->getPort()==newServer->getPort())
    {
      QString autoJoinChannel=chosenServer->getChannelName();
      if(autoJoinChannel!="")
      {
        newServer->setAutoJoin(true);
        newServer->setAutoJoinChannel(autoJoinChannel);
        newServer->setAutoJoinChannelKey(chosenServer->getChannelKey());
      }
      else newServer->setAutoJoin(false);

      newServer->connectToIRCServer();
      return;
    }

    newServer=serverList.next();
  }
  /* We came this far, so generate a new server */
  newServer=new Server(id);
  serverList.append(newServer);
  connect(newServer->getServerWindow(),SIGNAL(prefsChanged()),this,SLOT(saveOptions()));
  connect(newServer->getServerWindow(),SIGNAL(openPrefsDialog()),this,SLOT(openPrefsDialog()));
}

void KonversationApplication::quitKonversation()
{
  kdDebug() << "KonversationApplication::quitKonversation()" << endl;
  delete prefsDialog;
  this->exit();
}

void KonversationApplication::readOptions()
{
  kdDebug() << "KonversationApplication::readOptions()" << endl;

  // Read configuration and provide the default values
  config->setGroup("General Options");

  // Command char settings
  preferences.setCommandChar(config->readEntry("CommandChar",preferences.getCommandChar()));

  // Tool bar position settings
  preferences.serverWindowToolBarPos     =config->readNumEntry("ServerWindowToolBarPos",KToolBar::Top);
  preferences.serverWindowToolBarStatus  =config->readNumEntry("ServerWindowToolBarStatus",KToolBar::Show);
  preferences.serverWindowToolBarIconText=config->readNumEntry("ServerWindowToolBarIconText",KToolBar::IconTextBottom);
  preferences.serverWindowToolBarIconSize=config->readNumEntry("ServerWindowToolBarIconSize",0);

  // Status bar settings
  preferences.serverWindowStatusBarStatus=config->readBoolEntry("ServerWindowStatusBarStatus",true);

  // Window geometries
  preferences.setServerWindowSize(config->readSizeEntry("Geometry"));
  preferences.setHilightSize(config->readSizeEntry("HilightGeometry"));
  preferences.setButtonsSize(config->readSizeEntry("ButtonsGeometry"));
  preferences.setIgnoreSize(config->readSizeEntry("IgnoreGeometry"));
  preferences.setNotifySize(config->readSizeEntry("NotifyGeometry"));
  preferences.setNicksOnlineSize(config->readSizeEntry("NicksOnlineGeometry"));
  preferences.setNicknameSize(config->readSizeEntry("NicknameGeometry"));
  preferences.setColorConfigurationSize(config->readSizeEntry("ColorConfigurationGeometry"));

  // Reasons
  QString reason;
  reason=config->readEntry("PartReason","");
  if(reason!="") preferences.setPartReason(reason);
  reason=config->readEntry("KickReason","");
  if(reason!="") preferences.setKickReason(reason);

  // Appearance
  config->setGroup("Appearance");
  // Fonts
  preferences.setTextFontRaw(config->readEntry("TextFont",preferences.getTextFont().rawName()));
  preferences.setListFontRaw(config->readEntry("ListFont",preferences.getListFont().rawName()));

  // Colors
  config->setGroup("Message Text Colors");

  preferences.setActionMessageColor(config->readEntry("ActionMessage", preferences.defaultActionMessageColor));
  preferences.setBacklogMessageColor(config->readEntry("BacklogMessage", preferences.defaultBacklogMessageColor));
  preferences.setChannelMessageColor(config->readEntry("ChannelMessage", preferences.defaultChannelMessageColor));
  preferences.setCommandMessageColor(config->readEntry("CommandMessage", preferences.defaultCommandMessageColor));
  preferences.setLinkMessageColor(config->readEntry("LinkMessage", preferences.defaultLinkMessageColor));
  preferences.setQueryMessageColor(config->readEntry("QueryMessage", preferences.defaultQueryMessageColor));
  preferences.setServerMessageColor(config->readEntry("ServerMessage", preferences.defaultServerMessageColor));

  // Led Colors
  config->setGroup("Led Colors");
  preferences.setOpLedColor(config->readNumEntry("OperatorColor",preferences.getOpLedColor()));
  preferences.setVoiceLedColor(config->readNumEntry("VoiceColor",preferences.getVoiceLedColor()));
  preferences.setNoRightsLedColor(config->readNumEntry("NoRightsColor",preferences.getNoRightsLedColor()));

  // User identity
  config->setGroup("User Identity");
  preferences.ident=config->readEntry("Ident",preferences.ident);
  preferences.realname=config->readEntry("Realname",preferences.realname);

  QString nickList=config->readEntry("Nicknames",preferences.getNicknameList().join(","));
  preferences.setNicknameList(QStringList::split(",",nickList));

  // Notify Settings and list
  config->setGroup("Notify List");
  preferences.setNotifyDelay(config->readNumEntry("NotifyDelay",20));
  preferences.setUseNotify(config->readBoolEntry("UseNotify",true));
  QString notifyList=config->readEntry("NotifyList","");
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
  if(hilight=="")
    preferences.setHilightNickColor(preferences.getHilightNickColor().name());
  else
    preferences.setHilightNickColor("#"+hilight);

  preferences.setHilightOwnLines(config->readBoolEntry("HilightOwnLines",preferences.getHilightOwnLines()));
  hilight=config->readEntry("HilightOwnLinesColor");
  if(hilight=="")
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

  preferences.setBlinkingTabs(config->readBoolEntry("BlinkingTabs",preferences.getBlinkingTabs()));
  preferences.setBringToFront(config->readBoolEntry("BringToFront",preferences.getBringToFront()));

  preferences.setAutoReconnect(config->readBoolEntry("AutoReconnect",preferences.getAutoReconnect()));
  preferences.setAutoRejoin(config->readBoolEntry("AutoRejoin",preferences.getAutoRejoin()));

  preferences.setFixedMOTD(config->readBoolEntry("FixedMOTD",preferences.getFixedMOTD()));
}

void KonversationApplication::saveOptions()
{
  kdDebug() << "KonversationApplication::saveOptions()" << endl;

  config->setGroup("General Options");

  config->writeEntry("CommandChar",preferences.getCommandChar());

  config->writeEntry("Geometry",preferences.getServerWindowSize());
  config->writeEntry("HilightGeometry",preferences.getHilightSize());
  config->writeEntry("ButtonsGeometry",preferences.getButtonsSize());
  config->writeEntry("IgnoreGeometry",preferences.getIgnoreSize());
  config->writeEntry("NotifyGeometry",preferences.getNotifySize());
  config->writeEntry("NicksOnlineGeometry",preferences.getNicksOnlineSize());
  config->writeEntry("NicknameGeometry",preferences.getNicknameSize());
  config->writeEntry("ColorConfigurationGeometry", preferences.getColorConfigurationSize());

  config->writeEntry("ServerWindowToolBarPos",preferences.serverWindowToolBarPos);
  config->writeEntry("ServerWindowToolBarStatus",preferences.serverWindowToolBarStatus);
  config->writeEntry("ServerWindowToolBarIconText",preferences.serverWindowToolBarIconText);
  config->writeEntry("ServerWindowToolBarIconSize",preferences.serverWindowToolBarIconSize);

  config->writeEntry("ServerWindowStatusBarStatus",preferences.serverWindowStatusBarStatus);

  config->writeEntry("PartReason",preferences.getPartReason());
  config->writeEntry("KickReason",preferences.getKickReason());

  config->setGroup("Appearance");
  config->writeEntry("TextFont",preferences.getTextFont().rawName());
  config->writeEntry("ListFont",preferences.getListFont().rawName());

  config->setGroup("Message Text Colors");

  config->writeEntry("ActionMessage", preferences.getActionMessageColor());
  config->writeEntry("BacklogMessage", preferences.getBacklogMessageColor());
  config->writeEntry("ChannelMessage", preferences.getChannelMessageColor());
  config->writeEntry("CommandMessage", preferences.getCommandMessageColor());
  config->writeEntry("LinkMessage", preferences.getLinkMessageColor());
  config->writeEntry("QueryMessage", preferences.getQueryMessageColor());
  config->writeEntry("ServerMessage", preferences.getServerMessageColor());

  config->setGroup("Led Colors");
  config->writeEntry("OperatorColor", preferences.getOpLedColor());
  config->writeEntry("VoiceColor", preferences.getVoiceLedColor());
  config->writeEntry("NoRightsColor", preferences.getNoRightsLedColor());

  config->setGroup("User Identity");

  config->writeEntry("Ident",preferences.ident);
  config->writeEntry("Realname",preferences.realname);
  config->writeEntry("Nicknames",preferences.getNicknameList());

  config->setGroup("Notify List");
  config->writeEntry("NotifyDelay",preferences.getNotifyDelay());
  config->writeEntry("UseNotify",preferences.getUseNotify());
  config->writeEntry("NotifyList",preferences.getNotifyString());

  config->deleteGroup("Server List");
  config->setGroup("Server List");

  int index=0;
  QString serverEntry=preferences.getServerByIndex(0);

  while(serverEntry)
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

  config->writeEntry("BlinkingTabs",preferences.getBlinkingTabs());
  config->writeEntry("BringToFront",preferences.getBringToFront());

  config->writeEntry("AutoReconnect",preferences.getAutoReconnect());
  config->writeEntry("AutoRejoin",preferences.getAutoRejoin());

  config->writeEntry("FixedMOTD",preferences.getFixedMOTD());

  config->sync();

  Server* lookServer=serverList.first();
  while(lookServer)
  {
    lookServer->updateFonts();
    lookServer=serverList.next();
  }
}


void KonversationApplication::storeURL(QString& url)
{
  urlList.append(url);
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
}

void KonversationApplication::syncPrefs()
{
  config->sync();
}

void KonversationApplication::closePrefsDialog()
{
  delete prefsDialog;
  prefsDialog=0;
}
