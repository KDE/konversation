// Copyright 2009  Peter Simonsson <peter.simonsson@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License or (at your option) version 3 or any later version
// accepted by the membership of KDE e.V. (or its successor approved
// by the membership of KDE e.V.), which shall act as a proxy
// defined in Section 14 of version 3 of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "invitedialog.h"

#include <KLocale>
#include <KIcon>
#include <KConfigGroup>

#include <QLabel>
#include <QTreeView>
#include <QStringList>

InviteDialog::InviteDialog(QWidget* parent)
    : KDialog(parent), Ui::InviteDialog()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setCaption(i18n("Channel Invites"));
    setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget* mainWidget = new QWidget(this);
    setMainWidget(mainWidget);
    setupUi(mainWidget);

    m_iconLabel->setPixmap(KIcon("irc-join-channel").pixmap(48));

    m_channelModel = new InviteChannelListModel(m_channelView);
    m_channelView->setModel(m_channelModel);
    m_channelView->setRootIsDecorated(false);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
    connect(this, SIGNAL(buttonClicked(KDialog::ButtonCode)),
            this, SLOT (saveShowAgainSetting(KDialog::ButtonCode)));
}

void InviteDialog::addInvite(const QString& nickname, const QString& channel)
{
    m_channelModel->addInvite(nickname, channel);
}

void InviteDialog::slotOk()
{
    QString channels = m_channelModel->selectedChannels();

    if(!channels.isEmpty())
        emit joinChannelsRequested(channels);
}

void InviteDialog::saveShowAgainSetting(KDialog::ButtonCode buttonCode)
{
    if (m_showAgainCheck->isChecked())
    {
        KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
        KConfigGroup cg(KGlobal::config().data(), "Notification Messages");
        cg.writeEntry("Invitation", buttonCode == KDialog::Ok, flags);
        cg.sync();
    }
}

bool InviteDialog::shouldBeShown(KDialog::ButtonCode& buttonCode)
{
    KConfigGroup cg(KGlobal::config().data(), "Notification Messages");
    cg.sync();
    const QString dontAsk = cg.readEntry("Invitation", QString()).toLower();

    if (dontAsk == "yes" || dontAsk == "true")
    {
        buttonCode = KDialog::Ok;
        return false;
    }
    else if (dontAsk == "no" || dontAsk == "false")
    {
        buttonCode = KDialog::Cancel;
        return false;
    }

    return true;
}


//
// InviteChannelListModel
//

InviteChannelListModel::InviteChannelListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void InviteChannelListModel::addInvite(const QString& nickname, const QString& channel)
{
    if(m_channelMap.contains(channel))
    {
        if(!m_channelMap[channel].nicknames.contains(nickname))
        {
            m_channelMap[channel].nicknames += ", " + nickname;
        }
    }
    else
    {
        ChannelItem item;
        item.channel = channel;
        item.nicknames = nickname;
        item.checkState = Qt::Unchecked;
        m_channelMap.insert(channel, item);
    }

    reset();
}

int InviteChannelListModel::rowCount(const QModelIndex&) const
{
    return m_channelMap.count();
}

int InviteChannelListModel::columnCount(const QModelIndex&) const
{
    return 3;
}

QVariant InviteChannelListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid())
        return QVariant();

    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
            case 1:
                return m_channelMap.values().at(index.row()).channel;
            case 2:
                return m_channelMap.values().at(index.row()).nicknames;
            default:
                return QVariant();
        }
    }
    else if(Qt::CheckStateRole && index.column() == 0)
    {
        return m_channelMap.values().at(index.row()).checkState;
    }

    return QVariant();
}

QVariant InviteChannelListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch(section)
    {
        case 0:
            return i18n("Join");
        case 1:
            return i18n("Channel");
        case 2:
            return i18n("Nickname");
        default:
            return QVariant();
    }
}

Qt::ItemFlags InviteChannelListModel::flags(const QModelIndex& index) const
{
    if(!index.isValid() || index.column() > 0)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled;
}

QString InviteChannelListModel::selectedChannels() const
{
    QStringList channels;

    foreach(const ChannelItem& item, m_channelMap)
    {
        if(item.checkState == Qt::Checked)
            channels.append(item.channel);
    }

    return channels.join(",");
}

bool InviteChannelListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid() || !value.isValid() || index.column() > 0 || role != Qt::CheckStateRole)
        return false;

    QString channel = m_channelMap.values().at(index.row()).channel;
    m_channelMap[channel].checkState = (value.toBool() ? Qt::Checked : Qt::Unchecked);
    emit dataChanged(index, index);

    return true;
}

#include "invitedialog.moc"
