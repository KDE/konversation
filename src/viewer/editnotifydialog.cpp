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
#include "connectionmanager.h"
#include "servergroupsettings.h"

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

    QGridLayout* layout = new QGridLayout(page);

    QLabel* networkNameLabel=new QLabel(i18n("&Network name:"), page);
    QString networkNameWT = i18n(
        "Pick the server network you will connect to here.");
    networkNameLabel->setWhatsThis(networkNameWT);
    networkNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_networkNameCombo=new KComboBox(page);
    m_networkNameCombo->setWhatsThis(networkNameWT);
    networkNameLabel->setBuddy(m_networkNameCombo);

    QLabel* nicknameLabel=new QLabel(i18n("N&ickname:"), page);
    QString nicknameWT = i18n(
        "<qt>The nickname to watch for when connected to a server in the network.</qt>");
    nicknameLabel->setWhatsThis(nicknameWT);
    nicknameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_nicknameInput = new KLineEdit(nickname, page);
    m_nicknameInput->setWhatsThis(nicknameWT);
    nicknameLabel->setBuddy(m_nicknameInput);

    // Add network names to network combobox and select the one corresponding to argument.
    m_networkNameCombo->addItem(i18n("All Networks"), -1);
    QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
    for (int i = 0; i < serverList.count(); ++i)
    {
      Server *server = serverList.at(i);
      if (server->getServerGroup())
        m_networkNameCombo->addItem(server->getServerGroup()->name(), server->getServerGroup()->id());
    }
    m_networkNameCombo->setCurrentIndex(m_networkNameCombo->findData(serverGroupId, Qt::UserRole));
    layout->addWidget(networkNameLabel, 0, 0);
    layout->addWidget(m_networkNameCombo, 0, 1);
    layout->addWidget(nicknameLabel, 1, 0);
    layout->addWidget(m_nicknameInput, 1, 1);

    setButtonGuiItem( KDialog::Ok, KGuiItem(i18n("&OK"),"dialog-ok",i18n("Change notify information")));
    setButtonGuiItem( KDialog::Cancel, KGuiItem(i18n("&Cancel"),"dialog-cancel",i18n("Discards all changes made")));
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );

    m_nicknameInput->setFocus();
}

EditNotifyDialog::~EditNotifyDialog()
{
}

void EditNotifyDialog::slotOk()
{
    int id = m_networkNameCombo->itemData(m_networkNameCombo->currentIndex()).toInt();
    if (id == -1)
    {
      // add nickname to every server
      for (int i = 1; i < m_networkNameCombo->count(); ++i)
        emit notifyChanged(m_networkNameCombo->itemData(i).toInt(), m_nicknameInput->text());
    }
    else
      emit notifyChanged(id, m_nicknameInput->text());
    delayedDestruct();
}

#include "editnotifydialog.moc"
