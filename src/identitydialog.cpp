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
#include "identitydialog.h"

#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qvaluelist.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qtabwidget.h>
#include <qlistbox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

#include <kcombobox.h>
#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kuser.h>

#include "konversationapplication.h"
#include "irccharsets.h"

namespace Konversation
{

    IdentityDialog::IdentityDialog(QWidget *parent, const char *name)
        : KDialogBase(Plain, i18n("Identities"), Ok|Cancel, Ok, parent, name)
    {
        QFrame* mainWidget = plainPage();
        QGridLayout* mainLayout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());

        QLabel* identityLabel = new QLabel(i18n("&Identity:"), mainWidget);
        m_identityCBox = new KComboBox(mainWidget, "identity_combo");
        m_identityCBox->setEditable(false);
        identityLabel->setBuddy(m_identityCBox);

        QValueList<IdentityPtr> tmpList = Preferences::identityList();

        for(QValueList<IdentityPtr>::iterator it = tmpList.begin(); it != tmpList.end(); ++it)
        {
            m_identityCBox->insertItem((*it)->getName());
            m_identityList.append(new Identity(*(*it)));
        }

        QToolButton* newBtn = new QToolButton(mainWidget);
        newBtn->setIconSet(SmallIconSet("add"));
        newBtn->setTextLabel(i18n("Add"));
        connect(newBtn, SIGNAL(clicked()), this, SLOT(newIdentity()));

        QToolButton* copyBtn = new QToolButton(mainWidget);
        copyBtn->setIconSet(SmallIconSet("editcopy"));
        copyBtn->setTextLabel(i18n("Duplicate"));
        connect(copyBtn, SIGNAL(clicked()), this, SLOT(copyIdentity()));

        m_editBtn = new QToolButton(mainWidget);
        m_editBtn->setIconSet(SmallIconSet("edit"));
        m_editBtn->setTextLabel(i18n("Rename"));
        connect(m_editBtn, SIGNAL(clicked()), this, SLOT(renameIdentity()));

        m_delBtn = new QToolButton(mainWidget);
        m_delBtn->setIconSet(SmallIconSet("editdelete"));
        m_delBtn->setTextLabel(i18n("Remove"));
        connect(m_delBtn, SIGNAL(clicked()), this, SLOT(deleteIdentity()));

        QTabWidget* tabWidget = new QTabWidget(mainWidget);
        QWidget* generalWidget = new QWidget(tabWidget);
        tabWidget->addTab(generalWidget, i18n("General"));
        QGridLayout* generalLayout = new QGridLayout(generalWidget, 1, 2, marginHint(), spacingHint());

        QLabel* realNameLabel = new QLabel(i18n("&Real name:"), generalWidget);
        m_realNameEdit = new KLineEdit(generalWidget);
        QWhatsThis::add(m_realNameEdit, i18n("Enter your real name here. IRC is not intended to keep you hidden from your friends or enemies. Keep this in mind if you are tempted to behave maliciously. A fake \"real name\" can be a good way to mask your gender from all the nerds out there, but the PC you use can always be traced so you will never be truly anonymous."));
        realNameLabel->setBuddy(m_realNameEdit);

        QGroupBox* nicknameGBox = new QGroupBox(0, Qt::Horizontal, i18n("Nickname"), generalWidget);
        nicknameGBox->setMargin(marginHint());
        QGridLayout* nicknameLayout = new QGridLayout(nicknameGBox->layout(), 1, 2, spacingHint());

        m_nicknameLBox = new QListBox(nicknameGBox);
        QWhatsThis::add(m_nicknameLBox, i18n("This is your list of nicknames. A nickname is the name that other users will know you by. You may use any name you desire. The first character must be a letter.\n\nSince nicknames must be unique across an entire IRC network, your desired name may be rejected by the server because someone else is already using that nickname. Enter alternate nicknames for yourself. If your first choice is rejected by the server, Konversation will try the alternate nicknames."));
        m_addNicknameBtn = new QPushButton(i18n("Add..."), nicknameGBox);
        m_changeNicknameBtn = new QPushButton(i18n("Edit..."), nicknameGBox);
        m_changeNicknameBtn->setEnabled(false);
        m_removeNicknameBtn = new QPushButton(i18n("Delete"), nicknameGBox);
        m_removeNicknameBtn->setEnabled(false);
        m_upNicknameBtn = new QToolButton(nicknameGBox);
        m_upNicknameBtn->setIconSet(SmallIconSet("up"));
        m_upNicknameBtn->setAutoRepeat(true);
        m_upNicknameBtn->setEnabled(false);
        m_downNicknameBtn = new QToolButton(nicknameGBox);
        m_downNicknameBtn->setIconSet(SmallIconSet("down"));
        m_downNicknameBtn->setAutoRepeat(true);
        m_downNicknameBtn->setEnabled(false);

