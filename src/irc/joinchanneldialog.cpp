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



namespace Konversation
{

    JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(i18n("Join Channel"));
        setButtons( KDialog::Ok|KDialog::Cancel );
        setDefaultButton( KDialog::Ok );
        setModal( true );
        m_server = server;
        m_ui.setupUi(mainWidget());
        m_ui.networkNameCombo->setFocus();
        // Add network names to network combobox and select the one corresponding to argument.
        QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
        foreach (Server *server, serverList)
        {
          m_ui.networkNameCombo->addItem(i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()),
                                         server->connectionId());
          connect(server, SIGNAL(nicknameChanged(QString)), this, SLOT(slotNicknameChanged(QString)));
        }
        // Update channel history when selected connection changes
        connect(m_ui.networkNameCombo, SIGNAL(currentIndexChanged(int)),
                this, SLOT(slotSelectedConnectionChanged(int)));
        // Preselect the current network
        m_ui.networkNameCombo->setCurrentIndex(m_ui.networkNameCombo->findData(m_server->connectionId()));

        connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
        connect(Application::instance()->getConnectionManager(), SIGNAL(connectionListChanged()),
                this, SLOT(slotConnectionListChanged()));
    }

    JoinChannelDialog::~JoinChannelDialog()
    {
    }

    int JoinChannelDialog::connectionId() const
    {
      return m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex(), Qt::UserRole).toInt();
    }

    QString JoinChannelDialog::channel() const
    {
        QString channel = m_ui.channelCombo->currentText();

        if(!m_server->isAChannel(channel))
        {
            channel = '#' + channel;
        }

        return channel;
    }

    QString JoinChannelDialog::password() const
    {
        return m_ui.passwordEdit->text();
    }

    void JoinChannelDialog::slotOk()
    {
        // If the channel already exist in the history only the password will be updated.
        if (m_server->getServerGroup())
            m_server->getServerGroup()->appendChannelHistory(ChannelSettings(channel(), password()));

        accept();
    }

    void JoinChannelDialog::slotNicknameChanged(QString nickname)
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
          connect(server, SIGNAL(nicknameChanged(QString)), this, SLOT(slotNicknameChanged(QString)));
        }
      }
    }
    void JoinChannelDialog::slotSelectedConnectionChanged(int index)
    {
      int connectionId = m_ui.networkNameCombo->itemData(index).toInt();
      Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);
      if (server->getServerGroup())
      {
        ChannelList history = server->getServerGroup()->channelHistory();
        ChannelList::iterator endIt = history.end();
        const QList<Channel *> &channels = server->getChannelList();
        bool joined = false;

        for(ChannelList::iterator it = history.begin(); it != endIt; ++it)
        {
          joined = false;

          foreach (Channel* chan, channels)
          {
            if(chan->getName() == (*it).name())
              joined = true;
          }

          if(!joined)
            m_ui.channelCombo->addToHistory((*it).name());
        }
      }

      const int i = m_ui.channelCombo->findText("");
      if (i != -1)
        m_ui.channelCombo->setCurrentIndex(i);
      else
        m_ui.channelCombo->setEditText("");
    }
}
#include "joinchanneldialog.moc"
