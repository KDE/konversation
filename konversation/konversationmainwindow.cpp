/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  konversationmainwindow.cpp  -  The main window where all other views go
  begin:     Don Apr 17 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kmenubar.h>

/*
#include <qdir.h>

#include <kdialog.h>
#include <kaccel.h>
*/

#include "ledtabwidget.h"
#include "konversationmainwindow.h"
#include "konversationapplication.h"
#include "statuspanel.h"

KonversationMainWindow::KonversationMainWindow() : KMainWindow()
{
  kdDebug() << "KonversationMainWindow::KonversationMainWindow()" << endl;
/*
  // Init variables before anything else can happen
  hilightDialog=0;
  ignoreDialog=0;
  notifyDialog=0;
  buttonsDialog=0;
  colorConfigurationDialog=0;
  frontView=0;
  nicksOnlineWindow=0;
  dccPanel=0;
  dccPanelOpen=false;
  rawLog=0;

  setServer(newServer);
*/
  viewContainer=new LedTabWidget(this,"main_window_tab_widget");
  viewContainer->setTabPosition(QTabWidget::Bottom);
  
  setCentralWidget(viewContainer);
/*
  connect(getServer(),SIGNAL(repaintTabs()),getWindowContainer(),SLOT(updateTabs()) );
  
  KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); // file_quit
*/
  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); // options_show_toolbar
  showStatusBarAction=KStdAction::showStatusbar(this,SLOT(showStatusbar()),actionCollection()); // options_show_statusbar
  showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection()); // options_show_menubar
/*
  KStdAction::preferences(this,SLOT(openPreferences()),actionCollection()); // options_configure
  new KAction(i18n("Buttons"),0,0,this,SLOT (openButtons()),actionCollection(),"open_buttons_window");
  new KAction(i18n("Highlight List"),0,0,this,SLOT (openHilight()),actionCollection(),"open_hilight_window");
  new KAction(i18n("Notify List"),0,0,this,SLOT (openNotify()),actionCollection(),"open_notify_window");
  new KAction(i18n("Nicks Online"), 0, 0, this, SLOT(openNicksOnlineWindow()), actionCollection(), "open_nicksonline_window");
  new KAction(i18n("Ignore List"),0,0,this,SLOT (openIgnore()),actionCollection(),"open_ignore_window");
  new KAction(i18n("Configure Colors"), 0, 0, this, SLOT(openColorConfiguration()), actionCollection(), "open_colors_window");
  
  // Keyboard accelerators to navigate through the different pages
  KAccel* accelerator=accel();
  accelerator->insert("Next Tab",i18n("Next Tab"),i18n("Go to next tab"),KShortcut("Alt+Right"),this,SLOT(nextTab()));
  accelerator->insert("Previous Tab",i18n("Previous Tab"),i18n("Go to previous tab"),KShortcut("Alt+Left"),this,SLOT(previousTab()));
  accelerator->insert("Go to Tab 1",i18n("Tab %1").arg(1),i18n("Go to tab number %1").arg(1),KShortcut("Alt+1"),this,SLOT(goToTab0()));
  accelerator->insert("Go to Tab 2",i18n("Tab %1").arg(2),i18n("Go to tab number %1").arg(2),KShortcut("Alt+2"),this,SLOT(goToTab1()));
  accelerator->insert("Go to Tab 3",i18n("Tab %1").arg(3),i18n("Go to tab number %1").arg(3),KShortcut("Alt+3"),this,SLOT(goToTab2()));
  accelerator->insert("Go to Tab 4",i18n("Tab %1").arg(4),i18n("Go to tab number %1").arg(4),KShortcut("Alt+4"),this,SLOT(goToTab3()));
  accelerator->insert("Go to Tab 5",i18n("Tab %1").arg(5),i18n("Go to tab number %1").arg(5),KShortcut("Alt+5"),this,SLOT(goToTab4()));
  accelerator->insert("Go to Tab 6",i18n("Tab %1").arg(6),i18n("Go to tab number %1").arg(6),KShortcut("Alt+6"),this,SLOT(goToTab5()));
  accelerator->insert("Go to Tab 7",i18n("Tab %1").arg(7),i18n("Go to tab number %1").arg(7),KShortcut("Alt+7"),this,SLOT(goToTab6()));
  accelerator->insert("Go to Tab 8",i18n("Tab %1").arg(8),i18n("Go to tab number %1").arg(8),KShortcut("Alt+8"),this,SLOT(goToTab7()));
  accelerator->insert("Go to Tab 9",i18n("Tab %1").arg(9),i18n("Go to tab number %1").arg(9),KShortcut("Alt+9"),this,SLOT(goToTab8()));
  accelerator->insert("Go to Tab 0",i18n("Tab %1").arg(0),i18n("Go to tab number %1").arg(0),KShortcut("Alt+0"),this,SLOT(goToTab9()));
  accelerator->insert("Find text",i18n("Find text").arg(0),i18n("Find text").arg(0),KShortcut("F3"),this,SLOT(findTextShortcut()));
*/
  // Initialize KMainWindow->statusBar()
  statusBar();
