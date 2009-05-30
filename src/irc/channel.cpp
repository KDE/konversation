/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2004-2006, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "channel.h"
#include "application.h" ////// header renamed
#include "server.h"
#include "blowfish.h"
#include "nick.h"
#include "nicklistview.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"
#include "ircviewbox.h"
#include "ircview.h"
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>
#include "common.h"
#include "topiclabel.h"
#include "channeloptionsdialog.h"
#include "notificationhandler.h"
#include "viewcontainer.h"
#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"

#include <qlabel.h>
#include <qevent.h>
#include <q3grid.h>
#include <q3dragobject.h>
#include <qsizepolicy.h>
#include <q3header.h>
#include <qregexp.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qtimer.h>
#include <qtextcodec.h>
#include <qtoolbutton.h>
#include <qlayout.h>


#include <klineedit.h>
#include <kinputdialog.h>
#include <kpassworddialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kwindowsystem.h>
#include <KColorScheme>
#include <kvbox.h>
#include <khbox.h>
#include <kcombobox.h>

bool nickTimestampLessThan(const Nick* nick1, const Nick* nick2)
{
    int returnValue = nick2->getChannelNick()->timeStamp() - nick1->getChannelNick()->timeStamp();
    if( returnValue == 0) {
        returnValue = QString::compare(nick1->getChannelNick()->loweredNickname(),
                                       nick2->getChannelNick()->loweredNickname());
    }

    return (returnValue < 0);
}

bool nickLessThan(const Nick* nick1, const Nick* nick2)
{
    return nick1->getChannelNick()->loweredNickname() < nick2->getChannelNick()->loweredNickname();
}


using Konversation::ChannelOptionsDialog;

Channel::Channel(QWidget* parent, QString _name) : ChatWindow(parent)
{
    // init variables

    //HACK I needed the channel name at time of setServer, but setName needs m_server..
    //     This effectively assigns the name twice, but none of the other logic has been moved or updated.
    name=_name;
    m_processingTimer = 0;
    m_delayedSortTimer = 0;
    m_optionsDialog = NULL;
    m_pendingChannelNickLists.clear();
    m_currentIndex = 0;
    m_opsToAdd = 0;
    nicks = 0;
    ops = 0;
    completionPosition = 0;
    nickChangeDialog = 0;
    channelCommand = false;

    m_joined = false;

    quickButtonsChanged = false;
    quickButtonsState = false;

    modeButtonsChanged = false;
    modeButtonsState = false;

    awayChanged = false;
    awayState = false;

    splittersInitialized = false;
    topicSplitterHidden = false;
    channelSplitterHidden = false;

    // flag for first seen topic
    topicAuthorUnknown = true;

    setType(ChatWindow::Channel);

    setChannelEncodingSupported(true);

    // Build some size policies for the widgets
    QSizePolicy hfixed = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QSizePolicy hmodest = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QSizePolicy vmodest = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QSizePolicy vfixed = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QSizePolicy modest = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QSizePolicy greedy = QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_vertSplitter = new QSplitter(Qt::Vertical, this);
    m_vertSplitter->setOpaqueResize(KGlobalSettings::opaqueResize());


    QWidget* topicWidget = new QWidget(m_vertSplitter);
    m_vertSplitter->setStretchFactor(m_vertSplitter->indexOf(topicWidget), 0);

    QGridLayout* topicLayout = new QGridLayout(topicWidget);
    topicLayout->setMargin(0);
    topicLayout->setSpacing(0);

    m_topicButton = new QToolButton(topicWidget);
    m_topicButton->setIcon(KIcon("document-edit"));
    m_topicButton->setToolTip(i18n("Edit Channel Settings"));
    m_topicButton->setAutoRaise(true);
    connect(m_topicButton, SIGNAL(clicked()), this, SLOT(showOptionsDialog()));

    topicLine = new Konversation::TopicLabel(topicWidget);
    topicLine->setWordWrap(true);
    topicLine->setWhatsThis(i18n("<qt><p>Every channel on IRC has a topic associated with it.  This is simply a message that everybody can see.</p><p>If you are an operator, or the channel mode <em>'T'</em> has not been set, then you can change the topic by clicking the Edit Channel Properties button to the left of the topic.  You can also view the history of topics there.</p></qt>"));
    connect(topicLine, SIGNAL(setStatusBarTempText(const QString&)), this, SIGNAL(setStatusBarTempText(const QString&)));
    connect(topicLine, SIGNAL(clearStatusBarTempText()), this, SIGNAL(clearStatusBarTempText()));

    topicLayout->addWidget(m_topicButton, 0, 0);
    topicLayout->addWidget(topicLine, 0, 1, -1, 1);

    // The box holding the channel modes
    modeBox = new KHBox(topicWidget);
    modeBox->setSizePolicy(hfixed);
    modeT = new ModeButton("T",modeBox,0);
    modeN = new ModeButton("N",modeBox,1);
    modeS = new ModeButton("S",modeBox,2);
    modeI = new ModeButton("I",modeBox,3);
    modeP = new ModeButton("P",modeBox,4);
    modeM = new ModeButton("M",modeBox,5);
    modeK = new ModeButton("K",modeBox,6);
    modeL = new ModeButton("L",modeBox,7);

    modeT->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('T'));
    modeN->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('N'));
    modeS->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('S'));
    modeI->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('I'));
    modeP->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('P'));
    modeM->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('M'));
    modeK->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('K'));
    modeL->setWhatsThis(ChannelOptionsDialog::whatsThisForMode('L'));

    connect(modeT,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeN,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeS,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeI,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeP,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeM,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeK,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
    connect(modeL,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));

    limit=new KLineEdit(modeBox);
    limit->setToolTip(i18n("Maximum users allowed in channel"));
    limit->setWhatsThis(i18n("<qt><p>This is the channel user limit - the maximum number of users that can be in the channel at a time.  If you are an operator, you can set this.  The channel mode <b>T</b>opic (button to left) will automatically be set if set this.</p></qt>"));
    connect(limit,SIGNAL (returnPressed()),this,SLOT (channelLimitChanged()) );
    connect(limit,SIGNAL (editingFinished()), this, SLOT(channelLimitChanged()) );

    topicLayout->addWidget(modeBox, 0, 2);
    topicLayout->setRowStretch(1, 10);
    topicLayout->setColumnStretch(1, 10);

    showTopic(Preferences::self()->showTopic());
    showModeButtons(Preferences::self()->showModeButtons());

    // (this) The main Box, holding the channel view/topic and the input line
    m_horizSplitter = new QSplitter(m_vertSplitter);
    m_vertSplitter->setStretchFactor(m_vertSplitter->indexOf(m_horizSplitter), 1);
    m_horizSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );

    // Server will be set later in setServer()
    IRCViewBox* ircViewBox = new IRCViewBox(m_horizSplitter, NULL);
    m_horizSplitter->setStretchFactor(m_horizSplitter->indexOf(ircViewBox), 1);
    setTextView(ircViewBox->ircView());
    connect(textView,SIGNAL(popupCommand(int)),this,SLOT(popupChannelCommand(int)));

    // The box that holds the Nick List and the quick action buttons
    nickListButtons = new KVBox(m_horizSplitter);
    m_horizSplitter->setStretchFactor(m_horizSplitter->indexOf(nickListButtons), 0);
    nickListButtons->setSpacing(spacing());

    nicknameListView=new NickListView(nickListButtons, this);
    nicknameListView->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    nicknameListView->setSelectionModeExt(K3ListView::Extended);
    nicknameListView->setAllColumnsShowFocus(true);
    nicknameListView->setSorting(1,true);
    nicknameListView->addColumn(QString());
    nicknameListView->addColumn(QString());
    nicknameListView->setColumnWidthMode(1,K3ListView::Maximum);

    nicknameListView->header()->hide();

    // setResizeMode must be called after all the columns are added
    nicknameListView->setResizeMode(K3ListView::LastColumn);

    // separate LED from Text a little more
    nicknameListView->setColumnWidth(0, 10);
    nicknameListView->setColumnAlignment(0, Qt::AlignHCenter);

    nicknameListView->installEventFilter(this);

    // initialize buttons grid, will be set up in updateQuickButtons
    buttonsGrid=0;

    // The box holding the Nickname button and Channel input
    commandLineBox = new KHBox(this);
    commandLineBox->setSpacing(spacing());

    nicknameCombobox = new KComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->setSizeAdjustPolicy(KComboBox::AdjustToContents);
    KLineEdit* nicknameComboboxLineEdit = qobject_cast<KLineEdit*>(nicknameCombobox->lineEdit());
    if (nicknameComboboxLineEdit) nicknameComboboxLineEdit->setClearButtonShown(false);
    nicknameCombobox->setWhatsThis(i18n("<qt><p>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.</p><p>You can add change the alternative nicknames from the <em>Identities</em> option in the <em>File</em> menu.</p></qt>"));

    awayLabel = new QLabel(i18n("(away)"), commandLineBox);
    awayLabel->hide();
    blowfishLabel = new QLabel(commandLineBox);
    blowfishLabel->hide();
    blowfishLabel->setPixmap(KIconLoader::global()->loadIcon("document-encrypt", KIconLoader::Toolbar));
    channelInput = new IRCInput(commandLineBox);

    getTextView()->installEventFilter(channelInput);
    topicLine->installEventFilter(channelInput);
    channelInput->installEventFilter(this);

    // Set the widgets size policies
    m_topicButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    topicLine->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum));

    commandLineBox->setSizePolicy(vfixed);

    limit->setMaximumSize(40,100);
    limit->setSizePolicy(hfixed);

    modeT->setMaximumSize(20,100);
    modeT->setSizePolicy(hfixed);
    modeN->setMaximumSize(20,100);
    modeN->setSizePolicy(hfixed);
    modeS->setMaximumSize(20,100);
    modeS->setSizePolicy(hfixed);
    modeI->setMaximumSize(20,100);
    modeI->setSizePolicy(hfixed);
    modeP->setMaximumSize(20,100);
    modeP->setSizePolicy(hfixed);
    modeM->setMaximumSize(20,100);
    modeM->setSizePolicy(hfixed);
    modeK->setMaximumSize(20,100);
    modeK->setSizePolicy(hfixed);
    modeL->setMaximumSize(20,100);
    modeL->setSizePolicy(hfixed);

    getTextView()->setSizePolicy(greedy);
    nicknameListView->setSizePolicy(hmodest);

    connect(channelInput,SIGNAL (submit()),this,SLOT (channelTextEntered()) );
    connect(channelInput,SIGNAL (envelopeCommand()),this,SLOT (channelPassthroughCommand()) );
    connect(channelInput,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
    connect(channelInput,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
    connect(channelInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );

    connect(getTextView(), SIGNAL(textPasted(bool)), channelInput, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),channelInput,SLOT (setFocus()) );
    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendChannelText(const QString&)) );

    connect(nicknameListView,SIGNAL (popupCommand(int)),this,SLOT (popupCommand(int)) );
    connect(nicknameListView,SIGNAL (doubleClicked(Q3ListViewItem*)),this,SLOT (doubleClickCommand(Q3ListViewItem*)) );
    connect(nicknameListView,SIGNAL (dropped(QDropEvent*,Q3ListViewItem*)),this,SLOT (filesDropped(QDropEvent*)) );
    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));

    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (editingFinished()),this,SLOT(nicknameComboboxChanged()));


    setLog(Preferences::self()->log());

    connect(&userhostTimer,SIGNAL (timeout()),this,SLOT (autoUserhost()));

    // every few seconds try to get more userhosts
    userhostTimer.start(10000);

    m_whoTimer.setSingleShot(true);
    connect(&m_whoTimer,SIGNAL (timeout()),this,SLOT (autoWho()));

    // every 5 minutes decrease everyone's activity by 1 unit
    m_fadeActivityTimer.start(5*60*1000);

    connect(&m_fadeActivityTimer, SIGNAL(timeout()), this, SLOT(fadeActivity()));

    // re-schedule when the settings were changed
    connect(Preferences::self(), SIGNAL (autoContinuousWhoChanged()),this,SLOT (scheduleAutoWho()));

    updateAppearance();

    //FIXME JOHNFLUX
    // connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
    // connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));
}

