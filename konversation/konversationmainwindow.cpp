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

#include <kaccel.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <kmenubar.h>

#include "ledtabwidget.h"
#include "chatwindow.h"
#include "konversationmainwindow.h"
#include "konversationapplication.h"
#include "statuspanel.h"
#include "channel.h"
#include "query.h"
#include "rawlog.h"
#include "dccpanel.h"
#include "dcctransferhandler.h"
#include "highlightdialog.h"
#include "quickbuttonsdialog.h"
#include "ignoredialog.h"
#include "notifydialog.h"
#include "nicksonline.h"
#include "colorconfiguration.h"

KonversationMainWindow::KonversationMainWindow() : KMainWindow()
{
  kdDebug() << "KonversationMainWindow::KonversationMainWindow()" << endl;

  // Init variables before anything else can happen
  frontView=0;
  searchView=0;
  frontServer=0;
  dccPanel=0;
  dccPanelOpen=false;

  dccTransferHandler=new DccTransferHandler(this);

  hilightDialog=0;
  ignoreDialog=0;
  notifyDialog=0;
  buttonsDialog=0;
  colorConfigurationDialog=0;
  nicksOnlineWindow=0;

  viewContainer=new LedTabWidget(this,"main_window_tab_widget");
  viewContainer->setTabPosition(QTabWidget::Bottom);

  setCentralWidget(viewContainer);

  KStdAction::quit(this,SLOT(quitProgram()),actionCollection()); // file_quit

  showToolBarAction=KStdAction::showToolbar(this,SLOT(showToolbar()),actionCollection()); // options_show_toolbar
  showStatusBarAction=KStdAction::showStatusbar(this,SLOT(showStatusbar()),actionCollection()); // options_show_statusbar
  showMenuBarAction=KStdAction::showMenubar(this,SLOT(showMenubar()),actionCollection()); // options_show_menubar

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
  accelerator->insert("Find text",i18n("Find text"),i18n("Find text"),KShortcut("F3"),this,SLOT(findTextShortcut()));

  // Initialize KMainWindow->statusBar()
  statusBar();
  statusBar()->insertItem(i18n("Ready."),StatusText,1);
  statusBar()->insertItem("lagometer",LagOMeter,0,true);
  // Show "Lag unknown"
  resetLag();
  statusBar()->setItemAlignment(StatusText,QLabel::AlignLeft);

  // Initialize KMainWindow->menuBar()
  showMenubar();

  connect( viewContainer,SIGNAL (currentChanged(QWidget*)),this,SLOT (changeView(QWidget*)) );
  connect( viewContainer,SIGNAL (closeTab(QWidget*)),this,SLOT (closeView(QWidget*)) );

  createGUI();
  readOptions();
}

KonversationMainWindow::~KonversationMainWindow()
{
  kdDebug() << "KonversationMainWindow::~KonversationMainWindow()" << endl;

  if(nicksOnlineWindow) nicksOnlineWindow->closeButton();

  deleteDccPanel();
  if(dccTransferHandler) delete dccTransferHandler;
}

void KonversationMainWindow::openPreferences()
{
  emit openPrefsDialog();
}

void KonversationMainWindow::showToolbar()
{
  if(showToolBarAction->isChecked()) toolBar("mainToolBar")->show();
  else toolBar("mainToolBar")->hide();

  KonversationApplication::preferences.mainWindowToolBarStatus=showToolBarAction->isChecked();
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

  KonversationApplication::preferences.mainWindowMenuBarStatus=showMenuBarAction->isChecked();
}

void KonversationMainWindow::showStatusbar()
{
  if(showStatusBarAction->isChecked()) statusBar()->show();
  else statusBar()->hide();

  KonversationApplication::preferences.mainWindowStatusBarStatus=showStatusBarAction->isChecked();
}

void KonversationMainWindow::appendToFrontmost(const QString& type,const QString& message,ChatWindow* serverView)
{
  // TODO: Make it an option to direct all status stuff into the status panel

  updateFrontView();
  if(frontView==0) // Check if the frontView can actually display text
  {
    // if not, take server specified fallback view instead
    serverView->appendServerMessage(type,message);
    newText(serverView);
  }
  else
    frontView->appendServerMessage(type,message);
}

