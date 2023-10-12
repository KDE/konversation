/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#include "identitydialog.h"
#include "application.h"
#include "awaymanager.h"
#include "irccharsets.h"

#include <QInputDialog>

#include <KAuthorized>
#include <KEditListWidget>
#include <KMessageBox>
#include <KMessageWidget>
#include <KUser>
#include <KStandardGuiItem>
#include <kio_version.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QRegularExpressionValidator>
#include <QListView>

#include <algorithm>

namespace Konversation
{

    IdentityDialog::IdentityDialog(QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle( i18n("Identities") );
        auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        auto *mainLayout = new QVBoxLayout;
        setLayout(mainLayout);
        QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
        okButton->setDefault(true);
        okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &IdentityDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &IdentityDialog::reject);
        okButton->setDefault(true);

        // Initialize the dialog widget
        auto* w = new QWidget(this);
        setupUi(w);
        mainLayout->addWidget(w);
        mainLayout->addWidget(buttonBox);

        m_nicknameLBox->setWhatsThis(i18n("This is your list of nicknames. A nickname is the name that other users will "
                                          "know you by. You may use any name you desire. The first character must be a letter.\n\n"
                                          "Since nicknames must be unique across an entire IRC network, your desired name may be "
                                          "rejected by the server because someone else is already using that nickname. Enter "
                                          "alternate nicknames for yourself. If your first choice is rejected by the server, "
                                          "Konversation will try the alternate nicknames."));

        newBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        connect(newBtn, &QPushButton::clicked, this, &IdentityDialog::newIdentity);

        copyBtn->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
        connect(copyBtn, &QPushButton::clicked, this, &IdentityDialog::copyIdentity);

        m_editBtn->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
        connect(m_editBtn, &QPushButton::clicked, this, &IdentityDialog::renameIdentity);

        m_delBtn->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
        connect(m_delBtn, &QPushButton::clicked, this, &IdentityDialog::deleteIdentity);

        const auto ids = Preferences::identityList();
        m_identityList.reserve(ids.size());
        for (const IdentityPtr &id : ids) {
            m_identityCBox->addItem(id->getName());
            m_identityList.append( IdentityPtr( id ) );
        }

        m_additionalAuthInfo = new KMessageWidget(generalWidget);
        m_additionalAuthInfo->setWordWrap(true);
        m_additionalAuthInfo->setCloseButtonVisible(false);
        m_additionalAuthInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        connect(m_authTypeCombo, QOverload<int>::of(&KComboBox::currentIndexChanged),
                this, &IdentityDialog::authTypeChanged);
        m_authTypeCombo->addItem(i18n("SASL PLAIN"), QStringLiteral("saslplain"));
        m_authTypeCombo->addItem(i18nc("Cert = Certificate", "SASL EXTERNAL (Cert)"), QStringLiteral("saslexternal"));
        m_authTypeCombo->addItem(i18n("Standard NickServ"), QStringLiteral("nickserv"));
        m_authTypeCombo->addItem(i18n("Server Password"), QStringLiteral("serverpw"));
        m_authTypeCombo->addItem(i18n("SSL Client Certificate"), QStringLiteral("pemclientcert"));

        // add encodings to combo box
        m_codecCBox->addItems(Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames());

        // set the suffix for the inactivity time spinbox
        m_awayInactivitySpin->setSuffix(ki18np(" minute", " minutes"));

        // Don't allow spaces in username or nicks
        const QRegularExpression noSpaceRx(QStringLiteral("\\S+"));
        auto *validator = new QRegularExpressionValidator(noSpaceRx, this);
        m_loginEdit->setValidator(validator);
        m_nicknameLBox->lineEdit()->setValidator(validator);

        m_authPasswordEdit->setRevealPasswordAvailable(KAuthorized::authorize(QStringLiteral("lineedit_reveal_password")));

        m_pemClientCertFile->setNameFilter(QStringLiteral("*.pem"));

        // set values for the widgets
        updateIdentity(0);

        // Set up signals / slots for identity page
        //connect(m_identityCBox, SIGNAL(activated(int)), this, SLOT(updateIdentity(int)));

        okButton->setToolTip(i18n("Change identity information"));
        buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(i18n("Discards all changes made"));