//FIXME there is some logic in setLogfileName that needs to be split out and called here if the server display name gets changed
void Channel::setServer(Server* server)
{
    if (m_server != server)
        connect(server, SIGNAL(connectionStateChanged(Server*, Konversation::ConnectionState)),
                SLOT(connectionStateChanged(Server*, Konversation::ConnectionState)));
    ChatWindow::setServer(server);
    if (!server->getKeyForRecipient(getName()).isEmpty())
        blowfishLabel->show();
    topicLine->setServer(server);
    refreshModeButtons();
    nicknameCombobox->setModel(m_server->nickListModel());
}

void Channel::connectionStateChanged(Server* server, Konversation::ConnectionState state)
{
    if (server == m_server)
    {
        if (state !=  Konversation::SSConnected)
        {
            m_joined = false;

            //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
            if (m_currentTabNotify == Konversation::tnfNone || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
                KonversationApplication::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);
        }
    }
}

void Channel::setEncryptedOutput(bool e)
{
    if (e) {
        blowfishLabel->show();
        //scan the channel topic and decrypt it if necessary
        if (m_topicHistory.isEmpty())
            return;
        QString topic(m_topicHistory.at(0).section(' ',2));

        //prepend two colons to make it appear to be an irc message for decryption,
        // \r because it won't decrypt without it, even though the message did not have a \r
        // when encrypted. Bring on the QCA!
        QByteArray cipher = "::" + topic.toUtf8() + '\x0d';
        Konversation::decryptTopic(getName(), cipher, m_server);
        topic=QString::fromUtf8(cipher.data()+2, cipher.length()-2);
        m_topicHistory[0] = m_topicHistory[0].section(' ', 0, 1) + ' ' + topic;
        topicLine->setText(topic);
        emit topicHistoryChanged();
    }
    else
        blowfishLabel->hide();
}

Channel::~Channel()
{
    kDebug() << "(" << getName() << ")";

    // Purge nickname list
    purgeNicks();
    kDebug() << "nicks purged";

    // Unlink this channel from channel list
    m_server->removeChannel(this);
    kDebug() << "Channel removed.";

}

bool Channel::rejoinable()
{
    if (getServer() && getServer()->isConnected())
        return !m_joined;

    return false;
}

void Channel::rejoin()
{
    if (rejoinable())
        m_server->sendJoinCommand(getName(), getPassword());
}

ChannelNickPtr Channel::getOwnChannelNick()
{
    return m_ownChannelNick;
}

ChannelNickPtr Channel::getChannelNick(const QString &ircnick)
{
    return m_server->getChannelNick(getName(), ircnick);
}

void Channel::purgeNicks()
{
    // Purge nickname list
    qDeleteAll(nicknameList);
    nicknameList.clear();

    // clear stats counter
    nicks=0;
    ops=0;
}

void Channel::showOptionsDialog()
{
    if(!m_optionsDialog)
        m_optionsDialog = new Konversation::ChannelOptionsDialog(this);

    m_optionsDialog->refreshModes();
    m_optionsDialog->refreshTopicHistory();
    m_optionsDialog->show();
}

void Channel::filesDropped(QDropEvent* e)
{
    QPoint p(nicknameListView->contentsToViewport(e->pos()));
    Nick* it = dynamic_cast<Nick*>(nicknameListView->itemAt(p));
    if (!it) return;

    const KUrl::List uris = KUrl::List::fromMimeData(e->mimeData());
    m_server->sendURIs(uris, it->getChannelNick()->getNickname());
}

void Channel::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline = text.split('\n', QString::SkipEmptyParts);
        for(int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::self()->commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendChannelText(line);
        }
    }
}

// Will be connected to IRCView::popupCommand(int)
void Channel::popupChannelCommand(int id)
{
    channelCommand = true; // Context menu executed from ircview
    popupCommand(id);
    textView->clearContextNick();
    channelCommand = false;
}

