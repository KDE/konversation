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
#include "channeloptionsdialog.h"
#include "application.h"
#include "server.h"
#include "nick.h"
#include "nicklistview.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"
#include "ircviewbox.h"
#include "ircview.h"
#include "awaylabel.h"
#include "topiclabel.h"
#include "topichistorymodel.h"
#include "notificationhandler.h"
#include "viewcontainer.h"

#include <QSplitter>
#include <QTimer>
#include <QToolButton>
#include <QHeaderView>

#include <KLineEdit>
#include <KInputDialog>
#include <KPasswordDialog>
#include <KMessageBox>
#include <KIconLoader>
#include <KVBox>
#include <KHBox>
#include <KComboBox>


#define DELAYED_SORT_TRIGGER    10

using namespace Konversation;

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

Channel::Channel(QWidget* parent, const QString& _name) : ChatWindow(parent)
{
    // init variables

    //HACK I needed the channel name at time of setServer, but setName needs m_server..
    //     This effectively assigns the name twice, but none of the other logic has been moved or updated.
    name=_name;
    m_ownChannelNick = 0;
    m_optionsDialog = NULL;
    m_delayedSortTimer = 0;
    m_delayedSortTrigger = 0;
    m_processedNicksCount = 0;
    m_processedOpsCount = 0;
    m_initialNamesReceived = false;
    nicks = 0;
    ops = 0;
    completionPosition = 0;
    nickChangeDialog = 0;
    m_nicknameListViewTextChanged = 0;

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

    setType(ChatWindow::Channel);

    setChannelEncodingSupported(true);

    // Build some size policies for the widgets
    QSizePolicy hfixed = QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QSizePolicy hmodest = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    QSizePolicy vfixed = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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
    topicLine->setContextMenuOptions(IrcContextMenus::ShowChannelActions | IrcContextMenus::ShowLogAction, true);
    topicLine->setChannelName(getName());
    topicLine->setWordWrap(true);
    topicLine->setWhatsThis(i18n("<qt><p>Every channel on IRC has a topic associated with it.  This is simply a message that everybody can see.</p><p>If you are an operator, or the channel mode <em>'T'</em> has not been set, then you can change the topic by clicking the Edit Channel Properties button to the left of the topic.  You can also view the history of topics there.</p></qt>"));
    connect(topicLine, SIGNAL(setStatusBarTempText(QString)), this, SIGNAL(setStatusBarTempText(QString)));
    connect(topicLine, SIGNAL(clearStatusBarTempText()), this, SIGNAL(clearStatusBarTempText()));

    m_topicHistory = new TopicHistoryModel(this);
    connect(m_topicHistory, SIGNAL(currentTopicChanged(QString)), topicLine, SLOT(setText(QString)));

    topicLayout->addWidget(m_topicButton, 0, 0);
    topicLayout->addWidget(topicLine, 0, 1, -1, 1);

    // The box holding the channel modes
    modeBox = new KHBox(topicWidget);
    modeBox->hide();
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
    IRCViewBox* ircViewBox = new IRCViewBox(m_horizSplitter);
    m_horizSplitter->setStretchFactor(m_horizSplitter->indexOf(ircViewBox), 1);
    setTextView(ircViewBox->ircView());
    ircViewBox->ircView()->setContextMenuOptions(IrcContextMenus::ShowChannelActions, true);

    // The box that holds the Nick List and the quick action buttons
    nickListButtons = new KVBox(m_horizSplitter);
    m_horizSplitter->setStretchFactor(m_horizSplitter->indexOf(nickListButtons), 0);
    nickListButtons->setSpacing(spacing());

    nicknameListView=new NickListView(nickListButtons, this);
    nicknameListView->installEventFilter(this);

    // initialize buttons grid, will be set up in updateQuickButtons
    m_buttonsGrid = 0;

    // The box holding the Nickname button and Channel input
    commandLineBox = new KHBox(this);
    commandLineBox->setSpacing(spacing());

    nicknameCombobox = new KComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->setSizeAdjustPolicy(KComboBox::AdjustToContents);
    KLineEdit* nicknameComboboxLineEdit = qobject_cast<KLineEdit*>(nicknameCombobox->lineEdit());
    if (nicknameComboboxLineEdit) nicknameComboboxLineEdit->setClearButtonShown(false);
    nicknameCombobox->setWhatsThis(i18n("<qt><p>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.</p><p>You can edit the alternative nicknames from the <em>Identities</em> option in the <em>Settings</em> menu.</p></qt>"));

    awayLabel = new AwayLabel(commandLineBox);
    awayLabel->hide();
    cipherLabel = new QLabel(commandLineBox);
    cipherLabel->hide();
    cipherLabel->setPixmap(KIconLoader::global()->loadIcon("document-encrypt", KIconLoader::Toolbar));
    m_inputBar = new IRCInput(commandLineBox);

    getTextView()->installEventFilter(m_inputBar);
    topicLine->installEventFilter(m_inputBar);
    m_inputBar->installEventFilter(this);

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

    connect(m_inputBar,SIGNAL (submit()),this,SLOT (channelTextEntered()) );
    connect(m_inputBar,SIGNAL (envelopeCommand()),this,SLOT (channelPassthroughCommand()) );
    connect(m_inputBar,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
    connect(m_inputBar,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
    connect(m_inputBar,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

    connect(getTextView(), SIGNAL(textPasted(bool)), m_inputBar, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),m_inputBar,SLOT (setFocus()) );
    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(QString)),this,SLOT (sendText(QString)) );

    connect(nicknameListView,SIGNAL (itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT (doubleClickCommand(QTreeWidgetItem*,int)) );
    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));

    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (editingFinished()),this,SLOT(nicknameComboboxChanged()));


    connect(&userhostTimer,SIGNAL (timeout()),this,SLOT (autoUserhost()));

    m_whoTimer.setSingleShot(true);
    connect(&m_whoTimer, SIGNAL(timeout()), this, SLOT(autoWho()));
    connect(Application::instance(), SIGNAL(appearanceChanged()), this, SLOT(updateAutoWho()));

    // every 5 minutes decrease everyone's activity by 1 unit
    m_fadeActivityTimer.start(5*60*1000);

    connect(&m_fadeActivityTimer, SIGNAL(timeout()), this, SLOT(fadeActivity()));

    updateAppearance();

    #ifdef HAVE_QCA2
    m_cipher = 0;
    #endif
    //FIXME JOHNFLUX
    // connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL(addressBookChanged(AddressBook*)), this, SLOT(slotLoadAddressees()) );
    // connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));

    // Setup delayed sort timer
    m_delayedSortTimer = new QTimer(this);
    m_delayedSortTimer->setSingleShot(true);
    connect(m_delayedSortTimer, SIGNAL(timeout()), this, SLOT(delayedSortNickList()));
}