/*
  statusBar()->insertItem(i18n("Ready."),StatusText,1);
  statusBar()->insertItem("lagometer",LagOMeter,0,true);
  // Show "Lag unknown"
  resetLag();
  statusBar()->setItemAlignment(StatusText,QLabel::AlignLeft);
*/
  // Initialize KMainWindow->menuBar()
  showMenubar();
/*
  addStatusView();
  if(KonversationApplication::preferences.getRawLog()) addRawLog();

  connect( windowContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changedView(QWidget*)) );
  connect( windowContainer,SIGNAL (closeTab(QWidget*)),this,SLOT (closeTab(QWidget*)) );
  
  connect(&filter,SIGNAL (openQuery(const QString&,const QString&)),
         getServer(),SLOT (addQuery(const QString&,const QString&)) );

*/
  createGUI();
  readOptions();
}

KonversationMainWindow::~KonversationMainWindow()
{
  kdDebug() << "KonversationMainWindow::~KonversationMainWindow()" << endl;
/*
  if(nicksOnlineWindow) nicksOnlineWindow->closeButton();
  deleteDccPanel();
  if(rawLog) delete rawLog;
*/
}

/*
void ServerWindow::openPreferences()
{
  emit openPrefsDialog();
}
*/

void KonversationMainWindow::showToolbar()
{
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.serverWindowToolBarStatus=showToolBarAction->isChecked();
}

void KonversationMainWindow::showMenubar()
{
  if(showMenuBarAction->isChecked()) menuBar()->show();
  else
  {
    QString accel=showMenuBarAction->shortcut().toString();
    KMessageBox::information(this,i18n("<qt>This will hide the menu bar completely."
                                       "You can show it again by typing %1.</qt>").arg(accel),
                                       "Hide menu bar","HideMenuBarWarning");
    menuBar()->hide();
  }

  KonversationApplication::preferences.serverWindowMenuBarStatus=showMenuBarAction->isChecked();
}

void KonversationMainWindow::showStatusbar()
{
  if(showStatusBarAction->isChecked()) statusBar()->show();
  else statusBar()->hide();

  KonversationApplication::preferences.serverWindowStatusBarStatus=showStatusBarAction->isChecked();
}

/*
void ServerWindow::setServer(Server* newServer)
{
  // to make sure that the new server will open a fresh nicks online window
  if(nicksOnlineWindow)
  {
    delete nicksOnlineWindow;
    nicksOnlineWindow=0;
  }

  server_=newServer;

  connect(this,SIGNAL (closeChannel(const QString&)),getServer(),SLOT (closeChannel(const QString&)));
  connect(this,SIGNAL (closeQuery(const QString&)),getServer(),SLOT (closeQuery(const QString&)));
}

Server* ServerWindow::getServer()
{
  return server_;
}

void ServerWindow::setIdentity(const Identity *identity)
{
  filter.setIdentity(identity);
}

void ServerWindow::appendToRaw(const QString& message)
{
  if(rawLog) rawLog->appendRaw(message);
}

void ServerWindow::appendToStatus(const QString& type,const QString& message)
{
  statusPanel->appendServerMessage(type,message);
}

void ServerWindow::appendToFrontmost(const QString& type,const QString& message)
{
  // TODO: Make it an option to direct all status stuff into the status panel

  if(frontView==0) // Check if the frontView can actually display text
  {
    statusPanel->appendServerMessage(type,message);
    newText(statusPanel);
  }
  else
    frontView->appendServerMessage(type,message);
}
*/
void KonversationMainWindow::addView(QWidget* view,int color,const QString& label,bool on)
{
  // TODO: Make sure to add DCC status tab at the end of the list and all others
  // before the DCC tab. Maybe we should also make sure to order Channels
  // Queries and DCC chats in groups
  viewContainer->addTab(view,label,color,on);
  // TODO: Check, if user was typing in old input line
  if(KonversationApplication::preferences.getBringToFront())
  {
    showView(view);
  }
}

