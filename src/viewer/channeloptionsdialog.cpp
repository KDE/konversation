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
#include "topichistorymodel.h"

#include <KColorScheme>

#include <QCheckBox>
#include <QHeaderView>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QKeyEvent>
#include <QItemSelectionModel>
#include <QTreeWidget>
#include <KSharedConfig>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLocale>

namespace Konversation
{
    ChannelOptionsDialog::ChannelOptionsDialog(Channel *channel)
        : QDialog(channel)
    {
        setWindowTitle(  i18n("Channel Settings for %1", channel->getName() ) );
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        QWidget *mainWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ChannelOptionsDialog::changeOptions);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ChannelOptionsDialog::reject);
        mainLayout->addWidget(buttonBox);
        buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

        Q_ASSERT(channel);
        m_channel = channel;

        m_ui.setupUi(mainWidget);

        m_ui.addBan->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        m_ui.updateBan->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
        m_ui.removeBan->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));

        QStandardItemModel *modesModel = new QStandardItemModel(m_ui.otherModesList);
        m_ui.otherModesList->setModel(modesModel);
        m_ui.otherModesList->hide();

        m_ui.banListSearchLine->setTreeWidget(m_ui.banList);

        m_ui.topicHistoryView->setServer(m_channel->getServer());
        m_ui.topicHistoryView->setModel(m_channel->getTopicHistory());

        m_historySearchTimer = new QTimer(this);
        m_historySearchTimer->setSingleShot(true);

        connect(m_historySearchTimer, &QTimer::timeout,
                this, &ChannelOptionsDialog::updateHistoryFilter);

        connect(m_ui.topicHistorySearchLine, &QLineEdit::textChanged,
                this, &ChannelOptionsDialog::startHistorySearchTimer);

        m_editingTopic = false;

        m_ui.topicEdit->setChannel(channel);
        m_ui.topicEdit->setMaximumLength(m_channel->getServer()->topicLength());

        connect(m_ui.topicHistoryView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &ChannelOptionsDialog::topicHistoryItemClicked);
        connect(m_ui.toggleAdvancedModes, &QPushButton::clicked, this, &ChannelOptionsDialog::toggleAdvancedModes);
        connect(m_ui.topicEdit, &TopicEdit::undoAvailable, this, &ChannelOptionsDialog::topicBeingEdited);
        connect(this, &ChannelOptionsDialog::finished, m_ui.topicEdit, &TopicEdit::clear);

        connect(m_channel, &Channel::modesChanged, this, &ChannelOptionsDialog::refreshModes);
        connect(m_channel->getServer(), SIGNAL(channelNickChanged(QString)), this, SLOT(refreshEnableModes()));

        connect(m_channel, &Channel::banAdded, this, &ChannelOptionsDialog::addBan);
        connect(m_channel, &Channel::banRemoved, this, &ChannelOptionsDialog::removeBan);
        connect(m_channel, &Channel::banListCleared, m_ui.banList, &QTreeWidget::clear);

        connect(m_ui.addBan, &QPushButton::clicked, this, &ChannelOptionsDialog::addBanClicked);
        connect(m_ui.updateBan, &QPushButton::clicked, this, &ChannelOptionsDialog::updateBanClicked);
        connect(m_ui.removeBan, &QPushButton::clicked, this, &ChannelOptionsDialog::removeBanClicked);
        connect(m_ui.banList, &QTreeWidget::itemSelectionChanged, this, &ChannelOptionsDialog::banSelectionChanged);
        connect(m_ui.hostmask, &KLineEdit::textChanged, this, &ChannelOptionsDialog::hostmaskChanged);

        m_ui.topicModeChBox->setWhatsThis(whatsThisForMode('T'));
        m_ui.messageModeChBox->setWhatsThis(whatsThisForMode('N'));
        m_ui.secretModeChBox->setWhatsThis(whatsThisForMode('S'));
        m_ui.inviteModeChBox->setWhatsThis(whatsThisForMode('I'));
        m_ui.moderatedModeChBox->setWhatsThis(whatsThisForMode('M'));
        m_ui.keyModeChBox->setWhatsThis(whatsThisForMode('P'));
        m_ui.keyModeEdit->setWhatsThis(whatsThisForMode('P'));
        m_ui.userLimitEdit->setWhatsThis(whatsThisForMode('L'));
        m_ui.userLimitChBox->setWhatsThis(whatsThisForMode('L'));

        refreshBanList();

        resize(QSize(450, 420));
    }

    ChannelOptionsDialog::~ChannelOptionsDialog()
    {
    }

    void ChannelOptionsDialog::showEvent(QShowEvent* event)
    {
        if (!event->spontaneous())
        {
            refreshAllowedChannelModes();
            refreshModes();

            m_ui.topicEdit->clear();
            m_editingTopic = false;

            m_ui.topicHistoryView->selectionModel()->clearSelection();
            const QModelIndex& currentTopic = m_ui.topicHistoryView->model()->index(m_ui.topicHistoryView->model()->rowCount() - 1, 0);
            m_ui.topicHistoryView->selectionModel()->select(currentTopic, QItemSelectionModel::Select);
            m_ui.topicHistoryView->scrollTo(currentTopic, QAbstractItemView::EnsureVisible);

            if (!m_ui.topicEdit->isReadOnly())
                m_ui.topicEdit->setFocus();

            KConfigGroup config(KSharedConfig::openConfig(), "ChannelOptionsDialog");

            resize(config.readEntry("Size", sizeHint()));

            const QList<int>& sizes = config.readEntry("SplitterSizes", QList<int>());

            if (!sizes.isEmpty())
                m_ui.splitter->setSizes(sizes);

            Preferences::restoreColumnState(m_ui.banList, QStringLiteral("BanList ViewSettings"));
        }

        QDialog::showEvent(event);
    }

    void ChannelOptionsDialog::hideEvent(QHideEvent* event)
    {
        KConfigGroup config(KSharedConfig::openConfig(), "ChannelOptionsDialog");

        config.writeEntry("Size", size());
        config.writeEntry("SplitterSizes", m_ui.splitter->sizes());

        Preferences::saveColumnState(m_ui.banList, QStringLiteral("BanList ViewSettings"));

        QDialog::hideEvent(event);
    }

    void ChannelOptionsDialog::changeOptions()
    {
        const QString& newTopic = topic();
        const QString& oldTopic = m_channel->getTopic();

        if (newTopic != oldTopic)
        {
            // Pass a ^A so we can determine if we want to clear the channel topic.
            if (newTopic.isEmpty())
            {
                if (!oldTopic.isEmpty())
                    m_channel->sendText(Preferences::self()->commandChar() + "TOPIC " + m_channel->getName() + " \x01");
            }
            else
                m_channel->sendText(Preferences::self()->commandChar() + "TOPIC " + m_channel->getName() + ' ' + newTopic);
        }

        const QStringList newModeList = modes();
        const QStringList currentModeList = m_channel->getModeList();

        QString command(QStringLiteral("MODE %1 %2%3 %4"));

        for (const QString &mode : newModeList)
        {
            const QString modeString = mode.mid(1);
            const bool plus = mode.at(0) == QLatin1Char('+');
            const QStringList tmp = currentModeList.filter(QRegularExpression(QLatin1Char('^') + modeString));

            if(tmp.isEmpty() && plus)
            {
                m_channel->getServer()->queue(command.arg(m_channel->getName(), QStringLiteral("+"),
                                                          modeString.at(0), modeString.mid(1)));
            }
            else if(!tmp.isEmpty() && !plus)
            {
                //FIXME: Bahamuth requires the key parameter for -k, but ircd breaks on -l with limit number.
                //Hence two versions of this.
                if (modeString.at(0) == QLatin1Char('k')) {
                    m_channel->getServer()->queue(command.arg(m_channel->getName(), QStringLiteral("-"),
                                                              modeString.at(0), modeString.mid(1)));
                } else {
                    m_channel->getServer()->queue(command.arg(m_channel->getName(), QStringLiteral("-"),
                                                              modeString.at(0), QString()));
                }
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
            m_ui.toggleAdvancedModes->setText(i18n("&Hide Advanced Modes <<"));
        }
        else
        {
            m_ui.toggleAdvancedModes->setText(i18n("&Show Advanced Modes >>"));
        }
    }

    void ChannelOptionsDialog::topicBeingEdited(bool edited)
    {
        m_editingTopic = edited;
        m_ui.topicHistoryView->setTextSelectable(edited);
    }

    QString ChannelOptionsDialog::topic()
    {
        return m_ui.topicEdit->toPlainText().replace('\n', ' ');
    }

    void ChannelOptionsDialog::topicHistoryItemClicked(const QItemSelection& selection)
    {
        if (!m_editingTopic)
        {
            m_ui.topicEdit->clear();

            if (!selection.isEmpty())
            {
                m_ui.topicEdit->setUndoRedoEnabled(false);
                m_ui.topicEdit->setPlainText(m_ui.topicHistoryView->model()->data(selection.indexes().first()).toString());
                m_ui.topicEdit->setUndoRedoEnabled(true);
            }
        }
    }

    void ChannelOptionsDialog::refreshEnableModes(bool forceUpdate)
    {
        if(!m_channel->getOwnChannelNick() || m_channel->getOwnChannelNick()->isChanged() || forceUpdate)
        {
            // cache the value
            m_isAnyTypeOfOp = m_channel->getOwnChannelNick() ? m_channel->getOwnChannelNick()->isAnyTypeOfOp() : false;

            m_ui.topicEdit->setReadOnly(!m_isAnyTypeOfOp && m_ui.topicModeChBox->isChecked());

            m_ui.topicModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.messageModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.userLimitChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.userLimitEdit->setEnabled(m_isAnyTypeOfOp);
            m_ui.inviteModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.moderatedModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.secretModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.keyModeChBox->setEnabled(m_isAnyTypeOfOp);
            m_ui.keyModeEdit->setEnabled(m_isAnyTypeOfOp);

            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(m_ui.otherModesList->model());

            if (model)
            {
                QList<QStandardItem*> items = model->findItems(QStringLiteral("*"), Qt::MatchWildcard, 0);
                items += model->findItems(QStringLiteral("*"), Qt::MatchWildcard, 1);

                foreach (QStandardItem* item, items)
                    item->setEnabled(m_isAnyTypeOfOp);
            }

            m_ui.addBan->setEnabled(m_isAnyTypeOfOp);
            m_ui.updateBan->setEnabled(m_isAnyTypeOfOp);
            m_ui.removeBan->setEnabled(m_isAnyTypeOfOp);
            banSelectionChanged();

            m_ui.hostmask->setEnabled(m_isAnyTypeOfOp);
            hostmaskChanged(m_ui.hostmask->text());
        }
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

        modesModel->clear();
        modesModel->setHorizontalHeaderLabels(QStringList() << i18n("Mode") << i18n("Parameter"));

        for(int i = 0; i < modeString.length(); i++)
        {
            QList<QStandardItem *> newRow;
            QStandardItem *item = nullptr;

            if(!Preferences::self()->useLiteralModes() && getChannelModesHash().contains(modeString[i]))
                item = new QStandardItem(i18nc("<mode character> (<mode description>)","%1 (%2)", modeString[i], getChannelModesHash().value(modeString[i])));
            else
                item = new QStandardItem(QString(modeString[i]));

            item->setData(QString(modeString[i]));
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
        m_ui.keyModeEdit->setText(QString());

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
                    m_ui.userLimitEdit->setValue(currentMode.midRef(1).toInt());
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
                        if (item->data().toString() == modeString)
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

        refreshEnableModes(true);
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
            mode += modesModel->item(i, 0)->data().toString() + modesModel->item(i, 1)->text();
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
        BanListViewItem *item = new BanListViewItem(m_ui.banList, newban.section(' ', 0, 0), newban.section(' ', 1, 1).section('!', 0, 0), newban.section(' ', 2 ,2).toUInt());
        // set item as current item
        m_ui.banList->setCurrentItem(item);
        // update button states
        hostmaskChanged(m_ui.hostmask->text());
    }

    void ChannelOptionsDialog::removeBan(const QString& ban)
    {
        QList<QTreeWidgetItem *> items = m_ui.banList->findItems(ban, Qt::MatchCaseSensitive | Qt::MatchExactly, 0);
        if (items.count() > 0)
          delete items.at(0);
    }

    void ChannelOptionsDialog::addBanClicked()
    {
      QString newHostmask = m_ui.hostmask->text();
      if (!newHostmask.isEmpty())
        m_channel->getServer()->requestBan(QStringList(newHostmask), m_channel->getName(), QString());
    }

    void ChannelOptionsDialog::removeBanClicked()
    {
      QString oldHostmask = m_ui.banList->currentItem()->text(0);
      // We delete the existing item because it's possible the server may
      // Modify the ban causing us not to catch it. If that happens we'll be
      // stuck with a stale item and a new item with the modified hostmask.
      delete m_ui.banList->currentItem();
      // request unban
      m_channel->getServer()->requestUnban(oldHostmask, m_channel->getName());
    }

    void ChannelOptionsDialog::updateBanClicked()
    {
      QString oldHostmask = m_ui.banList->currentItem()->text(0);
      QString newHostmask = m_ui.hostmask->text();
      if (!newHostmask.isEmpty() && newHostmask.compare(oldHostmask))
      {
        // We delete the existing item because it's possible the server may
        // Modify the ban causing us not to catch it. If that happens we'll be
        // stuck with a stale item and a new item with the modified hostmask.
        delete m_ui.banList->currentItem();
        // request unban for the of the old hostmask
        m_channel->getServer()->requestUnban(oldHostmask, m_channel->getName());
        // request ban for the of the old hostmask
        m_channel->getServer()->requestBan(QStringList(newHostmask), m_channel->getName(), QString());
      }
    }
    /// Enables/disables updateBan and removeBan buttons depending on the currentItem of the banList
    void ChannelOptionsDialog::banSelectionChanged()
    {
      if (m_ui.banList->currentItem())
      {
        m_ui.updateBan->setEnabled(m_isAnyTypeOfOp);
        m_ui.removeBan->setEnabled(m_isAnyTypeOfOp);
        // update line edit content
        m_ui.hostmask->setText(m_ui.banList->currentItem()->text(0));
      }
      else
      {
        m_ui.updateBan->setEnabled(false);
        m_ui.removeBan->setEnabled(false);
      }
    }
    /// Enables/disables addBan and updateBan buttons depending on the value of @p text
    void ChannelOptionsDialog::hostmaskChanged(const QString& text)
    {
      if (text.trimmed().length() != 0)
      {
        if (m_isAnyTypeOfOp)
        {
          QList<QTreeWidgetItem*> items = m_ui.banList->findItems(text, Qt::MatchExactly | Qt::MatchCaseSensitive, 0);
          m_ui.addBan->setEnabled(items.isEmpty());
          m_ui.updateBan->setEnabled(items.isEmpty() && m_ui.banList->currentItem());
        }
      }
      else
      {
        m_ui.addBan->setEnabled(false);
        m_ui.updateBan->setEnabled(false);
      }
    }

    void ChannelOptionsDialog::startHistorySearchTimer(const QString &filter)
    {
        Q_UNUSED(filter)
        m_historySearchTimer->start(300);
    }

    void ChannelOptionsDialog::updateHistoryFilter()
    {
        QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(m_ui.topicHistoryView->model());

        if(!proxy)
            return;

        proxy->setFilterFixedString(m_ui.topicHistorySearchLine->text());
    }


    // This is our implementation of BanListViewItem

    BanListViewItem::BanListViewItem(QTreeWidget *parent)
      : QTreeWidgetItem()
    {
        parent->addTopLevelItem(this);
    }

    BanListViewItem::BanListViewItem (QTreeWidget *parent, const QString& label1, const QString& label2,
        uint timestamp) : QTreeWidgetItem()
    {
        setText(0, label1);
        setText(1, label2);
        m_timestamp.setTime_t(timestamp);
        setText(2, QLocale().toString(m_timestamp, QLocale::ShortFormat));
        setData(2, Qt::UserRole, m_timestamp);
        parent->addTopLevelItem(this);
    }

    bool BanListViewItem::operator<(const QTreeWidgetItem &item) const
    {
        if (treeWidget()->sortColumn() == 2)
        {
            QVariant userdata = item.data(2, Qt::UserRole);
            if (userdata.isValid() && userdata.type() == QVariant::DateTime)
            {
              return m_timestamp < userdata.toDateTime();
            }
        }

        return text(treeWidget()->sortColumn()) < item.text(treeWidget()->sortColumn());
    }
}

QString Konversation::ChannelOptionsDialog::whatsThisForMode(char mode)
{
    switch (mode) {
    case 'T':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>The <b>T</b>opic mode means that only the channel operator can change the topic for the channel.</p></qt>");
    case 'N':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p><b>N</b>o messages from outside means users who are not in the channel cannot send messages for everybody in the channel to see.  Almost all channels have this set to prevent nuisance messages.</p></qt>");
    case 'S':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>S</b>ecret channel will not show up in the channel list, nor will any user be able to see that you are in the channel with the <em>WHOIS</em> command or anything similar.  Only the people that are in the same channel will know that you are in this channel, if this mode is set.</p></qt>");
    case 'I':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>An <b>I</b>nvite only channel means that people can only join the channel if they are invited.  To invite someone, a channel operator needs to issue the command <em>/invite nick</em> from within the channel.</p></qt>");
    case 'P':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>P</b>rivate channel is shown in a listing of all channels, but the topic is not shown.  A user's <em>WHOIS</em> may or may not show them as being in a private channel depending on the IRC server.</p></qt>");
    case 'M':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A <b>M</b>oderated channel is one where only operators, half-operators and those with voice can talk.</p></qt>");
    case 'K':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A protected channel requires users to enter a password in order to join.</p></qt>");
    case 'L':
        return i18n("<qt><p>These control the <em>mode</em> of the channel.  Only an operator can change these.</p><p>A channel that has a user <b>L</b>imit means that only that many users can be in the channel at any one time.  Some channels have a bot that sits in the channel and changes this automatically depending on how busy the channel is.</p></qt>");
    default:
        qWarning() << "called for unknown mode" << mode;
        return QString();
    }
}