//FIXME there is some logic in setLogfileName that needs to be split out and called here if the server display name gets changed
void Channel::setServer(Server* server)
{
    if (m_server != server)
    {
        connect(server, SIGNAL(connectionStateChanged(Server*,Konversation::ConnectionState)),
                SLOT(connectionStateChanged(Server*,Konversation::ConnectionState)));
        connect(server, SIGNAL(nickInfoChanged()),
                this, SLOT(updateNickInfos()));
        connect(server, SIGNAL(channelNickChanged(QString)),
                this, SLOT(updateChannelNicks(QString)));
    }

    ChatWindow::setServer(server);
    if (!server->getKeyForRecipient(getName()).isEmpty())
        cipherLabel->show();
    topicLine->setServer(server);
    refreshModeButtons();
    nicknameCombobox->setModel(m_server->nickListModel());

    connect(awayLabel, SIGNAL(unaway()), m_server, SLOT(requestUnaway()));
    connect(awayLabel, SIGNAL(awayMessageChanged(QString)), m_server, SLOT(requestAway(QString)));
}

void Channel::connectionStateChanged(Server* server, Konversation::ConnectionState state)
{
    if (server == m_server)
    {
        if (state !=  Konversation::SSConnected)
        {
            m_joined = false;

            ViewContainer* viewContainer = Application::instance()->getMainWindow()->getViewContainer();

            //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
            if (viewContainer->getFrontView() == this
                || m_currentTabNotify == Konversation::tnfNone
                || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
            {
               viewContainer->unsetViewNotification(this);
            }
        }
    }
}

void Channel::setEncryptedOutput(bool e)
{
#ifdef HAVE_QCA2
    if (e)
    {
        cipherLabel->show();

        if  (!getCipher()->setKey(m_server->getKeyForRecipient(getName())))
            return;

        m_topicHistory->setCipher(getCipher());

        topicLine->setText(m_topicHistory->currentTopic());
    }
    else
    {
        cipherLabel->hide();
        m_topicHistory->clearCipher();
        topicLine->setText(m_topicHistory->currentTopic());

    }
#else
    Q_UNUSED(e)
#endif
}

Channel::~Channel()
{
    kDebug() << "(" << getName() << ")";

    // Purge nickname list
    purgeNicks();
    kDebug() << "Nicks purged.";

    // Unlink this channel from channel list
    m_server->removeChannel(this);
    kDebug() << "Channel removed.";

    if (m_recreationScheduled)
    {
        QMetaObject::invokeMethod(m_server, "sendJoinCommand", Qt::QueuedConnection,
            Q_ARG(QString, getName()), Q_ARG(QString, getPassword()));
    }
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

bool Channel::log()
{
    return ChatWindow::log() && !Preferences::self()->privateOnly();
}

ChannelNickPtr Channel::getOwnChannelNick() const
{
    return m_ownChannelNick;
}

ChannelNickPtr Channel::getChannelNick(const QString &ircnick) const
{
    return m_server->getChannelNick(getName(), ircnick);
}

void Channel::purgeNicks()
{
    m_ownChannelNick = 0;

    // Purge nickname list
    qDeleteAll(nicknameList);
    nicknameList.clear();
    m_nicknameNickHash.clear();

    // Execute this otherwise it may crash trying to access
    // deleted nicks
    nicknameListView->executeDelayedItemsLayout();

    // clear stats counter
    nicks=0;
    ops=0;
}

void Channel::showOptionsDialog()
{
    if (!m_optionsDialog)
        m_optionsDialog = new Konversation::ChannelOptionsDialog(this);

    m_optionsDialog->show();
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
            sendText(line);
        }
    }
}

// Will be connected to NickListView::doubleClicked()
void Channel::doubleClickCommand(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    if(item)
    {
        nicknameListView->clearSelection();
        item->setSelected(true);
        // TODO: put the quick button code in another function to make reusal more legitimate
        quickButtonClicked(Preferences::self()->channelDoubleClickAction());
    }
}

void Channel::completeNick()
{
    int pos, oldPos;
    QTextCursor cursor = m_inputBar->textCursor();

    pos = cursor.position();
    oldPos = m_inputBar->getOldCursorPosition();

    QString line=m_inputBar->toPlainText();
    QString newLine;
    // Check if completion position is out of range
    if(completionPosition >= nicknameList.count()) completionPosition = 0;

    // Check, which completion mode is active
    char mode = m_inputBar->getCompletionMode();

    if(mode == 'c')
    {
        line.remove(oldPos, pos - oldPos);
        pos = oldPos;
    }

    // If the cursor is at beginning of line, insert last completion if the nick is still around
    if(pos == 0 && !m_inputBar->lastCompletion().isEmpty() && nicknameList.containsNick(m_inputBar->lastCompletion()))
    {
        QString addStart(Preferences::self()->nickCompleteSuffixStart());
        newLine = m_inputBar->lastCompletion() + addStart;
        // New cursor position is behind nickname
        pos = newLine.length();
        // Add rest of the line
        newLine += line;
    }
    else
    {
        // remember old cursor position in input field
        m_inputBar->setOldCursorPosition(pos);
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
                        m_inputBar->showCompletionList(found);
                    }
                }
            } // Cycle completion
            else if(Preferences::self()->nickCompletionMode() == 0 && !nicknameList.isEmpty())
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
                m_inputBar->setCompletionMode('c');

                // remove pattern from line
                newLine.remove(pos, pattern.length());

                // did we find the nick in the middle of the line?
                if(pos && complete)
                {
                    m_inputBar->setLastCompletion(foundNick);
                    QString addMiddle = Preferences::self()->nickCompleteSuffixMiddle();
                    newLine.insert(pos, foundNick + addMiddle);
                    pos = pos + foundNick.length() + addMiddle.length();
                }
                // no, it was at the beginning
                else if(complete)
                {
                    m_inputBar->setLastCompletion(foundNick);
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
    m_inputBar->setText(newLine);
    cursor.setPosition(pos);
    m_inputBar->setTextCursor(cursor);
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
    QString line = m_inputBar->toPlainText();

    m_inputBar->clear();

    if (!line.isEmpty()) sendText(sterilizeUnicode(line));
}

void Channel::channelPassthroughCommand()
{
    QString commandChar = Preferences::self()->commandChar();
    QString line = m_inputBar->toPlainText();

    m_inputBar->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendText(sterilizeUnicode(line));
    }
}

