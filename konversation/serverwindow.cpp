/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverwindow.cpp  -  description
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <iostream>

#include <qdir.h>

#include <klocale.h>

#include "serverwindow.h"
#include "konversationapplication.h"

ServerWindow::ServerWindow(Server* server) : KMainWindow()
{
  setServer(server);

  windowContainer=new LedTabWidget(this,"container");
  windowContainer->setTabPosition(QTabWidget::Bottom);

  hilightWindow=0;
  buttonsDialog=0;

/*  KAction* quitAction= */ KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); /* file_quit */
  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); /* options_show_toolbar */
/*  KAction* prefsAction= */ KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); /* options_configure */
/*  KAction* open_quickbuttons_action= */new KAction(i18n("Buttons"),0,0,this,SLOT (openButtons()),actionCollection(),"open_buttons_window");
/*  KAction* open_hilight_action= */ new KAction(i18n("Hilight List"),0,0,this,SLOT (openHilight()),actionCollection(),"open_hilight_window");

  setCentralWidget(windowContainer);

  addStatusView();
  connect( windowContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changedView(QWidget*)) );

  createGUI();
  readOptions();
}

ServerWindow::~ServerWindow()
{
  cerr << "ServerWindow::~ServerWindow()" << endl;
}

void ServerWindow::openPreferences()
{
  KonversationApplication::preferences.openPrefsDialog();
}

void ServerWindow::showToolbar()
{
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.serverWindowToolBarStatus=showToolBarAction->isChecked();
}

void ServerWindow::setServer(Server* newServer)
{
  server=newServer;
  connect(&filter,SIGNAL (openQuery(const QString&,const QString&)),
           server,SLOT   (addQuery(const QString&,const QString&)) );
}

void ServerWindow::appendToStatus(const QString& type,const QString& message)
{
  statusView->appendServerMessage(type,message);
  /* Show activity indicator */
  newText(statusPane);
}

void ServerWindow::statusTextEntered()
{
  QString line=statusInput->text();
  if(line.lower()=="/clear") statusView->clear();  // FIXME: to get rid of too wide lines
  else
  {
    QString output=filter.parse(line,"");

    if(output!="") appendToStatus(filter.getType(),output);

    server->queue(filter.getServerOutput());
  }

  statusInput->clear();
}

void ServerWindow::addView(QWidget* pane,int color,const QString& label)
{
  windowContainer->addTab(pane,label,color,true,KonversationApplication::preferences.getBlinkingTabs());
}

void ServerWindow::showView(QWidget* pane)
{
  windowContainer->showPage(pane);
}

void ServerWindow::addStatusView()
{
  statusPane=new QVBox(windowContainer);
  statusPane->setSpacing(spacing());
  statusPane->setMargin(margin());

  statusView=new IRCView(statusPane);

  QHBox* commandLineBox=new QHBox(statusPane);
  commandLineBox->setSpacing(spacing());
  commandLineBox->setMargin(0);

  nicknameButton=new QPushButton(i18n("Nickname"),commandLineBox);
  statusInput=new IRCInput(commandLineBox);
  logCheckBox=new QCheckBox(i18n("Log"),commandLineBox);
  logCheckBox->setChecked(KonversationApplication::preferences.getLog());

  windowContainer->addTab(statusPane,i18n("Status"),2,false,KonversationApplication::preferences.getBlinkingTabs());

  connect(statusInput,SIGNAL (returnPressed()),this,SLOT(statusTextEntered()) );
  connect(statusView,SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );
  connect(statusView,SIGNAL(textToLog(const QString&)),this,SLOT (logText(const QString&)) );

  setLog(KonversationApplication::preferences.getLog());
}

void ServerWindow::logText(const QString& text)
{
  QDir logPath=QDir::home();

  /* Try to "cd" into the logfile path */
  if(!logPath.cd(KonversationApplication::preferences.logPath,true))
  {
    /* Try to create the logfile path and "cd" into it again */
    logPath.mkdir(KonversationApplication::preferences.logPath,true);
    logPath.cd(KonversationApplication::preferences.logPath,true);
  }

  /* add the logfile name to the path */
  logfile.setName(logPath.path()+"/konversation.log");

  if(logfile.open(IO_WriteOnly | IO_Append))
  {
    if(firstLog)
    {
      QString intro(i18n("\n*** Logfile started\n*** on %1\n\n").arg(QDateTime::currentDateTime().toString()));
      logfile.writeBlock(intro,intro.length());
      firstLog=false;
    }

    QTime time=QTime::currentTime();
    QString logLine(QString("[%1:%2:%3] %4\n").arg(time.hour()).arg(time.minute()).arg(time.second()).arg(text));
    logfile.writeBlock(logLine,logLine.length());
    logfile.close();
  }
  else cerr << "ServerWindow::logText(): open(IO_Append) for " << logfile.name() << " failed!" << endl;
}