// Will be connected to NickListView::popupCommand(int)
void Channel::popupCommand(int id)
{
    QString pattern;
    QString cc = Preferences::self()->commandChar();
    QString args;
    QString question;
    bool raw=false;
    QString mode;
    QStringList nickList = getSelectedNickList();

    switch(id)
    {
        case Konversation::AddressbookEdit:
        {
            ChannelNickList nickList=getSelectedChannelNicks();
            for(ChannelNickList::ConstIterator it=nickList.constBegin();it!=nickList.constEnd();++it)
            {
                if(!(*it)->getNickInfo()->editAddressee()) break;;
            }
            break;
        }
        case Konversation::AddressbookNew:
        case Konversation::AddressbookDelete:
        {
            Konversation::Addressbook *addressbook = Konversation::Addressbook::self();
            ChannelNickList nickList=getSelectedChannelNicks();
            //Handle all the selected nicks in one go.  Either they all save, or none do.
            if(addressbook->getAndCheckTicket())
            {
                for(ChannelNickList::ConstIterator it=nickList.constBegin();it!=nickList.constEnd();++it)
                {
                    if(id == Konversation::AddressbookDelete)
                    {
                        KABC::Addressee addr = (*it)->getNickInfo()->getAddressee();
                        addressbook->unassociateNick(addr, (*it)->getNickname(), m_server->getServerName(), m_server->getDisplayName());
                    }
                    else
                    {
                        //make new addressbook contact
                        KABC::Addressee addr;
                        NickInfoPtr nickInfo = (*it)->getNickInfo();
                        if(nickInfo->getRealName().isEmpty())
                            addr.setGivenName(nickInfo->getNickname());
                        else
                            addr.setGivenName(nickInfo->getRealName());
                        addr.setNickName(nickInfo->getNickname());
                        addressbook->associateNickAndUnassociateFromEveryoneElse(addr, (*it)->getNickname(), m_server->getServerName(), m_server->getDisplayName());
                    }
                }
                addressbook->saveTicket(); // This will refresh the nicks automatically for us. At least, if it doesn't, it's a bug :)
            }
            break;
        }
        case Konversation::AddressbookChange:
        {
            ChannelNickList nickList=getSelectedChannelNicks();
            for(ChannelNickList::ConstIterator it=nickList.constBegin();it!=nickList.constEnd();++it)
            {
                (*it)->getNickInfo()->showLinkAddressbookUI();
            }
            break;
        }
        case Konversation::SendEmail:
        {
            Konversation::Addressbook::self()->sendEmail(getSelectedChannelNicks());
            break;
        }
        case Konversation::GiveOp:
            pattern="MODE %c +%m %l";
            mode='o';
            raw=true;
            break;
        case Konversation::TakeOp:
            pattern="MODE %c -%m %l";
            mode='o';
            raw=true;
            break;
        case Konversation::GiveHalfOp:
            pattern="MODE %c +%m %l";
            mode='h';
            raw=true;
            break;
        case Konversation::TakeHalfOp:
            pattern="MODE %c -%m %l";
            mode='h';
            raw=true;
            break;
        case Konversation::GiveVoice:
            pattern="MODE %c +%m %l";
            mode='v';
            raw=true;
            break;
        case Konversation::TakeVoice:
            pattern="MODE %c -%m %l";
            mode='v';
            raw=true;
            break;
        case Konversation::Version:
            pattern=cc+"CTCP %u VERSION";
            break;
        case Konversation::Whois:
            pattern="WHOIS %u %u";
            raw=true;
            break;
        case Konversation::Topic:
            m_server->requestTopic(getTextView()->currentChannel());
            break;
        case Konversation::Names:
            m_server->queue("NAMES " + getTextView()->currentChannel(), Server::LowPriority);
            break;
        case Konversation::Join:
            m_server->queue("JOIN " + getTextView()->currentChannel());
            break;
        case Konversation::Ping:
        {
            unsigned int time_t = QDateTime::currentDateTime().toTime_t();
            pattern=QString(cc+"CTCP %u PING %1").arg(time_t);
        }
        break;
        case Konversation::Kick:
            pattern=cc+"KICK %u";
            break;
        case Konversation::KickBan:
            pattern=cc+"BAN %u\n"+
                cc+"KICK %u";
            break;
        case Konversation::BanNick:
            pattern=cc+"BAN %u";
            break;
        case Konversation::BanHost:
            pattern=cc+"BAN -HOST %u";
            break;
        case Konversation::BanDomain:
            pattern=cc+"BAN -DOMAIN %u";
            break;
        case Konversation::BanUserHost:
            pattern=cc+"BAN -USERHOST %u";
            break;
        case Konversation::BanUserDomain:
            pattern=cc+"BAN -USERDOMAIN %u";
            break;
        case Konversation::KickBanHost:
            pattern=cc+"KICKBAN -HOST %u";
            break;
        case Konversation::KickBanDomain:
            pattern=cc+"KICKBAN -DOMAIN %u";
            break;
        case Konversation::KickBanUserHost:
            pattern=cc+"KICKBAN -USERHOST %u";
            break;
        case Konversation::KickBanUserDomain:
            pattern=cc+"KICKBAN -USERDOMAIN %u";
            break;
        case Konversation::OpenQuery:
            pattern=cc+"QUERY %u";
            break;
        case Konversation::StartDccChat:
            pattern=cc+"DCC CHAT %u";
            break;
        case Konversation::DccSend:
            pattern=cc+"DCC SEND %u";
            break;
        case Konversation::IgnoreNick:
            if (nickList.size() == 1)
                question=i18n("Do you want to ignore %1?", nickList.first());
            else
                question = i18n("Do you want to ignore the selected users?");
            if (KMessageBox::warningContinueCancel(
                this,
                question,
                i18n("Ignore"),
                KGuiItem(i18n("Ignore")),
                KStandardGuiItem::cancel(),
                "IgnoreNick"
                ) ==
                KMessageBox::Continue)
                pattern = cc+"IGNORE -ALL %l";
            break;
        case Konversation::UnignoreNick:
        {
            QStringList selectedIgnoredNicks;

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
            {
                if (Preferences::isIgnored((*it)))
                    selectedIgnoredNicks.append((*it));
            }

            if (selectedIgnoredNicks.count() == 1)
                question=i18n("Do you want to stop ignoring %1?", selectedIgnoredNicks.first());
            else
                question = i18n("Do you want to stop ignoring the selected users?");
            if (KMessageBox::warningContinueCancel(
                this,
                question,
                i18n("Unignore"),
                KGuiItem(i18n("Unignore")),
                KStandardGuiItem::cancel(),
                "UnignoreNick") ==
                KMessageBox::Continue)
            {
                sendChannelText(cc+"UNIGNORE "+selectedIgnoredNicks.join(" "));
            }
            break;
        }
        case Konversation::AddNotify:
        {
            if (m_server->getServerGroup())
            {
                for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
                {
                    if (!Preferences::isNotify(m_server->getServerGroup()->id(), (*it)))
                        Preferences::addNotify(m_server->getServerGroup()->id(), (*it));
                }
            }
            break;
        }
    } // switch

    if (!pattern.isEmpty())
    {
        pattern.replace("%c",getName());

        QString command;

        if (pattern.contains("%l"))
        {
            QStringList list, partialList;
            int modesCount = m_server->getModesCount();

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
                list.append((*it));

            for (int index = 0; index<list.count(); index+=modesCount)
            {
                command = pattern;
                partialList = list.mid(index, modesCount);
                command = command.replace("%l", partialList.join(" "));
#if QT_VERSION >= 0x040500
                const QString repeatedMode = mode.repeated(partialList.count());
#else
                QString repeatedMode;
                for (int rr = 0; rr < partialList.count(); ++rr)
                    repeatedMode += mode;
#endif
                command = command.replace("%m", repeatedMode);
                if (raw)
                    m_server->queue(command);
                else
                    sendChannelText(command);                
            }

        }
        else
        {
            QStringList patternList = pattern.split('\n', QString::SkipEmptyParts);

            for (QStringList::Iterator it=nickList.begin(); it!=nickList.end(); ++it)
            {
                for (int index = 0; index<patternList.count(); index++)
                {
                    command = patternList[index];
                    command.replace("%u", (*it));

                    if (raw)
                        m_server->queue(command);
                    else
                        sendChannelText(command);
                }
            }
        }
    }
}

// Will be connected to NickListView::doubleClicked()
void Channel::doubleClickCommand(Q3ListViewItem* item)
{
    if(item)
    {
        nicknameListView->clearSelection();
        nicknameListView->setSelected(item, true);
        // TODO: put the quick button code in another function to make reusal more legitimate
        quickButtonClicked(Preferences::self()->channelDoubleClickAction());
    }
}

void Channel::completeNick()
{
    int pos, oldPos;
    QTextCursor cursor = channelInput->textCursor();

    pos = cursor.position();
    oldPos = channelInput->getOldCursorPosition();

    QString line=channelInput->toPlainText();
    QString newLine;
    // Check if completion position is out of range
    if(completionPosition >= nicknameList.count()) completionPosition = 0;

    // Check, which completion mode is active
    char mode = channelInput->getCompletionMode();

    if(mode == 'c')
    {
        line.remove(oldPos, pos - oldPos);
        pos = oldPos;
    }

    // If the cursor is at beginning of line, insert last completion
    if(pos == 0 && !channelInput->lastCompletion().isEmpty())
    {
        QString addStart(Preferences::self()->nickCompleteSuffixStart());
        newLine = channelInput->lastCompletion() + addStart;
        // New cursor position is behind nickname
        pos = newLine.length();
        // Add rest of the line
        newLine += line;
    }
    else
    {
        // remember old cursor position in input field
        channelInput->setOldCursorPosition(pos);
        // remember old cursor position locally
        oldPos = pos;
        // step back to last space or start of line
        while(pos && line[pos-1] != ' ') pos--;
        // copy search pattern (lowercase)
        QString pattern = line.mid(pos, oldPos - pos);
        // copy line to newLine-buffer
        newLine = line;

        // did we find any pattern?
        if(!pattern.isEmpty())
        {
            bool complete = false;
            QString foundNick;

            // try to find matching nickname in list of names
            if(Preferences::self()->nickCompletionMode() == 1 ||
                Preferences::self()->nickCompletionMode() == 2)
            { // Shell like completion
                QStringList found;
                foundNick = nicknameList.completeNick(pattern, complete, found,
                                                      (Preferences::self()->nickCompletionMode() == 2),
                                                      Preferences::self()->nickCompletionCaseSensitive());

                if(!complete && !found.isEmpty())
                {
                    if(Preferences::self()->nickCompletionMode() == 1)
                    {
                        QString nicksFound = found.join(" ");
                        appendServerMessage(i18n("Completion"), i18n("Possible completions: %1.", nicksFound));
                    }
                    else
                    {
                        channelInput->showCompletionList(found);
                    }
                }
            } // Cycle completion
            else if(Preferences::self()->nickCompletionMode() == 0)
            {
                if(mode == '\0') {
                    uint timeStamp = 0;
                    int listPosition = 0;

                    foreach (Nick* nick, nicknameList)
                    {
                        if(nick->getChannelNick()->getNickname().startsWith(pattern, Preferences::self()->nickCompletionCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive) &&
                          (nick->getChannelNick()->timeStamp() > timeStamp))
                        {
                            timeStamp = nick->getChannelNick()->timeStamp();
                            completionPosition = listPosition;
                        }
                        ++listPosition;
                    }
                }

                // remember old nick completion position
                int oldCompletionPosition = completionPosition;
                complete = true;
                QString prefixCharacter = Preferences::self()->prefixCharacter();

                do
                {
                    QString lookNick = nicknameList.at(completionPosition)->getChannelNick()->getNickname();

                    if(!prefixCharacter.isEmpty() && lookNick.contains(prefixCharacter))
                    {
                        lookNick = lookNick.section( prefixCharacter,1 );
                    }

                    if(lookNick.startsWith(pattern, Preferences::self()->nickCompletionCaseSensitive() ? Qt::CaseSensitive : Qt::CaseInsensitive))
                    {
                        foundNick = lookNick;
                    }

                    // increment search position
                    completionPosition++;

                    // wrap around
                    if(completionPosition == nicknameList.count())
                    {
                        completionPosition = 0;
                    }

                    // the search ends when we either find a suitable nick or we end up at the
                    // first search position
                } while((completionPosition != oldCompletionPosition) && foundNick.isEmpty());
            }

            // did we find a suitable nick?
            if(!foundNick.isEmpty())
            {
                // set channel nicks completion mode
                channelInput->setCompletionMode('c');

                // remove pattern from line
                newLine.remove(pos, pattern.length());

                // did we find the nick in the middle of the line?
                if(pos && complete)
                {
                    channelInput->setLastCompletion(foundNick);
                    QString addMiddle = Preferences::self()->nickCompleteSuffixMiddle();
                    newLine.insert(pos, foundNick + addMiddle);
                    pos = pos + foundNick.length() + addMiddle.length();
                }
                // no, it was at the beginning
                else if(complete)
                {
                    channelInput->setLastCompletion(foundNick);
                    QString addStart = Preferences::self()->nickCompleteSuffixStart();
                    newLine.insert(pos, foundNick + addStart);
                    pos = pos + foundNick.length() + addStart.length();
                }
                // the nick wasn't complete
                else
                {
                    newLine.insert(pos, foundNick);
                    pos = pos + foundNick.length();
                }
            }
            // no pattern found, so restore old cursor position
            else pos = oldPos;
        }
    }

    // Set new text and cursor position
    channelInput->setText(newLine);
    cursor.setPosition(pos);
    channelInput->setTextCursor(cursor);
}