void Channel::sendText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);

    // replace aliases and wildcards
    m_server->getOutputFilter()->replaceAliases(outputAll);

    // Send all strings, one after another
    QStringList outList = outputAll.split(QRegExp("[\r\n]+"), QString::SkipEmptyParts);
    for(int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, getName(), this);

        // Is there something we need to display for ourselves?
        if(!result.output.isEmpty())
        {
            if(result.type == Konversation::Action) appendAction(m_server->getNickname(), result.output);
            else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
            else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
            else if(result.type == Konversation::PrivateMessage) msgHelper(result.typeString, result.output);
            else append(m_server->getNickname(), result.output);
        }
        else if (result.outputList.count())
        {
            if (result.type == Konversation::Message)
            {
                QStringListIterator it(result.outputList);

                while (it.hasNext())
                    append(m_server->getNickname(), it.next());
            }
            else if (result.type == Konversation::Action)
            {
                for (int i = 0; i < result.outputList.count(); ++i)
                {
                    if (i == 0)
                        appendAction(m_server->getNickname(), result.outputList.at(i));
                    else
                        append(m_server->getNickname(), result.outputList.at(i));
                }
            }
        }

        // Send anything else to the server
        if (!result.toServerList.empty())
            m_server->queueList(result.toServerList);
        else
            m_server->queue(result.toServer);
    }
}

void Channel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(newNickname));
}

QStringList Channel::getSelectedNickList()
{
    QStringList selectedNicks;

    foreach (Nick* nick, nicknameList)
    {
        if (nick->isSelected())
            selectedNicks << nick->getChannelNick()->getNickname();
    }

    return selectedNicks;
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
            args=KInputDialog::getText(i18n("Channel User Limit"),
                i18n("Enter the new user limit for the channel:"),
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
    QString out=m_server->parseWildcards(buttonText,m_server->getNickname(),getName(),getPassword(),getSelectedNickList(), m_inputBar->toPlainText());

    // are there any newlines in the definition?
    if (out.contains('\n'))
        sendText(out);
    // single line without newline needs to be copied into input line
    else
        m_inputBar->setText(out, true);
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

    if (nick == 0)
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
        kWarning() << "Nickname " << channelnick->getNickname() << " has not been added as it is already in the nickname list."<< endl;
}

// Use with caution! Does not check for duplicates or may not
// sort if delayed sorting is in effect.
void Channel::fastAddNickname(ChannelNickPtr channelnick, Nick *nick)
{
    Q_ASSERT(channelnick);
    if(!channelnick) return;

    if (!nick || !nick->treeWidget())
    {
        // Deal with nicknameListView now (creating nick if necessary)
        NickListView::NoSorting noSorting(nicknameListView);
        int index = nicknameListView->topLevelItemCount();

        // Append nick to the lists
        if (nick)
        {
            nicknameListView->addTopLevelItem(nick);
        }
        else
        {
            nick = new Nick(nicknameListView, this, channelnick);
            m_nicknameListViewTextChanged |= 0xFF; // new nick, text changed.
        }

        if (!m_delayedSortTimer->isActive()) {
            // Find its right place and insert where it belongs
            int newindex = nicknameListView->findLowerBound(*nick);
            if (newindex != index) {
                if (newindex >= index)
                    newindex--;
                nicknameListView->takeTopLevelItem(index);
                nicknameListView->insertTopLevelItem(newindex, nick);
            }
        }
        // Otherwise it will be sorted by delayed sort.
    }

    // Now deal with nicknameList
    if (m_delayedSortTimer->isActive())
    {
        // nicks get sorted later
        nicknameList.append(nick);
    } else {
        NickList::iterator it = qLowerBound(nicknameList.begin(), nicknameList.end(), nick, nickLessThan);
        nicknameList.insert(it, nick);
    }

    m_nicknameNickHash.insert (channelnick->loweredNickname(), nick);
}

/* Determines whether Nick/Part/Join event should be shown or skipped based on user settings. */
bool Channel::shouldShowEvent(ChannelNickPtr channelNick)
{
    if (Preferences::self()->hideUnimportantEvents())
    {
        if (channelNick && Preferences::self()->hideUnimportantEventsExcludeActive())
        {
            uint activityThreshold = 3600;

            if (Preferences::self()->hideUnimportantEventsExcludeActiveThreshold() == 0) // last 10 minutes
                activityThreshold = 600;
            if (Preferences::self()->hideUnimportantEventsExcludeActiveThreshold() == 1) // last hour
                activityThreshold = 3600;
            else if (Preferences::self()->hideUnimportantEventsExcludeActiveThreshold() == 2) // last day
                activityThreshold = 86400;
            else if (Preferences::self()->hideUnimportantEventsExcludeActiveThreshold() == 3) // last week
                activityThreshold = 604800;

            if (m_server->isWatchedNick(channelNick->getNickname()))
                return true; // nick is on our watched list, so we probably want to see the event
            else if (channelNick->timeStamp()+activityThreshold > QDateTime::currentDateTime().toTime_t())
                return true; // the nick has spoken within activity threshold
            else
                return false;
        }
        else
            return false; // if hideUnimportantEventsExcludeActive is off, we hide all events
    }
    else
        return true; // if hideUnimportantEvents is off we don't care and just show the event
}

void Channel::nickRenamed(const QString &oldNick, const NickInfo& nickInfo)
{
    QString newNick = nickInfo.getNickname();
    Nick *nick = getNickByName(oldNick);
    bool displayCommandMessage;

    if (Preferences::self()->hideUnimportantEventsExcludeActive() && m_server->isWatchedNick(oldNick))
        displayCommandMessage = true; // this is for displaying watched people NICK events both ways (watched->unwatched and unwatched->watched)
    else if (nick)
        displayCommandMessage = shouldShowEvent(nick->getChannelNick());
    else
        displayCommandMessage = shouldShowEvent(ChannelNickPtr()); // passing null pointer

    /* Did we change our nick name? */
    if(newNick == m_server->getNickname()) /* Check newNick because  m_server->getNickname() is already updated to new nick */
    {
        setNickname(newNick);
        if (displayCommandMessage)
            appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.", newNick), true, true);
    }
    else if (displayCommandMessage)
    {
        /* No, must've been someone else */
        appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.", oldNick, newNick));
    }

    if (nick)
    {
        m_nicknameNickHash.remove(oldNick.toLower());
        m_nicknameNickHash.insert(newNick.toLower(), nick);

        repositionNick(nick);
    }
}

void Channel::joinNickname(ChannelNickPtr channelNick)
{
    bool displayCommandMessage = shouldShowEvent(channelNick);

    if(channelNick->getNickname() == m_server->getNickname())
    {
        m_joined = true;
        emit joined(this);
        if (displayCommandMessage)
            appendCommandMessage(i18nc("Message type", "Join"), i18nc("%1 = our hostmask, %2 = channel",
                                 "You (%1) have joined the channel %2.", channelNick->getHostmask(), getName()), false, true);

        // Prepare for impending NAMES.
        purgeNicks();
        nicknameListView->setUpdatesEnabled(false);

        m_ownChannelNick = channelNick;
        refreshModeButtons();
        setActive(true);

        ViewContainer* viewContainer = Application::instance()->getMainWindow()->getViewContainer();

        //HACK the way the notification priorities work sucks, this forces the tab text color to ungray right now.
        if (viewContainer->getFrontView() == this
            || m_currentTabNotify == Konversation::tnfNone
            || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
        {
            Application::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);
        }

        Application::instance()->notificationHandler()->channelJoin(this,getName());
    }
    else
    {
        QString nick = channelNick->getNickname();
        QString hostname = channelNick->getHostmask();
        if (displayCommandMessage)
            appendCommandMessage(i18nc("Message type", "Join"), i18nc("%1 is the nick joining and %2 the hostmask of that nick",
                                 "%1 (%2) has joined this channel.", nick, hostname), false);
        addNickname(channelNick);
    }
}

void Channel::removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit)
{
    bool displayCommandMessage = shouldShowEvent(channelNick);

    QString displayReason = reason;

    if(!displayReason.isEmpty())
    {
        // if the reason contains text markup characters, play it safe and reset all
        if (hasIRCMarkups(displayReason))
            displayReason += "\017";
    }

    if(channelNick->getNickname() == m_server->getNickname())
    {
        if (displayCommandMessage)
        {
            //If in the future we can leave a channel, but not close the window, refreshModeButtons() has to be called.
            if (quit)
            {
                if (displayReason.isEmpty())
                    appendCommandMessage(i18nc("Message type", "Quit"), i18n("You (%1) have left this server.", channelNick->getHostmask()));
                else
                    appendCommandMessage(i18nc("Message type", "Quit"), i18nc("%1 = our hostmask, %2 = reason", "You (%1) have left this server (%2).",
                        channelNick->getHostmask(), displayReason), false);
            }
            else
            {
                if (displayReason.isEmpty())
                    appendCommandMessage(i18nc("Message type", "Part"), i18n("You have left channel %1.", getName()));
                else
                    appendCommandMessage(i18nc("Message type", "Part"), i18nc("%1 = our hostmask, %2 = channel, %3 = reason",
                        "You (%1) have left channel %2 (%3).", channelNick->getHostmask(), getName(), displayReason), false);
            }
        }

        delete this;
    }
    else
    {
        if (displayCommandMessage)
        {
            if (quit)
            {
                if (displayReason.isEmpty())
                    appendCommandMessage(i18nc("Message type", "Quit"), i18n("%1 (%2) has left this server.", channelNick->getNickname(),
                        channelNick->getHostmask()), false);
                else
                    appendCommandMessage(i18nc("Message type", "Quit"), i18nc("%1 = nick, %2 = hostname, %3 = reason",
                        "%1 (%2) has left this server (%3).", channelNick->getNickname(), channelNick->getHostmask(), displayReason), false);
            }
            else
            {
                if (displayReason.isEmpty())
                    appendCommandMessage(i18nc("Message type", "Part"), i18n("%1 (%2) has left this channel.", channelNick->getNickname(),
                        channelNick->getHostmask()), false);
                else
                    appendCommandMessage(i18nc("Message type", "Part"), i18nc("%1 = nick, %2 = hostmask, %3 = reason",
                        "%1 (%2) has left this channel (%3).", channelNick->getNickname(), channelNick->getHostmask(), displayReason), false);
            }
        }

        if(channelNick->isAnyTypeOfOp())
        {
            adjustOps(-1);
        }

        adjustNicks(-1);
        Nick* nick = getNickByName(channelNick->loweredNickname());

        if(nick)
        {
            nicknameList.removeOne(nick);
            m_nicknameNickHash.remove(channelNick->loweredNickname());
            delete nick;
            // Execute this otherwise it may crash trying to access deleted nick
            nicknameListView->executeDelayedItemsLayout();
        }
        else
        {
            kWarning() << "Nickname " << channelNick->getNickname() << " not found!"<< endl;
        }
    }
}

