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
#include "serverdialog.h"
#include "serversettings.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcheckbox.h>

#include <klocale.h>
#include <kmessagebox.h>


namespace Konversation
{

    ServerDialog::ServerDialog(const QString& title, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(title);
        setButtons(Ok|Cancel);

        QGridLayout* mainLayout = new QGridLayout(mainWidget());
        mainLayout->setSpacing(spacingHint());
        mainLayout->setMargin(0);

        QLabel* serverLbl = new QLabel(i18n("&Server:"), mainWidget());
        m_serverEdit = new QLineEdit(mainWidget());
        m_serverEdit->setWhatsThis(i18n("The name or IP number of the server. irchelp.org maintains a list of servers."));
        serverLbl->setBuddy(m_serverEdit);

        QLabel* portLbl = new QLabel(i18n("&Port:"), mainWidget());

        m_portSBox = new QSpinBox(mainWidget());
        m_portSBox->setMinimum(1);
        m_portSBox->setMaximum(65535);
        m_portSBox->setValue(6667);
        m_portSBox->setWhatsThis(i18n("Enter the port number required to connect to the server. For most servers, this should be <b>6667</b>."));
        portLbl->setBuddy(m_portSBox);

        QLabel* passwordLbl = new QLabel(i18n("Pass&word:"), mainWidget());
        m_passwordEdit = new QLineEdit(mainWidget());
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        passwordLbl->setBuddy(m_passwordEdit);

        m_sslChBox = new QCheckBox(i18n("S&ecure connection (SSL)"), mainWidget());
        m_sslChBox->setWhatsThis(i18n("Check if you want to use Secure Socket Layer (SSL) protocol to communicate with the server. This protects the privacy of your communications between your computer and the IRC server. The server must support SSL protocol for this to work. In most cases, if the server does not support SSL, the connection will fail."));

        mainLayout->addWidget(serverLbl, 0, 0);
        mainLayout->addWidget(m_serverEdit, 0, 1);
        mainLayout->addWidget(portLbl, 0, 2);
        mainLayout->addWidget(m_portSBox, 0, 3);
        mainLayout->addWidget(passwordLbl, 1, 0);
        mainLayout->addWidget(m_passwordEdit, 1, 1, 1, 3);
        mainLayout->addWidget(m_sslChBox, 2, 0, 1, 4);

        m_serverEdit->setFocus();

        connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
        connect( m_serverEdit, SIGNAL(textChanged(const QString &)),this,SLOT( slotServerNameChanged( const QString& ) ) );
        slotServerNameChanged( m_serverEdit->text() );
    }

    ServerDialog::~ServerDialog()
    {
    }

    void ServerDialog::slotServerNameChanged( const QString &text )
    {
        enableButtonOk( !text.isEmpty() );
    }

    void ServerDialog::setServerSettings(const ServerSettings& server)
    {
        m_serverEdit->setText(server.host());
        m_portSBox->setValue(server.port());
        m_passwordEdit->setText(server.password());
        m_sslChBox->setChecked(server.SSLEnabled());
    }

    ServerSettings ServerDialog::serverSettings()
    {
        ServerSettings server;
        server.setHost(m_serverEdit->text());
        server.setPort(m_portSBox->value());
        server.setPassword(m_passwordEdit->text());
        server.setSSLEnabled(m_sslChBox->isChecked());

        return server;
    }

    void ServerDialog::slotOk()
    {
        if (m_serverEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The server address is required."));
        }
        else
        {
            accept();
        }
    }
}

#include "serverdialog.moc"
