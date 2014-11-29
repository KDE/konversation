/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004 by Peter Simonsson <psn@linux.se>
*/

#include "joinchanneldialog.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "channel.h"
#include "servergroupsettings.h"

#include <QPushButton>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace Konversation
{

    JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(i18n("Join Channel"));
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        QWidget *mainWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        mOkButton = buttonBox->button(QDialogButtonBox::Ok);
        mOkButton->setDefault(true);
        mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &JoinChannelDialog::reject);
        mOkButton->setDefault(true);
        setModal( true );
        m_ui.setupUi(mainWidget);
        mainLayout->addWidget(mainWidget);
        mainLayout->addWidget(buttonBox);
        m_ui.channelCombo->setFocus();

        mOkButton->setEnabled(false);
        connect(m_ui.channelCombo, &KHistoryComboBox::editTextChanged, this, &JoinChannelDialog::slotChannelChanged);

        // Add network names to network combobox and select the one corresponding to argument.
        QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
        foreach (Server *server, serverList)
        {
          m_ui.networkNameCombo->addItem(i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()),
                                         server->connectionId());
          connect(server, SIGNAL(nicknameChanged(QString)), this, SLOT(slotNicknameChanged(QString)));
        }
        // Update channel history when selected connection changes
        connect(m_ui.networkNameCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::currentIndexChanged), this, &JoinChannelDialog::slotSelectedConnectionChanged);
        // Clear channel history when the history combo box is cleared
        connect(m_ui.channelCombo, &KHistoryComboBox::cleared, this, &JoinChannelDialog::slotChannelHistoryCleared);
        // Preselect the current network
        m_ui.networkNameCombo->setCurrentIndex(m_ui.networkNameCombo->findData(server->connectionId()));
        // If the server is the first item, current index wont be changed
        // So channel history combo wont be populated, so force it
        slotSelectedConnectionChanged(m_ui.networkNameCombo->findData(server->connectionId()));

        connect(mOkButton, &QPushButton::clicked, this, &JoinChannelDialog::slotOk);
        connect(Application::instance()->getConnectionManager(), SIGNAL(connectionListChanged()),
                this, SLOT(slotConnectionListChanged()));
    }

    JoinChannelDialog::~JoinChannelDialog()
    {
    }

    int JoinChannelDialog::connectionId() const
    {
      return m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
    }

    QString JoinChannelDialog::channel() const
    {
        QString channel = m_ui.channelCombo->currentText();

        if (!channel.isEmpty())
        {
            int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

            if (server && !server->isAChannel(channel))
                channel = QLatin1Char('#') + channel;
        }

        return channel;
    }

    QString JoinChannelDialog::password() const
    {
        return m_ui.passwordEdit->text();
    }

    void JoinChannelDialog::slotOk()
    {
        int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

        // If the channel already exist in the history only the password will be updated.
        if (server && server->getServerGroup())
            server->getServerGroup()->appendChannelHistory(ChannelSettings(channel(), password()));

        accept();
    }

    void JoinChannelDialog::slotNicknameChanged(const QString& nickname)
    {
      Q_UNUSED(nickname);
      // Update all items
      QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
      foreach (Server *server, serverList)
      {
        int index = m_ui.networkNameCombo->findData(server->connectionId());
        m_ui.networkNameCombo->setItemText(index, i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()));
      }
    }

    void JoinChannelDialog::slotConnectionListChanged()
    {
      // Remove not-existing-anymore networks from the combobox
      for (int i = 0; i < m_ui.networkNameCombo->count(); i++)
      {
        int connectionId = m_ui.networkNameCombo->itemData(i).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);
        if (!server)
        {
          m_ui.networkNameCombo->removeItem(i);
          i--;
        }
      }
      // Add new network names to the combobox
      QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
      foreach (Server *server, serverList)
      {
        if (m_ui.networkNameCombo->findData(server->connectionId()) == -1)
        {
          m_ui.networkNameCombo->addItem(i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()),
                                         server->connectionId());
          connect(server, &Server::nicknameChanged, this, &JoinChannelDialog::slotNicknameChanged);
        }
      }
    }

    void JoinChannelDialog::slotSelectedConnectionChanged(int index)
    {
      m_ui.channelCombo->clear();
      int connectionId = m_ui.networkNameCombo->itemData(index).toInt();
      Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);
      if (server && server->getServerGroup())
      {
        ChannelList history = server->getServerGroup()->channelHistory();
        ChannelList::iterator endIt = history.end();
        const QList<Channel *> &channels = server->getChannelList();
        bool joined = false;
        // Append an empty string as first item
        QStringList channelHistory;
        channelHistory << QString();
        for(ChannelList::iterator it = history.begin(); it != endIt; ++it)
        {
          // Don't add empty items to the combobox
          if ((*it).name().isEmpty())
            continue;

          joined = false;

          foreach (Channel* chan, channels)
          {
            if(chan->getName() == (*it).name())
              joined = true;
          }

          if(!joined)
            channelHistory << (*it).name();
        }
        // Sort channel history for easier access
        channelHistory.sort();
        // Set history items
        m_ui.channelCombo->setHistoryItems(channelHistory);
      }
    }

    void JoinChannelDialog::slotChannelChanged(const QString& text)
    {
        mOkButton->setEnabled(!text.isEmpty());
    }


    void JoinChannelDialog::slotChannelHistoryCleared()
    {
        int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

        if (server && server->getServerGroup())
          server->getServerGroup()->clearChannelHistory();
    }
}

