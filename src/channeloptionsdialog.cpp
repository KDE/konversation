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

#include <qcheckbox.h>

#include <klocale.h>
#include <klistview.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <knuminput.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qheader.h>

#include "channeloptionsdialog.h"
#include "konversationapplication.h"
#include "channeloptionsui.h"
#include "channel.h"

namespace Konversation
{

    ChannelOptionsDialog::ChannelOptionsDialog(Channel *channel)
        : KDialogBase(channel, "channelOptions", false, i18n("Channel Options for %1").arg(channel->getName()), Ok|Cancel, Ok)
    {
        Q_ASSERT(channel);
        m_widget = new ChannelOptionsUI(this);
        setMainWidget(m_widget);

        m_widget->otherModesList->setRenameable(0, false);
        m_widget->otherModesList->setRenameable(1, true);
        m_widget->otherModesList->hide();

        // don't allow sorting. most recent topic is always first
        m_widget->topicHistoryList->setSortColumn(-1);
        // hide column where the complete topic will be put in for convenience
        m_widget->topicHistoryList->hideColumn(2);
        // do not allow the user to resize the hidden column back into view
        m_widget->topicHistoryList->header()->setResizeEnabled(false,2);

        m_channel = channel;
        m_editingTopic = false;

        connect(m_widget->topicHistoryList, SIGNAL(clicked(QListViewItem*)), this, SLOT(topicHistoryItemClicked(QListViewItem*)));
        connect(m_widget->toggleAdvancedModes, SIGNAL(clicked()), this, SLOT(toggleAdvancedModes()));
        connect(m_widget->topicEdit, SIGNAL(modificationChanged(bool)), this, SLOT(topicBeingEdited(bool)));

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

        if(newTopic != m_channel->getTopicHistory().first().section(' ', 2))
        {
            // Pass a \n to avoid whitespace stripping so we can determine if we want to clear the channel topic.
            if (newTopic.isEmpty())
                m_channel->sendChannelText(Preferences::commandChar() + "TOPIC " + m_channel->getName() + " \n");
            else
                m_channel->sendChannelText(Preferences::commandChar() + "TOPIC " + m_channel->getName() + " " + newTopic);
        }

        QStringList newModeList = modes();
        QStringList currentModeList = m_channel->getModeList();
        QStringList rmModes;
        QStringList addModes;
        QStringList tmp;
        QString modeString;
        bool plus;
        QString command("MODE %1 %2%3 %4");

        for(QStringList::iterator it = newModeList.begin(); it != newModeList.end(); ++it)
        {
            modeString = (*it).mid(1);
            plus = ((*it)[0] == '+');
            tmp = currentModeList.grep(QRegExp("^" + modeString));

            if(tmp.isEmpty() && plus)
            {
                m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("+").arg(modeString[0]).arg(modeString.mid(1)));
            }
            else if(!tmp.isEmpty() && !plus)
            {
                m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("-").arg(modeString[0]).arg(modeString.mid(1)));
            }
        }
        deleteLater();
    }

    void ChannelOptionsDialog::toggleAdvancedModes()
    {
        bool ison = m_widget->toggleAdvancedModes->isOn();
        m_widget->otherModesList->setShown(ison);
        if(ison)
        {
            m_widget->toggleAdvancedModes->setText(i18n("&Hide Advanced Modes <<"));
        }
        else
        {
            m_widget->toggleAdvancedModes->setText(i18n("&Show Advanced Modes >>"));
        }
    }

    void ChannelOptionsDialog::topicBeingEdited(bool state)
    {
        m_editingTopic = state;
    }

    QString ChannelOptionsDialog::topic()
    {
        return m_widget->topicEdit->text().replace("\n"," ");
    }

    void ChannelOptionsDialog::refreshTopicHistory()
    {
        QStringList history = m_channel->getTopicHistory();
        m_widget->topicHistoryList->clear();

        for(QStringList::const_iterator it = history.fromLast(); it != history.end(); --it)
        {
            QDateTime date;
            date.setTime_t((*it).section(' ', 0 ,0).toUInt());
            new KListViewItem(m_widget->topicHistoryList, (*it).section(' ', 1, 1), date.toString(Qt::LocalDate), (*it).section(' ', 2));
        }

        m_widget->topicEdit->setText(history.first().section(' ', 2));
    }

    void ChannelOptionsDialog::topicHistoryItemClicked(QListViewItem* item)
    {
        //if they didn't click on anything, item is null
        if(m_channel->getOwnChannelNick()->isAnyTypeOfOp() || !m_widget->topicModeChBox->isChecked())
        {
            if(item)
            {
                m_widget->topicPreview->setText(item->text(2));
                if(!m_editingTopic)
                    m_widget->topicEdit->setText(item->text(2));
            }
        }
    }

    void ChannelOptionsDialog::refreshEnableModes()
    {
        bool enable = m_channel->getOwnChannelNick()->isAnyTypeOfOp();
        m_widget->otherModesList->setEnabled(enable);
        m_widget->topicEdit->setReadOnly(!enable && m_widget->topicModeChBox->isChecked());

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

        for(unsigned int i = 0; i < modeString.length(); i++)
        {
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

        while(item)
        {
            static_cast<QCheckListItem*>(item)->setOn(false);
            item = item->nextSibling();
        }

        char mode;

        for(QStringList::const_iterator it = modes.begin(); it != modes.end(); ++it)
        {
            mode = (*it)[0];

            switch(mode)
            {
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

                    while(item && !found)
                    {
                        if(item->text(0) == modeString)
                        {
                            found = true;
                            static_cast<QCheckListItem*>(item)->setOn(true);
                            item->setText(1, (*it).mid(1));
                        }
                        else
                        {
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
        QString mode;

        mode = (m_widget->topicModeChBox->isChecked() ? "+" : "-");
        mode += "t";
        modes.append(mode);
        mode = (m_widget->messageModeChBox->isChecked() ? "+" : "-");
        mode += "n";
        modes.append(mode);
        mode = (m_widget->userLimitChBox->isChecked() ? "+" : "-");
        mode += "l" + m_widget->userLimitEdit->value();
        modes.append(mode);
        mode = (m_widget->inviteModeChBox->isChecked() ? "+" : "-");
        mode += "i";
        modes.append(mode);
        mode = (m_widget->moderatedModeChBox->isChecked() ? "+" : "-");
        mode += "m";
        modes.append(mode);
        mode = (m_widget->secretModeChBox->isChecked() ? "+" : "-");
        mode += "s";
        modes.append(mode);
        mode = (m_widget->keyModeChBox->isChecked() ? "+" : "-");
        mode += "k" + m_widget->keyModeEdit->text();
        modes.append(mode);

        QListViewItem* item = m_widget->otherModesList->firstChild();

        while(item)
        {
            mode = (static_cast<QCheckListItem*>(item)->isOn() ? "+" : "-");
            mode += item->text(0) + item->text(1);
            modes.append(mode);
            item = item->nextSibling();
        }

        return modes;
    }

}

#include "channeloptionsdialog.moc"