void Channel::flushNickQueue()
{
    processQueuedNicks(true);
}

void Channel::kickNick(ChannelNickPtr channelNick, const QString &kicker, const QString &reason)
{
    QString displayReason = reason;

    if(!displayReason.isEmpty())
    {
        // if the reason contains text markup characters, play it safe and reset all
        if (hasIRCMarkups(displayReason))
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
                                              "You have been kicked from channel %1 by %2.", getName(), kicker));
            }
            else
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the channel, %2 the kicker and %3 the reason",
                                              "You have been kicked from channel %1 by %2 (%3).", getName(), kicker, displayReason));
            }

            Application::instance()->notificationHandler()->kick(this,getName(), kicker);
        }

        m_joined=false;
        setActive(false);

        //HACK the way the notification priorities work sucks, this forces the tab text color to gray right now.
        if (m_currentTabNotify == Konversation::tnfNone || (!Preferences::self()->tabNotificationsEvents() && m_currentTabNotify == Konversation::tnfControl))
            Application::instance()->getMainWindow()->getViewContainer()->unsetViewNotification(this);

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
                                     "You have kicked %1 from the channel (%2).", channelNick->getNickname(), displayReason));
        }
        else
        {
            if (displayReason.isEmpty())
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the kicked nick, %2 adds the kicker",
                                     "%1 has been kicked from the channel by %2.", channelNick->getNickname(), kicker));
            }
            else
            {
                appendCommandMessage(i18n("Kick"), i18nc("%1 adds the kicked nick, %2 the kicker and %3 the reason",
                                     "%1 has been kicked from the channel by %2 (%3).", channelNick->getNickname(), kicker, displayReason));
            }
        }

        if(channelNick->isAnyTypeOfOp())
            adjustOps(-1);

        adjustNicks(-1);
        Nick* nick = getNickByName(channelNick->loweredNickname());

        if(nick == 0)
        {
            kWarning() << "Nickname " << channelNick->getNickname() << " not found!"<< endl;
        }
        else
        {
            nicknameList.removeOne(nick);
            m_nicknameNickHash.remove(channelNick->loweredNickname());
            delete nick;
        }
    }
}

