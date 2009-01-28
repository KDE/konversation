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
#include "joinchannelui.h"
#include "server.h"
#include "channel.h"
#include "servergroupsettings.h"

#include <qlabel.h>
//Added by qt3to4:
#include <Q3PtrList>

#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>


namespace Konversation
{

    JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent, const char *name)
        : KDialogBase(parent, name, true, i18n("Join Channel on %1").arg(server->getDisplayName()), Ok|Cancel, Ok)
    {
        m_server = server;
        m_widget = new JoinChannelUI(this);
        setMainWidget(m_widget);

        m_widget->serverLbl->setText(server->getDisplayName());

        if (m_server->getServerGroup())
        {
            ChannelList history = server->getServerGroup()->channelHistory();
            ChannelList::iterator endIt = history.end();
            const Q3PtrList<Channel> &channels = server->getChannelList();
            Q3PtrListIterator<Channel> chanIt(channels);
            Channel* chan = 0;
            bool joined = false;

            for(ChannelList::iterator it = history.begin(); it != endIt; ++it)
            {
                chan = chanIt.toFirst();
                joined = false;

                while(chan)
                {
                    if(chan->getName() == (*it).name())
                    {
                        joined = true;
                    }

                    ++chanIt;
                    chan = chanIt.current();
                }

                if(!joined)
                {
                    m_widget->channelCombo->addToHistory((*it).name());
                }
            }
        }

        m_widget->channelCombo->setCurrentText("");
    }

    JoinChannelDialog::~JoinChannelDialog()
    {
    }

    QString JoinChannelDialog::channel() const
    {
        QString channel = m_widget->channelCombo->currentText();

        if(!m_server->isAChannel(channel))
        {
            channel = '#' + channel;
        }

        return channel;
    }

    QString JoinChannelDialog::password() const
    {
        return m_widget->passwordEdit->text();
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