        connect(m_addNicknameBtn, SIGNAL(clicked()), this, SLOT(addNickname()));
        connect(m_changeNicknameBtn, SIGNAL(clicked()), this, SLOT(editNickname()));
        connect(m_removeNicknameBtn, SIGNAL(clicked()), this, SLOT(deleteNickname()));
        connect(m_nicknameLBox, SIGNAL(selectionChanged()), this, SLOT(updateButtons()));
        connect(m_upNicknameBtn, SIGNAL(clicked()), this, SLOT(moveNicknameUp()));
        connect(m_downNicknameBtn, SIGNAL(clicked()), this, SLOT(moveNicknameDown()));

        nicknameLayout->setColStretch(0, 10);
        nicknameLayout->setRowStretch(4, 10);
        nicknameLayout->addMultiCellWidget(m_nicknameLBox, 0, 4, 0, 0);
        nicknameLayout->addMultiCellWidget(m_addNicknameBtn, 0, 0, 1, 4);
        nicknameLayout->addMultiCellWidget(m_changeNicknameBtn, 1, 1, 1, 4);
        nicknameLayout->addMultiCellWidget(m_removeNicknameBtn, 2, 2, 1, 4);
        nicknameLayout->addWidget(m_upNicknameBtn, 3, 2);
        nicknameLayout->addWidget(m_downNicknameBtn, 3, 3);

        QGroupBox* autoIdentifyGBox = new QGroupBox(0, Qt::Horizontal, i18n("Auto Identify"), generalWidget);
        autoIdentifyGBox->setMargin(marginHint());
        QGridLayout* autoIdentifyLayout = new QGridLayout(autoIdentifyGBox->layout(), 1, 2, spacingHint());

        QLabel* botLabel=new QLabel(i18n("Ser&vice:"), autoIdentifyGBox);
        QWhatsThis::add(botLabel,i18n("Service name can be <b><i>nickserv</i></b> or a network dependant name like  <b><i>nickserv@services.dal.net</i></b>"));
        m_botEdit = new KLineEdit(autoIdentifyGBox);
        botLabel->setBuddy(m_botEdit);

        QLabel* passwordLabel = new QLabel(i18n("Pa&ssword:"), autoIdentifyGBox);
        m_passwordEdit = new KLineEdit(autoIdentifyGBox);
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        passwordLabel->setBuddy(m_passwordEdit);

        autoIdentifyLayout->addWidget(botLabel, 0, 0);
        autoIdentifyLayout->addWidget(m_botEdit, 0, 1);
        autoIdentifyLayout->addWidget(passwordLabel, 0, 2);
        autoIdentifyLayout->addWidget(m_passwordEdit, 0, 3);

        int row = 0;
        generalLayout->addWidget(realNameLabel, row, 0);
        generalLayout->addWidget(m_realNameEdit, row, 1);
        row++;
        generalLayout->addMultiCellWidget(nicknameGBox, row, row, 0, 1);
        row++;
        generalLayout->addMultiCellWidget(autoIdentifyGBox, row, row, 0, 1);

        QWidget* awayWidget = new QWidget(tabWidget);
        tabWidget->addTab(awayWidget, i18n("Tab name", "Away"));
        QGridLayout* awayLayout = new QGridLayout(awayWidget, 1, 2, marginHint(), spacingHint());

        m_insertRememberLineOnAwayChBox = new QCheckBox(i18n("Insert remember &line when going away"), awayWidget);
        QWhatsThis::add(m_insertRememberLineOnAwayChBox, i18n("If you check this box, whenever you perform an <b>/away</b> command, a horizontal line will appear in the channel, marking the point where you went away. Other IRC users do not see this horizontal line."));

