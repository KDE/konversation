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
#include <knuminput.h>
#include <qpushbutton.h>
#include <qregexp.h>

#include "konversationapplication.h"
#include "channeloptionsui.h"
#include "channel.h"

namespace Konversation {

ChannelOptionsDialog::ChannelOptionsDialog(Channel *channel)
  : KDialogBase(channel, "channelOptions", false, i18n("Channel Options for %1").arg(channel->getName()), Ok|Cancel, Ok)
{
  Q_ASSERT(channel);
  m_widget = new ChannelOptionsUI(this);
  setMainWidget(m_widget);

  m_widget->otherModesList->setRenameable(0, false);
  m_widget->otherModesList->setRenameable(1, true);
  m_widget->otherModesList->hide();

  m_channel = channel;

  connect(m_widget->topicHistoryList, SIGNAL(clicked(QListViewItem*)), this, SLOT(topicHistoryItemClicked(QListViewItem*)));
  connect(m_widget->toggleAdvancedModes, SIGNAL(clicked()), this, SLOT(toggleAdvancedModes()));
  connect(m_channel, SIGNAL(topicHistoryChanged()), this, SLOT(refreshTopicHistory()));

  connect(m_channel, SIGNAL(modesChanged()), this, SLOT(refreshModes()));
  connect(m_channel->getOwnChannelNick(), SIGNAL(channelNickChanged()), this, SLOT(refreshEnableModes()));

  connect(this, SIGNAL(cancelClicked()), this, SLOT(closeOptionsDialog()));
  connect(this, SIGNAL(okClicked()), this, SLOT(changeOptions()));
  
  refreshTopicHistory();
  refreshAllowedChannelModes();
  refreshModes();
}

ChannelOptionsDialog::~ChannelOptionsDialog()
{
}

void ChannelOptionsDialog::closeOptionsDialog()
{
  deleteLater();
}

void ChannelOptionsDialog::changeOptions()
{
  QString newTopic = topic();

  if(newTopic != m_channel->getTopicHistory().first().section(' ', 1)) {
    m_channel->sendChannelText(KonversationApplication::preferences.getCommandChar() + "TOPIC " + m_channel->getName() + " " + newTopic);
  }

  QStringList newModeList = modes();
  QStringList currentModeList = m_channel->getModeList();
  QStringList rmModes;
  QStringList addModes;
  QStringList tmp;
  QString modeString;
  bool plus;
  QString command("MODE %1 %2%3 %4");
  
  for(QStringList::iterator it = newModeList.begin(); it != newModeList.end(); ++it) {
    modeString = (*it).mid(1);
    plus = ((*it)[0] == '+');
    tmp = currentModeList.grep(QRegExp("^" + modeString));

    if(tmp.isEmpty() && plus) {
      m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("+").arg(modeString[0]).arg(modeString.mid(1)));
    } else if(!tmp.isEmpty() && !plus) {
      m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("-").arg(modeString[0]).arg(modeString.mid(1)));
    }
  }
  deleteLater();
}


void ChannelOptionsDialog::toggleAdvancedModes()
{
  bool ison = m_widget->toggleAdvancedModes->isOn();
  m_widget->otherModesList->setShown(ison);
  if(ison) {
    m_widget->toggleAdvancedModes->setText("&Hide Advanced Modes <<");
  } else {
    m_widget->toggleAdvancedModes->setText("&Show Advanced Modes >>");
  }
}

QString ChannelOptionsDialog::topic()
{
  return m_widget->topicEdit->text().replace("\n"," ");
}

void ChannelOptionsDialog::refreshTopicHistory()
{
  QStringList history = m_channel->getTopicHistory();
  m_widget->topicHistoryList->clear();

  for(QStringList::const_iterator it = history.begin(); it != history.end(); ++it) {
    new KListViewItem(m_widget->topicHistoryList, (*it).section(' ', 0, 0), (*it).section(' ', 1));
  }

  m_widget->topicEdit->setText(history.first().section(' ', 1));
}

void ChannelOptionsDialog::topicHistoryItemClicked(QListViewItem* item)
{
  //if they didn't click on anything, item is null
  if(item)
    m_widget->topicEdit->setText(item->text(1));
}

