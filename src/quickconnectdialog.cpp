/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Michael Goettsche <mail@tuxipuxi.de>
*/

#include "quickconnectdialog.h"
#include "application.h"

#include <QCheckBox>

#include <KLineEdit>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <KGuiItem>
#include <QVBoxLayout>


QuickConnectDialog::QuickConnectDialog(QWidget *parent)
:QDialog(parent)
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QuickConnectDialog::slotOk);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QuickConnectDialog::reject);
    mainLayout->addWidget(buttonBox);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    setWindowTitle(  i18n("Quick Connect") );
    setModal( true );
    QWidget* page = mainWidget;

    QGridLayout* layout = new QGridLayout(mainWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    //QT5 layout->setSpacing(spacingHint());

    QLabel* hostNameLabel = new QLabel(i18n("&Server host:"), page);
    QString hostNameWT = i18n("Enter the host of the network here.");
    hostNameLabel->setWhatsThis(hostNameWT);
    hostNameInput = new KLineEdit(page);
    hostNameInput->setWhatsThis(hostNameWT);
    hostNameLabel->setBuddy(hostNameInput);
    hostNameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* portLabel = new QLabel(i18n("&Port:"), page);
    QString portWT = i18n("The port that the IRC server is using.");
    portLabel->setWhatsThis(portWT);
    portInput = new KLineEdit(QStringLiteral("6667"), page );
    portInput->setWhatsThis(portWT);
    portLabel->setBuddy(portInput);
    portLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* nickLabel = new QLabel(i18n("&Nick:"), page);
    QString nickWT = i18n("The nick you want to use.");
    nickLabel->setWhatsThis(nickWT);
    nickInput = new KLineEdit(Preferences::identityById(0)->getNickname(0), page);
    nickInput->setWhatsThis(nickWT);
    nickLabel->setBuddy(nickInput);
    nickLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    QLabel* passwordLabel = new QLabel(i18n("P&assword:"), page);
    QString passwordWT = i18n("If the IRC server requires a password, enter it here (most servers do not require a password.)");
    passwordLabel->setWhatsThis(passwordWT);
    passwordInput = new KLineEdit(page);
    passwordInput->setPasswordMode(true);
    passwordInput->setWhatsThis(passwordWT);
    passwordLabel->setBuddy(passwordInput);
    passwordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    sslCheckBox = new QCheckBox(page);
    sslCheckBox->setObjectName(QStringLiteral("sslCheckBox"));
    sslCheckBox->setText(i18n("&Use SSL"));

    layout->addWidget(hostNameLabel, 0, 0);
    layout->addWidget(hostNameInput, 0, 1);
    layout->addWidget(portLabel, 0, 2);
    layout->addWidget(portInput, 0, 3);

    layout->addWidget(nickLabel, 1, 0);
    layout->addWidget(nickInput, 1, 1);
    layout->addWidget(passwordLabel, 1, 2);
    layout->addWidget(passwordInput, 1, 3);

    layout->addWidget(sslCheckBox, 2, 0);

    hostNameInput->setFocus();

    KGuiItem::assign(mOkButton, KGuiItem(i18n("C&onnect"),QStringLiteral("network-connect"),i18n("Connect to the server")));

    connect(hostNameInput, &KLineEdit::textChanged, this, &QuickConnectDialog::slotServerNameChanged);
    slotServerNameChanged( hostNameInput->text() );
}

QuickConnectDialog::~QuickConnectDialog()
{
}

void QuickConnectDialog::slotServerNameChanged( const QString &text )
{
    mOkButton->setEnabled( !text.isEmpty() );
}

void QuickConnectDialog::slotOk()
{
    if (!hostNameInput->text().isEmpty() &&
        !portInput->text().isEmpty() &&
        !nickInput->text().isEmpty())
    {

        emit connectClicked(Konversation::PromptToReuseConnection,
                            hostNameInput->text().trimmed(),
                            portInput->text(),
                            passwordInput->text(),
                            nickInput->text(),
                            QString(),
                            sslCheckBox->isChecked());
        delayedDestruct();
    }
}

void QuickConnectDialog::delayedDestruct()
{
    if (isVisible()) {
        hide();
    }

    deleteLater();
}

