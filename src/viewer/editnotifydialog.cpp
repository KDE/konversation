/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>
*/

#include "editnotifydialog.h"
#include "application.h"
#include "connectionmanager.h"
#include "servergroupsettings.h"

#include <QLabel>

#include <KLineEdit>
#include <KComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>


EditNotifyDialog::EditNotifyDialog(QWidget* parent,
int serverGroupId,
const QString& nickname):
    QDialog(parent)

{
    setWindowTitle( i18n("Edit Watched Nickname") );
    setModal( true );
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout;
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

    auto* layout = new QGridLayout(page);

    auto* networkNameLabel=new QLabel(i18n("&Network name:"), page);
    QString networkNameWT = i18n(
        "Pick the server network you will connect to here.");
    networkNameLabel->setWhatsThis(networkNameWT);
    networkNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_networkNameCombo=new KComboBox(page);
    m_networkNameCombo->setWhatsThis(networkNameWT);
    networkNameLabel->setBuddy(m_networkNameCombo);

    auto* nicknameLabel=new QLabel(i18n("N&ickname:"), page);
    QString nicknameWT = i18n(
        "<qt>The nickname to watch for when connected to a server in the network.</qt>");
    nicknameLabel->setWhatsThis(nicknameWT);
    nicknameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_nicknameInput = new KLineEdit(nickname, page);
    m_nicknameInput->setWhatsThis(nicknameWT);
    nicknameLabel->setBuddy(m_nicknameInput);

    // Add network names to network combobox and select the one corresponding to argument.
    m_networkNameCombo->addItem(i18n("All Networks"), -1);
    const QList<Server*> serverList = Application::instance()->getConnectionManager()->getServerList();
    for (Server *server : serverList) {
      if (server->getServerGroup())
        m_networkNameCombo->addItem(server->getServerGroup()->name(), server->getServerGroup()->id());
    }
    m_networkNameCombo->setCurrentIndex(m_networkNameCombo->findData(serverGroupId, Qt::UserRole));
    layout->addWidget(networkNameLabel, 0, 0);
    layout->addWidget(m_networkNameCombo, 0, 1);
    layout->addWidget(nicknameLabel, 1, 0);
    layout->addWidget(m_nicknameInput, 1, 1);


    okButton->setToolTip(i18n("Change notify information"));
    buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(i18n("Discards all changes made"));

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
        Q_EMIT notifyChanged(m_networkNameCombo->itemData(i).toInt(), m_nicknameInput->text());
    }
    else
      Q_EMIT notifyChanged(id, m_nicknameInput->text());
    delayedDestruct();
}

void EditNotifyDialog::delayedDestruct()
{
    if (isVisible()) {
        hide();
    }

    deleteLater();
}

#include "moc_editnotifydialog.cpp"
