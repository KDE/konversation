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

  $Id$
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include <klocale.h>
#include <kdebug.h>

#include "prefsdialog.h"
#include "prefspageserverlist.h"
#include "prefspagegeneralsettings.h"
#include "prefspageidentity.h"
#include "prefspageappearance.h"
#include "prefspagecolorsimages.h"
#include "prefspagebuttons.h"
#include "prefspagelog.h"
#include "prefspagedccsettings.h"
// #include "prefspagescripts.h"
#include "serverlistitem.h"
#include "editserverdialog.h"
#include "konversationapplication.h"
#include "prefspagedialogs.h"
#include "prefspagehighlight.h"
#include "prefspagenotify.h"
#include "prefspageignore.h"

PrefsDialog::PrefsDialog(Preferences* preferences,bool noServer) :
             KDialogBase (KDialogBase::TreeList,i18n("Edit preferences"),
                          KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                          KDialogBase::Ok,0,"edit_prefs",false,true)
{
  kdDebug() << "PrefsDialog::PrefsDialog()" << endl;
  setPreferences(preferences);

          serverListPane     =addPage(i18n("Server list"));
  QFrame* generalSettingsPane=addPage(i18n("General settings"));
  QFrame* identityPane       =addPage(i18n("Identity"));

  QFrame* appearancePane     =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("General")));
  QFrame* colorsImagesPane   =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("Colors and images")));
  QFrame* buttonsPane        =addPage(QStringList::split(',',i18n("Appearance")+","+i18n("Quick buttons")));

          notifyPane         =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Notify list")));
  QFrame* highlightPane      =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Highlight list")));
  QFrame* ignorePane         =addPage(QStringList::split(',',i18n("Chat")+","+i18n("Ignore list")));

  QFrame* logSettingsPane    =addPage(i18n("Log settings"));
  QFrame* dccSettingsPane    =addPage(i18n("DCC settings"));
  QFrame* dialogsPane        =addPage(i18n("Dialogs"));
  // TODO: Uncomment this again when it's ready to go
  //  QFrame* scriptsPane        =addPage(i18n("Scripting"));

  // Add pages to preferences dialog
  PrefsPage* serverListPage=new PrefsPageServerList(serverListPane,preferences);

  PrefsPageGeneralSettings* generalSettingsPage=new PrefsPageGeneralSettings(generalSettingsPane,preferences);
  PrefsPageIdentity*        identityPage       =new PrefsPageIdentity(identityPane,preferences); // FIXME: see class::applyPreferences()

  PrefsPageAppearance*      appearancePage     =new PrefsPageAppearance(appearancePane,preferences);
  PrefsPageColorsImages*    colorsImagesPage   =new PrefsPageColorsImages(colorsImagesPane,preferences);
  PrefsPageButtons*         buttonsPage        =new PrefsPageButtons(buttonsPane,preferences);

  PrefsPageNotify*          notifyPage         =new PrefsPageNotify(notifyPane,preferences);
  PrefsPageHighlight*       highlightPage      =new PrefsPageHighlight(highlightPane,preferences);
  PrefsPageIgnore*          ignorePage         =new PrefsPageIgnore(ignorePane,preferences);

  PrefsPageLog*             logSettingsPage    =new PrefsPageLog(logSettingsPane,preferences);
  PrefsPageDccSettings*     dccSettingsPage    =new PrefsPageDccSettings(dccSettingsPane,preferences);
  PrefsPageDialogs*         dialogsPage        =new PrefsPageDialogs(dialogsPane,preferences);

  // TODO: Uncomment this again when it's ready to go
  // new PrefsPageScripts(scriptsPane, preferences);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));

  if(noServer)
  {
    enableButtonOK(false);
    setButtonCancelText(i18n("Quit"),i18n("Quits application"));
  }
  else
  {
    setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));
  }

  // connect standard signals and slots
  connect(this,SIGNAL (applyPreferences()),generalSettingsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),identityPage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),appearancePage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),colorsImagesPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),buttonsPage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),notifyPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),highlightPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),ignorePage,SLOT (applyPreferences()) );

  connect(this,SIGNAL (applyPreferences()),logSettingsPage,SLOT (applyPreferences()) );
  connect(this,SIGNAL (applyPreferences()),dccSettingsPage,SLOT (applyPreferences()) );
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
  kdDebug() << "PrefsDialog::~PrefsDialog()" << endl;
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