Nick* Channel::getNickByName(const QString &lookname) const
{
    QString lcLookname(lookname.toLower());

    return m_nicknameNickHash.value(lcLookname);
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

QString Channel::getTopic()
{
    return m_topicHistory->currentTopic();
}

void Channel::setTopic(const QString& text)
{
    QString cleanTopic = text;

    // If the reason contains text markup characters, play it safe and reset all.
    if (!cleanTopic.isEmpty() && hasIRCMarkups(cleanTopic))
        cleanTopic += "\017";

    appendCommandMessage(i18n("Topic"), i18n("The channel topic is \"%1\".", cleanTopic));

    m_topicHistory->appendTopic(replaceIRCMarkups(Konversation::removeIrcMarkup(text)));
}

void Channel::setTopic(const QString& nickname, const QString& text)
{
    QString cleanTopic = text;

    // If the reason contains text markup characters, play it safe and reset all.
    if (!cleanTopic.isEmpty() && hasIRCMarkups(cleanTopic))
        cleanTopic += "\017";

    if (nickname == m_server->getNickname())
        appendCommandMessage(i18n("Topic"), i18n("You set the channel topic to \"%1\".", cleanTopic));
    else
        appendCommandMessage(i18n("Topic"), i18n("%1 sets the channel topic to \"%2\".", nickname, cleanTopic));

    m_topicHistory->appendTopic(replaceIRCMarkups(Konversation::removeIrcMarkup(text)), nickname);
}

void Channel::setTopicAuthor(const QString& author, QDateTime time)
{
    if (time.isNull() || !time.isValid())
        time = QDateTime::currentDateTime();

    m_topicHistory->setCurrentTopicMetadata(author, time);
}

void Channel::updateMode(const QString& sourceNick, char mode, bool plus, const QString &parameter)
{
    // Note for future expansion:
    //     m_server->getChannelNick(getName(), sourceNick);
    // may not return a valid channelNickPtr if the mode is updated by
    // the server.
    // --johnflux, 9 September 2004

    // Note: nick repositioning in the nicknameListView should be
    // triggered by nickinfo / channelnick signals

    QString message;
    ChannelNickPtr parameterChannelNick = m_server->getChannelNick(getName(), parameter);

    bool fromMe = false;
    bool toMe = false;

    // HACK right now Server only keeps type A modes
    bool banTypeThang = m_server->banAddressListModes().contains(QChar(mode));

    // remember if this nick had any type of op.
    bool wasAnyOp = false;
    if (parameterChannelNick)
    {
        addNickname(parameterChannelNick);

        wasAnyOp = parameterChannelNick->isAnyTypeOfOp();
    }

    if (sourceNick.toLower() == m_server->loweredNickname())
        fromMe = true;
    if (parameter.toLower() == m_server->loweredNickname())
        toMe = true;

    switch (mode)
    {
        case 'q':
            if (banTypeThang)
            {
                if (plus)
                {
                    if (fromMe) message = i18n("You set a quiet on %1.", parameter);
                    else        message = i18n("%1 sets a quiet on %2.", sourceNick, parameter);
                }
                else
                {
                    if (fromMe) message = i18n("You remove the quiet on %1.", parameter);
                    else        message = i18n("%1 removes the quiet on %2.", sourceNick, parameter);
                }
            }
            else
            {
                if (plus)
                {
                    if (fromMe)
                    {
                        if (toMe)   message = i18n("You give channel owner privileges to yourself.");
                        else        message = i18n("You give channel owner privileges to %1.", parameter);
                    }
                    else
                    {
                        if (toMe)   message = i18n("%1 gives channel owner privileges to you.", sourceNick);
                        else        message = i18n("%1 gives channel owner privileges to %2.", sourceNick, parameter);
                    }
                }
                else
                {
                    if (fromMe)
                    {
                        if (toMe)   message = i18n("You take channel owner privileges from yourself.");
                        else        message = i18n("You take channel owner privileges from %1.", parameter);
                    }
                    else
                    {
                        if (toMe)   message = i18n("%1 takes channel owner privileges from you.", sourceNick);
                        else        message = i18n("%1 takes channel owner privileges from %2.", sourceNick, parameter);
                    }
                }
                if (parameterChannelNick)
                {
                    parameterChannelNick->setOwner(plus);
                    emitUpdateInfo();
                }
            }
            break;

        case 'a':
            if (plus)
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You give channel admin privileges to yourself.");
                    else        message = i18n("You give channel admin privileges to %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 gives channel admin privileges to you.", sourceNick);
                    else        message = i18n("%1 gives channel admin privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You take channel admin privileges from yourself.");
                    else        message = i18n("You take channel admin privileges from %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 takes channel admin privileges from you.", sourceNick);
                    else        message = i18n("%1 takes channel admin privileges from %2.", sourceNick, parameter);
                }
            }
            if (parameterChannelNick)
            {
                parameterChannelNick->setAdmin(plus);
                emitUpdateInfo();
            }
            break;

        case 'o':
            if (plus)
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You give channel operator privileges to yourself.");
                    else        message = i18n("You give channel operator privileges to %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 gives channel operator privileges to you.", sourceNick);
                    else        message = i18n("%1 gives channel operator privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You take channel operator privileges from yourself.");
                    else        message = i18n("You take channel operator privileges from %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 takes channel operator privileges from you.", sourceNick);
                    else        message = i18n("%1 takes channel operator privileges from %2.", sourceNick, parameter);
                }
            }
            if (parameterChannelNick)
            {
                parameterChannelNick->setOp(plus);
                emitUpdateInfo();
            }
            break;

        case 'h':
            if (plus)
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You give channel halfop privileges to yourself.");
                    else        message = i18n("You give channel halfop privileges to %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 gives channel halfop privileges to you.", sourceNick);
                    else        message = i18n("%1 gives channel halfop privileges to %2.", sourceNick, parameter);
                }
            }
            else
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You take channel halfop privileges from yourself.");
                    else        message = i18n("You take channel halfop privileges from %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 takes channel halfop privileges from you.", sourceNick);
                    else        message = i18n("%1 takes channel halfop privileges from %2.", sourceNick, parameter);
                }
            }
            if (parameterChannelNick)
            {
                parameterChannelNick->setHalfOp(plus);
                emitUpdateInfo();
            }
            break;

        //case 'O': break;

        case 'v':
            if (plus)
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You give yourself permission to talk.");
                    else        message = i18n("You give %1 permission to talk.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 gives you permission to talk.", sourceNick);
                    else        message = i18n("%1 gives %2 permission to talk.", sourceNick, parameter);
                }
            }
            else
            {
                if (fromMe)
                {
                    if (toMe)   message = i18n("You take the permission to talk from yourself.");
                    else        message = i18n("You take the permission to talk from %1.", parameter);
                }
                else
                {
                    if (toMe)   message = i18n("%1 takes the permission to talk from you.", sourceNick);
                    else        message = i18n("%1 takes the permission to talk from %2.", sourceNick, parameter);
                }
            }
            if (parameterChannelNick)
            {
                parameterChannelNick->setVoice(plus);
            }
            break;

        case 'c':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'no colors allowed'.");
                else        message = i18n("%1 sets the channel mode to 'no colors allowed'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You set the channel mode to 'allow color codes'.");
                else        message = i18n("%1 sets the channel mode to 'allow color codes'.", sourceNick);
            }
            break;

        case 'i':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'invite only'.");
                else        message = i18n("%1 sets the channel mode to 'invite only'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You remove the 'invite only' mode from the channel.");
                else        message = i18n("%1 removes the 'invite only' mode from the channel.", sourceNick);
            }
            modeI->setDown(plus);
            break;

        case 'm':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'moderated'.");
                else        message = i18n("%1 sets the channel mode to 'moderated'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You set the channel mode to 'unmoderated'.");
                else        message = i18n("%1 sets the channel mode to 'unmoderated'.", sourceNick);
            }
            modeM->setDown(plus);
            break;

        case 'n':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'no messages from outside'.");
                else        message = i18n("%1 sets the channel mode to 'no messages from outside'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You set the channel mode to 'allow messages from outside'.");
                else        message = i18n("%1 sets the channel mode to 'allow messages from outside'.", sourceNick);
            }
            modeN->setDown(plus);
            break;

        case 'p':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'private'.");
                else        message = i18n("%1 sets the channel mode to 'private'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You set the channel mode to 'public'.");
                else        message = i18n("%1 sets the channel mode to 'public'.", sourceNick);
            }
            modeP->setDown(plus);
            if (plus) modeS->setDown(false);
            break;

        case 's':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel mode to 'secret'.");
                else        message = i18n("%1 sets the channel mode to 'secret'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You set the channel mode to 'visible'.");
                else        message = i18n("%1 sets the channel mode to 'visible'.", sourceNick);
            }
            modeS->setDown(plus);
            if (plus) modeP->setDown(false);
            break;

        //case 'r': break;

        case 't':
            if (plus)
            {
                if (fromMe) message = i18n("You switch on 'topic protection'.");
                else        message = i18n("%1 switches on 'topic protection'.", sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You switch off 'topic protection'.");
                else        message = i18n("%1 switches off 'topic protection'.", sourceNick);
            }
            modeT->setDown(plus);
            break;

        case 'k':
            if (plus)
            {
                if (fromMe) message = i18n("You set the channel key to '%1'.", parameter);
                else        message = i18n("%1 sets the channel key to '%2'.", sourceNick, parameter);
            }
            else
            {
                if (fromMe) message = i18n("You remove the channel key.");
                else        message = i18n("%1 removes the channel key.", sourceNick);
            }
            modeK->setDown(plus);
            break;

        case 'l':
            if (plus)
            {
                if (fromMe) message = i18np("You set the channel limit to 1 nick.", "You set the channel limit to %1 nicks.", parameter);
                else        message = i18np("%2 sets the channel limit to 1 nick.", "%2 sets the channel limit to %1 nicks.", parameter, sourceNick);
            }
            else
            {
                if (fromMe) message = i18n("You remove the channel limit.");
                else        message = i18n("%1 removes the channel limit.", sourceNick);
            }
            modeL->setDown(plus);
            if (plus) limit->setText(parameter);
            else limit->clear();
            break;

        case 'b':
            if (plus)
            {
                if (fromMe) message = i18n("You set a ban on %1.", parameter);
                else        message = i18n("%1 sets a ban on %2.", sourceNick, parameter);
            }
            else
            {
                if (fromMe) message = i18n("You remove the ban on %1.", parameter);
                else        message = i18n("%1 removes the ban on %2.", sourceNick, parameter);
            }
            break;

        case 'e':
            if (plus)
            {
                if (fromMe) message = i18n("You set a ban exception on %1.", parameter);
                else        message = i18n("%1 sets a ban exception on %2.", sourceNick, parameter);
            }
            else
            {
                if (fromMe) message = i18n("You remove the ban exception on %1.", parameter);
                else        message = i18n("%1 removes the ban exception on %2.", sourceNick, parameter);
            }
            break;

        case 'I':
            if (plus)
            {
                if (fromMe) message = i18n("You set invitation mask %1.", parameter);
                else        message = i18n("%1 sets invitation mask %2.", sourceNick, parameter);
            }
            else
            {
                if (fromMe) message = i18n("You remove the invitation mask %1.", parameter);
                else        message = i18n("%1 removes the invitation mask %2.", sourceNick, parameter);
            }
            break;
        default:
        if (plus)
        {
            if (Konversation::getChannelModesHash().contains(mode))
            {
                if (fromMe) message = i18n("You set the channel mode '%1'.", Konversation::getChannelModesHash().value(mode));
                else        message= i18n("%1 sets the channel mode '%2'.", sourceNick, Konversation::getChannelModesHash().value(mode));
            }
            else
            {
                if (fromMe) message = i18n("You set channel mode +%1", QString(mode));
                else        message = i18n("%1 sets channel mode +%2", sourceNick, QString(mode));
            }
        }
        else
        {
            if (Konversation::getChannelModesHash().contains(mode))
            {
                if (fromMe) message = i18n("You remove the channel mode '%1'.", Konversation::getChannelModesHash().value(mode));
                else        message= i18n("%1 removes the channel mode '%2'.", sourceNick, Konversation::getChannelModesHash().value(mode));
            }
            else
            {
                if (fromMe) message = i18n("You set channel mode -%1", QString(mode));
                else        message = i18n("%1 sets channel mode -%2", sourceNick, QString(mode));
            }
        }
    }

    // check if this nick's anyOp-status has changed and adjust ops accordingly
    if (parameterChannelNick)
    {
        if (wasAnyOp && (!parameterChannelNick->isAnyTypeOfOp()))
            adjustOps(-1);
        else if (!wasAnyOp && parameterChannelNick->isAnyTypeOfOp())
            adjustOps(1);
    }

    if (!message.isEmpty() && !Preferences::self()->useLiteralModes())
    {
        appendCommandMessage(i18n("Mode"), message);
    }

    updateModeWidgets(mode, plus, parameter);
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

    delete m_buttonsGrid;

    // the grid that holds the quick action buttons
    m_buttonsGrid = new QWidget (nickListButtons); //Q3Grid(2, nickListButtons);
    m_buttonsGrid->hide();
    QGridLayout* layout = new QGridLayout (m_buttonsGrid);
    layout->setMargin(0);

    int col = 0;
    int row = 0;
    // add new quick buttons
    for(int index=0;index<newButtonList.count();index++)
    {
        // generate empty buttons first, text will be added later
        QuickButton* quickButton = new QuickButton(QString(), QString(), m_buttonsGrid);
        col = index % 2;
        layout->addWidget (quickButton, row, col);
        row += col;
        buttonList.append(quickButton);

        connect(quickButton, SIGNAL(clicked(QString)), this, SLOT(quickButtonClicked(QString)));

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
    if(isHidden() || !m_buttonsGrid)
    {
        quickButtonsChanged=true;
        quickButtonsState=show;
    }
    else
    {
        if(show)
            m_buttonsGrid->show();
        else
            m_buttonsGrid->hide();
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
        // An approximation of a common NICKLEN plus the width of the icon,
        // tested with 8pt and 10pt DejaVu Sans and Droid Sans.
        int listWidth = fontMetrics().averageCharWidth() * 17 + 20;
        horizSizes << (width() - listWidth) << listWidth;
        Preferences::self()->setChannelSplitterSizes(horizSizes);
    }

    m_vertSplitter->setSizes(vertSizes);
    m_horizSplitter->setSizes(horizSizes);

    splittersInitialized = true;
}

