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
#include "server.h"
#include "channel.h"
#include "servergroupsettings.h"



namespace Konversation
{

    JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(i18n("Join Channel on %1", server->getDisplayName()));
        setButtons( KDialog::Ok|KDialog::Cancel );
        setDefaultButton( KDialog::Ok );
        setModal( true );
        m_server = server;
        m_ui.setupUi(mainWidget());
        m_ui.channelCombo->setFocus();
        m_ui.serverLbl->setText(server->getDisplayName());

        if (m_server->getServerGroup())
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
                    {
                        joined = true;
                    }
                }

                if(!joined)
                {
                    m_ui.channelCombo->addToHistory((*it).name());
                }
            }
        }

        const int i = m_ui.channelCombo->findText("");
        if (i != -1)
            m_ui.channelCombo->setCurrentIndex(i);
        else
            m_ui.channelCombo->setEditText("");

        connect( this, SIGNAL( okClicked() ), this, SLOT( slotOk() ) );
    }

    JoinChannelDialog::~JoinChannelDialog()
    {
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

}

#include "joinchanneldialog.moc"