void KonversationMainWindow::addView(ChatWindow* view,int color,const QString& label,bool on)
{
  // TODO: Make sure to add DCC status tab at the end of the list and all others
  // before the DCC tab. Maybe we should also make sure to order Channels
  // Queries and DCC chats in groups
  viewContainer->addTab(view,label,color,on);
  // TODO: Check, if user was typing in old input line

  // bring view to front unless it's a raw log window
  if(KonversationApplication::preferences.getBringToFront() &&
    view->getType()!=ChatWindow::RawLog)
  {
    showView(view);
  }
}

void KonversationMainWindow::showView(ChatWindow* view)
{
  // Don't bring Tab to front if TabWidget is hidden. Otherwise QT gets confused
  // and shows the Tab as active but will display the wrong pane
  if(viewContainer->isVisible())
  {
    // TODO: add adjustFocus() here?
    viewContainer->showPage(view);
  }
}

void KonversationMainWindow::closeView(QWidget* viewToClose)
{
//  kdDebug() << "KonversationMainWindow::closeView(" << viewToClose << ")" << endl;

  ChatWindow* view=static_cast<ChatWindow*>(viewToClose);
  ChatWindow::WindowType viewType=view->getType();

  QString viewName=view->getName();
// the views should know by themselves how to close

  if(viewType==ChatWindow::Status)        view->closeYourself();
  else if(viewType==ChatWindow::Channel)  view->closeYourself();
  else if(viewType==ChatWindow::Query)    view->closeYourself();
  else if(viewType==ChatWindow::DccPanel) closeDccPanel();
  else if(viewType==ChatWindow::RawLog)   view->closeYourself();
/*
  else if(viewType==ChatWindow::DccChat);
  else if(viewType==ChatWindow::Notice);
  else if(viewType==ChatWindow::SNotice);
*/
}

