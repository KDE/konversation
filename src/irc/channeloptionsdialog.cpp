/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005-2007 Peter Simonsson <psn@linux.se>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006-2007 Eike Hein <hein@kde.org>
*/

#include "channeloptionsdialog.h"
#include "application.h"
#include "channel.h"

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qstandarditemmodel.h>
#include <qtoolbutton.h>
#include <QKeyEvent>

#include <klocale.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kiconloader.h>


namespace Konversation
{

    ChannelOptionsDialog::ChannelOptionsDialog(Channel *channel)
        : KDialog(channel)
    {
        setCaption(  i18n("Channel Settings for %1", channel->getName() ) );
        setButtons( KDialog::Ok|KDialog::Cancel );
        setDefaultButton( KDialog::Ok );

        Q_ASSERT(channel);
        m_ui.setupUi(mainWidget());

        QStandardItemModel *modesModel = new QStandardItemModel(m_ui.otherModesList);
        modesModel->setHorizontalHeaderLabels(
            QStringList() << i18n("Mode") << i18n("Parameter"));
        m_ui.otherModesList->setModel(modesModel);
        m_ui.otherModesList->hide();

        m_ui.banList->setDefaultRenameAction(Q3ListView::Accept);
        m_ui.banListSearchLine->setListView(m_ui.banList);
        // hide column where the complete topic will be put in for convenience
        m_ui.topicHistoryList->hideColumn(2);

        m_channel = channel;
        m_editingTopic = false;

        connect(m_ui.topicHistoryList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(topicHistoryItemClicked(QTreeWidgetItem*)));
        connect(m_ui.toggleAdvancedModes, SIGNAL(clicked()), this, SLOT(toggleAdvancedModes()));
        connect(m_ui.topicEdit, SIGNAL(textChanged()), this, SLOT(topicBeingEdited()));

        connect(m_channel, SIGNAL(topicHistoryChanged()), this, SLOT(refreshTopicHistory()));

        connect(m_channel, SIGNAL(modesChanged()), this, SLOT(refreshModes()));
        connect(m_channel->getOwnChannelNick().data(), SIGNAL(channelNickChanged()), this, SLOT(refreshEnableModes()));

        connect(this, SIGNAL(cancelClicked()), this, SLOT(cancelClicked()));
        connect(this, SIGNAL(okClicked()), this, SLOT(changeOptions()));
        connect(this, SIGNAL(okClicked()), this, SLOT(okClicked()));

        connect(m_channel, SIGNAL(banAdded(const QString&)), this, SLOT(addBan(const QString&)));
        connect(m_channel, SIGNAL(banRemoved(const QString&)), this, SLOT(removeBan(const QString&)));
        connect(m_channel, SIGNAL(banListCleared()), m_ui.banList, SLOT(clear()));

        connect(m_ui.addBan, SIGNAL(clicked()), this, SLOT(addBanClicked()));
        connect(m_ui.removeBan, SIGNAL(clicked()), this, SLOT(removeBanClicked()));
        connect(m_ui.banList, SIGNAL(itemRenamed (Q3ListViewItem*)), this, SLOT(banEdited(Q3ListViewItem*)));
        connect(m_ui.banList, SIGNAL(itemRenamed (Q3ListViewItem*, int, const QString&)), this, SLOT(banEdited(Q3ListViewItem*)));

        m_ui.topicModeChBox->setWhatsThis(whatsThisForMode('T'));
        m_ui.messageModeChBox->setWhatsThis(whatsThisForMode('N'));
        m_ui.secretModeChBox->setWhatsThis(whatsThisForMode('S'));
        m_ui.inviteModeChBox->setWhatsThis(whatsThisForMode('I'));
        m_ui.moderatedModeChBox->setWhatsThis(whatsThisForMode('M'));
        m_ui.keyModeChBox->setWhatsThis(whatsThisForMode('P'));
        m_ui.keyModeEdit->setWhatsThis(whatsThisForMode('P'));
        m_ui.userLimitEdit->setWhatsThis(whatsThisForMode('L'));
        m_ui.userLimitChBox->setWhatsThis(whatsThisForMode('L'));

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
        QString newTopic = topic();
        QString oldTopic = m_channel->getTopicHistory().isEmpty() ? 0 : m_channel->getTopicHistory().first().section(' ', 2);

        if(newTopic != oldTopic)
        {
            // Pass a ^A so we can determine if we want to clear the channel topic.
            if (newTopic.isEmpty())
            {
                if (!oldTopic.isEmpty())
                    m_channel->sendChannelText(Preferences::self()->commandChar() + "TOPIC " + m_channel->getName() + " \x01");
            }
            else
                m_channel->sendChannelText(Preferences::self()->commandChar() + "TOPIC " + m_channel->getName() + ' ' + newTopic);
        }

        QStringList newModeList = modes();
        QStringList currentModeList = m_channel->getModeList();
        QStringList rmModes;
        QStringList addModes;
        QStringList tmp;
        QString modeString;
        bool plus;
        QString command("MODE %1 %2%3 %4");

        for(QStringList::ConstIterator it = newModeList.constBegin(); it != newModeList.constEnd(); ++it)
        {
            modeString = (*it).mid(1);
            plus = ((*it)[0] == '+');
            tmp = currentModeList.filter(QRegExp('^' + modeString));

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
        bool ison = m_ui.toggleAdvancedModes->isChecked();
        m_ui.otherModesList->setVisible(ison);
        if(ison)
        {
            m_ui.toggleAdvancedModes->setText(i18n("&Hide Advanced Modes &lt;&lt;"));
        }
        else
        {
            m_ui.toggleAdvancedModes->setText(i18n("&Show Advanced Modes &gt;&gt;"));
        }
    }

    void ChannelOptionsDialog::topicBeingEdited()
    {
        m_editingTopic = true;
    }

    QString ChannelOptionsDialog::topic()
    {
        return m_ui.topicEdit->toPlainText().replace('\n',' ');
    }

    void ChannelOptionsDialog::refreshTopicHistory()
    {
        QStringList history = m_channel->getTopicHistory();
        m_ui.topicHistoryList->clear();
        for(QStringList::ConstIterator it = --history.constEnd(); it != --history.constBegin(); --it)
        {
            QDateTime date;
            date.setTime_t((*it).section(' ', 0 ,0).toUInt());
            new QTreeWidgetItem(m_ui.topicHistoryList, QStringList() << (*it).section(' ', 1, 1) << date.toString(Qt::LocalDate) << (*it).section(' ', 2));
        }

        // update topic preview
        topicHistoryItemClicked(m_ui.topicHistoryList->currentItem());
        // don't destroy the user's edit box if they started editing
        if(!m_editingTopic && !history.isEmpty())
            m_ui.topicEdit->setText(history.first().section(' ', 2));
    }

    void ChannelOptionsDialog::topicHistoryItemClicked(QTreeWidgetItem* item)
    {
        // if they didn't click on anything, item is null
        if(item)
            // update topic preview
            m_ui.topicPreview->setText(item->text(2));
        else
            // clear topic preview
            m_ui.topicPreview->setText("");
    }

    void ChannelOptionsDialog::refreshEnableModes()
    {
        bool enable = m_channel->getOwnChannelNick()->isAnyTypeOfOp();
        m_ui.otherModesList->setEnabled(enable);
        m_ui.topicEdit->setReadOnly(!enable && m_ui.topicModeChBox->isChecked());

        m_ui.topicModeChBox->setEnabled(enable);
        m_ui.messageModeChBox->setEnabled(enable);
        m_ui.userLimitChBox->setEnabled(enable);
        m_ui.userLimitEdit->setEnabled(enable);
        m_ui.inviteModeChBox->setEnabled(enable);
        m_ui.moderatedModeChBox->setEnabled(enable);
        m_ui.secretModeChBox->setEnabled(enable);
        m_ui.keyModeChBox->setEnabled(enable);
        m_ui.keyModeEdit->setEnabled(enable);

        m_ui.banList->setItemsRenameable(enable);
        m_ui.addBan->setEnabled(enable);
        m_ui.removeBan->setEnabled(enable);
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

        QStandardItemModel *modesModel = qobject_cast<QStandardItemModel *>(m_ui.otherModesList->model());
        for(int i = 0; i < modeString.length(); i++)
        {
            QList<QStandardItem *> newRow;
            QStandardItem *item = 0;
            item = new QStandardItem(QString(modeString[i]));
            item->setCheckable(true);
            item->setEditable(false);
            newRow.append(item);
            item = new QStandardItem();
            item->setEditable(true);
            newRow.append(item);
            modesModel->invisibleRootItem()->appendRow(newRow);
        }
    }

    void ChannelOptionsDialog::refreshModes()
    {
        QStringList modes = m_channel->getModeList();

        m_ui.topicModeChBox->setChecked(false);
        m_ui.messageModeChBox->setChecked(false);
        m_ui.userLimitChBox->setChecked(false);
        m_ui.userLimitEdit->setValue(0);
        m_ui.inviteModeChBox->setChecked(false);
        m_ui.moderatedModeChBox->setChecked(false);
        m_ui.secretModeChBox->setChecked(false);
        m_ui.keyModeChBox->setChecked(false);
        m_ui.keyModeEdit->setText("");

        QStandardItemModel *modesModel = qobject_cast<QStandardItemModel *>(m_ui.otherModesList->model());
        for (int i = 0; i < modesModel->rowCount(); ++i)
        {
            modesModel->item(i, 0)->setCheckState(Qt::Unchecked);
        }

        char mode;

        foreach (const QString &currentMode, modes)
        {
            mode = currentMode[0].toLatin1();
            switch(mode)
            {
                case 't':
                    m_ui.topicModeChBox->setChecked(true);
                    break;
                case 'n':
                    m_ui.messageModeChBox->setChecked(true);
                    break;
                case 'l':
                    m_ui.userLimitChBox->setChecked(true);
                    m_ui.userLimitEdit->setValue(currentMode.mid(1).toInt());
                    break;
                case 'i':
                    m_ui.inviteModeChBox->setChecked(true);
                    break;
                case 'm':
                    m_ui.moderatedModeChBox->setChecked(true);
                    break;
                case 's':
                    m_ui.secretModeChBox->setChecked(true);
                    break;
                case 'k':
                    m_ui.keyModeChBox->setChecked(true);
                    m_ui.keyModeEdit->setText(currentMode.mid(1));
                    break;
                default:
                {
                    bool found = false;
                    QString modeString;
                    modeString = mode;

                    for (int i = 0; !found && i < modesModel->rowCount(); ++i)
                    {
                        QStandardItem *item = modesModel->item(i, 0);
                        if (item->text() == modeString)
                        {
                            found = true;
                            item->setCheckState(Qt::Checked);
                            modesModel->item(i, 1)->setText(currentMode.mid(1));
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

        mode = (m_ui.topicModeChBox->isChecked() ? "+" : "-");
        mode += 't';
        modes.append(mode);
        mode = (m_ui.messageModeChBox->isChecked() ? "+" : "-");
        mode += 'n';
        modes.append(mode);
        mode = (m_ui.userLimitChBox->isChecked() ? "+" : "-");
        mode += 'l' + QString::number( m_ui.userLimitEdit->value() );
        modes.append(mode);
        mode = (m_ui.inviteModeChBox->isChecked() ? "+" : "-");
        mode += 'i';
        modes.append(mode);
        mode = (m_ui.moderatedModeChBox->isChecked() ? "+" : "-");
        mode += 'm';
        modes.append(mode);
        mode = (m_ui.secretModeChBox->isChecked() ? "+" : "-");
        mode += 's';
        modes.append(mode);

        if (m_ui.keyModeChBox->isChecked() && !m_ui.keyModeEdit->text().isEmpty())
        {
            mode = '+';
            mode += 'k' + m_ui.keyModeEdit->text();
            modes.append(mode);
        }
        else if (!m_ui.keyModeChBox->isChecked())
        {
            mode = '-';
            mode += 'k' + m_ui.keyModeEdit->text();
            modes.append(mode);
        }

        QStandardItemModel *modesModel = qobject_cast<QStandardItemModel *>(m_ui.otherModesList->model());
        for (int i = 0; i < modesModel->rowCount(); ++i)
        {
            mode = (modesModel->item(i, 0)->checkState() == Qt::Checked ? "+" : "-");
            mode += modesModel->item(i, 0)->text() + modesModel->item(i, 1)->text();
            modes.append(mode);
        }

        return modes;
    }

    // Ban List tab related functions

    void ChannelOptionsDialog::refreshBanList()
    {
        QStringList banlist = m_channel->getBanList();
        m_ui.banList->clear();

        for (QStringList::const_iterator it = --banlist.constEnd(); it != --banlist.constBegin(); --it)
            addBan((*it));
    }

    void ChannelOptionsDialog::addBan(const QString& newban)
    {
        new BanListViewItem(m_ui.banList, newban.section(' ', 0, 0), newban.section(' ', 1, 1).section('!', 0, 0), newban.section(' ', 2 ,2).toUInt());
    }

    void ChannelOptionsDialog::removeBan(const QString& ban)
    {
        delete m_ui.banList->findItem(ban, 0);
    }

    void ChannelOptionsDialog::banEdited(Q3ListViewItem *edited)
    {
        if (edited == m_NewBan)
        {
            if (!m_NewBan->text(0).isEmpty())
            {
                m_channel->getServer()->requestBan(QStringList(m_NewBan->text(0)), m_channel->getName(), QString());
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
                m_channel->getServer()->requestBan(QStringList(new_edited->text(0)), m_channel->getName(), QString());
            }

            // We delete the existing item because it's possible the server may
            // Modify the ban causing us not to catch it. If that happens we'll be
            // stuck with a stale item and a new item with the modified hostmask.
            delete new_edited;
        }
    }

    void ChannelOptionsDialog::addBanClicked()
    {
        m_NewBan = new BanListViewItem(m_ui.banList, true);

        m_NewBan->setRenameEnabled(0,true);
        m_NewBan->startRename(0);
    }

    void ChannelOptionsDialog::removeBanClicked()
    {
        if (m_ui.banList->currentItem())
            m_channel->getServer()->requestUnban(m_ui.banList->currentItem()->text(0), m_channel->getName());
    }

    void ChannelOptionsDialog::cancelClicked()
    {
        if (m_ui.banList->renameLineEdit()->isVisible())
        {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QApplication::sendEvent(m_ui.banList->renameLineEdit(), &e);
        }

        m_editingTopic = false;
        hide();
    }

    void ChannelOptionsDialog::okClicked()
    {
        if (m_ui.banList->renameLineEdit()->isVisible())
        {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(m_ui.banList->renameLineEdit(), &e);
        }
    }

    // This is our implementation of BanListViewItem

    BanListViewItem::BanListViewItem(Q3ListView *parent)
      : K3ListViewItem(parent)
    {
        m_isNewBan = 0;
    }

    BanListViewItem::BanListViewItem(Q3ListView *parent, bool isNew)
      : K3ListViewItem(parent)
    {
        m_isNewBan = isNew;
    }

    BanListViewItem::BanListViewItem (Q3ListView *parent, const QString& label1, const QString& label2,
        uint timestamp) : K3ListViewItem(parent, label1, label2)
    {
        m_isNewBan = 0;
        m_timestamp.setTime_t(timestamp);
    }

    BanListViewItem::BanListViewItem (Q3ListView *parent, bool isNew, const QString& label1, const QString& label2,
        uint timestamp) : K3ListViewItem(parent, label1, label2)
    {
        m_isNewBan = isNew;
        m_timestamp.setTime_t(timestamp);
    }

    QString BanListViewItem::text(int column) const
    {
        if (column == 2)
            return KGlobal::locale()->formatDateTime(m_timestamp, KLocale::ShortDate, true);

        return K3ListViewItem::text(column);
    }

    int BanListViewItem::compare(Q3ListViewItem *i, int col, bool ascending) const
    {
        if (col == 2)
        {
            BanListViewItem* item = static_cast<BanListViewItem*>(i);

            if (m_timestamp == item->timestamp())
                return 0;
            else if (m_timestamp < item->timestamp())
                return -1;
            else
                return 1;
        }

        return K3ListViewItem::compare(i, col, ascending);
    }

    void BanListViewItem::startRename( int col )
    {
        m_oldValue = text(col);

        K3ListViewItem::startRename(col);
    }

    void BanListViewItem::cancelRename( int col )
    {
        if (text(col).isEmpty() && m_isNewBan)
            delete this;
        else
            K3ListViewItem::cancelRename(col);
    }
}

QString Konversation::ChannelOptionsDialog::whatsThisForMode(char mode)
{
    switch (mode) {
    case 'T':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>The <b>T</b>opic mode means that only the channel operator can change the topic for the channel.</p></qt>");
    case 'N':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p><b>N</b>o messages from outside means that users that are not in the channel cannot send messages that everybody in the channel can see.  Almost all channels have this set to prevent nuisance messages.</p></qt>");
    case 'S':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>S</b>ecret channel will not show up in the channel list, nor will any user be able to see that you are in the channel with the <em>WHOIS</em> command or anything similar.  Only the people that are in the same channel will know that you are in this channel, if this mode is set.</p></qt>");
    case 'I':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>An <b>I</b>nvite only channel means that people can only join the channel if they are invited.  To invite someone, a channel operator needs to issue the command <em>/invite nick</em> from within the channel.</p></qt>");
    case 'P':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>P</b>rivate channel is shown in a listing of all channels, but the topic is not shown.  A user's <em>WHOIS</em> may or may not show them as being in a private channel depending on the IRC server.</p></qt>");
    case 'M':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>M</b>oderated channel is one where only operators, half-operators and those with voice can talk.</p></qt>");
    case 'K':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>P</b>rotected channel requires users to enter a password in order to join.</p></qt>");
    case 'L':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A channel that has a user <b>L</b>imit means that only that many users can be in the channel at any one time.  Some channels have a bot that sits in the channel and changes this automatically depending on how busy the channel is.</p></qt>");
    default:
        kWarning() << "called for unknown mode" << mode;
        return QString();
    }
}

#include "channeloptionsdialog.moc"
