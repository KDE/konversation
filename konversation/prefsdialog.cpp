/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefsdialog.cpp  -  This class holds the subpages for the preferences dialog
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "prefsdialog.h"

#include "serverlistitem.h"
#include "editserverdialog.h"
#include "konversationapplication.h"
#include "prefspagebehaviour.h"
#include "prefspagechatwinbehavior.h"
#include "prefspagechatwinappearance.h"
#include "prefspagecolorsappearance.h"

PrefsDialog::PrefsDialog(Preferences* preferences,bool noServer) :
             KDialogBase (KDialogBase::TreeList,i18n("Edit Preferences"),
                          KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                          KDialogBase::Ok,0,"edit_prefs",false,true)
{
  setPreferences(preferences);
  setShowIconsInTreeList(true);

  serverListPane = addPage(i18n("Server List"),QString::null,SmallIcon("network_local" ));
  identityPane = addPage(i18n("Identity"),QString::null,SmallIcon("identity"));

  setFolderIcon(QStringList::split(',', i18n("Appearance")), SmallIcon("looknfeel"));
  QFrame* chatWinAppearancePane = addPage(QStringList::split(',', i18n("Appearance") + "," + i18n("Chat Window")), QString::null, SmallIcon("window_new"));
  QFrame* colorsAppearancePane = addPage(QStringList::split(',', i18n("Appearance") + "," + i18n("Colors")),
    QString::null, SmallIcon("colorize"));

  setFolderIcon(QStringList::split(',', i18n("Behavior")), SmallIcon("configure"));
  QFrame* generalBehaviorPane = addPage(QStringList::split(',', i18n("Behavior") + "," + i18n("General")),
    QString::null,SmallIcon("exec"));
  QFrame* chatWinBehaviorPane = addPage(QStringList::split(',', i18n("Behavior") + "," + i18n("Chat Window")),QString::null, SmallIcon("window_new"));
  QFrame* tabBehaviorPane = addPage(QStringList::split(',', i18n("Behavior") + "," + i18n("Tab Bar")),
    QString::null, SmallIcon("tab_new"));
  QFrame* ignorePane = addPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Ignored Nicknames")),
    QString::null, SmallIcon("editdelete"));
  QFrame* aliasesPane = addPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Command Aliases")),
    QString::null,SmallIcon("editcopy"));
  QFrame* buttonsPane = addPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Quick Buttons")),
    QString::null, SmallIcon("keyboard"));
  QFrame* logSettingsPane = addPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Logging")),
    QString::null,SmallIcon("log"));
  QFrame* dccSettingsPane = addPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("DCC")),
    QString::null,SmallIcon("2rightarrow" ));

  setFolderIcon(QStringList::split(',', i18n("Notification")), SmallIcon("knotify"));
  notifyPane = addPage(QStringList::split(',', i18n("Notification") + "," + i18n("Watched Nicknames")),
    QString::null,SmallIcon("kfind"));
  QFrame* highlightPane = addPage(QStringList::split(',', i18n("Notification") + "," + i18n("Highlighting"))   ,QString::null,SmallIcon("paintbrush"));
  QFrame* OSDPane = addPage(QStringList::split(',', i18n("Notification") + "," + i18n("On Screen Display")),
    QString::null, SmallIcon("tv"));

  QFrame* dialogsPane = addPage(i18n("Warning Dialogs"), QString::null, SmallIcon("messagebox_warning"));


  // TODO: Uncomment this again when it's ready to go
  // QFrame* scriptsPane        =addPage(i18n("Scripting"));

  // Add pages to preferences dialog
  serverListPage=new PrefsPageServerList(serverListPane,preferences);
  identityPage = new PrefsPageIdentity(identityPane, preferences); // FIXME: see class::applyPreferences()

  PrefsPageChatWinAppearance* chatWinAppearancePage = new PrefsPageChatWinAppearance(chatWinAppearancePane, preferences);
  PrefsPageColorsAppearance* colorsAppearancePage = new PrefsPageColorsAppearance(colorsAppearancePane, preferences);

  PrefsPageBehaviour* generalBehaviorPage = new PrefsPageBehaviour(generalBehaviorPane, preferences);
  PrefsPageChatWinBehavior* chatWinBehaviorPage = new PrefsPageChatWinBehavior(chatWinBehaviorPane, preferences);
  tabBehaviorPage = new PrefsPageTabBehavior(tabBehaviorPane, preferences);
  ignorePage = new PrefsPageIgnore(ignorePane, preferences);
  aliasesPage = new PrefsPageAliases(aliasesPane, preferences);
  buttonsPage = new PrefsPageButtons(buttonsPane, preferences);
  logSettingsPage = new PrefsPageLog(logSettingsPane, preferences);
  dccSettingsPage = new PrefsPageDccSettings(dccSettingsPane, preferences);

  notifyPage = new PrefsPageNotify(notifyPane,preferences);
  highlightPage = new PrefsPageHighlight(highlightPane,preferences);
  OSDPage = new PrefsPageOSD(OSDPane,preferences);

  dialogsPage = new PrefsPageDialogs(dialogsPane,preferences);

  // TODO: Uncomment this again when it's ready to go
  // PrefsPageScripts* scriptsPage=new PrefsPageScripts(scriptsPane, preferences);

  setButtonOK(KGuiItem(i18n("&OK"),"button_ok",i18n("Keep changes made to configuration and close the window")));
  setButtonApply(KGuiItem(i18n("&Apply"),"apply",i18n("Keep changes made to configuration")));

  if(noServer)
  {
    enableButtonOK(false);
    setButtonCancel(KGuiItem(i18n("&Quit"),"exit",i18n("Quits application")));
  }
  else
  {
    setButtonCancel(KGuiItem(i18n("&Cancel"),"button_cancel",i18n("Discards all changes made")));
  }

  // connect standard signals and slots
  connect(this, SIGNAL(applyPreferences()), identityPage, SLOT(applyPreferences()));

  connect(this, SIGNAL(applyPreferences()), chatWinAppearancePage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), colorsAppearancePage, SLOT(applyPreferences()));

  connect(this, SIGNAL(applyPreferences()), generalBehaviorPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), chatWinBehaviorPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), tabBehaviorPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), buttonsPage, SLOT(applyPreferences()));

  connect(this, SIGNAL(applyPreferences()), notifyPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), highlightPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), OSDPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), ignorePage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), aliasesPage, SLOT(applyPreferences()));

  connect(this, SIGNAL(applyPreferences()), logSettingsPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), dccSettingsPage, SLOT(applyPreferences()));
  connect(this, SIGNAL(applyPreferences()), dialogsPage, SLOT(applyPreferences()));

  // connect all individual signals and slots
  connect(serverListPage, SIGNAL(connectToServer(int)), this, SLOT(connectRequest(int)));

