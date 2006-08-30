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
        : KDialogBase(channel, "channelOptions", false, i18n("Channel Settings for %1").arg(channel->getName()), Ok|Cancel, Ok)
    {
        Q_ASSERT(channel);
        m_widget = new ChannelOptionsUI(this);
        setMainWidget(m_widget);

        m_widget->otherModesList->setRenameable(0, false);
        m_widget->otherModesList->setRenameable(1, true);
        m_widget->otherModesList->hide();

        // don't allow sorting. most recent topic is always first
        m_widget->topicHistoryList->setSortColumn(-1);
        m_widget->banList->setDefaultRenameAction(QListView::Accept);
        // hide column where the complete topic will be put in for convenience
        m_widget->topicHistoryList->hideColumn(2);
        // do not allow the user to resize the hidden column back into view
        m_widget->topicHistoryList->header()->setResizeEnabled(false,2);

        m_channel = channel;
        m_editingTopic = false;

        connect(m_widget->topicHistoryList, SIGNAL(clicked(QListViewItem*)), this, SLOT(topicHistoryItemClicked(QListViewItem*)));
        connect(m_widget->topicHistoryList, SIGNAL(selectionChanged(QListViewItem*)), this, SLOT(topicHistoryItemClicked(QListViewItem*)));
        connect(m_widget->toggleAdvancedModes, SIGNAL(clicked()), this, SLOT(toggleAdvancedModes()));
        connect(m_widget->topicEdit, SIGNAL(modificationChanged(bool)), this, SLOT(topicBeingEdited(bool)));

        connect(m_channel, SIGNAL(topicHistoryChanged()), this, SLOT(refreshTopicHistory()));

        connect(m_channel, SIGNAL(modesChanged()), this, SLOT(refreshModes()));
        connect(m_channel->getOwnChannelNick(), SIGNAL(channelNickChanged()), this, SLOT(refreshEnableModes()));

        connect(this, SIGNAL(cancelClicked()), this, SLOT(hide()));
        connect(this, SIGNAL(okClicked()), this, SLOT(changeOptions()));

        connect(m_channel, SIGNAL(banAdded(const QString&)), this, SLOT(addBan(const QString&)));
        connect(m_channel, SIGNAL(banRemoved(const QString&)), this, SLOT(removeBan(const QString&)));
        connect(m_channel, SIGNAL(banListCleared()), m_widget->banList, SLOT(clear()));

        connect(m_widget->addBan, SIGNAL(clicked()), this, SLOT(addBanClicked()));
        connect(m_widget->removeBan, SIGNAL(clicked()), this, SLOT(removeBanClicked()));
        connect(m_widget->banList, SIGNAL(itemRenamed (QListViewItem*)), this, SLOT(banEdited(QListViewItem*)));
        connect(m_widget->banList, SIGNAL(itemRenamed (QListViewItem*, int, const QString&)), this, SLOT(banEdited(QListViewItem*)));

        refreshTopicHistory();
        refreshBanList();
        refreshAllowedChannelModes();
        refreshModes();
    }

    ChannelOptionsDialog::~ChannelOptionsDialog()
    {
    }

    void ChannelOptionsDialog::changeOptions()
    {
        QString newTopic = topic(), oldTopic=m_channel->getTopicHistory().first().section(' ', 2);

        if(newTopic != oldTopic)
        {
            // Pass a ^A so we can determine if we want to clear the channel topic.
            if (newTopic.isEmpty())
            {
                if (!oldTopic.isEmpty())
                    m_channel->sendChannelText(Preferences::commandChar() + "TOPIC " + m_channel->getName() + " \x01");
            }
            else
                m_channel->sendChannelText(Preferences::commandChar() + "TOPIC " + m_channel->getName() + ' ' + newTopic);
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
            tmp = currentModeList.grep(QRegExp('^' + modeString));

            if(tmp.isEmpty() && plus)
            {
                m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("+").arg(modeString[0]).arg(modeString.mid(1)));
            }
            else if(!tmp.isEmpty() && !plus)
            {
                //FIXME: Bahamuth requires the key parameter for -k, but ircd breaks on -l with limit number.
                //Hence two versions of this.
                if (modeString[0] == 'k')
                    m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("-").arg(modeString[0]).arg(modeString.mid(1)));
                else
                    m_channel->getServer()->queue(command.arg(m_channel->getName()).arg("-").arg(modeString[0]).arg(""));
            }
        }
        hide();
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

        // update topic preview
        topicHistoryItemClicked(m_widget->topicHistoryList->selectedItem());
        // don't destroy the user's edit box if they started editing
        if(!m_editingTopic)
            m_widget->topicEdit->setText(history.first().section(' ', 2));
    }

    void ChannelOptionsDialog::topicHistoryItemClicked(QListViewItem* item)
    {
        // if they didn't click on anything, item is null
        if(item)
            // update topic preview
            m_widget->topicPreview->setText(item->text(2));
        else
            // clear topic preview
            m_widget->topicPreview->clear();
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

        m_widget->banList->setItemsRenameable(enable);
        m_widget->addBan->setEnabled(enable);
        m_widget->removeBan->setEnabled(enable);
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
        mode += 't';
        modes.append(mode);
        mode = (m_widget->messageModeChBox->isChecked() ? "+" : "-");
        mode += 'n';
        modes.append(mode);
        mode = (m_widget->userLimitChBox->isChecked() ? "+" : "-");
        mode += 'l' + QString::number( m_widget->userLimitEdit->value() );
        modes.append(mode);
        mode = (m_widget->inviteModeChBox->isChecked() ? "+" : "-");
        mode += 'i';
        modes.append(mode);
        mode = (m_widget->moderatedModeChBox->isChecked() ? "+" : "-");
        mode += 'm';
        modes.append(mode);
        mode = (m_widget->secretModeChBox->isChecked() ? "+" : "-");
        mode += 's';
        modes.append(mode);

        if (m_widget->keyModeChBox->isChecked() && !m_widget->keyModeEdit->text().isEmpty())
        {
            mode = '+';
            mode += 'k' + m_widget->keyModeEdit->text();
            modes.append(mode);
        }
        else if (!m_widget->keyModeChBox->isChecked())
        {
            mode = '-';
            mode += 'k' + m_widget->keyModeEdit->text();
            modes.append(mode);
        }

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

    // Ban List tab related functions

    void ChannelOptionsDialog::refreshBanList()
    {
        QStringList banlist = m_channel->getBanList();
        m_widget->banList->clear();

        for(QStringList::const_iterator it = banlist.fromLast(); it != banlist.end(); --it)
        {
            QDateTime date;
            date.setTime_t((*it).section(' ', 2 ,2).toUInt());

            new BanListViewItem(m_widget->banList, (*it).section(' ', 0, 0), (*it).section(' ', 1, 1).section('!', 0, 0), date.toString(Qt::LocalDate));
        }
    }

    void ChannelOptionsDialog::addBan(const QString& newban)
    {
        QDateTime date;
        date.setTime_t(newban.section(' ', 2 ,2).toUInt());

        new BanListViewItem(m_widget->banList, newban.section(' ', 0, 0), newban.section(' ', 1, 1).section('!', 0, 0), date.toString(Qt::LocalDate));
    }

    void ChannelOptionsDialog::removeBan(const QString& ban)
    {
        delete m_widget->banList->findItem(ban, 0);
    }

    void ChannelOptionsDialog::banEdited(QListViewItem *edited)
    {
        if (edited == m_NewBan)
        {
            if (!m_NewBan->text(0).isEmpty())
            {
                m_channel->getServer()->requestBan(QStringList(m_NewBan->text(0)), m_channel->getName(), QString::null);
            }

            // We will delete the item and let the addBan slot handle
            // readding the item because for some odd reason using
            // startRename causes further attempts to rename the item
            // using 2 mouse clicks to fail in odd ways.
            delete edited;

            return;
        }

        BanListViewItem *new_edited = dynamic_cast <BanListViewItem*> (edited);
        if (new_edited == NULL) return; // Should not happen.

        if (new_edited->getOldValue() != new_edited->text(0))
        {
            m_channel->getServer()->requestUnban(new_edited->getOldValue(), m_channel->getName());

            if (!new_edited->text(0).isEmpty())
            {
                m_channel->getServer()->requestBan(QStringList(new_edited->text(0)), m_channel->getName(), QString::null);
            }

            // We delete the existing item because it's possible the server may
            // Modify the ban causing us not to catch it. If that happens we'll be
            // stuck with a stale item and a new item with the modified hostmask.
            delete new_edited;
        }
    }

    void ChannelOptionsDialog::addBanClicked()
    {
        m_NewBan = new BanListViewItem(m_widget->banList, true);

        m_NewBan->setRenameEnabled(0,true);
        m_NewBan->startRename(0);
    }

    void ChannelOptionsDialog::removeBanClicked()
    {
        if (m_widget->banList->currentItem())
        {
            m_channel->getServer()->requestUnban(m_widget->banList->currentItem()->text(0), m_channel->getName());
        }
    }

    // This is our implementation of BanListViewItem

    BanListViewItem::BanListViewItem(QListView *parent)
      : KListViewItem(parent)
    {
        m_isNewBan = 0;
    }

    BanListViewItem::BanListViewItem(QListView *parent, bool isNew)
      : KListViewItem(parent)
    {
        m_isNewBan = isNew;
    }

    BanListViewItem::BanListViewItem ( QListView *parent, QString label1, QString label2, QString label3, QString label4, QString label5, QString label6, QString label7, QString label8 ) : KListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
    {
        m_isNewBan = 0;
    }

    BanListViewItem::BanListViewItem ( QListView *parent, bool isNew, QString label1, QString label2, QString label3, QString label4, QString label5, QString label6, QString label7, QString label8 ) : KListViewItem(parent, label1, label2, label3, label4, label5, label6, label7, label8)
    {
        m_isNewBan = isNew;
    }

    void BanListViewItem::startRename( int col )
    {
        m_oldValue = text(col);

        KListViewItem::startRename(col);
    }

    void BanListViewItem::cancelRename( int col )
    {
        if (text(col).isEmpty() && m_isNewBan)
            delete this;
        else
            KListViewItem::cancelRename(col);
    }
}

#include "channeloptionsdialog.moc"
