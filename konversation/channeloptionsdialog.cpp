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

#include "channeloptionsui.h"

namespace Konversation {

ChannelOptionsDialog::ChannelOptionsDialog(const QString& channel, QWidget *parent, const char *name)
  : KDialogBase(parent, name, false, i18n("Channel Options for %1").arg(channel), Ok|Cancel, Ok)
{
  m_widget = new ChannelOptionsUI(this);
  setMainWidget(m_widget);

  m_widget->otherModesList->setRenameable(0, false);
  m_widget->otherModesList->setRenameable(1, true);

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
  if(m_widget && m_widget->topiEdit)
    m_widget->topicEdit->setText(item->text(1));
}

void ChannelOptionsDialog::setAllowedChannelModes(const QString& modes)
{
  QString modeString = modes;
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

void ChannelOptionsDialog::setModes(const QStringList& modes)
{
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