        QLabel* awayNickLabel = new QLabel(i18n("Away nickname:"), awayWidget);
        m_awayNickEdit = new KLineEdit(awayWidget);
        QWhatsThis::add(m_awayNickEdit, i18n("Enter a nickname that indicates you are away. Whenever you perform an <b>/away msg</b> command in any channel joined with this Identity, Konversation will automatically change your nickname to the Away nickname. Other users will be able to tell you are away from your computer. Whenever you perform an <b>/away</b> command in any channel in which you are away, Konversation will automatically change your nickname back to the original. If you do not wish to automatically change your nickname when going away, leave blank."));
        awayNickLabel->setBuddy(m_awayNickEdit);

        QGroupBox* m_awayMessageGBox = new QGroupBox(i18n("Messages"), awayWidget);
        //m_awayMessageGBox->setCheckable(true);
        m_awayMessageGBox->setColumnLayout(0, Qt::Horizontal);
        m_awayMessageGBox->setMargin(marginHint());
        QGridLayout* messagesLayout = new QGridLayout(m_awayMessageGBox->layout(), 1, 2, spacingHint());

        m_showAwayMessage = new QCheckBox(i18n("Show a&way messages"), m_awayMessageGBox);
        QWhatsThis::add(m_showAwayMessage, i18n("If you check this box, Konversation will automatically send the Away message to all channels joined with this Identity. <b>%s</b> is replaced with <b>msg</b>. Whenever you perform an <b>/away</b> command, the Return message will be displayed in all channels joined with this Identity."));
        m_showAwayMessage->setChecked(true);

        QLabel* awayLabel = new QLabel(i18n("Away &message:"), m_awayMessageGBox);
        m_awayEdit = new KLineEdit(m_awayMessageGBox);
        awayLabel->setBuddy(m_awayEdit);

        QLabel* unAwayLabel = new QLabel(i18n("Re&turn message:"), m_awayMessageGBox);
        m_unAwayEdit = new KLineEdit(m_awayMessageGBox);
        unAwayLabel->setBuddy(m_unAwayEdit);

        connect(m_showAwayMessage, SIGNAL(toggled(bool)), awayLabel, SLOT(setEnabled(bool)));
        connect(m_showAwayMessage, SIGNAL(toggled(bool)), m_awayEdit, SLOT(setEnabled(bool)));
        connect(m_showAwayMessage, SIGNAL(toggled(bool)), unAwayLabel, SLOT(setEnabled(bool)));
        connect(m_showAwayMessage, SIGNAL(toggled(bool)), m_unAwayEdit, SLOT(setEnabled(bool)));

        row = 0;
        messagesLayout->addMultiCellWidget(m_showAwayMessage, row, row, 0, 1);
        row++;
        messagesLayout->addWidget(awayLabel, row, 0);
        messagesLayout->addWidget(m_awayEdit, row, 1);
        row++;
        messagesLayout->addWidget(unAwayLabel, row, 0);
        messagesLayout->addWidget(m_unAwayEdit, row, 1);

        row = 0;
        awayLayout->addMultiCellWidget(m_insertRememberLineOnAwayChBox, row, row, 0, 1);
        row++;
        awayLayout->addWidget(awayNickLabel, row, 0);
        awayLayout->addWidget(m_awayNickEdit, row, 1);
        row++;
        awayLayout->addMultiCellWidget(m_awayMessageGBox, row, row, 0, 1);
        row++;
        awayLayout->setRowStretch(row, 10);

        QWidget* advancedWidget = new QWidget(tabWidget);
        tabWidget->addTab(advancedWidget, i18n("Advanced"));
        QGridLayout* advancedLayout = new QGridLayout(advancedWidget, 1, 2, marginHint(), spacingHint());

        QLabel* commandLabel = new QLabel(i18n("&Pre-shell command:"), advancedWidget);
        m_sCommandEdit = new KLineEdit(advancedWidget);
        QWhatsThis::add(m_sCommandEdit,i18n("Here you can enter a command to be executed before connection to server starts<br>If you have multiple servers in this identity this command will be executed for each server"));
        commandLabel->setBuddy(m_sCommandEdit);

