/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#include "channeloptionsdialog.h"

#include <klocale.h>
#include <klistview.h>
#include <ktextedit.h>

#include "channeloptionsui.h"

namespace Konversation {

ChannelOptionsDialog::ChannelOptionsDialog(const QString& channel, QWidget *parent, const char *name)
  : KDialogBase(parent, name, false, i18n("Channel options for %1").arg(channel), Ok|Cancel, Ok)
{
  m_widget = new ChannelOptionsUI(this);
  setMainWidget(m_widget);

  connect(m_widget->topicHistoryList, SIGNAL(clicked(QListViewItem*)), this, SLOT(topicHistoryItemClicked(QListViewItem*)));
}

ChannelOptionsDialog::~ChannelOptionsDialog()
{
}

QString ChannelOptionsDialog::topic()
{
  return m_widget->topicEdit->text();
}

void ChannelOptionsDialog::setTopicHistory(const QStringList& history)
{
  m_widget->topicHistoryList->clear();

  for(QStringList::const_iterator it = history.begin(); it != history.end(); ++it) {
    new KListViewItem(m_widget->topicHistoryList, (*it).section(' ', 0, 0), (*it).section(' ', 1));
  }

  m_widget->topicEdit->setText(history.first().section(' ', 1));
}

void ChannelOptionsDialog::topicHistoryItemClicked(QListViewItem* item)
{
  m_widget->topicEdit->setText(item->text(1));
}

}

#include "channeloptionsdialog.moc"
