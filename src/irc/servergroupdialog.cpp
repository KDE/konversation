/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Peter Simonsson <psn@linux.se>
*/

#include "servergroupdialog.h"
#include "identity.h"
#include "application.h"
#include "viewcontainer.h"
#include "preferences.h"
#include "serversettings.h"
#include "identitydialog.h"
#include "ui_servergroupdialogui.h"
#include "ui_serverdialogui.h"
#include "ui_channeldialogui.h"

#include <KAuthorized>

#include <QPushButton>
#include <QCheckBox>

#include <KMessageBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>


namespace Konversation
{

    ServerGroupDialog::ServerGroupDialog(const QString& title, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(title);
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *mainWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ServerGroupDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerGroupDialog::reject);
        //PORTING SCRIPT: WARNING mainLayout->addWidget(buttonBox) must be last item in layout. Please move it.
        mainLayout->addWidget(buttonBox);

        m_id = -1;
        m_identitiesNeedsUpdate = false;
        m_editedServer = false;

        m_mainWidget = new Ui::ServerGroupDialogUI();
        m_mainWidget->setupUi(mainWidget);

        connect(m_mainWidget->m_editIdentityButton, &QPushButton::clicked, this, &ServerGroupDialog::editIdentity);

        const IdentityList identities = Preferences::identityList();

        for (const auto& identity : identities) {
            m_mainWidget->m_identityCBox->addItem(identity->getName());
        }