        QLabel* loginLabel = new QLabel(i18n("I&dent:"), advancedWidget);
        m_loginEdit = new KLineEdit(advancedWidget);
        QWhatsThis::add(m_loginEdit, i18n("When you connect, many servers query your computer for an IDENT response. If you computer is not running an IDENT server, this response is sent by Konversation. No spaces are allowed."));
        loginLabel->setBuddy(m_loginEdit);

        // encoding combo box
        QLabel* codecLabel = new QLabel(i18n("&Encoding:"), advancedWidget);
        m_codecCBox = new KComboBox(advancedWidget,"codec_combo_box");
        QWhatsThis::add(m_codecCBox, i18n("This setting affects how characters you type are encoded for sending to the server. It also affects how messages are displayed. When you first open Konversation, it automatically retrieves this setting from the operating system. If you seem to be having trouble seeing other user's messages correctly, try changing this setting."));
        codecLabel->setBuddy(m_codecCBox);
        // add encodings to combo box
        m_codecCBox->insertStringList(Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames());

        QLabel* partLabel = new QLabel(i18n("&Part reason:"), advancedWidget);
        m_partEdit = new KLineEdit(advancedWidget);
        QWhatsThis::add(m_partEdit, i18n("Whenever you leave a channel, this message is sent to the channel."));
        partLabel->setBuddy(m_partEdit);

        QLabel* kickLabel = new QLabel(i18n("&Kick reason:"), advancedWidget);
        m_kickEdit = new KLineEdit(advancedWidget);
        QWhatsThis::add(m_kickEdit, i18n("Whenever you are kicked from a channel (usually by an IRC operator), this message is sent to the channel."));
        kickLabel->setBuddy(m_kickEdit);

        row = 0;
        advancedLayout->addWidget(commandLabel,row,0);
        advancedLayout->addWidget(m_sCommandEdit, row, 1);
        row++;
        advancedLayout->addWidget(codecLabel,row,0);
        advancedLayout->addWidget(m_codecCBox, row, 1);
        row++;
        advancedLayout->addWidget(loginLabel,row,0);
        advancedLayout->addWidget(m_loginEdit, row, 1);
        row++;
        advancedLayout->addWidget(partLabel, row, 0);
        advancedLayout->addWidget(m_partEdit, row, 1);
        row++;
        advancedLayout->addWidget(kickLabel, row, 0);
        advancedLayout->addWidget(m_kickEdit, row, 1);
        row++;
        advancedLayout->setRowStretch(row, 10);

        row = 0;
        mainLayout->addWidget(identityLabel, row, 0);
        mainLayout->addMultiCellWidget(m_identityCBox, row, row, 1, 2);
        mainLayout->addWidget(newBtn, row, 3);
        mainLayout->addWidget(copyBtn, row, 4);
        mainLayout->addWidget(m_editBtn, row, 5);
        mainLayout->addWidget(m_delBtn, row, 6);
        mainLayout->setColStretch(1, 10);
        row++;
        mainLayout->addMultiCellWidget(tabWidget, row, row, 0, 6);

        // set values for the widgets
        updateIdentity(0);

        // Set up signals / slots for identity page
        connect(m_identityCBox, SIGNAL(activated(int)), this, SLOT(updateIdentity(int)));