// make sure to step back one position when completion ends so the user starts
// with the last complete they made
void Channel::endCompleteNick()
{
    if(completionPosition) completionPosition--;
    else completionPosition=nicknameList.count()-1;
}

void Channel::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.toLower());
}

bool Channel::autoJoin()
{
    if (!m_server->getServerGroup()) return false;

    Konversation::ChannelList channelList = m_server->getServerGroup()->channelList();

    return channelList.contains(channelSettings());
}

void Channel::setAutoJoin(bool autojoin)
{
    if (autojoin && !(autoJoin()))
    {
        Konversation::ChannelSettings before;

        QList<Channel *> channelList = m_server->getChannelList();

        if (channelList.count() > 1)
        {
            QMap<int, Channel*> channelMap;

            int index = -1;
            int ownIndex = m_server->getViewContainer()->getViewIndex(this);

            foreach (Channel* channel, channelList)
            {
                index = m_server->getViewContainer()->getViewIndex(channel);

                if (index && index > ownIndex) channelMap.insert(index, channel);
            }

            if (channelMap.count())
            {
                QMap<int, Channel*>::Iterator it2;
                Channel* channel;

                for (it2 = channelMap.begin(); it2 != channelMap.end(); ++it2)
                {
                    channel = it2.value();

                    if (channel->autoJoin())
                    {
                        before = channel->channelSettings();

                        break;
                    }
                }
            }
        }

        if (m_server->getServerGroup())
            m_server->getServerGroup()->addChannel(channelSettings(), before);
    }
    else
    {
        if (m_server->getServerGroup())
            m_server->getServerGroup()->removeChannel(channelSettings());
    }
}



QString Channel::getPassword()
{
    QString password;

    for (QStringList::const_iterator it = m_modeList.constBegin(); it != m_modeList.constEnd(); ++it)
    {
        if ((*it)[0] == 'k') password = (*it).mid(1);
    }

    if (password.isEmpty() && m_server->getServerGroup())
    {
        Konversation::ChannelList channelSettingsList = m_server->getServerGroup()->channelList();
        Konversation::ChannelSettings channelSettings(getName());
        int index = channelSettingsList.indexOf(channelSettings);
        if(index >= 0)
           password = channelSettingsList.at(index).password();
    }

    return password;
}

const Konversation::ChannelSettings Channel::channelSettings()
{
    Konversation::ChannelSettings channel;

    channel.setName(getName());
    channel.setPassword(getPassword());
    channel.setNotificationsEnabled(notificationsEnabled());

    return channel;
}

void Channel::sendFileMenu()
{
    emit sendFile();
}

void Channel::channelTextEntered()
{
    QString line = channelInput->toPlainText();
    channelInput->clear();

    if(line.toLower().trimmed() == Preferences::self()->commandChar()+"clear")
    {
        textView->clear();
    }
    else if(line.toLower().trimmed() == Preferences::self()->commandChar()+"cycle")
    {
        cycleChannel();
    }
    else
    {
        if(!line.isEmpty())
            sendChannelText(line);
    }
}

void Channel::channelPassthroughCommand()
{
    QString commandChar = Preferences::self()->commandChar();
    QString line = channelInput->toPlainText();

    channelInput->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendChannelText(line);
    }
}

void Channel::sendChannelText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll,m_server->getNickname(),getName(),getPassword(),
            getSelectedNickList(),QString());
    }

    // Send all strings, one after another
    QStringList outList = outputAll.split(QRegExp("[\r\n]+"), QString::SkipEmptyParts);
    for(int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(),output,getName());

        // Is there something we need to display for ourselves?
        if(!result.output.isEmpty())
        {
            if(result.type == Konversation::Action) appendAction(m_server->getNickname(), result.output);
            else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
            else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
            else if(result.type == Konversation::PrivateMessage) appendQuery(result.typeString, result.output, true);
            else append(m_server->getNickname(), result.output);
        }
        else if (result.outputList.count())
        {
            Q_ASSERT(result.type==Konversation::Message);
            for ( QStringList::Iterator it = result.outputList.begin(); it != result.outputList.end(); ++it )
            {
                append(m_server->getNickname(), *it);
            }
        }
        // Send anything else to the server
        if(!result.toServerList.empty())
        {
            m_server->queueList(result.toServerList);
        }
        else
        {
            m_server->queue(result.toServer);
        }
    } // for
}

void Channel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(newNickname));
}

QStringList Channel::getSelectedNickList()
{
    QStringList result;

    if (channelCommand)
        result.append(textView->getContextNick());
    else
    {
        foreach (Nick* nick, nicknameList)
        {
            if(nick->isSelected()) result.append(nick->getChannelNick()->getNickname());
        }
    }

    return result;
}

ChannelNickList Channel::getSelectedChannelNicks()
{
    ChannelNickList result;

    foreach (Nick* nick, nicknameList)
    {
        if(channelCommand)
        {
            if(nick->getChannelNick()->getNickname() == textView->getContextNick())
            {
                result.append(nick->getChannelNick());
                return result;
            }
        }
        else if(nick->isSelected())
            result.append(nick->getChannelNick());
    }

    return result;

}

void Channel::channelLimitChanged()
{
    unsigned int lim=limit->text().toUInt();

    modeButtonClicked(7,lim>0);
}

void Channel::modeButtonClicked(int id, bool on)
{
    char mode[]={'t','n','s','i','p','m','k','l'};
    QString command("MODE %1 %2%3 %4");
    QString args = getPassword();

    if (mode[id] == 'k')
    {
        if (args.isEmpty())
        {
            QPointer<KPasswordDialog> dlg = new KPasswordDialog(this);
            dlg->setPrompt(i18n("Channel Password"));
            if (dlg->exec() && !dlg->password().isEmpty())
            {
                args = dlg->password();
            }
            delete dlg;
        }

    }
    else if(mode[id]=='l')
    {
        if(limit->text().isEmpty() && on)
        {
            bool ok=false;
            // ask user how many nicks should be the limit
            args=KInputDialog::getText(i18n("Nick Limit"),
                i18n("Enter the new nick limit:"),
                limit->text(),                    // will be always "" but what the hell ;)
                &ok,
                this);
            // leave this function if user cancels
            if(!ok) return;
        }
        else if(on)
            args=limit->text();
    }
    // put together the mode command and send it to the server queue
    m_server->queue(command.arg(getName()).arg((on) ? "+" : "-").arg(mode[id]).arg(args));
}

void Channel::quickButtonClicked(const QString &buttonText)
{
    // parse wildcards (toParse,nickname,channelName,nickList,queryName,parameter)
    QString out=m_server->parseWildcards(buttonText,m_server->getNickname(),getName(),getPassword(),getSelectedNickList(),QString());

    // are there any newlines in the definition?
    if (out.contains('\n'))
        sendChannelText(out);
    // single line without newline needs to be copied into input line
    else
        channelInput->setText(out);
}

void Channel::addNickname(ChannelNickPtr channelnick)
{

    QString nickname = channelnick->loweredNickname();

    Nick* nick=0;

    foreach (Nick* lookNick, nicknameList)
    {
        if(lookNick->getChannelNick()->loweredNickname() == nickname)
        {
            nick = lookNick;
            break;
        }
    }

    if(nick == 0)
    {
        fastAddNickname(channelnick);

        if(channelnick->isAnyTypeOfOp())
        {
            adjustOps(1);
        }

        adjustNicks(1);
        requestNickListSort();
    }
    else
    {
        Q_ASSERT(false); // We shouldn't be adding someone that is already in the channel.
    }
}

// Use with caution! Does not sort or check for duplicates!
void Channel::fastAddNickname(ChannelNickPtr channelnick)
{
    Q_ASSERT(channelnick);
    if(!channelnick) return;
    Nick* nick = new Nick(nicknameListView, channelnick);
    // nicks get sorted later
    nicknameList.append(nick);
}

void Channel::nickRenamed(const QString &oldNick, const NickInfo& nickInfo)
{

    /* Did we change our nick name? */
    QString newNick = nickInfo.getNickname();

    if(newNick == m_server->getNickname()) /* Check newNick because  m_server->getNickname() is already updated to new nick */
    {
        setNickname(newNick);
        appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.", newNick), false, true, true);
    }
    else
    {
        /* No, must've been someone else */
        appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.", oldNick, newNick),false);
    }

    nicknameListView->sort();

}

void Channel::joinNickname(ChannelNickPtr channelNick)
{
    if(channelNick->getNickname() == m_server->getNickname())
    {
        m_joined = true;
        emit joined(this);
        appendCommandMessage(i18n("Join"), i18nc("%1 is the channel and %2 is our hostmask",
                             "You have joined the channel %1 (%2).", getName(), channelNick->getHostmask()),false, false, true);
        m_ownChannelNick = channelNick;
        connect(m_ownChannelNick.data(), SIGNAL(channelNickChanged()), SLOT(refreshModeButtons()));
        refreshModeButtons();
        setActive(true);

        //HACK the way the notification priorities work sucks, this forces the tab text color to ungray right now.
        if (m_currentTabNotify == Konversation::tnfNone || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
            KonversationApplication::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);

        KonversationApplication::instance()->notificationHandler()->channelJoin(this,getName());
    }
    else
    {
        QString nick = channelNick->getNickname();
        QString hostname = channelNick->getHostmask();
        appendCommandMessage(i18n("Join"), i18nc("%1 is the nick joining and %2 the hostmask of that nick",
                             "%1 has joined this channel (%2).", nick, hostname),false, false);
        addNickname(channelNick);
    }
}