void Channel::updateAppearance()
{
    QPalette palette;

    if (Preferences::self()->inputFieldsBackgroundColor())
    {
        palette.setColor(QPalette::Text, Preferences::self()->color(Preferences::ChannelMessage));
        palette.setColor(QPalette::Base, Preferences::self()->color(Preferences::TextViewBackground));
        palette.setColor(QPalette::AlternateBase, Preferences::self()->color(Preferences::AlternateBackground));
    }

    limit->setPalette(palette);
    topicLine->setPalette(QPalette());

    if (Preferences::self()->customTextFont())
    {
        topicLine->setFont(Preferences::self()->textFont());
        m_inputBar->setFont(Preferences::self()->textFont());
        nicknameCombobox->setFont(Preferences::self()->textFont());
        limit->setFont(Preferences::self()->textFont());
    }
    else
    {
        topicLine->setFont(KGlobalSettings::generalFont());
        m_inputBar->setFont(KGlobalSettings::generalFont());
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
        limit->setFont(KGlobalSettings::generalFont());
    }

    nicknameListView->resort();
    nicknameListView->setPalette(palette);
    nicknameListView->setAlternatingRowColors(Preferences::self()->inputFieldsBackgroundColor());

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

    // Nick sorting settings might have changed. Trigger timer
    if (m_delayedSortTimer)
    {
        m_delayedSortTrigger = DELAYED_SORT_TRIGGER + 1;
        m_delayedSortTimer->start(500 + qrand()/2000);
    }

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
        m_inputBar->setFocus();
    }
}