void KonversationMainWindow::showView(QWidget* view)
{
  // Don't bring Tab to front if TabWidget is hidden. Otherwise QT gets confused
  // and shows the Tab as active but will display the wrong pane
  if(viewContainer->isVisible())
  {
    // TODO: add adjustFocus() here?
    viewContainer->showPage(view);
  }
}
/*
void ServerWindow::closeTab(QWidget* viewToClose)
{
  ChatWindow* view=(ChatWindow*) viewToClose;
  QString viewName=view->getName();
  ChatWindow::WindowType viewType=view->getType();

  if(viewType==ChatWindow::Status)
  {
    int result= KMessageBox::warningYesNo(
                  this,
                  i18n("Do you really want to disconnect from %1?").arg(getServer()->getIrcName()),
                  i18n("Quit server"),
                  KStdGuiItem::yes(),
                  KStdGuiItem::no(),
                  "QuitServerOnTabClose");

    if(result==KMessageBox::Yes)
    {
      QString command=filter.parse(getServer()->getNickname(),KonversationApplication::preferences.getCommandChar()+"quit",QString::null);
      getServer()->queue(filter.getServerOutput());
    }
  }
  else if(viewType==ChatWindow::Channel)  emit closeChannel(viewName);
  else if(viewType==ChatWindow::Query)    emit closeQuery(viewName);
  else if(viewType==ChatWindow::DccPanel) closeDccPanel();
  else if(viewType==ChatWindow::DccChat);
  else if(viewType==ChatWindow::RawLog)   closeRawLog();
  else if(viewType==ChatWindow::Notice);
  else if(viewType==ChatWindow::SNotice);
}

void ServerWindow::addDccPanel()
{
  // if the panel wasn't open yet
  if(dccPanel==0)
  {
    dccPanel=new DccPanel(getWindowContainer());
    addView(dccPanel,3,i18n("DCC Status"));
    dccPanelOpen=true;
    connect(dccPanel,SIGNAL(requestDccSend()),getServer(),SLOT(requestDccSend()));
    kdDebug() << "ServerWindow::addDccPanel(): " << dccPanel << endl;
  }
  // show already opened panel
  else
  {
    if(!dccPanelOpen)
    {
      addView(dccPanel,3,i18n("DCC Status"));
      dccPanelOpen=true;
    }
    newText(dccPanel);
  }
}

DccPanel* ServerWindow::getDccPanel()
{
  return dccPanel;
}

void ServerWindow::closeDccPanel()
{
  // if there actually is a dcc panel
  if(dccPanel)
  {
     // hide it from view, does not delete it
    windowContainer->removePage(dccPanel);
    dccPanelOpen=false;
  }
}

void ServerWindow::deleteDccPanel()
{
  if(dccPanel)
  {
    closeDccPanel();
    delete dccPanel;
    dccPanel=0;
  }
}
*/
StatusPanel* KonversationMainWindow::addStatusView(Server* server)
{
  StatusPanel* statusView=new StatusPanel(getViewContainer());
  addView(statusView,2,i18n(server->getServerName()));
  statusView->setServer(server);
  statusView->setIdentity(server->getIdentity());
  
  connect(statusView,SIGNAL (newText(QWidget*)),this,SLOT (newText(QWidget*)) );
  connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );

  return statusView;
}
/*
void ServerWindow::addRawLog()
{
  kdDebug() << "ServerWindow::addRawLog()" << endl;
  if(rawLog==0)
  {
    rawLog=new RawLog(getWindowContainer());
    addView(rawLog,2,i18n("Raw Log"),false);
    
    rawLog->setLog(false);
  }
  else
    windowContainer->showPage(rawLog);
}

void ServerWindow::closeRawLog()
{
  if(rawLog)
  {
    getWindowContainer()->removePage(rawLog);
    delete rawLog;
    rawLog=0;
  }
}

void ServerWindow::setNickname(const QString& newNickname)
{
  // TODO: connect this to the appropriate server's nickname signal
  statusPanel->setNickname(newNickname);
}
*/
void KonversationMainWindow::newText(QWidget* view)
{
  // FIXME: Should be compared to ChatWindow* but the status Window currently is something else
  // Now that the status Window is a ChatWindow* we can start cleaning up here
  if(view!=static_cast<QWidget*>(getViewContainer()->currentPage()))
  {
    getViewContainer()->changeTabState(view,true);
  }
}
/*
void ServerWindow::changedView(QWidget* view)
{
  frontView=0;
  searchView=0;
  
  ChatWindow* pane=static_cast<ChatWindow*>(view);
  // Make sure that only views with info output get to be the frontView
  if(pane->getType()!=ChatWindow::DccPanel &&
     pane->getType()!=ChatWindow::RawLog) frontView=pane;
  // Make sure that only text views get to be the searchView
  if(pane->getType()!=ChatWindow::DccPanel) searchView=pane;

  windowContainer->changeTabState(view,false);
}
*/
void KonversationMainWindow::readOptions()
{
/*
  // Tool bar settings
  showToolBarAction->setChecked(KonversationApplication::preferences.serverWindowToolBarStatus);
  toolBar("mainToolBar")->setBarPos((KToolBar::BarPosition) KonversationApplication::preferences.serverWindowToolBarPos);
  toolBar("mainToolBar")->setIconText((KToolBar::IconText) KonversationApplication::preferences.serverWindowToolBarIconText);
  toolBar("mainToolBar")->setIconSize(KonversationApplication::preferences.serverWindowToolBarIconSize);
  showToolbar();

  // Status bar settings
  showStatusBarAction->setChecked(KonversationApplication::preferences.serverWindowStatusBarStatus);
  showStatusbar();

  // Menu bar settings
  showMenuBarAction->setChecked(KonversationApplication::preferences.serverWindowMenuBarStatus);
  showMenubar();
*/
  QSize size=KonversationApplication::preferences.getServerWindowSize();
  if(!size.isEmpty())
  {
    resize(size);
  }
}
/*
// Will not actually save the options but write them into the prefs structure
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
  QString command=filter.parse(getServer()->getNickname(),KonversationApplication::preferences.getCommandChar()+"quit",QString::null);
  getServer()->queue(filter.getServerOutput());

  saveOptions();

  getServer()->setServerWindow(0);
  return true;
}

void ServerWindow::quitProgram()
{
  kdDebug() << "ServerWindow::quitProgram()" << endl;
  // will call queryClose()
  close();
}

void ServerWindow::openHilight()
{
  if(!hilightDialog)
  {
    hilightDialog=new HighlightDialog(this,KonversationApplication::preferences.getHilightList(),
                                      KonversationApplication::preferences.getHilightSize());
    connect(hilightDialog,SIGNAL (cancelClicked(QSize)),this,SLOT (closeHilight(QSize)) );
    connect(hilightDialog,SIGNAL (applyClicked(QPtrList<Highlight>)),this,SLOT (applyHilight(QPtrList<Highlight>)) );
  }
}

void ServerWindow::applyHilight(QPtrList<Highlight> hilightList)
{
  KonversationApplication::preferences.setHilightList(hilightList);
  emit prefsChanged();
}

void ServerWindow::closeHilight(QSize newHilightSize)
{
  KonversationApplication::preferences.setHilightSize(newHilightSize);
  emit prefsChanged();

  delete hilightDialog;
  hilightDialog=0;
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
  getServer()->updateChannelQuickButtons(newButtonList);
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

  // Restart notify timer if desired
  if(use) getServer()->startNotifyTimer();

  emit prefsChanged();
}

void ServerWindow::closeNotify(QSize newSize)
{
  KonversationApplication::preferences.setNotifySize(newSize);
  emit prefsChanged();

  delete notifyDialog;
  notifyDialog=0;
}

void ServerWindow::openNicksOnlineWindow()
{
  if(!nicksOnlineWindow)
  {
    nicksOnlineWindow=new NicksOnline(KonversationApplication::preferences.getNicksOnlineSize());
    connect(nicksOnlineWindow,SIGNAL (editClicked()),this,SLOT (openNotify()) );
    connect(nicksOnlineWindow,SIGNAL (doubleClicked(QListViewItem*)),getServer(),SLOT (notifyAction(QListViewItem*)) );
    connect(nicksOnlineWindow,SIGNAL (closeClicked(QSize)),this,SLOT (closeNicksOnlineWindow(QSize)) );
    connect(getServer(),SIGNAL (nicksNowOnline(const QStringList&)),nicksOnlineWindow,SLOT (setOnlineList(const QStringList&)) );
    nicksOnlineWindow->show();
  }
}

void ServerWindow::closeNicksOnlineWindow(QSize newSize)
{
  KonversationApplication::preferences.setNicksOnlineSize(newSize);
  emit prefsChanged();

  delete nicksOnlineWindow;
  nicksOnlineWindow=0;
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
																										KonversationApplication::preferences.getTimeColor(),
																										KonversationApplication::preferences.getTextViewBackground(),
																										KonversationApplication::preferences.getColorConfigurationSize());

  connect(colorConfigurationDialog, SIGNAL(saveFontColorSettings(QString, QString, QString, QString, QString, QString, QString, QString, QString)),
					this, SLOT(applyColorConfiguration(QString, QString, QString, QString, QString, QString, QString, QString, QString)));
	connect(colorConfigurationDialog, SIGNAL(closeFontColorConfiguration(QSize)),
					this, SLOT(closeColorConfiguration(QSize)));

	colorConfigurationDialog->show();
}

void ServerWindow::applyColorConfiguration(QString actionTextColor, QString backlogTextColor, QString channelTextColor,
														 							 QString commandTextColor, QString linkTextColor, QString queryTextColor,
																					 QString serverTextColor, QString timeColor, QString backgroundColor)
{
	KonversationApplication::preferences.setActionMessageColor(actionTextColor);
	KonversationApplication::preferences.setBacklogMessageColor(backlogTextColor);
	KonversationApplication::preferences.setChannelMessageColor(channelTextColor);
	KonversationApplication::preferences.setCommandMessageColor(commandTextColor);
	KonversationApplication::preferences.setLinkMessageColor(linkTextColor);
	KonversationApplication::preferences.setQueryMessageColor(queryTextColor);
	KonversationApplication::preferences.setServerMessageColor(serverTextColor);
	KonversationApplication::preferences.setTimeColor(timeColor);
	KonversationApplication::preferences.setTextViewBackground(backgroundColor);
	emit prefsChanged();
}

void ServerWindow::closeColorConfiguration(QSize windowSize)
{
	KonversationApplication::preferences.setColorConfigurationSize(windowSize);
	emit prefsChanged();
	disconnect(colorConfigurationDialog, SIGNAL(saveFontColorSettings(QString, QString, QString, QString, QString, QString, QString, QString, QString)),
				 		 this, SLOT(applyColorConfiguration(QString, QString, QString, QString, QString, QString, QString, QString, QString)));
	disconnect(colorConfigurationDialog, SIGNAL(closeFontColorConfiguration(QSize)),
						 this, SLOT(closeColorConfiguration(QSize)));
	delete colorConfigurationDialog;
	colorConfigurationDialog = 0;
}

void ServerWindow::channelPrefsChanged()
{
  emit prefsChanged();
}
*/
LedTabWidget* KonversationMainWindow::getViewContainer()
{
  return viewContainer;
}
/*
int ServerWindow::spacing()
{
  return KDialog::spacingHint();
}

int ServerWindow::margin()
{
  return KDialog::marginHint();
}

void ServerWindow::updateFonts()
{
  kdDebug() << "ServerWindow::updateFonts()" << endl;

  if(rawLog) rawLog->updateFonts(); // FIXME: should be done by the respective server, no?
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

bool ServerWindow::isFrontView(const ChatWindow* view)
{
  return(frontView==view);
}

void ServerWindow::nextTab()
{
  goToTab(windowContainer->currentPageIndex()+1);
}

void ServerWindow::previousTab()
{
  goToTab(windowContainer->currentPageIndex()-1);
}

void ServerWindow::goToTab(int page)
{
  if(page>=0 && page<windowContainer->count())
  {
    windowContainer->setCurrentPage(page);
    ChatWindow* newPage=static_cast<ChatWindow*>(windowContainer->page(page));
    newPage->adjustFocus();
  }
}

void ServerWindow::findTextShortcut()
{
  if(searchView==0)
  {
    KMessageBox::sorry(this,
                       i18n("You can only search in text fields!"),
                       i18n("Find text information"));
  }
  else
  {
    searchView->getTextView()->search();
  }
}

// TODO: remove this once the StatusPanel is managed by the Server class
StatusPanel* ServerWindow::getStatusView()
{
  return statusPanel;
}

// I hope we can find a better way soon ... this is ridiculous"
void ServerWindow::goToTab0() { goToTab(0); }
void ServerWindow::goToTab1() { goToTab(1); }
void ServerWindow::goToTab2() { goToTab(2); }
void ServerWindow::goToTab3() { goToTab(3); }
void ServerWindow::goToTab4() { goToTab(4); }
void ServerWindow::goToTab5() { goToTab(5); }
void ServerWindow::goToTab6() { goToTab(6); }
void ServerWindow::goToTab7() { goToTab(7); }
void ServerWindow::goToTab8() { goToTab(8); }
void ServerWindow::goToTab9() { goToTab(9); }

*/
#include "konversationmainwindow.moc"
