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

#include <qlayout.h>
#include <qframe.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>

#include <klocale.h>

#include "serversettings.h"

namespace Konversation
{

    ServerDialog::ServerDialog(const QString& title, QWidget *parent, const char *name)
        : KDialogBase(Plain, title, Ok|Cancel, Ok, parent, name)
    {
        QFrame* mainWidget = plainPage();
        QGridLayout* mainLayout = new QGridLayout(mainWidget, 1, 4, 0, spacingHint());

        QLabel* serverLbl = new QLabel(i18n("&Server:"), mainWidget);
        m_serverEdit = new QLineEdit(mainWidget);
        QWhatsThis::add(m_serverEdit, i18n("The name or IP number of the server. irchelp.org maintains a list of servers."));
        serverLbl->setBuddy(m_serverEdit);

        QLabel* portLbl = new QLabel(i18n("&Port:"), mainWidget);
                                                  // FIXME Need to check the real min & max
        m_portSBox = new QSpinBox(1, 99999, 1, mainWidget);
        m_portSBox->setValue(6667);
        QWhatsThis::add(m_portSBox, i18n("Enter the port number required to connect to the server. For most servers, this should be <b>6667</b>."));
        portLbl->setBuddy(m_portSBox);

        QLabel* passwordLbl = new QLabel(i18n("Pass&word:"), mainWidget);
        m_passwordEdit = new QLineEdit(mainWidget);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        passwordLbl->setBuddy(m_passwordEdit);

        m_sslChBox = new QCheckBox(i18n("S&ecure connection (SSL)"), mainWidget);
        QWhatsThis::add(m_sslChBox, i18n("Check if you want to use Secure Socket Layer (SSL) protocol to communicate with the server. This protects the privacy of your communications between your computer and the IRC server. The server must support SSL protocol for this to work. In most cases, if the server does not support SSL, the connection will fail."));

        mainLayout->addWidget(serverLbl, 0, 0);
        mainLayout->addWidget(m_serverEdit, 0, 1);
        mainLayout->addWidget(portLbl, 0, 2);
        mainLayout->addWidget(m_portSBox, 0, 3);
        mainLayout->addWidget(passwordLbl, 1, 0);
        mainLayout->addMultiCellWidget(m_passwordEdit, 1, 1, 1, 3);
        mainLayout->addMultiCellWidget(m_sslChBox, 2, 2, 0, 3);

        m_serverEdit->setFocus();
    }

    ServerDialog::~ServerDialog()
    {
    }

    void ServerDialog::setServerSettings(const ServerSettings& server)
    {
        m_serverEdit->setText(server.server());
        m_portSBox->setValue(server.port());
        m_passwordEdit->setText(server.password());
        m_sslChBox->setChecked(server.SSLEnabled());
    }

    ServerSettings ServerDialog::serverSettings()
    {
        ServerSettings server;
        server.setServer(m_serverEdit->text());
        server.setPort(m_portSBox->value());
        server.setPassword(m_passwordEdit->text());
        server.setSSLEnabled(m_sslChBox->isChecked());

        return server;
    }

}

#include "serverdialog.moc"