void ServerWindow::setNickname(const QString& newNickname)
{
  nicknameButton->setText(newNickname);
}

void ServerWindow::newText(QWidget* view)
{
  if(view!=windowContainer->currentPage())
  {
    windowContainer->changeTabState(view,true);
  }
}

void ServerWindow::changedView(QWidget* view)
{
  windowContainer->changeTabState(view,false);
}

void ServerWindow::readOptions()
{

//  config->setGroup("General Options");

  // bar status settings
//  bool bViewToolbar = config->readBoolEntry("Show Toolbar", true);
//  viewToolBar->setChecked(bViewToolbar);
//  slotViewToolBar();

//  bool bViewStatusbar = config->readBoolEntry("Show Statusbar", true);
//  viewStatusBar->setChecked(bViewStatusbar);
//  slotViewStatusBar();

  // bar position settings
  toolBar("mainToolBar")->setBarPos((KToolBar::BarPosition) KonversationApplication::preferences.serverWindowToolBarPos);
  if(KonversationApplication::preferences.serverWindowToolBarStatus)
  {
    toolBar("mainToolBar")->show();
    showToolBarAction->setChecked(true);
  }
  else
  {
    toolBar("mainToolBar")->hide();
    showToolBarAction->setChecked(false);
  }
  toolBar("mainToolBar")->setIconText((KToolBar::IconText) KonversationApplication::preferences.serverWindowToolBarIconText);
  toolBar("mainToolBar")->setIconSize(KonversationApplication::preferences.serverWindowToolBarIconSize);

// initialize the recent file list
//  fileOpenRecent->loadEntries(config,"Recent Files");

  QSize size=KonversationApplication::preferences.serverWindowSize;
  if(!size.isEmpty())
  {
    resize(size);
  }
}
/* Will not actually save the options but write them into the prefs structure */
void ServerWindow::saveOptions()
{
  KonversationApplication::preferences.serverWindowSize=size();

//  config->writeEntry("Show Toolbar", viewToolBar->isChecked());
//  config->writeEntry("Show Statusbar",viewStatusBar->isChecked());
  KonversationApplication::preferences.serverWindowToolBarPos=toolBar("mainToolBar")->barPos();
//  KonversationApplication::preferences.serverWindowToolBarStatus=(int) toolBar("mainToolBar")->barStatus();
  KonversationApplication::preferences.serverWindowToolBarIconText=toolBar("mainToolBar")->iconText();
  KonversationApplication::preferences.serverWindowToolBarIconSize=toolBar("mainToolBar")->iconSize();
//  fileOpenRecent->saveEntries(config,"Recent Files");
}

bool ServerWindow::queryExit()
{
  cerr << "ServerWindow::queryExit()" << endl;
  saveOptions();
  return true;
}

void ServerWindow::quitProgram()
{
  /* Calls queryExit() */
  close();
}

void ServerWindow::openHilight()
{
  if(!hilightWindow)
  {
    hilightWindow=new HighLightBox(KonversationApplication::preferences.getHilightList(),
                                   KonversationApplication::preferences.getHilightSize());
    connect(hilightWindow, SIGNAL(highLightListChange(QStringList)), this, SLOT(saveHilight(QStringList)));
    connect(hilightWindow, SIGNAL(highLightListClose(QSize)), this, SLOT(closeHilight(QSize)));
  }
}

void ServerWindow::saveHilight(QStringList passed_highlightList)
{
  KonversationApplication::preferences.setHilightList(passed_highlightList);
}

void ServerWindow::closeHilight(QSize newHilightSize)
{
  KonversationApplication::preferences.setHilightSize(newHilightSize);
  emit prefsChanged();

  disconnect(hilightWindow, SIGNAL(highLightListChange(QStringList)), this, SLOT(saveHilight(QStringList)));
  disconnect(hilightWindow, SIGNAL(highLightListClose(QSize)), this, SLOT(closeHilight(QSize)));

  delete hilightWindow;
  hilightWindow=0;
}

void ServerWindow::openButtons()
{
  if(!buttonsDialog)
  {
    buttonsDialog=new QuickButtonsDialog(KonversationApplication::preferences.buttonList,
                                         KonversationApplication::preferences.getButtonsSize());
    connect(buttonsDialog,SIGNAL (cancelClicked(QSize)),this,SLOT (closeButtons(QSize)) );
    connect(buttonsDialog,SIGNAL (applyClicked(QStringList)),this,SLOT (applyButtons(QStringList)) );
    buttonsDialog->show();
  }
}

void ServerWindow::applyButtons(QStringList newButtonList)
{
  KonversationApplication::preferences.setButtonList(newButtonList);
  emit prefsChanged();
  server->updateChannelQuickButtons(newButtonList);
}

void ServerWindow::closeButtons(QSize newButtonsSize)
{
  KonversationApplication::preferences.setButtonsSize(newButtonsSize);
  emit prefsChanged();

  delete buttonsDialog;
  buttonsDialog=0;
}
