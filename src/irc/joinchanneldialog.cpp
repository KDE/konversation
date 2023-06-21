/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#include "joinchanneldialog.h"
#include "application.h"
#include "connectionmanager.h"
#include "server.h"
#include "channel.h"
#include "servergroupsettings.h"

#include <KAuthorized>

#include <QPushButton>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <KMessageBox>
#include <KStandardGuiItem>

#include <algorithm>

namespace Konversation
{

    JoinChannelDialog::JoinChannelDialog(Server* server, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(i18n("Join Channel"));
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *mainWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        mOkButton = buttonBox->button(QDialogButtonBox::Ok);
        mOkButton->setDefault(true);
        mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &JoinChannelDialog::reject);
        mOkButton->setDefault(true);
        setModal( true );
        m_ui.setupUi(mainWidget);
        mainLayout->addWidget(mainWidget);
        mainLayout->addWidget(buttonBox);
        m_ui.channelCombo->setFocus();

        mOkButton->setEnabled(false);

        m_ui.passwordEdit->setRevealPasswordAvailable(KAuthorized::authorize(QStringLiteral("lineedit_reveal_password")));

        connect(m_ui.channelCombo, &KHistoryComboBox::editTextChanged, this, &JoinChannelDialog::slotChannelChanged);

        m_ui.delBtn->setEnabled(false);
        connect(m_ui.delBtn, &QPushButton::clicked, this, &JoinChannelDialog::deleteChannel);

        // Add network names to network combobox and select the one corresponding to argument.
        const QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
        for (Server *server : serverList) {
          m_ui.networkNameCombo->addItem(i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()),
                                         server->connectionId());
          connect(server, &Server::nicknameChanged, this, &JoinChannelDialog::slotNicknameChanged);
        }
        // Update channel history when selected connection changes
        connect(m_ui.networkNameCombo, QOverload<int>::of(&KComboBox::currentIndexChanged),
                this, &JoinChannelDialog::slotSelectedConnectionChanged);
        // Clear channel history when the history combo box is cleared
        connect(m_ui.channelCombo, &KHistoryComboBox::cleared, this, &JoinChannelDialog::slotChannelHistoryCleared);
        // Preselect the current network
        m_ui.networkNameCombo->setCurrentIndex(m_ui.networkNameCombo->findData(server->connectionId()));
        // If the server is the first item, current index wont be changed
        // So channel history combo wont be populated, so force it
        slotSelectedConnectionChanged(m_ui.networkNameCombo->findData(server->connectionId()));

        connect(mOkButton, &QPushButton::clicked, this, &JoinChannelDialog::slotOk);
        connect(Application::instance()->getConnectionManager(), &ConnectionManager::connectionListChanged,
                this, &JoinChannelDialog::slotConnectionListChanged);
    }

    JoinChannelDialog::~JoinChannelDialog()
    {
    }

    int JoinChannelDialog::connectionId() const
    {
      return m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
    }

    QString JoinChannelDialog::channel() const
    {
        QString channel = m_ui.channelCombo->currentText();

        if (!channel.isEmpty())
        {
            int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

            if (server && !server->isAChannel(channel))
                channel = QLatin1Char('#') + channel;
        }

        return channel;
    }

    QString JoinChannelDialog::password() const
    {
        return m_ui.passwordEdit->password();
    }

    void JoinChannelDialog::slotOk()
    {
        int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

        // If the channel already exist in the history only the password will be updated.
        if (server && server->getServerGroup())
            server->getServerGroup()->appendChannelHistory(ChannelSettings(channel(), password()));

        accept();
    }

    void JoinChannelDialog::slotNicknameChanged(const QString& nickname)
    {
      Q_UNUSED(nickname)

      // Update all items
      const QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
      for (Server *server : serverList) {
        int index = m_ui.networkNameCombo->findData(server->connectionId());
        m_ui.networkNameCombo->setItemText(index, i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()));
      }
    }

    void JoinChannelDialog::slotConnectionListChanged()
    {
      // Remove not-existing-anymore networks from the combobox
      for (int i = 0; i < m_ui.networkNameCombo->count(); i++)
      {
        int connectionId = m_ui.networkNameCombo->itemData(i).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);
        if (!server)
        {
          m_ui.networkNameCombo->removeItem(i);
          i--;
        }
      }
      // Add new network names to the combobox
      const QList<Server *> serverList = Application::instance()->getConnectionManager()->getServerList();
      for (Server *server : serverList) {
        if (m_ui.networkNameCombo->findData(server->connectionId()) == -1)
        {
          m_ui.networkNameCombo->addItem(i18nc("network (nickname)", "%1 (%2)", server->getDisplayName(), server->getNickname()),
                                         server->connectionId());
          connect(server, &Server::nicknameChanged, this, &JoinChannelDialog::slotNicknameChanged);
        }
      }
    }

    void JoinChannelDialog::slotSelectedConnectionChanged(int index)
    {
      m_ui.channelCombo->clear();
      int connectionId = m_ui.networkNameCombo->itemData(index).toInt();
      Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);
      if (server && server->getServerGroup())
      {
        const ChannelList history = server->getServerGroup()->channelHistory();
        const QList<Channel *> &channels = server->getChannelList();
        // Append an empty string as first item
        QStringList channelHistory;
        channelHistory << QString();
        for (const auto& channel : history) {
          const QString channelName = channel.name();
          // Don't add empty items to the combobox
          if (channelName.isEmpty())
            continue;

          const bool joined = std::any_of(channels.begin(), channels.end(), [&](Channel* chan) {
            return (chan->getName() == channelName);
          });

          if(!joined)
            channelHistory << channel.name();
        }
        // Sort channel history for easier access
        channelHistory.sort();
        // Set history items
        m_ui.channelCombo->setHistoryItems(channelHistory);
      }
    }

    void JoinChannelDialog::slotChannelChanged(const QString& text)
    {
        QStringList history = m_ui.channelCombo->historyItems();
        if (history.contains(text) && !text.isEmpty())
        {
            m_ui.delBtn->setEnabled(true);
        }
        else
        {
            m_ui.delBtn->setEnabled(false);
        }
        mOkButton->setEnabled(!text.isEmpty());
    }

    void JoinChannelDialog::slotChannelHistoryCleared()
    {
        int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
        Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

        if (server && server->getServerGroup())
          server->getServerGroup()->clearChannelHistory();
    }

    void JoinChannelDialog::deleteChannel()
    {
        QString channel = m_ui.channelCombo->currentText();
        QString warningTxt = i18n("Are you sure you want to remove this channel from your history?");

        if(KMessageBox::warningContinueCancel(this, warningTxt, i18n("Remove channel"), KStandardGuiItem::del()) == KMessageBox::Continue)
        {
            int connectionId = m_ui.networkNameCombo->itemData(m_ui.networkNameCombo->currentIndex()).toInt();
            Server *server = Application::instance()->getConnectionManager()->getServerByConnectionId(connectionId);

            if (server && server->getServerGroup())
            {
                Konversation::ChannelSettings channelSettings = server->getServerGroup()->channelByNameFromHistory(channel);
                server->getServerGroup()->removeChannelFromHistory(channelSettings);
            }

            m_ui.channelCombo->removeFromHistory(channel);
            m_ui.channelCombo->clearEditText();

        }
    }

}

#include "moc_joinchanneldialog.cpp"