        setButtonOK(KGuiItem(i18n("&OK"), "button_ok", i18n("Change identity information")));
        setButtonCancel(KGuiItem(i18n("&Cancel"), "button_cancel", i18n("Discards all changes made")));
    }

    IdentityDialog::~IdentityDialog()
    {
    }

    void IdentityDialog::updateIdentity(int index)
    {
        if(m_currentIdentity && (m_nicknameLBox->count() == 0))
        {
            KMessageBox::error(this, i18n("You must add at least one nick to the identity."));
            m_identityCBox->setCurrentText(m_currentIdentity->getName());
            return;
        }

        refreshCurrentIdentity();

        m_currentIdentity = m_identityList[index];

        m_realNameEdit->setText(m_currentIdentity->getRealName());
        m_nicknameLBox->clear();
        m_nicknameLBox->insertStringList(m_currentIdentity->getNicknameList());
        m_botEdit->setText(m_currentIdentity->getBot());
        m_passwordEdit->setText(m_currentIdentity->getPassword());

        m_insertRememberLineOnAwayChBox->setChecked(m_currentIdentity->getInsertRememberLineOnAway());
        m_awayNickEdit->setText(m_currentIdentity->getAwayNick());
        m_showAwayMessage->setChecked(m_currentIdentity->getShowAwayMessage());
        m_awayEdit->setText(m_currentIdentity->getAwayMessage());
        m_unAwayEdit->setText(m_currentIdentity->getReturnMessage());

        m_sCommandEdit->setText(m_currentIdentity->getShellCommand());
        m_codecCBox->setCurrentItem(Konversation::IRCCharsets::self()->shortNameToIndex(m_currentIdentity->getCodecName()));
        m_loginEdit->setText(m_currentIdentity->getIdent());
        m_partEdit->setText(m_currentIdentity->getPartReason());
        m_kickEdit->setText(m_currentIdentity->getKickReason());

        if(index == 0)
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

    void IdentityDialog::addNickname()
    {
        bool ok = false;
        QString txt = KInputDialog::getText(i18n("Add Nickname"), i18n("Nickname:"), QString(), &ok, this);

        if(ok && !txt.isEmpty())
        {
            m_nicknameLBox->insertItem(txt);
            updateButtons();
        }
    }

    void IdentityDialog::editNickname()
    {
        bool ok = false;
        QString txt = KInputDialog::getText(i18n("Edit Nickname"), i18n("Nickname:"), m_nicknameLBox->currentText(), &ok, this);

        if(ok && !txt.isEmpty())
        {
            m_nicknameLBox->changeItem(txt, m_nicknameLBox->currentItem());
        }
    }

    void IdentityDialog::deleteNickname()
    {
        m_nicknameLBox->removeItem(m_nicknameLBox->currentItem());
        updateButtons();
    }

    void IdentityDialog::updateButtons()
    {
        m_changeNicknameBtn->setEnabled(m_nicknameLBox->selectedItem());
        m_removeNicknameBtn->setEnabled(m_nicknameLBox->selectedItem());

        m_upNicknameBtn->setEnabled(m_nicknameLBox->selectedItem() && m_nicknameLBox->count()>1
            && m_nicknameLBox->currentItem()>0);

        // FIXME: find another way than casting to overcome signedness warning
        m_downNicknameBtn->setEnabled(m_nicknameLBox->selectedItem() && m_nicknameLBox->count()>1
            && m_nicknameLBox->currentItem()<static_cast<int>(m_nicknameLBox->count()-1) );

    }

    void IdentityDialog::moveNicknameUp()
    {
        uint current = m_nicknameLBox->currentItem();

        if(current > 0)
        {
            QString txt = m_nicknameLBox->text(current);
            m_nicknameLBox->removeItem(current);
            m_nicknameLBox->insertItem(txt, current - 1);
            m_nicknameLBox->setCurrentItem(current - 1);
        }

        updateButtons();
    }

    void IdentityDialog::moveNicknameDown()
    {
        uint current = m_nicknameLBox->currentItem();

        if(current < (m_nicknameLBox->count() - 1))
        {
            QString txt = m_nicknameLBox->text(current);
            m_nicknameLBox->removeItem(current);
            m_nicknameLBox->insertItem(txt, current + 1);
            m_nicknameLBox->setCurrentItem(current + 1);
        }

        updateButtons();
    }

    void IdentityDialog::refreshCurrentIdentity()
    {
        if(!m_currentIdentity)
        {
            return;
        }

        m_currentIdentity->setRealName(m_realNameEdit->text());
        QStringList nicks;

        for(unsigned int i = 0; i < m_nicknameLBox->count(); ++i)
        {
            nicks.append(m_nicknameLBox->text(i));
        }

        m_currentIdentity->setNicknameList(nicks);
        m_currentIdentity->setBot(m_botEdit->text());
        m_currentIdentity->setPassword(m_passwordEdit->text());

        m_currentIdentity->setInsertRememberLineOnAway(m_insertRememberLineOnAwayChBox->isChecked());
        m_currentIdentity->setAwayNick(m_awayNickEdit->text());
        m_currentIdentity->setShowAwayMessage(m_showAwayMessage->isChecked());
        m_currentIdentity->setAwayMessage(m_awayEdit->text());
        m_currentIdentity->setReturnMessage(m_unAwayEdit->text());

        m_currentIdentity->setShellCommand(m_sCommandEdit->text());
        m_currentIdentity->setCodecName(Konversation::IRCCharsets::self()->availableEncodingShortNames()[m_codecCBox->currentItem()]);
        m_currentIdentity->setIdent(m_loginEdit->text());
        m_currentIdentity->setPartReason(m_partEdit->text());
        m_currentIdentity->setKickReason(m_kickEdit->text());
    }

    void IdentityDialog::slotOk()
    {
        if(m_nicknameLBox->count() == 0)
        {
            KMessageBox::error(this, i18n("You must add at least one nick to the identity."));
            m_identityCBox->setCurrentText(m_currentIdentity->getName());
            return;
        }

        refreshCurrentIdentity();
        Preferences::setIdentityList(m_identityList);
        static_cast<KonversationApplication*>(kapp)->saveOptions(true);
        accept();
    }

    void IdentityDialog::newIdentity()
    {
        bool ok = false;
        QString txt = KInputDialog::getText(i18n("Add Identity"), i18n("Identity name:"), QString(), &ok, this);

        if(ok && !txt.isEmpty())
        {
            KUser user(KUser::UseRealUserID);
            IdentityPtr identity = new Identity;
            identity->setName(txt);
            identity->setIdent(user.loginName());
            m_identityList.append(identity);
            m_identityCBox->insertItem(txt);
            m_identityCBox->setCurrentItem(m_identityCBox->count() - 1);
            updateIdentity(m_identityCBox->currentItem());
        }
        else if(ok && txt.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to give the identity a name."));
            newIdentity();
        }
        updateButtons();
    }

    void IdentityDialog::renameIdentity()
    {
        bool ok = false;
        QString currentTxt = m_identityCBox->currentText();
        QString txt = KInputDialog::getText(i18n("Rename Identity"), i18n("Identity name:"), currentTxt, &ok, this);

        if(ok && !txt.isEmpty())
        {
            m_currentIdentity->setName(txt);
            m_identityCBox->changeItem(txt, m_identityCBox->currentItem());
        }
        else if(ok && txt.isEmpty())
        {
            KMessageBox::error(this, i18n("You need to give the identity a name."));
            renameIdentity();
        }
    }

    void IdentityDialog::deleteIdentity()
    {
        int current = m_identityCBox->currentItem();

        if(current <= 0)
        {
            return;
        }

        ServerGroupList serverGroups = Preferences::serverGroupList();
        ServerGroupList::iterator it = serverGroups.begin();
        bool found = false;

        while((it != serverGroups.end()) && !found)
        {
            if((*it)->identityId() == m_currentIdentity->id())
            {
                found = true;
            }

            ++it;
        }

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
            KGuiItem(i18n("Delete"), "editdelete")) == KMessageBox::Continue)
        {
            m_identityCBox->removeItem(current);
            m_identityList.remove(m_currentIdentity);
            m_currentIdentity = 0;
            updateIdentity(m_identityCBox->currentItem());
            updateButtons();
        }
    }

    void IdentityDialog::copyIdentity()
    {
        bool ok = false;
        QString currentTxt = m_identityCBox->currentText();
        QString txt = KInputDialog::getText(i18n("Duplicate Identity"), i18n("Identity name:"), currentTxt, &ok, this);

        if(ok && !txt.isEmpty())
        {
            IdentityPtr identity = new Identity;
            identity->copy(*m_currentIdentity);
            identity->setName(txt);
            m_identityList.append(identity);
            m_identityCBox->insertItem(txt);
            m_identityCBox->setCurrentItem(m_identityCBox->count() - 1);
            updateIdentity(m_identityCBox->currentItem());
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

        m_identityCBox->setCurrentItem(index);
        updateIdentity(index);
    }

    IdentityPtr IdentityDialog::setCurrentIdentity(IdentityPtr identity)
    {
        int index = Preferences::identityList().findIndex(identity);
        setCurrentIdentity(index);

        return m_currentIdentity;
    }

    IdentityPtr IdentityDialog::currentIdentity() const
    {
        return m_currentIdentity;
    }
}

#include "identitydialog.moc"