        AwayManager* awayManager = Application::instance()->getAwayManager();
        connect(m_identityCBox, QOverload<int>::of(&KComboBox::currentIndexChanged),
                this, &IdentityDialog::updateIdentity);
        connect(this, &IdentityDialog::identitiesChanged, awayManager, &AwayManager::identitiesChanged);

        // Workaround to make sure the taborder is correct for the nickname editor
        QWidget::setTabOrder(m_identityCBox, newBtn);
        QWidget::setTabOrder(newBtn, copyBtn);
        QWidget::setTabOrder(copyBtn, m_editBtn);
        QWidget::setTabOrder(m_editBtn, m_delBtn);
        QWidget::setTabOrder(m_delBtn, tabWidget);
        QWidget::setTabOrder(tabWidget, m_realNameEdit);
        QWidget::setTabOrder(m_realNameEdit, m_nicknameLBox->lineEdit());
        QWidget::setTabOrder(m_nicknameLBox->lineEdit(), m_nicknameLBox->listView());
        QWidget::setTabOrder(m_nicknameLBox->listView(), m_authTypeCombo);
        QWidget::setTabOrder(m_authTypeCombo, m_nickservNicknameEdit);
        QWidget::setTabOrder(m_nickservNicknameEdit, m_nickservCommandEdit);
        QWidget::setTabOrder(m_nickservCommandEdit, m_saslAccountEdit);
        QWidget::setTabOrder(m_saslAccountEdit, m_authPasswordEdit);
        QWidget::setTabOrder(m_authPasswordEdit, m_pemClientCertFile);
        QWidget::setTabOrder(m_pemClientCertFile, m_insertRememberLineOnAwayChBox);
        QWidget::setTabOrder(m_insertRememberLineOnAwayChBox, m_awayMessageEdit);
        QWidget::setTabOrder(m_awayMessageEdit, m_awayNickEdit);
        QWidget::setTabOrder(m_awayNickEdit, automaticAwayGroup);
        QWidget::setTabOrder(automaticAwayGroup, m_awayInactivitySpin);
        QWidget::setTabOrder(m_awayInactivitySpin, m_automaticUnawayChBox);
        QWidget::setTabOrder(m_automaticUnawayChBox, awayCommandsGroup);
        QWidget::setTabOrder(awayCommandsGroup, m_awayEdit);
        QWidget::setTabOrder(m_awayEdit, m_unAwayEdit);
        QWidget::setTabOrder(m_unAwayEdit, m_sCommandEdit);
        QWidget::setTabOrder(m_sCommandEdit, m_codecCBox);
        QWidget::setTabOrder(m_codecCBox, m_loginEdit);
        QWidget::setTabOrder(m_loginEdit, m_quitEdit);
        QWidget::setTabOrder(m_quitEdit, m_partEdit);
        QWidget::setTabOrder(m_partEdit, m_kickEdit);
    }

    void IdentityDialog::updateIdentity(int index)
    {
        if (m_currentIdentity && !checkCurrentIdentity())
        {
            return;
        }

        refreshCurrentIdentity();

        m_currentIdentity = m_identityList[index];

        m_realNameEdit->setText(m_currentIdentity->getRealName());
        m_nicknameLBox->clear();
        m_nicknameLBox->insertStringList(m_currentIdentity->getNicknameList());

        m_authTypeCombo->setCurrentIndex(m_authTypeCombo->findData(m_currentIdentity->getAuthType()));
        m_authPasswordEdit->setPassword(m_currentIdentity->getAuthPassword());
        m_nickservNicknameEdit->setText(m_currentIdentity->getNickservNickname());
        m_nickservCommandEdit->setText(m_currentIdentity->getNickservCommand());
        m_saslAccountEdit->setText(m_currentIdentity->getSaslAccount());
        m_pemClientCertFile->setUrl(m_currentIdentity->getPemClientCertFile());

        m_insertRememberLineOnAwayChBox->setChecked(m_currentIdentity->getInsertRememberLineOnAway());
        m_awayMessageEdit->setText(m_currentIdentity->getAwayMessage());
        m_awayNickEdit->setText(m_currentIdentity->getAwayNickname());
        awayCommandsGroup->setChecked(m_currentIdentity->getRunAwayCommands());
        m_awayEdit->setText(m_currentIdentity->getAwayCommand());
        m_unAwayEdit->setText(m_currentIdentity->getReturnCommand());
        automaticAwayGroup->setChecked(m_currentIdentity->getAutomaticAway());
        m_awayInactivitySpin->setValue(m_currentIdentity->getAwayInactivity());
        m_automaticUnawayChBox->setChecked(m_currentIdentity->getAutomaticUnaway());

        m_sCommandEdit->setText(m_currentIdentity->getShellCommand());
        m_codecCBox->setCurrentIndex(Konversation::IRCCharsets::self()->shortNameToIndex(m_currentIdentity->getCodecName()));
        m_loginEdit->setText(m_currentIdentity->getIdent());
        m_quitEdit->setText(m_currentIdentity->getQuitReason());
        m_partEdit->setText(m_currentIdentity->getPartReason());
        m_kickEdit->setText(m_currentIdentity->getKickReason());

        if(m_identityList.count() <= 1)
        {
            m_editBtn->setEnabled(false);
            m_delBtn->setEnabled(false);
        }
        else
        {
            m_editBtn->setEnabled(true);
            m_delBtn->setEnabled(true);
        }
    }

    void IdentityDialog::refreshCurrentIdentity()
    {
        if(!m_currentIdentity)
        {
            return;
        }

        m_currentIdentity->setRealName(m_realNameEdit->text());
        const QStringList nicks = m_nicknameLBox->items();
        m_currentIdentity->setNicknameList(nicks);

        m_currentIdentity->setAuthType(m_authTypeCombo->itemData(m_authTypeCombo->currentIndex()).toString());
        m_currentIdentity->setAuthPassword(m_authPasswordEdit->password());
        m_currentIdentity->setNickservNickname(m_nickservNicknameEdit->text());
        m_currentIdentity->setNickservCommand(m_nickservCommandEdit->text());
        m_currentIdentity->setSaslAccount(m_saslAccountEdit->text());
        m_currentIdentity->setPemClientCertFile(m_pemClientCertFile->url());

        m_currentIdentity->setInsertRememberLineOnAway(m_insertRememberLineOnAwayChBox->isChecked());
        m_currentIdentity->setAwayMessage(m_awayMessageEdit->text());
        m_currentIdentity->setAwayNickname(m_awayNickEdit->text());
        m_currentIdentity->setRunAwayCommands(awayCommandsGroup->isChecked());
        m_currentIdentity->setAwayCommand(m_awayEdit->text());
        m_currentIdentity->setReturnCommand(m_unAwayEdit->text());
        m_currentIdentity->setAutomaticAway(automaticAwayGroup->isChecked());
        m_currentIdentity->setAwayInactivity(m_awayInactivitySpin->value());
        m_currentIdentity->setAutomaticUnaway(m_automaticUnawayChBox->isChecked());

        m_currentIdentity->setShellCommand(m_sCommandEdit->text());
        if(m_codecCBox->currentIndex() >= 0 && m_codecCBox->currentIndex() < Konversation::IRCCharsets::self()->availableEncodingShortNames().count())
            m_currentIdentity->setCodecName(Konversation::IRCCharsets::self()->availableEncodingShortNames()[m_codecCBox->currentIndex()]);
        m_currentIdentity->setIdent(m_loginEdit->text());
        m_currentIdentity->setQuitReason(m_quitEdit->text());
        m_currentIdentity->setPartReason(m_partEdit->text());
        m_currentIdentity->setKickReason(m_kickEdit->text());
    }

    void IdentityDialog::accept()
    {
        if (!checkCurrentIdentity())
        {
            return;
        }

        refreshCurrentIdentity();
        Preferences::setIdentityList(m_identityList);
        Application::instance()->saveOptions(true);
        Q_EMIT identitiesChanged();
        QDialog::accept();
    }

    void IdentityDialog::newIdentity()
    {
        bool ok = false;
        QString txt = QInputDialog::getText(this, i18n("Add Identity"), i18n("Identity name:"), QLineEdit::Normal, QString(), &ok);

        if(ok && !txt.isEmpty())
        {
            KUser user(KUser::UseRealUserID);
            IdentityPtr identity=IdentityPtr(new Identity);
            identity->setName(txt);
            identity->setIdent(user.loginName());
            m_identityList.append(identity);
            m_identityCBox->addItem(txt);
            m_identityCBox->setCurrentIndex(m_identityCBox->count() - 1);
        }
        else if(ok && txt.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to give the identity a name."));
            newIdentity();
        }
    }

    void IdentityDialog::renameIdentity()
    {
        bool ok = false;
        QString currentTxt = m_identityCBox->currentText();
        QString txt = QInputDialog::getText(this, i18n("Rename Identity"), i18n("Identity name:"), QLineEdit::Normal, currentTxt, &ok);

        if(ok && !txt.isEmpty())
        {
            m_currentIdentity->setName(txt);
            m_identityCBox->setItemText(m_identityCBox->currentIndex(), txt);
        }
        else if(ok && txt.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to give the identity a name."));
            renameIdentity();
        }
    }

    void IdentityDialog::deleteIdentity()
    {
        int current = m_identityCBox->currentIndex();

        if(m_identityList.count() <= 1)
        {
            return;
        }

        const ServerGroupHash serverGroups = Preferences::serverGroupHash();
        const int identityId = m_currentIdentity->id();
        const bool found = std::any_of(serverGroups.begin(), serverGroups.end(),
                                       [identityId](const ServerGroupSettingsPtr& serverGroup) {
                                            return (serverGroup->identityId() == identityId);
                                       });

        QString warningTxt;

        if(found)
        {
            warningTxt = i18n("This identity is in use, if you remove it the network settings using it will"
                " fall back to the default identity. Should it be deleted anyway?");
        }
        else
        {
            warningTxt = i18n("Are you sure you want to delete all information for this identity?");
        }

        if(KMessageBox::warningContinueCancel(this, warningTxt, i18n("Delete Identity"),
            KStandardGuiItem::del()) == KMessageBox::Continue)
        {
            m_identityList.removeOne(m_currentIdentity);
            m_currentIdentity = nullptr;
            m_identityCBox->removeItem(current);
        }
    }

    void IdentityDialog::copyIdentity()
    {
        bool ok = false;
        QString currentTxt = m_identityCBox->currentText();
        QString txt = QInputDialog::getText(this, i18n("Duplicate Identity"), i18n("Identity name:"), QLineEdit::Normal, currentTxt, &ok);

        if(ok && !txt.isEmpty())
        {
            IdentityPtr identity(new Identity);
            identity->copy(*m_currentIdentity);
            identity->setName(txt);
            m_identityList.append(identity);
            m_identityCBox->addItem(txt);
            m_identityCBox->setCurrentIndex(m_identityCBox->count() - 1);
        }
        else if(ok && txt.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to give the identity a name."));
            renameIdentity();
        }
    }

    void IdentityDialog::setCurrentIdentity(int index)
    {
        if (index >= m_identityCBox->count())
            index = 0;

        m_identityCBox->setCurrentIndex(index);
    }

    IdentityPtr IdentityDialog::setCurrentIdentity(const IdentityPtr &identity)
    {
        int index = Preferences::identityList().indexOf(identity);
        setCurrentIdentity(index);

        return m_currentIdentity;
    }

    IdentityPtr IdentityDialog::currentIdentity() const
    {
        return m_currentIdentity;
    }

    bool IdentityDialog::checkCurrentIdentity()
    {
        if(m_nicknameLBox->count() == 0)
        {
            KMessageBox::error(this, i18n("You must add at least one nick to the identity."));
            bool block = m_identityCBox->blockSignals(true);
            m_identityCBox->setCurrentIndex(m_identityCBox->findText(m_currentIdentity->getName()));
            m_identityCBox->blockSignals(block);
            tabWidget->setCurrentIndex(0);
            m_nicknameLBox->lineEdit()->setFocus();
            return false;
        } else {
            for(const QString &nick : m_nicknameLBox->items())
            {
                if(nick.contains(QLatin1Char(' ')))
                {
                    KMessageBox::error(this, i18n("Nicks must not contain spaces."));
                    bool block = m_identityCBox->blockSignals(true);
                    m_identityCBox->setCurrentIndex(m_identityCBox->findText(m_currentIdentity->getName()));
                    m_identityCBox->blockSignals(block);
                    tabWidget->setCurrentIndex(0);
                    m_nicknameLBox->lineEdit()->setFocus();
                    return false;
                }
            }
        }

        if(m_realNameEdit->text().isEmpty())
        {
            KMessageBox::error(this, i18n("Please enter a real name."));
            bool block = m_identityCBox->blockSignals(true);
            m_identityCBox->setCurrentIndex(m_identityCBox->findText(m_currentIdentity->getName()));
            m_identityCBox->blockSignals(block);
            tabWidget->setCurrentIndex(0);
            m_realNameEdit->setFocus();
            return false;
        }

        if(!m_loginEdit->hasAcceptableInput())
        {
            KMessageBox::error(this, i18n("Ident must not contain spaces."));
            bool block = m_identityCBox->blockSignals(true);
            m_identityCBox->setCurrentIndex(m_identityCBox->findText(m_currentIdentity->getName()));
            m_identityCBox->blockSignals(block);
            tabWidget->setCurrentIndex(2);
            m_loginEdit->setFocus();
            return false;
        }

        return true;
    }

    void IdentityDialog::authTypeChanged(int index)
    {
        QString authType = m_authTypeCombo->itemData(index).toString();

        bool isNickServ = (authType == QLatin1String("nickserv"));
        bool isSaslPlain = (authType == QLatin1String("saslplain"));
        bool isSaslExernal = (authType == QLatin1String("saslexternal"));
        bool isServerPw = (authType == QLatin1String("serverpw"));
        bool isPemClientCert = (isSaslExernal || (authType == QLatin1String("pemclientcert")));

        nickservNicknameLabel->setVisible(isNickServ);
        m_nickservNicknameEdit->setVisible(isNickServ);
        nickservCommandLabel->setVisible(isNickServ);
        m_nickservCommandEdit->setVisible(isNickServ);
        saslAccountLabel->setVisible(isSaslPlain || isSaslExernal);
        m_saslAccountEdit->setVisible(isSaslPlain || isSaslExernal);
        authPasswordLabel->setVisible(!isPemClientCert);
        m_authPasswordEdit->setVisible(!isPemClientCert);
        pemClientCertFileLabel->setVisible(isPemClientCert);
        m_pemClientCertFile->setVisible(isPemClientCert);
        m_additionalAuthInfo->setVisible(isNickServ || isServerPw || isPemClientCert);

        // Clear.
        m_saslAccountEdit->setPlaceholderText(QString());

        for (int i = 0; i < autoIdentifyLayout->count(); ++i)
            autoIdentifyLayout->removeItem(autoIdentifyLayout->itemAt(0));

        autoIdentifyLayout->addRow(authTypeLabel, m_authTypeCombo);

        if (isNickServ)
        {
            autoIdentifyLayout->addRow(nickservNicknameLabel, m_nickservNicknameEdit);
            autoIdentifyLayout->addRow(nickservCommandLabel, m_nickservCommandEdit);
            autoIdentifyLayout->addRow(authPasswordLabel, m_authPasswordEdit);
            m_additionalAuthInfo->setText(i18n("NickServ may not function with auto join;  SASL is recommended."));
            autoIdentifyLayout->addRow(nullptr, m_additionalAuthInfo);
        }
        else if (isServerPw)
        {
            autoIdentifyLayout->addRow(authPasswordLabel, m_authPasswordEdit);

            m_additionalAuthInfo->setText(i18n("The password entered here will override the one set in the server settings, if any."));
            autoIdentifyLayout->addRow(nullptr, m_additionalAuthInfo);
        }
        else if (isSaslPlain)
        {
            autoIdentifyLayout->addRow(saslAccountLabel, m_saslAccountEdit);
            autoIdentifyLayout->addRow(authPasswordLabel, m_authPasswordEdit);
        }
        else if (isSaslExernal)
        {
            autoIdentifyLayout->addRow(saslAccountLabel, m_saslAccountEdit);
            m_saslAccountEdit->setPlaceholderText(i18nc("Shown in unfilled line edit", "(optional)"));
        }

        if (isPemClientCert)
        {
            autoIdentifyLayout->addRow(pemClientCertFileLabel, m_pemClientCertFile);

            m_additionalAuthInfo->setText(i18n("Certificate-based authentication forces SSL to be enabled for a connection, overriding any server settings."));
            autoIdentifyLayout->addRow(nullptr, m_additionalAuthInfo);
        }
    }
}

#include "moc_identitydialog.cpp"
