/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2009 Peter Simonsson <peter.simonsson@gmail.com>
*/

#include "invitedialog.h"

#include <KLocalizedString>
#include <QIcon>
#include <KConfigGroup>

#include <QStringList>
#include <KSharedConfig>
#include <QPushButton>
#include <QVBoxLayout>

InviteDialog::InviteDialog(QWidget* parent)
    : QDialog(parent), Ui::InviteDialog()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(i18n("Channel Invites"));
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &InviteDialog::slotOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &InviteDialog::reject);

    auto* mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);
    setupUi(mainWidget);

    m_iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("irc-join-channel")).pixmap(48));

    m_channelModel = new InviteChannelListModel(m_channelView);
    m_channelView->setModel(m_channelModel);
    m_channelView->setRootIsDecorated(false);
    m_channelView->setUniformRowHeights(true);

}

void InviteDialog::addInvite(const QString& nickname, const QString& channel)
{
    m_channelModel->addInvite(nickname, channel);
    m_channelView->resizeColumnToContents(0);
}

void InviteDialog::slotOk()
{
    QString channels = m_channelModel->selectedChannels();

    if(!channels.isEmpty())
        Q_EMIT joinChannelsRequested(channels);
    KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
    KConfigGroup cg(KSharedConfig::openConfig().data(), QStringLiteral("Notification Messages"));
    cg.writeEntry("Invitation", m_joinPreferences->currentIndex(), flags);
    cg.sync();

    accept();
}

bool InviteDialog::shouldBeShown(QDialogButtonBox::StandardButton& buttonCode)
{
    KConfigGroup cg(KSharedConfig::openConfig().data(), QStringLiteral("Notification Messages"));
    cg.sync();
    const QString dontAsk = cg.readEntry("Invitation", QString()).toLower();

    if (dontAsk == QLatin1Char('1'))
    {
    buttonCode = QDialogButtonBox::Ok;
    return false;
    }
    else if (dontAsk == QLatin1Char('2'))
    {
    buttonCode = QDialogButtonBox::Cancel;
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
            m_channelMap[channel].nicknames += QLatin1String(", ") + nickname;
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

    beginResetModel();
    endResetModel();
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
    if(role == Qt::SizeHintRole)
    {
        QFontMetricsF fntMetrics(qApp->font());

        return QSize(0, qMax(qApp->style()->sizeFromContents(QStyle::CT_CheckBox, nullptr, QSize(0, 0), nullptr).height(),
                             static_cast<int>(fntMetrics.height())));
    }
    else if((role == Qt::CheckStateRole) && (index.column() == 0))
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

    for (const ChannelItem& item : m_channelMap) {
        if(item.checkState == Qt::Checked)
            channels.append(item.channel);
    }

    return channels.join(QLatin1Char(','));
}

bool InviteChannelListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(!index.isValid() || !value.isValid() || index.column() > 0 || role != Qt::CheckStateRole)
        return false;

    QString channel = m_channelMap.values().at(index.row()).channel;
    m_channelMap[channel].checkState = (value.toBool() ? Qt::Checked : Qt::Unchecked);
    Q_EMIT dataChanged(index, index);

    return true;
}

#include "moc_invitedialog.cpp"
