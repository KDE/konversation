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

  $Id$
*/

#include <qdir.h>

#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>

#include "serverwindow.h"
#include "konversationapplication.h"

ServerWindow::ServerWindow(Server* server) : KMainWindow()
{
  setServer(server);

  windowContainer=new LedTabWidget(this,"server_window_tab_widget");
  windowContainer->setTabPosition(QTabWidget::Bottom);

  hilightWindow=0;
  hilightDialog=0;
  ignoreDialog=0;
  notifyDialog=0;
  buttonsDialog=0;
	colorConfigurationDialog = 0;

/*  KAction* quitAction= */ KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); /* file_quit */
  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); /* options_show_toolbar */
  showStatusBarAction=KStdAction::showStatusbar(this,SLOT(showStatusbar()),actionCollection()); /* options_show_statusbar */
/*  KAction* prefsAction= */ KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); /* options_configure */
/*  KAction* open_quickbuttons_action= */ new KAction(i18n("Buttons"),0,0,this,SLOT (openButtons()),actionCollection(),"open_buttons_window");
/*  KAction* open_hilight_action=      */ new KAction(i18n("Hilight List"),0,0,this,SLOT (openHilight()),actionCollection(),"open_hilight_window");
/*  KAction* open_notify_action=       */ new KAction(i18n("Notify List"),0,0,this,SLOT (openNotify()),actionCollection(),"open_notify_window");
/*  KAction* open_ignore_action=       */ new KAction(i18n("Ignore List"),0,0,this,SLOT (openIgnore()),actionCollection(),"open_ignore_window");
/*	KAction* open_colors_action=			 */ new KAction(i18n("Configure Colors"), 0, 0, this, SLOT(openColorConfiguration()), actionCollection(), "open_colors_window");
  setCentralWidget(windowContainer);

  /* Initialize KMainWindow->statusBar() */
  statusBar();
  statusBar()->insertItem(i18n("Ready."),StatusText,1);
  statusBar()->insertItem("lagometer",LagOMeter,0,true);
  /* Show "Lag unknown" */
  resetLag();
  statusBar()->setItemAlignment(StatusText,QLabel::AlignLeft);

  addStatusView();
  connect( windowContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changedView(QWidget*)) );

  createGUI();
  readOptions();
}

ServerWindow::~ServerWindow()
{
  kdDebug() << "ServerWindow::~ServerWindow()" << endl;
}

void ServerWindow::openPreferences()
{
  emit openPrefsDialog();
}

void ServerWindow::showToolbar()
{
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.serverWindowToolBarStatus=showToolBarAction->isChecked();
}

void ServerWindow::showStatusbar()
{
  if(showStatusBarAction->isChecked()) statusBar()->show();
  else statusBar()->hide();

  KonversationApplication::preferences.serverWindowStatusBarStatus=showStatusBarAction->isChecked();
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
    QString logLine(QString("[%1] %2\n").arg(time.toString("hh:mm:ss")).arg(text));
    logfile.writeBlock(logLine,logLine.length());
    logfile.close();
  }
  else kdWarning() << "ServerWindow::logText(): open(IO_Append) for " << logfile.name() << " failed!" << endl;
}

void ServerWindow::setNickname(const QString& newNickname)
{
  nicknameButton->setText(newNickname);
}

void ServerWindow::newText(QWidget* view)
{
  /* FIXME: Should be compared to ChatWindow* but the status Window currently is something else */
  if(view!=(QWidget*) windowContainer->currentPage())
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
  /* Tool bar settings */
  showToolBarAction->setChecked(KonversationApplication::preferences.serverWindowToolBarStatus);
  toolBar("mainToolBar")->setBarPos((KToolBar::BarPosition) KonversationApplication::preferences.serverWindowToolBarPos);
  toolBar("mainToolBar")->setIconText((KToolBar::IconText) KonversationApplication::preferences.serverWindowToolBarIconText);
  toolBar("mainToolBar")->setIconSize(KonversationApplication::preferences.serverWindowToolBarIconSize);
  showToolbar();

  /* Status bar settings */
  showStatusBarAction->setChecked(KonversationApplication::preferences.serverWindowStatusBarStatus);
  showStatusbar();

  QSize size=KonversationApplication::preferences.getServerWindowSize();
  if(!size.isEmpty())
  {
    resize(size);
  }
}
/* Will not actually save the options but write them into the prefs structure */
void ServerWindow::saveOptions()
{
  KonversationApplication::preferences.setServerWindowSize(size());

  KonversationApplication::preferences.serverWindowToolBarPos=toolBar("mainToolBar")->barPos();
  KonversationApplication::preferences.serverWindowToolBarIconText=toolBar("mainToolBar")->iconText();
  KonversationApplication::preferences.serverWindowToolBarIconSize=toolBar("mainToolBar")->iconSize();
}

