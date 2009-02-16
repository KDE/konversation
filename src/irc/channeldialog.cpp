/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/

#include "channeldialog.h"
#include "servergroupsettings.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>

#include <klocale.h>
#include <kmessagebox.h>


namespace Konversation
{

    ChannelDialog::ChannelDialog(const QString& title, QWidget *parent)
        : KDialog(parent)
    {
        setCaption(title);
        setButtons(Ok|Cancel);

        QGridLayout* mainLayout = new QGridLayout(mainWidget());
        mainLayout->setSpacing(spacingHint());
        mainLayout->setMargin(0);

        QLabel* channelLbl = new QLabel(i18n("C&hannel:"), mainWidget());
        m_channelEdit = new QLineEdit(mainWidget());
        m_channelEdit->setMaxLength(50);
        channelLbl->setBuddy(m_channelEdit);

        QLabel* passwordLbl = new QLabel(i18n("Pass&word:"), mainWidget());
        m_passwordEdit = new QLineEdit(mainWidget());
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        passwordLbl->setBuddy(m_passwordEdit);

        mainLayout->addWidget(channelLbl, 0, 0);
        mainLayout->addWidget(m_channelEdit, 0, 1);
        mainLayout->addWidget(passwordLbl, 1, 0);
        mainLayout->addWidget(m_passwordEdit, 1, 1);

        m_channelEdit->setFocus();
        connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
        connect( m_channelEdit, SIGNAL(textChanged(const QString &)),this,SLOT( slotServerNameChanged( const QString& ) ) );
        slotServerNameChanged( m_channelEdit->text() );
    }

    ChannelDialog::~ChannelDialog()
    {
    }

    void ChannelDialog::slotServerNameChanged( const QString &text )
    {
        enableButtonOk( !text.isEmpty() );
    }
    void ChannelDialog::setChannelSettings(const ChannelSettings& channel)
    {
        m_channelEdit->setText(channel.name());
        m_passwordEdit->setText(channel.password());
    }

    ChannelSettings ChannelDialog::channelSettings()
    {
        ChannelSettings channel;
        channel.setName(m_channelEdit->text());
        channel.setPassword(m_passwordEdit->text());

        return channel;
    }

    void ChannelDialog::slotOk()
    {
        if (m_channelEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("The channel name is required."));
        }
        else
        {
            accept();
        }
    }
}

#include "channeldialog.moc"