void Channel::removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit)
{
    QString displayReason = reason;

    if(!displayReason.isEmpty())
    {
        // if the reason contains text markup characters, play it safe and reset all
        if(displayReason.contains(QRegExp("[\\0000-\\0037]")))
            displayReason += "\017";
    }

    if(channelNick->getNickname() == m_server->getNickname())
    {
        //If in the future we can leave a channel, but not close the window, refreshModeButtons() has to be called.
        if (quit)
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Quit"), i18n("You have left this server."), false);
            else
                appendCommandMessage(i18n("Quit"), i18nc("%1 adds the reason", "You have left this server (%1).", displayReason), false);
        }
        else
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Part"), i18n("You have left channel %1.", getName()), false);
            else
                appendCommandMessage(i18n("Part"), i18nc("%1 adds the channel and %2 the reason",
                                     "You have left channel %1 (%2).", getName(), displayReason), false);

        }

        delete this;
    }
    else
    {
        if (quit)
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Quit"), i18n("%1 has left this server.", channelNick->getNickname()), false);
            else
                appendCommandMessage(i18n("Quit"), i18nc("%1 adds the nick and %2 the reason",
                                     "%1 has left this server (%2).", channelNick->getNickname(), displayReason), false);
        }
        else
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Part"), i18n("%1 has left this channel.", channelNick->getNickname()), false);
            else
                appendCommandMessage(i18n("Part"), i18nc("%1 adds the nick and %2 the reason",
                                     "%1 has left this channel (%2).", channelNick->getNickname(), displayReason), false);
        }

        if(channelNick->isAnyTypeOfOp())
        {
            adjustOps(-1);
        }

        adjustNicks(-1);
        Nick* nick = getNickByName(channelNick->getNickname());

        if(nick)
        {
            nicknameList.removeOne(nick);
            delete nick;
        }
        else
        {
            kWarning() << "Nickname " << channelNick->getNickname() << " not found!"<< endl;
        }
    }
}

void Channel::flushPendingNicks()
{
    if (m_processingTimer)
    {
        m_processingTimer->stop();
    }

    while (!m_pendingChannelNickLists.isEmpty())
    {
        processPendingNicks();
    }
}

void Channel::kickNick(ChannelNickPtr channelNick, const QString &kicker, const QString &reason)
{
    QString displayReason = reason;

    if(!displayReason.isEmpty())
    {
        // if the reason contains text markup characters, play it safe and reset all
        if(displayReason.contains(QRegExp("[\\0000-\\0037]")))
            displayReason += "\017";
    }

    if(channelNick->getNickname() == m_server->getNickname())
    {
        if(kicker == m_server->getNickname())
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Kick"), i18n("You have kicked yourself from channel %1.", getName()));
            else
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the channel and %2 the reason",
                                              "You have kicked yourself from channel %1 (%2).", getName(), displayReason));
        }
        else
        {
            if (displayReason.isEmpty())
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the channel, %2 adds the kicker",
                                              "You have been kicked from channel %1 by %2.", getName(), kicker), true);
            }
            else
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the channel, %2 the kicker and %3 the reason",
                                              "You have been kicked from channel %1 by %2 (%3).", getName(), kicker, displayReason), true);
            }

            KonversationApplication::instance()->notificationHandler()->kick(this,getName(), kicker);
        }

        m_joined=false;
        setActive(false);

        //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
        if (m_currentTabNotify == Konversation::tnfNone || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
            KonversationApplication::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);

        return;
    }
    else
    {
        if(kicker == m_server->getNickname())
        {
            if (displayReason.isEmpty())
                appendCommandMessage(i18n("Kick"), i18n("You have kicked %1 from the channel.", channelNick->getNickname()));
            else
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the kicked nick and %2 the reason",
                                     "You have kicked %1 from the channel (%2).", channelNick->getNickname(), displayReason), true);
        }
        else
        {
            if (displayReason.isEmpty())
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the kicked nick, %2 adds the kicker",
                                     "%1 has been kicked from the channel by %2.", channelNick->getNickname(), kicker), true);
            }
            else
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the kicked nick, %2 the kicker and %3 the reason",
                                     "%1 has been kicked from the channel by %2 (%3).", channelNick->getNickname(), kicker, displayReason), true);
            }
        }

        if(channelNick->isAnyTypeOfOp())
            adjustOps(-1);

        adjustNicks(-1);
        Nick* nick = getNickByName(channelNick->getNickname());

        if(nick == 0)
        {
            kWarning() << "Nickname " << channelNick->getNickname() << " not found!"<< endl;
        }
        else
        {
            nicknameList.removeOne(nick);
            delete nick;
        }
    }
}

Nick* Channel::getNickByName(const QString &lookname)
{
    QString lcLookname(lookname.toLower());

    foreach (Nick* nick, nicknameList)
    {
        if(nick->getChannelNick()->loweredNickname() == lcLookname)
            return nick;
    }

    return 0;
}

void Channel::adjustNicks(int value)
{
    if((nicks == 0) && (value <= 0))
    {
        return;
    }

    nicks += value;

    if(nicks < 0)
    {
        nicks = 0;
    }

    emitUpdateInfo();
}

void Channel::adjustOps(int value)
{
    if((ops == 0) && (value <= 0))
    {
        return;
    }

    ops += value;

    if(ops < 0)
    {
        ops = 0;
    }

    emitUpdateInfo();
}

void Channel::emitUpdateInfo()
{
    QString info = getName() + " - ";
    info += i18np("%1 nick", "%1 nicks", numberOfNicks());
    info += i18np(" (%1 op)", " (%1 ops)", numberOfOps());

    emit updateInfo(info);
}

void Channel::setTopic(const QString &newTopic)
{
    appendCommandMessage(i18n("Topic"), i18n("The channel topic is \"%1\".", newTopic));
    QString topic = Konversation::removeIrcMarkup(newTopic);
    topicLine->setText(topic);
    topicAuthorUnknown=true; // if we only get called with a topic, it was a 332, which usually has a 333 next

    // cut off "nickname" and "time_t" portion of the topic before comparing, otherwise the history
    // list will fill up with the same entries while the user only requests the topic to be seen.

    if(m_topicHistory.isEmpty() || (m_topicHistory.first().section(' ', 2) != newTopic))
    {
        m_topicHistory.prepend(QString("%1 "+i18n("unknown")+" %2").arg(QDateTime::currentDateTime().toTime_t()).arg(newTopic));
        emit topicHistoryChanged();
    }
}

void Channel::setTopic(const QString &nickname, const QString &newTopic) // Overloaded
{
    if(nickname == m_server->getNickname())
    {
        appendCommandMessage(i18n("Topic"), i18n("You set the channel topic to \"%1\".", newTopic));
    }
    else
    {
        appendCommandMessage(i18n("Topic"), i18n("%1 sets the channel topic to \"%2\".", nickname, newTopic));
    }

    m_topicHistory.prepend(QString("%1 %2 %3").arg(QDateTime::currentDateTime().toTime_t()).arg(nickname).arg(newTopic));
    QString topic = Konversation::removeIrcMarkup(newTopic);
    topicLine->setText(topic);

    emit topicHistoryChanged();
}

QStringList Channel::getTopicHistory()
{
    return m_topicHistory;
}

QString Channel::getTopic()
{
    return m_topicHistory[0];
}

void Channel::setTopicAuthor(const QString& newAuthor, QDateTime time)
{
    if (time.isNull() || !time.isValid())
        time=QDateTime::currentDateTime();

    if(topicAuthorUnknown && !m_topicHistory.isEmpty())
    {
        m_topicHistory[0] =  QString("%1").arg(time.toTime_t()) + ' ' + newAuthor + ' ' + m_topicHistory[0].section(' ', 2);
        topicAuthorUnknown = false;

        emit topicHistoryChanged();
    }
}

