/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson <psn@linux.se>
*/
#include "joinchanneldialog.h"

#include <qlabel.h>

#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>

#include "joinchannelui.h"
#include "server.h"
#include "channel.h"
#include "servergroupsettings.h"

namespace Konversation {

JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent, const char *name)
  : KDialogBase(parent, name, true, i18n("Join Channel on %1").arg(server->getServerGroup()), Ok|Cancel, Ok)
{
  m_server = server;
  m_widget = new JoinChannelUI(this);
  setMainWidget(m_widget);

  m_widget->serverLbl->setText(server->getServerGroup());

  ChannelList history = server->serverGroupSettings()->channelHistory();
  ChannelList::iterator endIt = history.end();
  QPtrList<Channel> channels = server->getChannelList();
  QPtrListIterator<Channel> chanIt(channels);
  Channel* chan = 0;
  bool joined = false;

  for(ChannelList::iterator it = history.begin(); it != endIt; ++it) {
    chan = chanIt.toFirst();
    joined = false;

    while(chan) {
      if(chan->getName() == (*it).name()) {
        joined = true;
      }

      ++chanIt;
      chan = chanIt.current();
    }

    if(!joined) {
      m_widget->channelCombo->addToHistory((*it).name());
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

  if(!m_server->isAChannel(channel)) {
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
  m_server->serverGroupSettings()->appendChannelHistory(ChannelSettings(channel(), password()));

  accept();
}

}

#include "joinchanneldialog.moc"
