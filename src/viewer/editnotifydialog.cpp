/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Wed Sep 1 2004
  copyright: (C) 2004 by Gary Cramblitt
  email:     garycramblitt@comcast.net
*/

#include "editnotifydialog.h"
#include "application.h"
#include "servergroupsettings.h"

#include <QLayout>
#include <QLabel>

#include <KLineEdit>
#include <KComboBox>


EditNotifyDialog::EditNotifyDialog(QWidget* parent,
int serverGroupId,
const QString& nickname):
    KDialog(parent)

{
    setCaption( i18n("Edit Watched Nickname") );
    setModal( true );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setDefaultButton( KDialog::Ok );
    QWidget* page = mainWidget();

    QHBoxLayout* layout = new QHBoxLayout(page);

    QLabel* networkNameLabel=new QLabel(i18n("&Network name:"), page);
    QString networkNameWT = i18n(
        "Pick the server network you will connect to here.");
    networkNameLabel->setWhatsThis(networkNameWT);
    m_networkNameCombo=new KComboBox(page);
    m_networkNameCombo->setWhatsThis(networkNameWT);
    networkNameLabel->setBuddy(m_networkNameCombo);

    QLabel* nicknameLabel=new QLabel(i18n("N&ickname:"), page);
    QString nicknameWT = i18n(
        "<qt>The nickname to watch for when connected to a server in the network.</qt>");
    nicknameLabel->setWhatsThis(nicknameWT);
    m_nicknameInput = new KLineEdit(nickname, page);
    m_nicknameInput->setWhatsThis(nicknameWT);
    nicknameLabel->setBuddy(m_nicknameInput);

    // Add network names to network combobox and select the one corresponding to argument.
    Konversation::ServerGroupHash serverNetworks = Preferences::serverGroupHash();
    QHashIterator<int, Konversation::ServerGroupSettingsPtr> it(serverNetworks);
    while(it.hasNext())
    {
        m_networkNameCombo->addItem(it.value()->name(),it.key());
    }
    m_networkNameCombo->setCurrentIndex(m_networkNameCombo->findData(serverGroupId, Qt::UserRole));
    layout->addWidget(networkNameLabel);
    layout->addWidget(m_networkNameCombo);
    layout->addWidget(nicknameLabel);
    layout->addWidget(m_nicknameInput);

    setButtonGuiItem( KDialog::Ok, KGuiItem(i18n("&OK"),"dialog-ok",i18n("Change notify information")));
    setButtonGuiItem( KDialog::Cancel, KGuiItem(i18n("&Cancel"),"dialog-cancel",i18n("Discards all changes made")));
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );

}

EditNotifyDialog::~EditNotifyDialog()
{
}

void EditNotifyDialog::slotOk()
{
    emit notifyChanged(m_networkNameCombo->itemData(m_networkNameCombo->currentIndex()).toInt(),
        m_nicknameInput->text());
    delayedDestruct();
}

#include "editnotifydialog.moc"