bool ServerWindow::queryExit()
{
  kdDebug() << "ServerWindow::queryExit()" << endl;
  QString command=filter.parse("/quit","");
  server->queue(filter.getServerOutput());
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


  if(!hilightDialog)
  {
    hilightDialog=new HighlightDialog(this,KonversationApplication::preferences.getHilightList2(),
                                      KonversationApplication::preferences.getHilightSize());
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
    buttonsDialog=new QuickButtonsDialog(KonversationApplication::preferences.getButtonList(),
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

void ServerWindow::openIgnore()
{
  if(!ignoreDialog)
  {
    ignoreDialog=new IgnoreDialog(KonversationApplication::preferences.getIgnoreList(),
                                  KonversationApplication::preferences.getIgnoreSize());
    connect(ignoreDialog,SIGNAL (cancelClicked(QSize)),this,SLOT (closeIgnore(QSize)) );
    connect(ignoreDialog,SIGNAL (applyClicked(QPtrList<Ignore>)),this,SLOT (applyIgnore(QPtrList<Ignore>)) );
    ignoreDialog->show();
  }
}

void ServerWindow::applyIgnore(QPtrList<Ignore> newList)
{
  KonversationApplication::preferences.setIgnoreList(newList);
  emit prefsChanged();
}

void ServerWindow::closeIgnore(QSize newSize)
{
  KonversationApplication::preferences.setIgnoreSize(newSize);
  emit prefsChanged();

  delete ignoreDialog;
  ignoreDialog=0;
}

void ServerWindow::openNotify()
{
  if(!notifyDialog)
  {
    notifyDialog=new NotifyDialog(KonversationApplication::preferences.getNotifyList(),
                                  KonversationApplication::preferences.getNotifySize(),
                                  KonversationApplication::preferences.getUseNotify(),
                                  KonversationApplication::preferences.getNotifyDelay());
    connect(notifyDialog,SIGNAL (cancelClicked(QSize)),this,SLOT (closeNotify(QSize)) );
    connect(notifyDialog,SIGNAL (applyClicked(QStringList,bool,int)),this,SLOT (applyNotify(QStringList,bool,int)) );
    notifyDialog->show();
  }
}

void ServerWindow::applyNotify(QStringList newList,bool use,int delay)
{
  KonversationApplication::preferences.setNotifyList(newList);
  KonversationApplication::preferences.setNotifyDelay(delay);
  KonversationApplication::preferences.setUseNotify(use);

  /* Restart notify timer if desired */
  if(use) server->startNotifyTimer();

  emit prefsChanged();
}

void ServerWindow::closeNotify(QSize newSize)
{
  KonversationApplication::preferences.setNotifySize(newSize);
  emit prefsChanged();

  delete notifyDialog;
  notifyDialog=0;
}

void ServerWindow::openColorConfiguration()
{
	colorConfigurationDialog = new ColorConfiguration(KonversationApplication::preferences.getActionMessageColor(),
																										KonversationApplication::preferences.getBacklogMessageColor(),
																										KonversationApplication::preferences.getChannelMessageColor(),
																										KonversationApplication::preferences.getCommandMessageColor(),
																										KonversationApplication::preferences.getLinkMessageColor(),
																										KonversationApplication::preferences.getQueryMessageColor(),
																										KonversationApplication::preferences.getServerMessageColor(),
																										KonversationApplication::preferences.getColorConfigurationSize());
	connect(colorConfigurationDialog, SIGNAL(saveFontColorSettings(QString, QString, QString, QString, QString, QString, QString)),
					this, SLOT(applyColorConfiguration(QString, QString, QString, QString, QString, QString, QString)));
	connect(colorConfigurationDialog, SIGNAL(closeFontColorConfiguration(QSize)),
					this, SLOT(closeColorConfiguration(QSize)));

	colorConfigurationDialog->show();
}

void ServerWindow::applyColorConfiguration(QString actionTextColor, QString backlogTextColor, QString channelTextColor,
														 							 QString commandTextColor, QString linkTextColor, QString queryTextColor,
																					 QString serverTextColor)
{
	KonversationApplication::preferences.setActionMessageColor(actionTextColor);
	KonversationApplication::preferences.setBacklogMessageColor(backlogTextColor);
	KonversationApplication::preferences.setChannelMessageColor(channelTextColor);
	KonversationApplication::preferences.setCommandMessageColor(commandTextColor);
	KonversationApplication::preferences.setLinkMessageColor(linkTextColor);
	KonversationApplication::preferences.setQueryMessageColor(queryTextColor);
	KonversationApplication::preferences.setServerMessageColor(serverTextColor);
	emit prefsChanged();
}

void ServerWindow::closeColorConfiguration(QSize windowSize)
{
	KonversationApplication::preferences.setColorConfigurationSize(windowSize);
	emit prefsChanged();
	disconnect(colorConfigurationDialog, SIGNAL(saveFontColorSettings(QString, QString, QString, QString, QString, QString, QString)),
				 		 this, SLOT(applyColorConfiguration(QString, QString, QString, QString, QString, QString, QString)));
	disconnect(colorConfigurationDialog, SIGNAL(closeFontColorConfiguration(QSize)),
						 this, SLOT(closeColorConfiguration(QSize)));
	delete colorConfigurationDialog;
	colorConfigurationDialog = 0;
}

void ServerWindow::channelPrefsChanged()
{
  emit prefsChanged();
}

void ServerWindow::setLog(bool activated)
{
  log=activated;
}

LedTabWidget* ServerWindow::getWindowContainer()
{
  return windowContainer;
}

int ServerWindow::spacing()
{
  return KDialog::spacingHint();
}

int ServerWindow::margin()
{
  return KDialog::marginHint();
}

void ServerWindow::updateLag(int msec)
{
  statusBar()->changeItem(i18n("Ready."),StatusText);

  QString lagString(i18n("Lag: %1 ms").arg(msec));
  statusBar()->changeItem(lagString,LagOMeter);
}

void ServerWindow::tooLongLag(int msec)
{
  if((msec % 5000)==0)
  {
    QString lagString(i18n("No answer from server for more than %1 seconds").arg(msec/1000));
    statusBar()->changeItem(lagString,StatusText);
  }

  QString lagString(i18n("Lag: %1 s").arg(msec/1000));
  statusBar()->changeItem(lagString,LagOMeter);
}

void ServerWindow::resetLag()
{
  statusBar()->changeItem(i18n("Lag: not known"),LagOMeter);
}
