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
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include <klocale.h>
#include <kdebug.h>

#include "prefsdialog.h"

#include "serverlistitem.h"
#include "editserverdialog.h"
#include "konversationapplication.h"

PrefsDialog::PrefsDialog(Preferences* preferences,bool noServer) :
             KDialogBase (KDialogBase::TreeList,i18n("Edit Preferences"),
                          KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                          KDialogBase::Ok,0,"edit_prefs",false,true)
{
  setPreferences(preferences);

          serverListPane     =addPage(i18n("Server List"));
  QFrame* generalSettingsPane=addPage(i18n("General Settings"));
  QFrame* identityPane       =addPage(i18n("Identity"));

  QFrame* appearancePane     =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("General")));
  QFrame* tabBehaviorPane    =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("Tab Behavior")));
  QFrame* colorsImagesPane   =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("Colors & Images")));
  QFrame* ircColorsPane      =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("IRC Colors")));
  QFrame* buttonsPane        =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("Quick Buttons")));

  QFrame* nickCompletionPane =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Nickname Completion")));
          notifyPane         =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Nick Watch List")));
  QFrame* highlightPane      =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Highlight List")));
  QFrame* OSDPane            =addPage(QStringList::split(',',i18n("Chat")+","+i18n("On Screen Display")));
  QFrame* ignorePane         =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Ignore List")));
  QFrame* aliasesPane        =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Aliases")));

  QFrame* logSettingsPane    =addPage(i18n("Log Settings"));
  QFrame* dccSettingsPane    =addPage(i18n("DCC Settings"));
  QFrame* webBrowserPane   =addPage(i18n("Web Browser"));
  QFrame* dialogsPane        =addPage(i18n("Dialogs"));
  // TODO: Uncomment this again when it's ready to go
  // QFrame* scriptsPane        =addPage(i18n("Scripting"));

  // Add pages to preferences dialog
  serverListPage=new PrefsPageServerList(serverListPane,preferences);

  generalSettingsPage=new PrefsPageGeneralSettings(generalSettingsPane,preferences);
  identityPage       =new PrefsPageIdentity(identityPane,preferences); // FIXME: see class::applyPreferences()

  appearancePage     =new PrefsPageAppearance(appearancePane,preferences);
  tabBehaviorPage    =new PrefsPageTabBehavior(tabBehaviorPane,preferences);
  colorsImagesPage   =new PrefsPageColorsImages(colorsImagesPane,preferences);
  ircColorsPage      =new PrefsPageIRCColors(ircColorsPane,preferences);
  buttonsPage        =new PrefsPageButtons(buttonsPane,preferences);

  nickCompletionPage =new PrefsPageNickCompletion(nickCompletionPane,preferences);
  notifyPage         =new PrefsPageNotify(notifyPane,preferences);
  highlightPage      =new PrefsPageHighlight(highlightPane,preferences);
  OSDPage            =new PrefsPageOSD(OSDPane,preferences);
  ignorePage         =new PrefsPageIgnore(ignorePane,preferences);
  aliasesPage        =new PrefsPageAliases(aliasesPane,preferences);

  logSettingsPage    =new PrefsPageLog(logSettingsPane,preferences);
  dccSettingsPage    =new PrefsPageDccSettings(dccSettingsPane,preferences);
  webBrowserPage   =new PrefsPageWebBrowser(webBrowserPane,preferences);
  dialogsPage        =new PrefsPageDialogs(dialogsPane,preferences);

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
  connect(this,SIGNAL (applyPreferences()),generalSettingsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),identityPage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),appearancePage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),tabBehaviorPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),colorsImagesPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),ircColorsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),buttonsPage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),nickCompletionPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),notifyPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),highlightPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),OSDPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),ignorePage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),aliasesPage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),logSettingsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),dccSettingsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),webBrowserPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),dialogsPage,SLOT (applyPreferences()) );

  // connect all individual signals and slots
  connect(serverListPage,SIGNAL(connectToServer(int)),this,SLOT(connectRequest(int)) );

// TODO: Uncomment this again when it's ready to go
// but ... is this really the way it's meant to be done?
// scriptsPage should use applyPreferences()
//  connect(this, SIGNAL(prefsChanged()), scriptsPage, SLOT(saveChanges()));
}

PrefsDialog::~PrefsDialog()
{
  delete serverListPage;
  delete generalSettingsPage;
  delete identityPage;
  delete appearancePage;
  delete tabBehaviorPage;
  delete colorsImagesPage;
  delete ircColorsPage;
  delete buttonsPage;
  delete nickCompletionPage;
  delete notifyPage;
  delete highlightPage;
  delete OSDPage;
  delete ignorePage;
  delete aliasesPage;
  delete logSettingsPage;
  delete dccSettingsPage;
  delete dialogsPage;
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
}

#include "prefsdialog.moc"