void KonversationMainWindow::addDccPanel()
{
  // if the panel wasn't open yet
  if(dccPanel==0)
  {
    dccPanel=new DccPanel(getViewContainer());
    addView(dccPanel,3,i18n("DCC Status"));
    dccPanelOpen=true;
/*
    FIXME: where to connect this? It's not very sensible anyway, since
           the dcc panel is not server dependant
    connect(dccPanel,SIGNAL(requestDccSend()),getServer(),SLOT(requestDccSend()));
*/
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

DccPanel* KonversationMainWindow::getDccPanel()
{
  return dccPanel;
}

void KonversationMainWindow::closeDccPanel()
{
  // if there actually is a dcc panel
  if(dccPanel)
  {
     // hide it from view, does not delete it
    getViewContainer()->removePage(dccPanel);
    dccPanelOpen=false;
  }
}

void KonversationMainWindow::deleteDccPanel()
{
  if(dccPanel)
  {
    closeDccPanel();
    delete dccPanel;
    dccPanel=0;
  }
}

StatusPanel* KonversationMainWindow::addStatusView(Server* server)
{
  StatusPanel* statusView=new StatusPanel(getViewContainer());
  
  // first set up internal data ...
  statusView->setServer(server);
  statusView->setIdentity(server->getIdentity());

  // ... then put it into the tab widget, otherwise we'd have a race with server member
  addView(statusView,2,i18n(server->getServerName()),false);

  connect(statusView,SIGNAL (newText(QWidget*)),this,SLOT (newText(QWidget*)) );
  connect(statusView,SIGNAL (sendFile()),server,SLOT (requestDccSend()) );

  return statusView;
}

Channel* KonversationMainWindow::addChannel(Server* server, const QString& name)
{
  Channel* channel=new Channel(getViewContainer());
  channel->setServer(server);
  channel->setName(name);

  addView(channel,1,name);

  connect(channel,SIGNAL (newText(QWidget*)),this,SLOT (newText(QWidget*)) );
  connect(channel,SIGNAL (prefsChanged()),this,SLOT (channelPrefsChanged()) );

  return channel;
}

Query* KonversationMainWindow::addQuery(Server* server, const QString& name)
{
  Query* query=new Query(getViewContainer());
  query->setServer(server);
  query->setName(name);

  addView(query,0,name);

  connect(query,SIGNAL (newText(QWidget*)),this,SLOT (newText(QWidget*)) );

  return query;
}

RawLog* KonversationMainWindow::addRawLog(Server* server)
{
  kdDebug() << "KonversationMainWindow::addRawLog()" << endl;
  RawLog* rawLog=new RawLog(getViewContainer());

  rawLog->setServer(server);
  rawLog->setLog(false);

  addView(rawLog,2,i18n("Raw Log"),false);

  return rawLog;
}

void KonversationMainWindow::newText(QWidget* view)
{
  if(view!=getViewContainer()->currentPage())
  {
    getViewContainer()->changeTabState(view,true);
  }
}

void KonversationMainWindow::updateFrontView()
{
  ChatWindow* view=static_cast<ChatWindow*>(getViewContainer()->currentPage());

  // Make sure that only views with info output get to be the frontView
  if(view->getType()!=ChatWindow::DccPanel &&
     view->getType()!=ChatWindow::RawLog) frontView=view;
  // Make sure that only text views get to be the searchView
  if(view->getType()!=ChatWindow::DccPanel &&
     view->getType()!=ChatWindow::ChannelList) searchView=view;
}

void KonversationMainWindow::changeView(QWidget* viewToChange)
{
  frontView=0;
  searchView=0;

  ChatWindow* view=static_cast<ChatWindow*>(viewToChange);

  // display this server's lag time
  frontServer=view->getServer();
  if(frontServer) updateLag(frontServer,frontServer->getLag());

  updateFrontView();
/*
  // Make sure that only views with info output get to be the frontView
  if(view->getType()!=ChatWindow::DccPanel &&
     view->getType()!=ChatWindow::RawLog) frontView=view;
  // Make sure that only text views get to be the searchView
  if(view->getType()!=ChatWindow::DccPanel) searchView=view;
*/
  viewContainer->changeTabState(view,false);
}

void KonversationMainWindow::readOptions()
{
  // Tool bar settings
  showToolBarAction->setChecked(KonversationApplication::preferences.mainWindowToolBarStatus);
  toolBar("mainToolBar")->setBarPos((KToolBar::BarPosition) KonversationApplication::preferences.mainWindowToolBarPos);
  toolBar("mainToolBar")->setIconText((KToolBar::IconText) KonversationApplication::preferences.mainWindowToolBarIconText);
  toolBar("mainToolBar")->setIconSize(KonversationApplication::preferences.mainWindowToolBarIconSize);
  showToolbar();

  // Status bar settings
  showStatusBarAction->setChecked(KonversationApplication::preferences.mainWindowStatusBarStatus);
  showStatusbar();

  // Menu bar settings
  showMenuBarAction->setChecked(KonversationApplication::preferences.mainWindowMenuBarStatus);
  showMenubar();

  QSize size=KonversationApplication::preferences.getMainWindowSize();
  if(!size.isEmpty())
  {
    resize(size);
  }
}

// Will not actually save the options but write them into the prefs structure
void KonversationMainWindow::saveOptions()
{
  KonversationApplication::preferences.setMainWindowSize(size());

  KonversationApplication::preferences.mainWindowToolBarPos=toolBar("mainToolBar")->barPos();
  KonversationApplication::preferences.mainWindowToolBarIconText=toolBar("mainToolBar")->iconText();
  KonversationApplication::preferences.mainWindowToolBarIconSize=toolBar("mainToolBar")->iconSize();
}

bool KonversationMainWindow::queryClose()
{
  kdDebug() << "KonversationMainWindow::queryClose()" << endl;

  // send quit to all servers
  emit quitServer();
  saveOptions();

  return true;
}

void KonversationMainWindow::quitProgram()
{
  kdDebug() << "KonversationMainWindow::quitProgram()" << endl;
  // will call queryClose()
  close();
}

void KonversationMainWindow::openHilight()
{
  if(!hilightDialog)
  {
    hilightDialog=new HighlightDialog(this,KonversationApplication::preferences.getHilightList(),
                                      KonversationApplication::preferences.getHilightSize());
    connect(hilightDialog,SIGNAL (cancelClicked(QSize)),this,SLOT (closeHilight(QSize)) );
    connect(hilightDialog,SIGNAL (applyClicked(QPtrList<Highlight>)),this,SLOT (applyHilight(QPtrList<Highlight>)) );
  }
}

void KonversationMainWindow::applyHilight(QPtrList<Highlight> hilightList)
{
  KonversationApplication::preferences.setHilightList(hilightList);
  emit prefsChanged();
}

void KonversationMainWindow::closeHilight(QSize newHilightSize)
{
  KonversationApplication::preferences.setHilightSize(newHilightSize);
  emit prefsChanged();

  delete hilightDialog;
  hilightDialog=0;
}

void KonversationMainWindow::openButtons()
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

void KonversationMainWindow::applyButtons(QStringList newButtonList)
{
  KonversationApplication::preferences.setButtonList(newButtonList);
  emit prefsChanged();
  emit channelQuickButtonsChanged();
}

void KonversationMainWindow::closeButtons(QSize newButtonsSize)
{
  KonversationApplication::preferences.setButtonsSize(newButtonsSize);
  emit prefsChanged();

  delete buttonsDialog;
  buttonsDialog=0;
}

void KonversationMainWindow::openIgnore()
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

void KonversationMainWindow::applyIgnore(QPtrList<Ignore> newList)
{
  KonversationApplication::preferences.setIgnoreList(newList);
  emit prefsChanged();
}

void KonversationMainWindow::closeIgnore(QSize newSize)
{
  KonversationApplication::preferences.setIgnoreSize(newSize);
  emit prefsChanged();

  delete ignoreDialog;
  ignoreDialog=0;
}

void KonversationMainWindow::openNotify()
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

void KonversationMainWindow::applyNotify(QStringList newList,bool use,int delay)
{
  KonversationApplication::preferences.setNotifyList(newList);
  KonversationApplication::preferences.setNotifyDelay(delay);
  KonversationApplication::preferences.setUseNotify(use);

  // Restart notify timer if desired
  if(use) emit startNotifyTimer(0);
  emit prefsChanged();
}

void KonversationMainWindow::closeNotify(QSize newSize)
{
  KonversationApplication::preferences.setNotifySize(newSize);
  emit prefsChanged();

  delete notifyDialog;
  notifyDialog=0;
}

void KonversationMainWindow::openNicksOnlineWindow()
{
  if(!nicksOnlineWindow)
  {
    nicksOnlineWindow=new NicksOnline(KonversationApplication::preferences.getNicksOnlineSize());
    
    connect(nicksOnlineWindow,SIGNAL (editClicked()),this,SLOT (openNotify()) );
    connect(nicksOnlineWindow,SIGNAL (closeClicked(QSize)),this,SLOT (closeNicksOnlineWindow(QSize)) );
    
    connect(nicksOnlineWindow,SIGNAL (doubleClicked(const QString&,const QString&)),this,SLOT (notifyAction(const QString&,const QString&)) );
    
    connect(this,SIGNAL (nicksNowOnline(const QString&,const QStringList&)),nicksOnlineWindow,SLOT (setOnlineList(const QString&,const QStringList&)) );

    nicksOnlineWindow->show();
  }
}

void KonversationMainWindow::closeNicksOnlineWindow(QSize newSize)
{
  KonversationApplication::preferences.setNicksOnlineSize(newSize);
  emit prefsChanged();

  delete nicksOnlineWindow;
  nicksOnlineWindow=0;
}

// TODO: Let an own class handle notify things
void KonversationMainWindow::setOnlineList(Server* notifyServer,const QStringList& list)
{
  emit nicksNowOnline(notifyServer->getServerName(),list);
}

void KonversationMainWindow::notifyAction(const QString& serverName,const QString& nick)
{
  KonversationApplication* konv_app=static_cast<KonversationApplication*>(KApplication::kApplication());
  Server* server=konv_app->getServerByName(serverName);
  server->notifyAction(nick);
}

void KonversationMainWindow::openColorConfiguration()
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

void KonversationMainWindow::applyColorConfiguration(QString actionTextColor, QString backlogTextColor, QString channelTextColor,
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

void KonversationMainWindow::closeColorConfiguration(QSize windowSize)
{
  KonversationApplication::preferences.setColorConfigurationSize(windowSize);

  disconnect(colorConfigurationDialog, SIGNAL(saveFontColorSettings(QString, QString, QString, QString, QString, QString, QString, QString, QString)),
              this, SLOT(applyColorConfiguration(QString, QString, QString, QString, QString, QString, QString, QString, QString)));
  disconnect(colorConfigurationDialog, SIGNAL(closeFontColorConfiguration(QSize)),
             this, SLOT(closeColorConfiguration(QSize)));
  delete colorConfigurationDialog;
  colorConfigurationDialog=0;
}

void KonversationMainWindow::channelPrefsChanged()
{
  emit prefsChanged();
}

LedTabWidget* KonversationMainWindow::getViewContainer()
{
  return viewContainer;
}

void KonversationMainWindow::updateFonts()
{
  kdDebug() << "KonversationMainWindow::updateFonts()" << endl;

  getViewContainer()->updateTabs();
}

void KonversationMainWindow::updateLag(Server* lagServer,int msec)
{
  // show lag only of actual server
  if(lagServer==frontServer)
  {
    statusBar()->changeItem(i18n("Ready."),StatusText);

    QString lagString(i18n("Lag: %1 ms").arg(msec));
    statusBar()->changeItem(lagString,LagOMeter);
  }
}

void KonversationMainWindow::tooLongLag(Server* lagServer,int msec)
{
  if((msec % 5000)==0)
  {
    QString lagString(i18n("No answer from server %1 for more than %2 seconds").arg(lagServer->getServerName()).arg(msec/1000));
    statusBar()->changeItem(lagString,StatusText);
  }

  QString lagString(i18n("Lag: %1 s").arg(msec/1000));
  statusBar()->changeItem(lagString,LagOMeter);
}

// TODO: Make this server dependant
void KonversationMainWindow::resetLag()
{
  statusBar()->changeItem(i18n("Lag: not known"),LagOMeter);
}

void KonversationMainWindow::nextTab()
{
  goToTab(getViewContainer()->currentPageIndex()+1);
}

void KonversationMainWindow::previousTab()
{
  goToTab(getViewContainer()->currentPageIndex()-1);
}

void KonversationMainWindow::goToTab(int page)
{
  if(page>=0 && page<getViewContainer()->count())
  {
    getViewContainer()->setCurrentPage(page);
    ChatWindow* newPage=static_cast<ChatWindow*>(getViewContainer()->page(page));
    newPage->adjustFocus();
  }
}

void KonversationMainWindow::findTextShortcut()
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

// I hope we can find a better way soon ... this is ridiculous"
void KonversationMainWindow::goToTab0() { goToTab(0); }
void KonversationMainWindow::goToTab1() { goToTab(1); }
void KonversationMainWindow::goToTab2() { goToTab(2); }
void KonversationMainWindow::goToTab3() { goToTab(3); }
void KonversationMainWindow::goToTab4() { goToTab(4); }
void KonversationMainWindow::goToTab5() { goToTab(5); }
void KonversationMainWindow::goToTab6() { goToTab(6); }
void KonversationMainWindow::goToTab7() { goToTab(7); }
void KonversationMainWindow::goToTab8() { goToTab(8); }
void KonversationMainWindow::goToTab9() { goToTab(9); }

#include "konversationmainwindow.moc"
