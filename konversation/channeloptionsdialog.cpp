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

#include <qcheckbox.h>

#include <klocale.h>
#include <klistview.h>
#include <ktextedit.h>
#include <klineedit.h>

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

void ChannelOptionsDialog::setAllowedChannelModes(const QString& modes)
{
  QString modeString = modes;
  // These modes are handled in a special way: ntimslk
  modeString.remove('t');
  modeString.remove('n');
  modeString.remove('l');
  modeString.remove('i');
  modeString.remove('m');
  modeString.remove('s');
  modeString.remove('k');
}

void ChannelOptionsDialog::setModes(const QStringList& modes)
{
  m_widget->topicModeChBox->setChecked(false);
  m_widget->messageModeChBox->setChecked(false);
  m_widget->userLimitChBox->setChecked(false);
  m_widget->userLimitEdit->clear();
  m_widget->inviteModeChBox->setChecked(false);
  m_widget->moderatedModeChBox->setChecked(false);
  m_widget->secretModeChBox->setChecked(false);
  m_widget->keyModeChBox->setChecked(false);
  m_widget->keyModeEdit->clear();
  
  char mode;
  
  for(QStringList::const_iterator it = modes.begin(); it != modes.end(); ++it) {
    mode = (*it)[0];

    switch(mode) {
      case 't':
        m_widget->topicModeChBox->setChecked(true);
        break;
      case 'n':
        m_widget->messageModeChBox->setChecked(true);
        break;
      case 'l':
        m_widget->userLimitChBox->setChecked(true);
        m_widget->userLimitEdit->setText((*it).mid(1));
        break;
      case 'i':
        m_widget->inviteModeChBox->setChecked(true);
        break;
      case 'm':
        m_widget->moderatedModeChBox->setChecked(true);
        break;
      case 's':
        m_widget->secretModeChBox->setChecked(true);
        break;
      case 'k':
        m_widget->keyModeChBox->setChecked(true);
        m_widget->keyModeEdit->setText((*it).mid(1));
        break;
      default:
        break;
    }
  }
}

QStringList ChannelOptionsDialog::modes()
{
  QStringList modes;

  modes.append(QString(m_widget->topicModeChBox->isChecked() ? "+" : "-") + "t");
  modes.append(QString(m_widget->messageModeChBox->isChecked() ? "+" : "-") + "n");
  modes.append(QString(m_widget->userLimitChBox->isChecked() ? "+" : "-") + "l" + m_widget->userLimitEdit->text());
  modes.append(QString(m_widget->inviteModeChBox->isChecked() ? "+" : "-") + "i");
  modes.append(QString(m_widget->moderatedModeChBox->isChecked() ? "+" : "-") + "m");
  modes.append(QString(m_widget->secretModeChBox->isChecked() ? "+" : "-") + "s");
  modes.append(QString(m_widget->keyModeChBox->isChecked() ? "+" : "-") + "k" + m_widget->keyModeEdit->text());

  return modes;
}

}

#include "channeloptionsdialog.moc"