void Channel::updateMode(const QString& sourceNick, char mode, bool plus, const QString &parameter)
{
    //Note for future expansion: doing m_server->getChannelNick(getName(), sourceNick);  may not return a valid channelNickPtr if the
    //mode is updated by the server.

    QString message;
    ChannelNickPtr parameterChannelNick=m_server->getChannelNick(getName(), parameter);

    bool fromMe=false;
    bool toMe=false;

    // remember if this nick had any type of op.
    bool wasAnyOp=false;
    if(parameterChannelNick)
        wasAnyOp=parameterChannelNick->isAnyTypeOfOp();

    if(sourceNick.toLower()==m_server->loweredNickname())
        fromMe=true;
    if(parameter.toLower()==m_server->loweredNickname())
        toMe=true;

    switch(mode)
    {
        case 'q':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel owner privileges to yourself.");
                    else
                        message=i18n("You give channel owner privileges to %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel owner privileges to you.", sourceNick);
                    else
                        message=i18n("%1 gives channel owner privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel owner privileges from yourself.");
                    else
                        message=i18n("You take channel owner privileges from %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel owner privileges from you.", sourceNick);
                    else
                        message=i18n("%1 takes channel owner privileges from %2.", sourceNick, parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOwner(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'a':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel admin privileges to yourself.");
                    else
                        message=i18n("You give channel admin privileges to %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel admin privileges to you.", sourceNick);
                    else
                        message=i18n("%1 gives channel admin privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel admin privileges from yourself.");
                    else
                        message=i18n("You take channel admin privileges from %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel admin privileges from you.", sourceNick);
                    else
                        message=i18n("%1 takes channel admin privileges from %2.", sourceNick, parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOwner(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'o':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel operator privileges to yourself.");
                    else
                        message=i18n("You give channel operator privileges to %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel operator privileges to you.", sourceNick);
                    else
                        message=i18n("%1 gives channel operator privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel operator privileges from yourself.");
                    else
                        message=i18n("You take channel operator privileges from %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel operator privileges from you.", sourceNick);
                    else
                        message=i18n("%1 takes channel operator privileges from %2.", sourceNick, parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setOp(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

        case 'h':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You give channel halfop privileges to yourself.");
                    else
                        message=i18n("You give channel halfop privileges to %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 gives channel halfop privileges to you.", sourceNick);
                    else
                        message=i18n("%1 gives channel halfop privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe)
                        message=i18n("You take channel halfop privileges from yourself.");
                    else
                        message=i18n("You take channel halfop privileges from %1.", parameter);
                }
                else
                {
                    if(toMe)
                        message=i18n("%1 takes channel halfop privileges from you.", sourceNick);
                    else
                        message=i18n("%1 takes channel halfop privileges from %2.", sourceNick, parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setHalfOp(plus);
                emitUpdateInfo();
                nicknameListView->sort();
            }
            break;

	//case 'O': break;

        case 'v':
            if(plus)
            {
                if(fromMe)
                {
                    if(toMe) message=i18n("You give yourself permission to talk.");
                    else     message=i18n("You give %1 permission to talk.", parameter);
                }
                else
                {
                    if(toMe) message=i18n("%1 gives you permission to talk.", sourceNick);
                    else     message=i18n("%1 gives %2 permission to talk.", sourceNick, parameter);
                }
            }
            else
            {
                if(fromMe)
                {
                    if(toMe) message=i18n("You take the permission to talk from yourself.");
                    else     message=i18n("You take the permission to talk from %1.", parameter);
                }
                else
                {
                    if(toMe) message=i18n("%1 takes the permission to talk from you.", sourceNick);
                    else     message=i18n("%1 takes the permission to talk from %2.", sourceNick, parameter);
                }
            }
            if(parameterChannelNick)
            {
                parameterChannelNick->setVoice(plus);
                nicknameListView->sort();
            }
            break;

        case 'c':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'no colors allowed'.");
                else message=i18n("%1 sets the channel mode to 'no colors allowed'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'allow color codes'.");
                else message=i18n("%1 sets the channel mode to 'allow color codes'.", sourceNick);
            }
            break;

        case 'i':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'invite only'.");
                else message=i18n("%1 sets the channel mode to 'invite only'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You remove the 'invite only' mode from the channel.");
                else message=i18n("%1 removes the 'invite only' mode from the channel.", sourceNick);
            }
            modeI->setDown(plus);
            break;

        case 'm':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'moderated'.");
                else message=i18n("%1 sets the channel mode to 'moderated'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'unmoderated'.");
                else message=i18n("%1 sets the channel mode to 'unmoderated'.", sourceNick);
            }
            modeM->setDown(plus);
            break;

        case 'n':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'no messages from outside'.");
                else message=i18n("%1 sets the channel mode to 'no messages from outside'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'allow messages from outside'.");
                else message=i18n("%1 sets the channel mode to 'allow messages from outside'.", sourceNick);
            }
            modeN->setDown(plus);
            break;

        case 'p':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'private'.");
                else message=i18n("%1 sets the channel mode to 'private'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'public'.");
                else message=i18n("%1 sets the channel mode to 'public'.", sourceNick);
            }
            modeP->setDown(plus);
            if(plus) modeS->setDown(false);
            break;

        case 's':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel mode to 'secret'.");
                else message=i18n("%1 sets the channel mode to 'secret'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You set the channel mode to 'visible'.");
                else message=i18n("%1 sets the channel mode to 'visible'.", sourceNick);
            }
            modeS->setDown(plus);
            if(plus) modeP->setDown(false);
            break;

	//case 'r': break;

        case 't':
            if(plus)
            {
                if(fromMe) message=i18n("You switch on 'topic protection'.");
                else message=i18n("%1 switches on 'topic protection'.", sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You switch off 'topic protection'.");
                else message=i18n("%1 switches off 'topic protection'.", sourceNick);
            }
            modeT->setDown(plus);
            break;

        case 'k':
            if(plus)
            {
                if(fromMe) message=i18n("You set the channel key to '%1'.", parameter);
                else message=i18n("%1 sets the channel key to '%2'.", sourceNick, parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the channel key.");
                else message=i18n("%1 removes the channel key.", sourceNick);
            }
            modeK->setDown(plus);
            break;

        case 'l':
            if(plus)
            {
                if(fromMe) message=i18np("You set the channel limit to 1 nick.", "You set the channel limit to %1 nicks.", parameter);
                else message=i18np("%2 sets the channel limit to 1 nick.", "%2 sets the channel limit to %1 nicks.", parameter, sourceNick);
            }
            else
            {
                if(fromMe) message=i18n("You remove the channel limit.");
                else message=i18n("%1 removes the channel limit.", sourceNick);
            }
            modeL->setDown(plus);
            if(plus) limit->setText(parameter);
            else limit->clear();
            break;

        case 'b':
            if(plus)
            {
                if(fromMe) message=i18n("You set a ban on %1.", parameter);
                else message=i18n("%1 sets a ban on %2.", sourceNick, parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the ban on %1.", parameter);
                else message=i18n("%1 removes the ban on %2.", sourceNick, parameter);
            }
            break;

        case 'e':
            if(plus)
            {
                if(fromMe) message=i18n("You set a ban exception on %1.", parameter);
                else message=i18n("%1 sets a ban exception on %2.", sourceNick, parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the ban exception on %1.", parameter);
                else message=i18n("%1 removes the ban exception on %2.", sourceNick, parameter);
            }
            break;

        case 'I':
            if(plus)
            {
                if(fromMe) message=i18n("You set invitation mask %1.", parameter);
                else message=i18n("%1 sets invitation mask %2.", sourceNick, parameter);
            }
            else
            {
                if(fromMe) message=i18n("You remove the invitation mask %1.", parameter);
                else message=i18n("%1 removes the invitation mask %2.", sourceNick, parameter);
            }
            break;
        default:
	    if(plus)
	    {
	        if(fromMe) message=i18n("You set channel mode +%1", mode);
		else message=i18n("%1 sets channel mode +%2", sourceNick, mode);
	    }
	    else
	    {
	        if (fromMe) message=i18n("You set channel mode -%1", mode);
		else message= i18n("%1 sets channel mode -%2", sourceNick, mode);
	    }
    }

    // check if this nick's anyOp-status has changed and adjust ops accordingly
    if(parameterChannelNick)
    {
        if(wasAnyOp && (!parameterChannelNick->isAnyTypeOfOp()))
            adjustOps(-1);
        else if((!wasAnyOp) && parameterChannelNick->isAnyTypeOfOp())
            adjustOps(1);
    }

    if(!message.isEmpty() && !Preferences::self()->useLiteralModes())
    {
        appendCommandMessage(i18n("Mode"),message);
    }

    updateModeWidgets(mode,plus,parameter);
}

void Channel::clearModeList()
{
    QString k;

    // Keep channel password in the backing store, for rejoins.
    for (QStringList::const_iterator it = m_modeList.constBegin(); it != m_modeList.constEnd(); ++it)
    {
        if ((*it)[0] == 'k') k = (*it);
    }

    m_modeList.clear();

    if (!k.isEmpty()) m_modeList << k;

    modeT->setOn(0);
    modeT->setDown(0);

    modeN->setOn(0);
    modeN->setDown(0);

    modeS->setOn(0);
    modeS->setDown(0);

    modeI->setOn(0);
    modeI->setDown(0);

    modeP->setOn(0);
    modeP->setDown(0);

    modeM->setOn(0);
    modeM->setDown(0);

    modeK->setOn(0);
    modeK->setDown(0);

    modeL->setOn(0);
    modeL->setDown(0);

    limit->clear();

    emit modesChanged();
}

void Channel::updateModeWidgets(char mode, bool plus, const QString &parameter)
{
    ModeButton* widget=0;

    if(mode=='t') widget=modeT;
    else if(mode=='n') widget=modeN;
    else if(mode=='s') widget=modeS;
    else if(mode=='i') widget=modeI;
    else if(mode=='p') widget=modeP;
    else if(mode=='m') widget=modeM;
    else if(mode=='k') widget=modeK;
    else if(mode=='l')
    {
        widget=modeL;
        if(plus) limit->setText(parameter);
        else limit->clear();
    }

    if(widget) widget->setOn(plus);

    if(plus)
    {
        m_modeList.append(QString(mode + parameter));
    }
    else
    {
        QStringList removable = m_modeList.filter(QRegExp(QString("^%1.*").arg(mode)));
        foreach(const QString &mode, removable)
        {
            m_modeList.removeOne(mode);
        }
    }
    emit modesChanged();
}

void Channel::updateQuickButtons(const QStringList &newButtonList)
{
    // remove quick buttons from memory and GUI
    qDeleteAll(buttonList);
    buttonList.clear();

    if(buttonsGrid)delete buttonsGrid;

    // the grid that holds the quick action buttons
    buttonsGrid = new Q3Grid(2, nickListButtons);

    // add new quick buttons
    for(int index=0;index<newButtonList.count();index++)
    {
        // generate empty buttons first, text will be added later
        QuickButton* quickButton = new QuickButton(QString(), QString(), buttonsGrid);
        buttonList.append(quickButton);

        connect(quickButton, SIGNAL(clicked(const QString &)), this, SLOT(quickButtonClicked(const QString &)));

        // Get the button definition
        QString buttonText=newButtonList[index];
        // Extract button label
        QString buttonLabel=buttonText.section(',',0,0);
        // Extract button definition
        buttonText=buttonText.section(',',1);

        quickButton->setText(buttonLabel);
        quickButton->setDefinition(buttonText);

        // Add tool tips
        QString toolTip=buttonText.replace('&',"&amp;").
            replace('<',"&lt;").
            replace('>',"&gt;");

        quickButton->setToolTip(toolTip);

        quickButton->show();
    } // for

    // set hide() or show() on grid
    showQuickButtons(Preferences::self()->showQuickButtons());
}

void Channel::showQuickButtons(bool show)
{
    // Qt does not redraw the buttons properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden() || !buttonsGrid)
    {
        quickButtonsChanged=true;
        quickButtonsState=show;
    }
    else
    {
        if(show)
            buttonsGrid->show();
        else
            buttonsGrid->hide();
    }
}

void Channel::showModeButtons(bool show)
{
    // Qt does not redraw the buttons properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        modeButtonsChanged=true;
        modeButtonsState=show;
    }
    else
    {
        if(show)
        {
            topicSplitterHidden = false;
            modeBox->show();
            modeBox->parentWidget()->show();
        }
        else
        {
            modeBox->hide();

            if(topicLine->isHidden())
            {
                topicSplitterHidden = true;
                modeBox->parentWidget()->hide();
            }
        }
    }
}

void Channel::indicateAway(bool show)
{
    // Qt does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

void Channel::showEvent(QShowEvent*)
{
    // If the show quick/mode button settings have changed, apply the changes now
    if(quickButtonsChanged)
    {
        quickButtonsChanged=false;
        showQuickButtons(quickButtonsState);
    }

    if(modeButtonsChanged)
    {
        modeButtonsChanged=false;
        showModeButtons(modeButtonsState);
    }

    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }

    syncSplitters();
}

void Channel::syncSplitters()
{
    QList<int> vertSizes = Preferences::self()->topicSplitterSizes();
    QList<int> horizSizes = Preferences::self()->channelSplitterSizes();

    if (vertSizes.isEmpty())
    {
        vertSizes << m_topicButton->height() << (height() - m_topicButton->height());
        Preferences::self()->setTopicSplitterSizes(vertSizes);
    }

    if (horizSizes.isEmpty())
    {
        int listWidth = nicknameListView->columnWidth(0) + nicknameListView->columnWidth(1);
        horizSizes << (width() - listWidth) << listWidth;
        Preferences::self()->setChannelSplitterSizes(horizSizes);
    }

    m_vertSplitter->setSizes(vertSizes);
    m_horizSplitter->setSizes(horizSizes);

    splittersInitialized = true;
}

void Channel::updateAppearance()
{
    QColor fg,bg,abg;

    if(Preferences::self()->inputFieldsBackgroundColor())
    {
        fg=Preferences::self()->color(Preferences::ChannelMessage);
        bg=Preferences::self()->color(Preferences::TextViewBackground);
        abg=Preferences::self()->color(Preferences::AlternateBackground);
    }
    else
    {
        fg = palette().windowText().color();
        bg = palette().base().color();
        abg = palette().alternateBase().color();
    }

    QPalette channelInputPalette;
    channelInputPalette.setColor(QPalette::WindowText, fg);
    channelInputPalette.setColor(QPalette::Text, fg);
    channelInputPalette.setColor(QPalette::Base, bg);
    channelInput->setPalette(channelInputPalette);

    QPalette limitPalette;
    limitPalette.setColor(QPalette::WindowText, fg);
    limitPalette.setColor(QPalette::Text, fg);
    limitPalette.setColor(QPalette::Base, bg);
    limit->setPalette(limitPalette);

    getTextView()->setPalette(QPalette());

    if(Preferences::self()->showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::self()->color(Preferences::TextViewBackground),
            Preferences::self()->backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::self()->color(Preferences::TextViewBackground),
            QString());
    }

    if (Preferences::self()->customTextFont())
    {
        getTextView()->setFont(Preferences::self()->textFont());
        topicLine->setFont(Preferences::self()->textFont());
        channelInput->setFont(Preferences::self()->textFont());
        nicknameCombobox->setFont(Preferences::self()->textFont());
        limit->setFont(Preferences::self()->textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        topicLine->setFont(KGlobalSettings::generalFont());
        channelInput->setFont(KGlobalSettings::generalFont());
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
        limit->setFont(KGlobalSettings::generalFont());
    }

    QPalette nicknameListViewPalette;
    nicknameListViewPalette.setColor(QPalette::WindowText, fg);
    nicknameListViewPalette.setColor(QPalette::Text, fg);
    nicknameListViewPalette.setColor(QPalette::Base, bg);
    nicknameListViewPalette.setColor(QPalette::AlternateBase, abg);
    nicknameListView->resort();
    nicknameListView->setPalette(nicknameListViewPalette);
    nicknameListView->setAlternateBackground(abg);

    if (Preferences::self()->customListFont())
        nicknameListView->setFont(Preferences::self()->listFont());
    else
        nicknameListView->setFont(KGlobalSettings::generalFont());

    nicknameListView->refresh();

    showModeButtons(Preferences::self()->showModeButtons());
    showNicknameList(Preferences::self()->showNickList());
    showNicknameBox(Preferences::self()->showNicknameBox());
    showTopic(Preferences::self()->showTopic());
    setAutoUserhost(Preferences::self()->autoUserhost());

    updateQuickButtons(Preferences::quickButtonList());

    ChatWindow::updateAppearance();
}

void Channel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick=m_server->getNickname();
    if (oldNick != newNick)
    {
        nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(oldNick));
        changeNickname(newNick);
        // return focus to input line
        channelInput->setFocus();
    }
}

void Channel::changeNickname(const QString& newNickname)
{
    if (!newNickname.isEmpty())
        m_server->queue("NICK "+newNickname);
}

void Channel::resetNickList()
{
    nicknameListView->setUpdatesEnabled(false);
    purgeNicks();
}

void Channel::addPendingNickList(const QStringList& pendingChannelNickList)
{
    if(pendingChannelNickList.isEmpty())
      return;

    if (!m_processingTimer)
    {
        m_processingTimer = new QTimer(this);
        connect(m_processingTimer, SIGNAL(timeout()), this, SLOT(processPendingNicks()));
    }

    m_pendingChannelNickLists << pendingChannelNickList;

    if (!m_processingTimer->isActive())
        m_processingTimer->start(0);
}

void Channel::childAdjustFocus()
{
    channelInput->setFocus();
    refreshModeButtons();
}

void Channel::refreshModeButtons()
{
    bool enable = true;
    if(getOwnChannelNick())
    {
        enable=getOwnChannelNick()->isAnyTypeOfOp();
    } // if not channel nick, then enable is true - fall back to assuming they are op

    //don't disable the mode buttons since you can't then tell if they are enabled or not.
    //needs to be fixed somehow

    /*  modeT->setEnabled(enable);
      modeN->setEnabled(enable);
      modeS->setEnabled(enable);
      modeI->setEnabled(enable);
      modeP->setEnabled(enable);
      modeM->setEnabled(enable);
      modeK->setEnabled(enable);
      modeL->setEnabled(enable);*/
    limit->setEnabled(enable);

    // Tooltips for the ModeButtons
    QString opOnly;
    if(!enable) opOnly = i18n("You have to be an operator to change this.");

    modeT->setToolTip(i18n("Topic can be changed by channel operator only.  %1", opOnly));
    modeN->setToolTip(i18n("No messages to channel from clients on the outside.  %1", opOnly));
    modeS->setToolTip(i18n("Secret channel.  %1", opOnly));
    modeI->setToolTip(i18n("Invite only channel.  %1", opOnly));
    modeP->setToolTip(i18n("Private channel.  %1", opOnly));
    modeM->setToolTip(i18n("Moderated channel.  %1", opOnly));
    modeK->setToolTip(i18n("Protect channel with a password."));
    modeL->setToolTip(i18n("Set user limit to channel."));

}

void Channel::cycleChannel()
{
    closeYourself();
    m_server->sendJoinCommand(getName(), getPassword());
}

void Channel::autoUserhost()
{
    if(Preferences::self()->autoUserhost() && !Preferences::self()->autoWhoContinuousEnabled())
    {
        int limit = 5;

        QString nickString;

        foreach (Nick* nick, getNickList())
        {
            if(nick->getChannelNick()->getHostmask().isEmpty())
            {
                if(limit--) nickString = nickString + nick->getChannelNick()->getNickname() + ' ';
                else break;
            }
        }

        if(!nickString.isEmpty()) m_server->requestUserhost(nickString);
    }
}

void Channel::setAutoUserhost(bool state)
{
    if(state)
    {
        // we can't have automatic resizing with three columns; the hostname column is too wide
        nicknameListView->setHScrollBarMode(Q3ScrollView::Auto);

        // restart userhost timer
        userhostTimer.start(10000);
        // if the column was actually gone (just to be sure) ...
        if(nicknameListView->columns()==2)
        {
            // re-add the hostmask column
            nicknameListView->addColumn(QString());
            nicknameListView->setColumnWidthMode(2,K3ListView::Maximum);
            nicknameListView->setResizeMode(Q3ListView::NoColumn);

            // re-add already known hostmasks
            Q3ListViewItem* item=nicknameListView->itemAtIndex(0);
            while(item)
            {
                Nick* lookNick=getNickByName(item->text(1));
                if(lookNick) item->setText(2,lookNick->getChannelNick()->getHostmask());
                item=item->itemBelow();
            }
        }
    }
    else
    {
        userhostTimer.stop();
        if(nicknameListView->columns()==3) nicknameListView->removeColumn(2);
        nicknameListView->setHScrollBarMode(Q3ScrollView::AlwaysOff);
        // make the nick column resize itself automatically to prevent horizontal scrollbar
        nicknameListView->setResizeMode(Q3ListView::LastColumn);
    }
}

void Channel::scheduleAutoWho() // slot
{
    if(m_whoTimer.isActive())
        m_whoTimer.stop();
    if(Preferences::self()->autoWhoContinuousEnabled())
        m_whoTimer.start(Preferences::self()->autoWhoContinuousInterval() * 1000);
}

void Channel::autoWho()
{
    // don't use auto /WHO when the number of nicks is too large, or get banned.
    if((nicks > Preferences::self()->autoWhoNicksLimit()) ||
       m_server->getInputFilter()->isWhoRequestUnderProcess(getName()))
    {
        scheduleAutoWho();
        return;
    }

    m_server->requestWho(getName());
}

void Channel::fadeActivity()
{
    foreach (Nick *nick,  nicknameList) {
        nick->getChannelNick()->lessActive();
    }
}

QString Channel::getTextInLine()
{
  return channelInput->toPlainText();
}

bool Channel::canBeFrontView()
{
  return true;
}

bool Channel::searchView()
{
  return true;
}

void Channel::appendInputText(const QString& s, bool fromCursor)
{
    if(!fromCursor)
    {
        channelInput->append(s);
    }
    else
    {
        const int position = channelInput->textCursor().position();
        channelInput->textCursor().insertText(s);
        QTextCursor cursor = channelInput->textCursor();
        cursor.setPosition(position + s.length());
        channelInput->setTextCursor(cursor);
    }
}

bool Channel::closeYourself(bool confirm)
{
    int result=KMessageBox::Continue;
    if (confirm)
        result = KMessageBox::warningContinueCancel(this,
            i18n("Do you want to leave %1?", getName()),
            i18n("Leave Channel"),
            KGuiItem(i18n("Leave")),
            KStandardGuiItem::cancel(),
            "QuitChannelTab");

    if (result==KMessageBox::Continue)
    {
        m_server->closeChannel(getName());
        m_server->removeChannel(this);
        Preferences::self()->setSpellChecking(channelInput->checkSpellingEnabled());
        deleteLater();
        return true;
    }
    return false;
}

void Channel::serverOnline(bool online)
{
    setActive(online);
}

//Used to disable functions when not connected, does not necessarily mean the server is offline
void Channel::setActive(bool active)
{
    if (active)
    {
        getTextView()->setNickAndChannelContextMenusEnabled(true);
        nicknameCombobox->setEnabled(true);
    }
    else
    {
        purgeNicks();
        getTextView()->setNickAndChannelContextMenusEnabled(false);
        nicknameCombobox->setEnabled(false);
        topicLine->clear();
        clearModeList();
        clearBanList();
    }
}

void Channel::showTopic(bool show)
{
    if(show)
    {
        topicSplitterHidden = false;
        topicLine->show();
        m_topicButton->show();
        topicLine->parentWidget()->show();
    }
    else
    {
        topicLine->hide();
        m_topicButton->hide();

        if(modeBox->isHidden())
        {
            topicSplitterHidden = true;
            topicLine->parentWidget()->hide();
        }
    }
}

void Channel::processPendingNicks()
{
    QString nickname = m_pendingChannelNickLists.first()[m_currentIndex];

    bool admin = false;
    bool owner = false;
    bool op = false;
    bool halfop = false;
    bool voice = false;

    // Remove possible mode characters from nickname and store the resulting mode
    m_server->mangleNicknameWithModes(nickname, admin, owner, op, halfop, voice);

    // TODO: make these an enumeration in KApplication or somewhere, we can use them as well
    unsigned int mode = (admin  ? 16 : 0) +
                        (owner  ?  8 : 0) +
                        (op     ?  4 : 0) +
                        (halfop ?  2 : 0) +
                        (voice  ?  1 : 0);

    // Check if nick is already in the nicklist
    if (!getNickByName(nickname))
    {
        ChannelNickPtr nick = m_server->addNickToJoinedChannelsList(getName(), nickname);
        Q_ASSERT(nick);
        nick->setMode(mode);

        fastAddNickname(nick);

        if (nick->isAdmin() || nick->isOwner() || nick->isOp() || nick->isHalfOp())
            m_opsToAdd++;

        m_currentIndex++;
    }
    else
    {
        m_pendingChannelNickLists.first().pop_front();
    }

    if (m_pendingChannelNickLists.first().count() <= m_currentIndex)
    {
        adjustNicks(m_pendingChannelNickLists.first().count());
        adjustOps(m_opsToAdd);
        m_pendingChannelNickLists.pop_front();
        m_currentIndex = 0;
        m_opsToAdd = 0;
    }

    if (m_pendingChannelNickLists.isEmpty())
    {
        m_processingTimer->stop();
        nicknameListView->sort();
        sortNickList();
        nicknameListView->setUpdatesEnabled(true);
        nicknameListView->triggerUpdate();
    }
}

void Channel::setChannelEncoding(const QString& encoding) // virtual
{
    Preferences::setChannelEncoding(m_server->getDisplayName(), getName(), encoding);
}

QString Channel::getChannelEncoding() // virtual
{
    return Preferences::channelEncoding(m_server->getDisplayName(), getName());
}

QString Channel::getChannelEncodingDefaultDesc()  // virtual
{
    return i18n("Identity Default ( %1 )", getServer()->getIdentity()->getCodecName());
}

void Channel::showNicknameBox(bool show)
{
    if(show)
    {
        nicknameCombobox->show();
    }
    else
    {
        nicknameCombobox->hide();
    }
}

void Channel::showNicknameList(bool show)
{
    if (show)
    {
        channelSplitterHidden = false;
        nickListButtons->show();
    }
    else
    {
        channelSplitterHidden = true;
        nickListButtons->hide();
    }
}

void Channel::requestNickListSort()
{
    if(!m_delayedSortTimer)
    {
        m_delayedSortTimer = new QTimer(this);
        m_delayedSortTimer->setSingleShot(true);
        connect(m_delayedSortTimer, SIGNAL(timeout()), this, SLOT(sortNickList()));
    }

    if(!m_delayedSortTimer->isActive())
    {
        m_delayedSortTimer->start(1000);
    }
}

void Channel::sortNickList()
{
    qSort(nicknameList.begin(), nicknameList.end(), nickLessThan);
    nicknameListView->resort();

    if(m_delayedSortTimer)
    {
        m_delayedSortTimer->stop();
    }
}

bool Channel::eventFilter(QObject* watched, QEvent* e)
{
    if((watched == nicknameListView) && (e->type() == QEvent::Resize) && splittersInitialized && isVisible())
    {
        if (!topicSplitterHidden && !channelSplitterHidden)
        {
            Preferences::self()->setChannelSplitterSizes(m_horizSplitter->sizes());
            Preferences::self()->setTopicSplitterSizes(m_vertSplitter->sizes());
        }
        if (!topicSplitterHidden && channelSplitterHidden)
        {
            Preferences::self()->setTopicSplitterSizes(m_vertSplitter->sizes());
        }
        if (!channelSplitterHidden && topicSplitterHidden)
        {
            Preferences::self()->setChannelSplitterSizes(m_horizSplitter->sizes());
        }
    }

    return ChatWindow::eventFilter(watched, e);
}

void Channel::addBan(const QString& ban)
{
    for ( QStringList::iterator it = m_BanList.begin(); it != m_BanList.end(); ++it )
    {
        if ((*it).section(' ', 0, 0) == ban.section(' ', 0, 0))
        {
            // Ban is already in list.
            it = m_BanList.erase(it);

            emit banRemoved(ban.section(' ', 0, 0));
            if (it == m_BanList.end())
                break;
        }
    }

    m_BanList.prepend(ban);

    emit banAdded(ban);
}

void Channel::removeBan(const QString& ban)
{
  foreach(const QString &string, m_BanList)
  {
    if (string.section(' ', 0, 0) == ban)
    {
      m_BanList.removeOne(string);

      emit banRemoved(ban);
    }
  }
}

void Channel::clearBanList()
{
  m_BanList.clear();

  emit banListCleared();
}

void Channel::append(const QString& nickname, const QString& message)
{
    if(nickname != getServer()->getNickname()) {
        Nick* nick = getNickByName(nickname);

        if(nick) {
            nick->getChannelNick()->setTimeStamp(QDateTime::currentDateTime().toTime_t());
        }
    }

    ChatWindow::append(nickname, message);
    nickActive(nickname);
}

void Channel::appendAction(const QString& nickname, const QString& message)
{
    if(nickname != getServer()->getNickname()) {
        Nick* nick = getNickByName(nickname);

        if(nick) {
            nick->getChannelNick()->setTimeStamp(QDateTime::currentDateTime().toTime_t());
        }
    }

    ChatWindow::appendAction(nickname, message);
    nickActive(nickname);
}

void Channel::nickActive(const QString& nickname) //FIXME reported to crash, can't reproduce
{
    ChannelNickPtr nick=getChannelNick(nickname);
    //XXX Would be nice to know why it can be null here...
    if (nick) {
        nick->moreActive();
        sortNickList(); //FIXME: no need to completely resort, we can just see if this particular nick should move
    }
}

//
// NickList
//

NickList::NickList() : QList<Nick*>()
{
}

QString NickList::completeNick(const QString& pattern, bool& complete, QStringList& found,
			       bool skipNonAlfaNum, bool caseSensitive)
{
    found.clear();
    QString prefix('^');
    QString newNick;
    QString prefixCharacter = Preferences::self()->prefixCharacter();
    NickList foundNicks;

    if((pattern.contains(QRegExp("^(\\d|\\w)"))) && skipNonAlfaNum)
    {
        prefix = "^([^\\d\\w]|[\\_]){0,}";
    }

    QRegExp regexp(prefix + QRegExp::escape(pattern));
    regexp.setCaseSensitivity(caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

    foreach (Nick* nick, *this)
    {
        newNick = nick->getChannelNick()->getNickname();

        if(!prefix.isEmpty() && newNick.contains(prefixCharacter))
        {
            newNick = newNick.section( prefixCharacter,1 );
        }

        if(newNick.contains(regexp))
        {
            foundNicks.append(nick);
        }
    }

    qSort(foundNicks.begin(), foundNicks.end(), nickTimestampLessThan);

    foreach (Nick *nick, foundNicks)
    {
        found.append(nick->getChannelNick()->getNickname());
    }

    if(found.count() > 1)
    {
        bool ok = true;
        int patternLength = pattern.length();
        QString firstNick = found[0];
        int firstNickLength = firstNick.length();
        int foundCount = found.count();

        while(ok && ((patternLength) < firstNickLength))
        {
            ++patternLength;
            QStringList tmp = found.filter(firstNick.left(patternLength), caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

            if(tmp.count() != foundCount)
            {
                ok = false;
                --patternLength;
            }
        }

        complete = false;
        return firstNick.left(patternLength);
    }
    else if(found.count() == 1)
    {
        complete = true;
        return found[0];
    }

    return QString();
}

bool NickList::containsNick(const QString& nickname)
{
    foreach (Nick* nick, *this)
    {
        if (nick->getChannelNick()->getNickname()==nickname)
            return true;
    }

    return false;
}

#include "channel.moc"

// kate: space-indent on; tab-width 4; indent-width 4; mixed-indent off; replace-tabs on;
// vim: set et sw=4 ts=4 cino=l1,cs,U1:
