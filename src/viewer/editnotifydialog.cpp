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
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <KGuiItem>
#include <QVBoxLayout>


EditNotifyDialog::EditNotifyDialog(QWidget* parent,
int serverGroupId,
const QString& nickname):
    QDialog(parent)

{
    setWindowTitle( i18n("Edit Watched Nickname") );
    setModal( true );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &EditNotifyDialog::slotOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &EditNotifyDialog::reject);
    //PORTING SCRIPT: WARNING mainLayout->addWidget(buttonBox) must be last item in layout. Please move it.
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    QWidget* page = mainWidget;

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


    KGuiItem::assign(okButton, KGuiItem(i18n("&OK"),"dialog-ok",i18n("Change notify information")));
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Cancel), KGuiItem(i18n("&Cancel"),"dialog-cancel",i18n("Discards all changes made")));

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

void EditNotifyDialog::delayedDestruct()
{
    if (isVisible()) {
        hide();
    }

    deleteLater();
}

