/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  prefsdialog.cpp  -  description
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
#include "prefspageidentity.h"
#include "prefspagescripts.h"
#include "serverlistitem.h"
#include "editserverdialog.h"
#include "konversationapplication.h"

PrefsDialog::PrefsDialog(Preferences* preferences,bool noServer) :
             KDialogBase (KDialogBase::TreeList,i18n("Edit Preferences"),
                          KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
                          KDialogBase::Ok,0,"edit_prefs",false,true)
{
  kdDebug() << "PrefsDialog::PrefsDialog()" << endl;
  setPreferences(preferences);

  QFrame* serverListPane=addPage(i18n("Server List"));
  QFrame* identityPane=addPage(i18n("Identity"));
	QFrame* scriptsPane = addPage(i18n("Scripting"));

  // Add Server List page
  PrefsPage* serverListPage=new PrefsPageServerList(serverListPane,preferences);
  connect(serverListPage,SIGNAL(connectToServer(int)),this,SLOT(connectRequest(int)) );
  // Add Identity page
  PrefsPage* identityPage=new PrefsPageIdentity(identityPane,preferences);
	// Add scripts page
	PrefsPage* scriptsPage = new PrefsPageScripts(scriptsPane, preferences);

  setButtonOKText(i18n("OK"),i18n("Keep changes made to configuration and close the window"));
  setButtonApplyText(i18n("Apply"),i18n("Keep changes made to configuration"));

  if(noServer)
  {
    enableButtonOK(false);
    setButtonCancelText(i18n("Quit"),i18n("Quits Application"));
  }
  else
  {
    setButtonCancelText(i18n("Cancel"),i18n("Discards all changes made"));
  }
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
  emit prefsChanged();
}

void PrefsDialog::slotCancel()
{
  kdDebug() << "PrefsDialog::slotCancel()" << endl;
  emit cancelClicked();
}

void PrefsDialog::setPreferences(Preferences* newPrefs)
{
  preferences=newPrefs;
}