void ChannelOptionsDialog::refreshEnableModes() {
  bool enable = m_channel->getOwnChannelNick()->isAnyTypeOfOp();
  m_widget->otherModesList->setEnabled(enable);
  m_widget->topicEdit->setEnabled(enable || !m_widget->topicModeChBox->isChecked());
  m_widget->topicHistoryList->setEnabled(enable || !m_widget->topicModeChBox->isChecked());

  m_widget->topicModeChBox->setEnabled(enable);
  m_widget->messageModeChBox->setEnabled(enable);
  m_widget->userLimitChBox->setEnabled(enable);
  m_widget->userLimitEdit->setEnabled(enable);
  m_widget->inviteModeChBox->setEnabled(enable);
  m_widget->moderatedModeChBox->setEnabled(enable);
  m_widget->secretModeChBox->setEnabled(enable);
  m_widget->keyModeChBox->setEnabled(enable);
  m_widget->keyModeEdit->setEnabled(enable);
}
void ChannelOptionsDialog::refreshAllowedChannelModes()
{
  QString modeString = m_channel->getServer()->allowedChannelModes();
  // These modes are handled in a special way: ntimslkbeI
  modeString.remove('t');
  modeString.remove('n');
  modeString.remove('l');
  modeString.remove('i');
  modeString.remove('m');
  modeString.remove('s');
  modeString.remove('k');
  modeString.remove('b');
  modeString.remove('e');
  modeString.remove('I');
  modeString.remove('O');
  modeString.remove('o');
  modeString.remove('v');

  for(unsigned int i = 0; i < modeString.length(); i++) {
    new QCheckListItem(m_widget->otherModesList, QString(modeString[i]), QCheckListItem::CheckBox);
  }
}

void ChannelOptionsDialog::refreshModes()
{
  QStringList modes = m_channel->getModeList();
  
  m_widget->topicModeChBox->setChecked(false);
  m_widget->messageModeChBox->setChecked(false);
  m_widget->userLimitChBox->setChecked(false);
  m_widget->userLimitEdit->setValue(0);
  m_widget->inviteModeChBox->setChecked(false);
  m_widget->moderatedModeChBox->setChecked(false);
  m_widget->secretModeChBox->setChecked(false);
  m_widget->keyModeChBox->setChecked(false);
  m_widget->keyModeEdit->clear();

  QListViewItem* item = m_widget->otherModesList->firstChild();

  while(item) {
    static_cast<QCheckListItem*>(item)->setOn(false);
    item = item->nextSibling();
  }

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
        m_widget->userLimitEdit->setValue((*it).mid(1).toInt());
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
      {
        bool found = false;
        item = m_widget->otherModesList->firstChild();
        QString modeString;
        modeString = mode;

        while(item && !found) {
          if(item->text(0) == modeString) {
            found = true;
            static_cast<QCheckListItem*>(item)->setOn(true);
            item->setText(1, (*it).mid(1));
          } else {
            item = item->nextSibling();
          }
        }

        break;
      }
    }
  }

  refreshEnableModes();
}

QStringList ChannelOptionsDialog::modes()
{
  QStringList modes;
  QString sign;

  sign = (m_widget->topicModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "t");
  sign = (m_widget->messageModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "n");
  sign = (m_widget->userLimitChBox->isChecked() ? "+" : "-");
  modes.append(sign + "l" + m_widget->userLimitEdit->value());
  sign = (m_widget->inviteModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "i");
  sign = (m_widget->moderatedModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "m");
  sign = (m_widget->secretModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "s");
  sign = (m_widget->keyModeChBox->isChecked() ? "+" : "-");
  modes.append(sign + "k" + m_widget->keyModeEdit->text());

  QListViewItem* item = m_widget->otherModesList->firstChild();

  while(item) {
    sign = (static_cast<QCheckListItem*>(item)->isOn() ? "+" : "-");
    modes.append(sign + item->text(0) + item->text(1));
    item = item->nextSibling();
  }

  return modes;
}

}

#include "channeloptionsdialog.moc"
