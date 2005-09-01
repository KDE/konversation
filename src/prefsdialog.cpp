/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class holds the subpages for the preferences dialog
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
  email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qvbox.h>

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include "prefsdialog.h"

#include "konversationapplication.h"
#include "prefspagebehaviour.h"
#include "prefspagechatwinappearance.h"
#include "prefspageconnectionbehavior.h"
#include "prefspagefontsappearance.h"
#include "prefspagenicklistbehavior.h"

PrefsDialog::PrefsDialog(QWidget* parent) :
KDialogBase (KDialogBase::TreeList,i18n("Edit Preferences"),
KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel,
KDialogBase::Ok, parent, "edit_prefs", false, true)
{
    setShowIconsInTreeList(true);

    lastPane = 0;

    setFolderIcon(QStringList::split(',', i18n("Appearance")), SmallIcon("looknfeel"));
    chatWinAppearancePane = addVBoxPage(QStringList::split(',', i18n("Appearance") + "," + i18n("Chat Window")), QString::null, SmallIcon("view_text"));
    QWidget* fontsAppearancePane = addVBoxPage(QStringList::split(',', i18n("Appearance") + "," + i18n("Fonts")), QString::null, SmallIcon("fonts"));
    QWidget* themesPane = addVBoxPage(QStringList::split(',', i18n("Appearance") + "," + i18n("Themes")),
        QString::null, SmallIcon("iconthemes"));

    setFolderIcon(QStringList::split(',', i18n("Behavior")), SmallIcon("configure"));
    QWidget* generalBehaviorPane = addVBoxPage(QStringList::split(',', i18n("Behavior") + "," + i18n("General")),
        QString::null,SmallIcon("exec"));
    QWidget* connectionBehaviorPane = addVBoxPage(QStringList::split(',', i18n("Behavior") + "," + i18n("Connection")),
        QString::null,SmallIcon("connect_creating"));
    QWidget* nicklistBehaviorPane = addVBoxPage(QStringList::split(',', i18n("Behavior") + "," + i18n("Nickname List")),QString::null, SmallIcon("player_playlist"));
    QWidget* tabBehaviorPane = addVBoxPage(QStringList::split(',', i18n("Behavior") + "," + i18n("Tab Bar")),
        QString::null, SmallIcon("tab_new"));
    QWidget* ignorePane = addVBoxPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Ignored Nicknames")),
        QString::null, SmallIcon("editdelete"));
    QWidget* aliasesPane = addVBoxPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Command Aliases")),
        QString::null,SmallIcon("editcopy"));
    QWidget* buttonsPane = addVBoxPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Quick Buttons")),
        QString::null, SmallIcon("keyboard"));
    QWidget* logSettingsPane = addVBoxPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("Logging")),
        QString::null,SmallIcon("log"));
    QWidget* dccSettingsPane = addVBoxPage(QStringList::split(',', i18n("Behavior")+ "," + i18n("DCC")),
        QString::null,SmallIcon("2rightarrow" ));

    setFolderIcon(QStringList::split(',', i18n("Notification")), SmallIcon("knotify"));
    notifyPane = addVBoxPage(QStringList::split(',', i18n("Notification") + "," + i18n("Watched Nicknames")),
        QString::null,SmallIcon("kfind"));
    QWidget* highlightPane = addVBoxPage(QStringList::split(',', i18n("Notification") + "," + i18n("Highlighting"))   ,QString::null,SmallIcon("paintbrush"));
    OSDPane = addVBoxPage(QStringList::split(',', i18n("Notification") + "," + i18n("On Screen Display")),
        QString::null, SmallIcon("tv"));

    QWidget* dialogsPane = addVBoxPage(i18n("Warning Dialogs"), QString::null, SmallIcon("messagebox_warning"));

    // TODO: Uncomment this again when it's ready to go
    // QWidget* scriptsPane        =addPage(i18n("Scripting"));

    // Add pages to preferences dialog
    PrefsPageChatWinAppearance* chatWinAppearancePage = new PrefsPageChatWinAppearance(chatWinAppearancePane);
    PrefsPageFontsAppearance* fontsAppearancePage = new PrefsPageFontsAppearance(fontsAppearancePane);

    PrefsPageBehaviour* generalBehaviorPage = new PrefsPageBehaviour(generalBehaviorPane);
    PrefsPageConnectionBehavior* connectionBehaviorPage = new PrefsPageConnectionBehavior(connectionBehaviorPane);
    PrefsPageNicklistBehavior* nicklistBehaviorPage = new PrefsPageNicklistBehavior(nicklistBehaviorPane);
    tabBehaviorPage = new PrefsPageTabBehavior(tabBehaviorPane);
    ignorePage = new PrefsPageIgnore(ignorePane);
    aliasesPage = new PrefsPageAliases(aliasesPane);
    buttonsPage = new PrefsPageButtons(buttonsPane);
    logSettingsPage = new PrefsPageLog(logSettingsPane);
    dccSettingsPage = new PrefsPageDccSettings(dccSettingsPane);

    notifyPage = new PrefsPageNotify(notifyPane);
    highlightPage = new PrefsPageHighlight(highlightPane);
    OSDPage = new PrefsPageOSD(OSDPane);

    dialogsPage = new PrefsPageDialogs(dialogsPane);

    themesPage = new PrefsPageThemes(themesPane);

    // TODO: Uncomment this again when it's ready to go
    // PrefsPageScripts* scriptsPage=new PrefsPageScripts(scriptsPane);

    setButtonOK(KGuiItem(i18n("&OK"),"button_ok",i18n("Keep changes made to configuration and close the window")));
    setButtonApply(KGuiItem(i18n("&Apply"),"apply",i18n("Keep changes made to configuration")));
    setButtonCancel(KGuiItem(i18n("&Cancel"),"button_cancel",i18n("Discards all changes made")));

    // connect standard signals and slots
    connect(this, SIGNAL(applyPreferences()), chatWinAppearancePage, SLOT(applyPreferences()));
    connect(this, SIGNAL(applyPreferences()), fontsAppearancePage, SLOT(applyPreferences()));
    connect(this, SIGNAL(applyPreferences()), themesPage, SLOT(applyPreferences()));

    connect(this, SIGNAL(applyPreferences()), generalBehaviorPage, SLOT(applyPreferences()));
    connect(this, SIGNAL(applyPreferences()), connectionBehaviorPage, SLOT(applyPreferences()));
    connect(this, SIGNAL(applyPreferences()), nicklistBehaviorPage, SLOT(applyPreferences()));
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

    // TODO: Uncomment this again when it's ready to go
    // but ... is this really the way it's meant to be done?
    // scriptsPage should use applyPreferences()
    //  connect(this, SIGNAL(prefsChanged()), scriptsPage, SLOT(saveChanges()));

    connect(this, SIGNAL(aboutToShowPage(QWidget*)), this, SLOT(slotAboutToShowPage(QWidget*)));

    KConfig* config = kapp->config();
    config->setGroup("PreferencesDialog");
    QSize newSize = size();
    newSize = config->readSizeEntry("Size", &newSize);
    resize(newSize);

    int lastActiveModule = config->readNumEntry("LastActiveModule",0);
    showPage(lastActiveModule);
    if(lastActiveModule == pageIndex(OSDPane))
        OSDPage->aboutToShow();
    unfoldTreeList(true);
}

PrefsDialog::~PrefsDialog()
{
    KConfig* config = kapp->config();
    config->setGroup("PreferencesDialog");
    config->writeEntry("Size", size());
    config->writeEntry("LastActiveModule",activePageIndex());
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

void PrefsDialog::openPage(Preferences::Pages page)
{
    if(page==Preferences::NotifyPage)
    {
        showPage(pageIndex(notifyPane));
    }
    else
    {
        showPage(pageIndex(chatWinAppearancePane));
    }
}

void PrefsDialog::slotAboutToShowPage(QWidget* page)
{
    if(lastPane == OSDPane)
        OSDPage->aboutToHide();
    else if(page == OSDPane)
        OSDPage->aboutToShow();

    lastPane = page;
}

#include "prefsdialog.moc"