// TODO: Uncomment this again when it's ready to go
// but ... is this really the way it's meant to be done?
// scriptsPage should use applyPreferences()
//  connect(this, SIGNAL(prefsChanged()), scriptsPage, SLOT(saveChanges()));
}

PrefsDialog::~PrefsDialog()
{
}

void PrefsDialog::connectRequest(int id)
{
  // Save changes before trying to connect
  slotApply();
  connectToServer(id);
}

void PrefsDialog::slotOk()
{
  slotApply();
  slotCancel();
}

void PrefsDialog::slotApply()
{
  // tell all preferences pages to save their new values
  emit applyPreferences();
  // tell the rest of the application to re-read the settings
  // The current (sep 2004) sequence is:
  // signal PrefDialog::prefsChanged -> KonversationApplication::saveOptions ->
  // signal KonversationApplication::prefsChanged -> KonversationMainWindow::slotPrefsChanged ->
  // signal KonversationMainWindow::prefsChanged -> rest of program.
  emit prefsChanged();
}

void PrefsDialog::slotCancel()
{
  emit cancelClicked();
}

void PrefsDialog::setPreferences(Preferences* newPrefs)
{
  preferences=newPrefs;
}

void PrefsDialog::openPage(Preferences::Pages page)
{
  if     (page==Preferences::ServerListPage) showPage(pageIndex(serverListPane));
  else if(page==Preferences::NotifyPage)     showPage(pageIndex(notifyPane));
  else if(page==Preferences::IdentityPage)   showPage(pageIndex(identityPane));
}

#include "prefsdialog.moc"