void Channel::changeNickname(const QString& newNickname)
{
    if (!newNickname.isEmpty())
        m_server->queue("NICK "+newNickname);
}

void Channel::queueNicks(const QStringList& nicknameList)
{
    if (nicknameList.isEmpty())
        return;

    m_nickQueue.append(nicknameList);

    processQueuedNicks();
}

void Channel::endOfNames()
{
    if (!m_initialNamesReceived)
    {
        m_initialNamesReceived = true;

        scheduleAutoWho();
    }
}

void Channel::childAdjustFocus()
{
    m_inputBar->setFocus();
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

void Channel::nicknameListViewTextChanged(int textChangedFlags)
{
    m_nicknameListViewTextChanged |= textChangedFlags;
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

    if(!nicknameList.isEmpty())
    {
        resizeNicknameListViewColumns();
    }
}

void Channel::setAutoUserhost(bool state)
{
    nicknameListView->setColumnHidden(Nick::HostmaskColumn, !state);
    if (state)
    {
        nicknameListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // Cannot use QHeaderView::ResizeToContents here because it is slow
        // and it gets triggered by setSortingEnabled(). Using timed resize
        // instead, see Channel::autoUserhost() above.
        nicknameListView->header()->setResizeMode(Nick::NicknameColumn, QHeaderView::Fixed);
        nicknameListView->header()->setResizeMode(Nick::HostmaskColumn, QHeaderView::Fixed);
        userhostTimer.start(10000);
        m_nicknameListViewTextChanged |= 0xFF; // ResizeColumnsToContents
        QTimer::singleShot(0, this, SLOT(autoUserhost())); // resize columns ASAP
    }
    else
    {
        nicknameListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        nicknameListView->header()->setResizeMode(Nick::NicknameColumn, QHeaderView::Stretch);
        userhostTimer.stop();
    }
}

void Channel::scheduleAutoWho(int msec)
{
    // The first auto-who is scheduled by ENDOFNAMES in InputFilter, which means
    // the first auto-who occurs one interval after it. This has two desirable
    // consequences specifically related to the startup phase: auto-who dispatch
    // doesn't occur at the same time for all channels that are auto-joined, and
    // it gives some breathing room to process the NAMES replies for all channels
    // first before getting started on WHO.
    // Subsequent auto-whos are scheduled by ENDOFWHO in InputFilter. However,
    // autoWho() might refuse to actually do the request if the number of nicks
    // in the channel exceeds the threshold, and will instead schedule another
    // attempt later. Thus scheduling an auto-who does not guarantee it will be
    // performed.
    // If this is called mid-interval (e.g. due to the ENDOFWHO from a manual WHO)
    // it will reset the interval to avoid cutting it short.

    if (m_whoTimer.isActive())
        m_whoTimer.stop();

    if (Preferences::self()->autoWhoContinuousEnabled())
    {
        if (msec > 0)
            m_whoTimer.start(msec);
        else
            m_whoTimer.start(Preferences::self()->autoWhoContinuousInterval() * 1000);
    }
}

void Channel::autoWho()
{
    // Try again later if there are too many nicks or we're already processing a WHO request.
    if ((nicks > Preferences::self()->autoWhoNicksLimit()) ||
       m_server->getInputFilter()->isWhoRequestUnderProcess(getName()))
    {
        scheduleAutoWho();

        return;
    }

    m_server->requestWho(getName());
}

void Channel::updateAutoWho()
{
    if (!Preferences::self()->autoWhoContinuousEnabled())
        m_whoTimer.stop();
    else if (Preferences::self()->autoWhoContinuousEnabled() && !m_whoTimer.isActive())
        autoWho();
    else if (m_whoTimer.isActive())
    {
        // The below tries to meet user expectations on an interval settings change,
        // making two assumptions:
        // - If the new interval is lower than the old one, the user may be impatient
        //   and desires an information update.
        // - If the new interval is longer than the old one, the user may be trying to
        //   avoid Konversation producing too much traffic in a given timeframe, and
        //   wants it to stop doing so sooner rather than later.
        // Both require rescheduling the next auto-who request.

        int interval = Preferences::self()->autoWhoContinuousInterval() * 1000;

        if (interval != m_whoTimer.interval())
        {
            if (m_whoTimerStarted.elapsed() >= interval)
            {
                // If the time since the last auto-who request is longer than (or
                // equal to) the new interval setting, it follows that the new
                // setting is lower than the old setting. In this case issue a new
                // request immediately, which is the closest we can come to acting
                // as if the new setting had been active all along, short of tra-
                // velling back in time to change history. This handles the impa-
                // tient user.
                // FIXME: Adjust algorithm when time machine becomes available.

                m_whoTimer.stop();
                autoWho();
            }
            else
            {
                // If on the other hand the elapsed time is shorter than the new
                // interval setting, the new setting could be either shorter or
                // _longer_ than the old setting. Happily, this time we can actually
                // behave as if the new setting had been active all along, by sched-
                // uling the next request to happen in the new interval time minus
                // the already elapsed time, meeting user expecations for both cases
                // originally laid out.

                scheduleAutoWho(interval - m_whoTimerStarted.elapsed());
            }
        }
    }
}

void Channel::fadeActivity()
{
    foreach (Nick *nick,  nicknameList) {
        nick->getChannelNick()->lessActive();
    }
}

bool Channel::canBeFrontView()
{
    return true;
}

bool Channel::searchView()
{
    return true;
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

        deleteLater();

        return true;
    }
    else
        m_recreationScheduled = false;

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
        nicknameCombobox->setEnabled(true);
    else
    {
        m_initialNamesReceived = false;
        purgeNicks();
        nicknameCombobox->setEnabled(false);
        topicLine->clear();
        clearModeList();
        clearBanList();
        m_whoTimer.stop();
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

void Channel::processQueuedNicks(bool flush)
{
// This pops nicks from the front of a queue added to by incoming NAMES
// messages and adds them to the channel nicklist, calling itself via
// the event loop until the last invocation finds the queue empty and
// adjusts the nicks/ops counters and requests a nicklist sort, but only
// if previous invocations actually processed any nicks. The latter is
// an optimization for the common case of processing being kicked off by
// flushNickQueue(), which is done e.g. before a nick rename or part to
// make sure the channel is up to date and will usually find an empty
// queue. This is also the use case for the 'flush' parameter, which if
// true causes the recursion to block in a tight loop instead of queueing
// via the event loop.

    if (m_nickQueue.isEmpty())
    {
        if (m_processedNicksCount)
        {
            adjustNicks(m_processedNicksCount);
            adjustOps(m_processedOpsCount);
            m_processedNicksCount = 0;
            m_processedOpsCount = 0;

            sortNickList();
            nicknameListView->setUpdatesEnabled(true);

            if (Preferences::self()->autoUserhost())
                resizeNicknameListViewColumns();
        }
    }
    else
    {
        QString nickname;

        while (nickname.isEmpty() && !m_nickQueue.isEmpty())
            nickname = m_nickQueue.takeFirst();

        bool admin = false;
        bool owner = false;
        bool op = false;
        bool halfop = false;
        bool voice = false;

        // Remove possible mode characters from nickname and store the resulting mode.
        m_server->mangleNicknameWithModes(nickname, admin, owner, op, halfop, voice);

        // TODO: Make these an enumeration in KApplication or somewhere, we can use them as well.
        unsigned int mode = (admin  ? 16 : 0) +
                            (owner  ?  8 : 0) +
                            (op     ?  4 : 0) +
                            (halfop ?  2 : 0) +
                            (voice  ?  1 : 0);

        // Check if nick is already in the nicklist.
        if (!nickname.isEmpty() && !getNickByName(nickname))
        {
            ChannelNickPtr nick = m_server->addNickToJoinedChannelsList(getName(), nickname);
            Q_ASSERT(nick);
            nick->setMode(mode);

            fastAddNickname(nick);

            ++m_processedNicksCount;

            if (nick->isAdmin() || nick->isOwner() || nick->isOp() || nick->isHalfOp())
                ++m_processedOpsCount;
        }

        QMetaObject::invokeMethod(this, "processQueuedNicks",
            flush ? Qt::DirectConnection : Qt::QueuedConnection, Q_ARG(bool, flush));
    }
}

void Channel::setChannelEncoding(const QString& encoding) // virtual
{
    if(m_server->getServerGroup())
        Preferences::setChannelEncoding(m_server->getServerGroup()->id(), getName(), encoding);
    else
        Preferences::setChannelEncoding(m_server->getDisplayName(), getName(), encoding);
}

QString Channel::getChannelEncoding() // virtual
{
    if(m_server->getServerGroup())
        return Preferences::channelEncoding(m_server->getServerGroup()->id(), getName());
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
    m_delayedSortTrigger++;
    if (m_delayedSortTrigger == DELAYED_SORT_TRIGGER &&
        !m_delayedSortTimer->isActive())
    {
        nicknameListView->fastSetSortingEnabled(false);
        m_delayedSortTimer->start(1000);
    }
}

void Channel::delayedSortNickList()
{
    sortNickList(true);
}

void Channel::sortNickList(bool delayed)
{
    if (!delayed || m_delayedSortTrigger > DELAYED_SORT_TRIGGER) {
        qSort(nicknameList.begin(), nicknameList.end(), nickLessThan);
        nicknameListView->resort();
    }
    if (!nicknameListView->isSortingEnabled())
        nicknameListView->fastSetSortingEnabled(true);
    m_delayedSortTrigger = 0;
    m_delayedSortTimer->stop();
}

void Channel::repositionNick(Nick *nick)
{
    int index = nicknameList.indexOf(nick);

    if (index > -1) {
        // Trigger nick reposition in the nicklist including
        // field updates
        nick->refresh();
        // Readd nick to the nicknameList
        nicknameList.removeAt(index);
        fastAddNickname(nick->getChannelNick(), nick);
    } else {
        kWarning() << "Nickname " << nick->getChannelNick()->getNickname() << " not found!"<< endl;
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
    ChannelNickPtr channelnick=getChannelNick(nickname);
    //XXX Would be nice to know why it can be null here...
    if (channelnick)
    {
        channelnick->moreActive();
        if (Preferences::self()->sortByActivity())
        {
            Nick* nick = getNickByName(nickname);
            if (nick)
            {
                nick->repositionMe();
            }
        }
    }
}

#ifdef HAVE_QCA2
Konversation::Cipher* Channel::getCipher()
{
    if(!m_cipher)
        m_cipher = new Konversation::Cipher();
    return m_cipher;
}
#endif

void Channel::updateNickInfos()
{
    foreach(Nick* nick, nicknameList)
    {
        if(nick->getChannelNick()->getNickInfo()->isChanged())
        {
            nick->refresh();
        }
    }
}

void Channel::updateChannelNicks(const QString& channel)
{
    if(channel != name.toLower())
        return;

    foreach(Nick* nick, nicknameList)
    {
        if(nick->getChannelNick()->isChanged())
        {
            nick->refresh();

            if(nick->getChannelNick() == m_ownChannelNick)
            {
                refreshModeButtons();
            }
        }
    }
}

void Channel::resizeNicknameListViewColumns()
{
    // Resize columns if needed (on regular basis)
    if (m_nicknameListViewTextChanged & (1 << Nick::NicknameColumn))
        nicknameListView->resizeColumnToContents(Nick::NicknameColumn);
    if (m_nicknameListViewTextChanged & (1 << Nick::HostmaskColumn))
        nicknameListView->resizeColumnToContents(Nick::HostmaskColumn);
    m_nicknameListViewTextChanged = 0;
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