        m_mainWidget->m_removeServerButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
        m_mainWidget->m_upServerBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
        m_mainWidget->m_downServerBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));

        connect(m_mainWidget->m_addServerButton, &QPushButton::clicked, this, &ServerGroupDialog::addServer);
        connect(m_mainWidget->m_changeServerButton, &QPushButton::clicked, this, QOverload<>::of(&ServerGroupDialog::editServer));
        connect(m_mainWidget->m_removeServerButton, &QToolButton::clicked, this, &ServerGroupDialog::deleteServer);
        connect(m_mainWidget->m_serverLBox, &QListWidget::itemSelectionChanged, this, &ServerGroupDialog::updateServerArrows);
        connect(m_mainWidget->m_upServerBtn, &QToolButton::clicked, this, &ServerGroupDialog::moveServerUp);
        connect(m_mainWidget->m_downServerBtn, &QToolButton::clicked, this, &ServerGroupDialog::moveServerDown);

        m_mainWidget->m_removeChannelButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
        m_mainWidget->m_upChannelBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-up")));
        m_mainWidget->m_downChannelBtn->setIcon(QIcon::fromTheme(QStringLiteral("arrow-down")));

        connect(m_mainWidget->m_addChannelButton, &QPushButton::clicked, this, &ServerGroupDialog::addChannel);
        connect(m_mainWidget->m_changeChannelButton, &QPushButton::clicked, this, &ServerGroupDialog::editChannel);
        connect(m_mainWidget->m_removeChannelButton, &QToolButton::clicked, this, &ServerGroupDialog::deleteChannel);
        connect(m_mainWidget->m_channelLBox, &QListWidget::itemSelectionChanged, this, &ServerGroupDialog::updateChannelArrows);
        connect(m_mainWidget->m_upChannelBtn, &QToolButton::clicked, this, &ServerGroupDialog::moveChannelUp);
        connect(m_mainWidget->m_downChannelBtn, &QToolButton::clicked, this, &ServerGroupDialog::moveChannelDown);

        okButton->setToolTip(i18n("Change network information"));
        buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(i18n("Discards all changes made"));

        m_mainWidget->m_nameEdit->setFocus();

        resize(QSize(320, 400));
    }

    ServerGroupDialog::~ServerGroupDialog()
    {
        delete m_mainWidget;
    }

    void ServerGroupDialog::setServerGroupSettings(ServerGroupSettingsPtr settings)
    {
        m_id = settings->id();
        m_sortIndex = settings->sortIndex();
        m_expanded = settings->expanded();
        m_enableNotifications = settings->enableNotifications();
        m_mainWidget->m_nameEdit->setText(settings->name());

        const int i = m_mainWidget->m_identityCBox->findText(settings->identity()->getName());
        if (i != -1)
            m_mainWidget->m_identityCBox->setCurrentIndex(i);
        else
            m_mainWidget->m_identityCBox->setItemText(m_mainWidget->m_identityCBox->currentIndex(), settings->identity()->getName());

        m_mainWidget->m_commandEdit->setText(settings->connectCommands());
        m_mainWidget->m_autoConnectCBox->setChecked(settings->autoConnectEnabled());
        m_serverList = settings->serverList();
        m_channelHistory = settings->channelHistory();
        m_mainWidget->m_serverLBox->clear();

        for (const auto& server : std::as_const(m_serverList)) {
            m_mainWidget->m_serverLBox->addItem(server.host());
        }

        m_channelList = settings->channelList();

        for (const auto& channel : std::as_const(m_channelList)) {
            m_mainWidget->m_channelLBox->addItem(channel.name());
        }
    }

    ServerGroupSettingsPtr ServerGroupDialog::serverGroupSettings()
    {
        ServerGroupSettingsPtr settings(new ServerGroupSettings(m_id));
        settings->setSortIndex(m_sortIndex);
        settings->setName(m_mainWidget->m_nameEdit->text());
        IdentityList identities = Preferences::identityList();
        settings->setIdentityId(identities[m_mainWidget->m_identityCBox->currentIndex()]->id());
        settings->setConnectCommands(m_mainWidget->m_commandEdit->text());
        settings->setAutoConnectEnabled(m_mainWidget->m_autoConnectCBox->isChecked());
        settings->setServerList(m_serverList);
        settings->setChannelList(m_channelList);
        settings->setChannelHistory(m_channelHistory);
        settings->setNotificationsEnabled(m_enableNotifications);
        settings->setExpanded(m_expanded);

        return settings;
    }

    ServerSettings ServerGroupDialog::editedServer() const
    {
        if (m_editedServer && m_editedServerIndex < m_serverList.count())
        {
            return m_serverList[m_editedServerIndex];
        }

        return ServerSettings(QString());
    }

    int ServerGroupDialog::execAndEditServer(const ServerSettings &server)
    {
        show();
        editServer(server);
        return exec();
    }

    void ServerGroupDialog::addServer()
    {
        QPointer<ServerDialog> dlg = new ServerDialog(i18n("Add Server"), this);

        if(dlg->exec() == QDialog::Accepted)
        {
            ServerSettings server = dlg->serverSettings();
            m_mainWidget->m_serverLBox->addItem(server.host());
            m_mainWidget->m_serverLBox->setCurrentItem(m_mainWidget->m_serverLBox->item(m_mainWidget->m_serverLBox->count() - 1));
            m_serverList.append(server);
        }
        delete dlg;
    }

    void ServerGroupDialog::editServer()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if(current < m_serverList.count())
        {
            QPointer <ServerDialog> dlg = new ServerDialog(i18n("Edit Server"), this);
            dlg->setServerSettings(m_serverList[current]);

            if(dlg->exec() == QDialog::Accepted)
            {
                ServerSettings server = dlg->serverSettings();
                m_mainWidget->m_serverLBox->item(current)->setText(server.host());
                m_serverList[current] = server;
            }
            delete dlg;
        }
    }

    void ServerGroupDialog::editServer(const ServerSettings &server)
    {
        // Track the server the Server List dialog told us to edit
        // and find out which server to select in the listbox
        m_editedServer = true;
        m_editedServerIndex = m_serverList.indexOf(server);
        m_mainWidget->m_serverLBox->setCurrentRow(m_editedServerIndex);

        editServer();
    }

    void ServerGroupDialog::deleteServer()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current < m_serverList.count())
        {
            m_serverList.removeAt(current);
            delete m_mainWidget->m_serverLBox->takeItem(current);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServer = false;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::updateServerArrows()
    {
        QList<QListWidgetItem*> serverBoxSelection = m_mainWidget->m_serverLBox->selectedItems();

        const bool hasServerSelected = !serverBoxSelection.isEmpty();
        if (m_mainWidget->m_serverLBox->count() && hasServerSelected) {
            QListWidgetItem* selectedServer = serverBoxSelection.first();
            int selectedServerRow = m_mainWidget->m_serverLBox->row(selectedServer);

            m_mainWidget->m_upServerBtn->setEnabled(selectedServerRow > 0);
            m_mainWidget->m_downServerBtn->setEnabled(selectedServerRow < m_mainWidget->m_serverLBox->count() - 1);
        }
        else
        {
            m_mainWidget->m_upServerBtn->setEnabled(false);
            m_mainWidget->m_downServerBtn->setEnabled(false);
        }

        m_mainWidget->m_removeServerButton->setEnabled(hasServerSelected);
        m_mainWidget->m_changeServerButton->setEnabled(hasServerSelected);
    }

    void ServerGroupDialog::moveServerUp()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current > 0)
        {
            ServerSettings server = m_serverList[current];
            delete m_mainWidget->m_serverLBox->takeItem(current);
            m_mainWidget->m_serverLBox->insertItem(current - 1, server.host());
            m_mainWidget->m_serverLBox->setCurrentRow(current - 1);
            m_serverList.move(current, current - 1);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current - 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::moveServerDown()
    {
        int current = m_mainWidget->m_serverLBox->currentRow();

        if (current < (m_serverList.count() - 1))
        {
            ServerSettings server = m_serverList[current];
            delete m_mainWidget->m_serverLBox->takeItem(current);
            m_mainWidget->m_serverLBox->insertItem(current + 1, server.host());
            m_mainWidget->m_serverLBox->setCurrentRow(current + 1);
            m_serverList.move(current, current + 1);

            // Track the server the Server List dialog told us to edit
            if (m_editedServer && m_editedServerIndex==current)
                m_editedServerIndex = current + 1;
        }

        updateServerArrows();
    }

    void ServerGroupDialog::addChannel()
    {
        QPointer<ChannelDialog> dlg = new ChannelDialog(i18n("Add Channel"), this);

        if(dlg->exec() == QDialog::Accepted)
        {
            ChannelSettings channel = dlg->channelSettings();
            m_mainWidget->m_channelLBox->addItem(channel.name());
            m_mainWidget->m_channelLBox->setCurrentItem(m_mainWidget->m_channelLBox->item(m_mainWidget->m_channelLBox->count() - 1));
            m_channelList.append(channel);
        }
        delete dlg;
    }

    void ServerGroupDialog::editChannel()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < m_channelList.count())
        {
            QPointer<ChannelDialog> dlg = new ChannelDialog(i18n("Edit Channel"), this);
            dlg->setChannelSettings(m_channelList[current]);

            if(dlg->exec() == QDialog::Accepted)
            {
                ChannelSettings channel = dlg->channelSettings();
                m_mainWidget->m_channelLBox->item(current)->setText(channel.name());
                m_channelList[current] = channel;
            }
            delete dlg;
        }
    }

    void ServerGroupDialog::deleteChannel()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < m_channelList.count())
        {
            m_channelList.removeOne(m_channelList.at(current));
            delete m_mainWidget->m_channelLBox->takeItem(current);
            updateChannelArrows();
        }
    }

    void ServerGroupDialog::updateChannelArrows()
    {
        QList<QListWidgetItem*> channelBoxSelection = m_mainWidget->m_channelLBox->selectedItems();

        const bool hasChannelSelected = !channelBoxSelection.isEmpty();
        if (m_mainWidget->m_channelLBox->count() && hasChannelSelected) {
            QListWidgetItem* selectedChannel = channelBoxSelection.first();
            int selectedChannelRow = m_mainWidget->m_channelLBox->row(selectedChannel);

            m_mainWidget->m_upChannelBtn->setEnabled(selectedChannelRow > 0);
            m_mainWidget->m_downChannelBtn->setEnabled(selectedChannelRow < m_mainWidget->m_channelLBox->count() - 1);
        }
        else
        {
            m_mainWidget->m_upChannelBtn->setEnabled(false);
            m_mainWidget->m_downChannelBtn->setEnabled(false);
        }

        m_mainWidget->m_removeChannelButton->setEnabled(hasChannelSelected);
        m_mainWidget->m_changeChannelButton->setEnabled(hasChannelSelected);
    }

    void ServerGroupDialog::moveChannelUp()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current > 0)
        {
            ChannelSettings channel = m_channelList[current];
            delete m_mainWidget->m_channelLBox->takeItem(current);
            m_mainWidget->m_channelLBox->insertItem(current - 1, channel.name());
            m_mainWidget->m_channelLBox->setCurrentRow(current - 1);
            m_channelList.move(current, current - 1);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::moveChannelDown()
    {
        int current = m_mainWidget->m_channelLBox->currentRow();

        if(current < (m_channelList.count() - 1))
        {
            ChannelSettings channel = m_channelList[current];
            delete m_mainWidget->m_channelLBox->takeItem(current);
            m_mainWidget->m_channelLBox->insertItem(current + 1, channel.name());
            m_mainWidget->m_channelLBox->setCurrentRow(current + 1);
            m_channelList.move(current, current + 1);
        }

        updateChannelArrows();
    }

    void ServerGroupDialog::editIdentity()
    {
        QPointer<IdentityDialog> dlg = new IdentityDialog(this);
        dlg->setCurrentIdentity(m_mainWidget->m_identityCBox->currentIndex());

        if(dlg->exec() == QDialog::Accepted)
        {
            const IdentityList identities = Preferences::identityList();
            m_mainWidget->m_identityCBox->clear();

            for (const auto& identity : identities) {
                m_mainWidget->m_identityCBox->addItem(identity->getName());
            }

            const int i = m_mainWidget->m_identityCBox->findText(dlg->currentIdentity()->getName());
            if (i != -1)
            {
                m_mainWidget->m_identityCBox->setCurrentIndex(i);
            }
            else
            {
                m_mainWidget->m_identityCBox->setItemText(m_mainWidget->m_identityCBox->currentIndex(), dlg->currentIdentity()->getName());
            }

            m_identitiesNeedsUpdate = true; // and what's this for?
            ViewContainer* vc = Application::instance()->getMainWindow()->getViewContainer();
            vc->updateViewEncoding(vc->getFrontView());
        }
        delete dlg;
    }

    void ServerGroupDialog::accept()
    {
        if (m_mainWidget->m_nameEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The network name is required."));
        }
        else if (m_serverList.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to add at least one server to the network."));
        }
        else
        {
            QDialog::accept();
        }
    }

    ServerDialog::ServerDialog(const QString& title, QWidget *parent)
    : QDialog(parent)
    {
        setWindowTitle(title);
        auto *mainWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);

        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        m_okButton = buttonBox->button(QDialogButtonBox::Ok);
        m_okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ServerDialog::reject);
        mainLayout->addWidget(buttonBox);

        m_mainWidget = new Ui::ServerDialogUI();
        m_mainWidget->setupUi(mainWidget);
        m_mainWidget->m_passwordEdit->setRevealPasswordAvailable(KAuthorized::authorize(QStringLiteral("lineedit_reveal_password")));

        m_mainWidget->m_serverEdit->setFocus();

        connect(m_okButton, &QPushButton::clicked, this, &ServerDialog::slotOk);
        connect(m_mainWidget->m_serverEdit, &KLineEdit::textChanged, this, &ServerDialog::slotServerNameChanged);
        slotServerNameChanged( m_mainWidget->m_serverEdit->text() );
    }

    ServerDialog::~ServerDialog()
    {
        delete m_mainWidget;
    }

    void ServerDialog::slotServerNameChanged( const QString &text )
    {
        m_okButton->setEnabled( !text.isEmpty() );
    }

    void ServerDialog::setServerSettings(const ServerSettings& server)
    {
        m_mainWidget->m_serverEdit->setText(server.host());
        m_mainWidget->m_portSBox->setValue(server.port());
        m_mainWidget->m_passwordEdit->setPassword(server.password());
        m_mainWidget->m_sslChBox->setChecked(server.SSLEnabled());
        m_mainWidget->m_proxyChBox->setChecked(!server.bypassProxy());
    }

    ServerSettings ServerDialog::serverSettings() const
    {
        ServerSettings server;
        server.setHost(m_mainWidget->m_serverEdit->text());
        server.setPort(m_mainWidget->m_portSBox->value());
        server.setPassword(m_mainWidget->m_passwordEdit->password());
        server.setSSLEnabled(m_mainWidget->m_sslChBox->isChecked());
        server.setBypassProxy(!m_mainWidget->m_proxyChBox->isChecked());

        return server;
    }

    void ServerDialog::slotOk()
    {
        if (m_mainWidget->m_serverEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The server address is required."));
        }
        else
        {
            accept();
        }
    }

    ChannelDialog::ChannelDialog(const QString& title, QWidget *parent)
    : QDialog(parent)
    {
        setWindowTitle(title);
        auto *mainWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        mainLayout->addWidget(mainWidget);
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        m_okButton = buttonBox->button(QDialogButtonBox::Ok);
        m_okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &ChannelDialog::reject);
        mainLayout->addWidget(buttonBox);

        m_mainWidget = new Ui::ChannelDialogUI();
        m_mainWidget->setupUi(mainWidget);
        m_mainWidget->m_passwordEdit->setRevealPasswordAvailable(KAuthorized::authorize(QStringLiteral("lineedit_reveal_password")));

        m_mainWidget->m_channelEdit->setFocus();
        connect(m_okButton, &QPushButton::clicked, this, &ChannelDialog::slotOk);
        connect(m_mainWidget->m_channelEdit, &KLineEdit::textChanged, this, &ChannelDialog::slotServerNameChanged);
        slotServerNameChanged( m_mainWidget->m_channelEdit->text() );
    }

    ChannelDialog::~ChannelDialog()
    {
        delete m_mainWidget;
    }

    void ChannelDialog::slotServerNameChanged( const QString &text )
    {
        m_okButton->setEnabled( !text.isEmpty() );
    }

    void ChannelDialog::setChannelSettings(const ChannelSettings& channel)
    {
        m_mainWidget->m_channelEdit->setText(channel.name());
        m_mainWidget->m_passwordEdit->setPassword(channel.password());
    }

    ChannelSettings ChannelDialog::channelSettings() const
    {
        ChannelSettings channel;
        channel.setName(m_mainWidget->m_channelEdit->text());
        channel.setPassword(m_mainWidget->m_passwordEdit->password());

        return channel;
    }

    void ChannelDialog::slotOk()
    {
        if (m_mainWidget->m_channelEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The channel name is required."));
        }
        else
        {
            accept();
        }
    }

}

#include "moc_servergroupdialog.cpp"
