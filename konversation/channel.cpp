/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
               (C) 2004 by Peter Simonsson <psn@linux.se>
    email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qvbox.h>
#include <qevent.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qdragobject.h>
#include <qsizepolicy.h>
#include <qheader.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qtimer.h>
#include <qcombobox.h>
#include <qtextcodec.h>
#include <qwhatsthis.h>
#include <qtoolbutton.h>
#include <qlayout.h>

#include <kprocess.h>

#include <klineedit.h>
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kwin.h>

#include "konversationapplication.h"
#include "konversationmainwindow.h"
#include "channel.h"
#include "server.h"
#include "nick.h"
#include "nicklistview.h"
#include "nicklistviewitem.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "ircinput.h"
#include "ircview.h"
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>
#include "common.h"
#include "topiclabel.h"
#include "channeloptionsdialog.h"
#include "notificationhandler.h"

#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"

#define OPAQUE_CONF

#ifdef USE_MDI
Channel::Channel(const QString &caption) : ChatWindow(caption)
#else
Channel::Channel(QWidget* parent) : ChatWindow(parent)
#endif
{
  // init variables
  m_processingTimer = 0;
  m_delayedSortTimer = 0;
  m_pendingChannelNickLists.clear();
  m_currentIndex = 0;
  m_opsToAdd = 0;
  nicks = 0;
  ops = 0;
  completionPosition = 0;
  nickChangeDialog = 0;

  quickButtonsChanged = false;
  quickButtonsState = false;

  splitterChanged = true;
  modeButtonsChanged = false;
  modeButtonsState = false;

  awayChanged = false;
  awayState = false;

  // no nicks pending from /names reply
  setPendingNicks(false);

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
  
#ifdef OPAQUE_CONF
  m_vertSplitter->setOpaqueResize(KGlobalSettings::opaqueResize());
#else
  m_vertSplitter->setOpaqueResize(true);
#endif
  
  QWidget* topicWidget = new QWidget(m_vertSplitter);
  QGridLayout* topicLayout = new QGridLayout(topicWidget, 2, 3, 0, 0);

  m_topicButton = new QToolButton(topicWidget);
  m_topicButton->setIconSet(SmallIconSet("edit", 16));
  QToolTip::add(m_topicButton, i18n("Edit topic"));
  connect(m_topicButton, SIGNAL(clicked()), this, SLOT(showOptionsDialog()));
  topicLine = new Konversation::TopicLabel(topicWidget);
  QWhatsThis::add(topicLine, i18n("<qt>Every channel on IRC has a topic associated with it.  This is simply a message that everybody can see.<p>If you are an operator, or the channel mode <em>'T'</em> has not been set, then you can change the topic by clicking the Edit Topic button to the left of the topic.  You can also view the history of topics there.</qt>"));

  topicLayout->addWidget(m_topicButton, 0, 0);
  topicLayout->addMultiCellWidget(topicLine, 0, 1, 1, 1);

  // The box holding the channel modes
  modeBox = new QHBox(topicWidget);
  modeBox->setSizePolicy(hfixed);
  modeT = new ModeButton("T",modeBox,0);
  modeN = new ModeButton("N",modeBox,1);
  modeS = new ModeButton("S",modeBox,2);
  modeI = new ModeButton("I",modeBox,3);
  modeP = new ModeButton("P",modeBox,4);
  modeM = new ModeButton("M",modeBox,5);
  modeK = new ModeButton("K",modeBox,6);
  modeL = new ModeButton("L",modeBox,7);

  QWhatsThis::add(modeT, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>The <b>T</b>opic mode means that only the channel operator can change the topic for the channel.</qt>"));
  QWhatsThis::add(modeN, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p><b>N</b>o messages from outside means that users that are not in the channel cannot sends messages that everybody in the channel can see.  Almost all channels have this set to prevent nuisance messages.</qt>"));
  QWhatsThis::add(modeS, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>S</b>ecret channel will not show up in the channel list, nor will any user be able to see that you are in the channel with the <em>WHOIS</em> command or anything similar.  Only the people that are in the same channel will know that you are in this channel, if this mode is set.</qt>"));
  QWhatsThis::add(modeI, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>An <b>I</b>nvite only channel means that people can only join the channel if they are invited.  To invite someone, a channel operator needs to issue the command <em>/invite nick</em> from within the channel.</qt>"));
  QWhatsThis::add(modeP, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>P</b>rivate channel is shown in a listing of all channels, but the topic is not shown.  A user's <em>WHOIS</e> may or may not show them as being in a private channel depending on the IRC server.</qt>"));
  QWhatsThis::add(modeM, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>M</b>oderated channel is one where only operators, half-operators and those with voice can talk.</qt>"));
  QWhatsThis::add(modeK, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A <b>P</b>rotected channel requires users to enter a password in order to join.</qt>"));
  QWhatsThis::add(modeL, i18n("<qt>These control the <em>mode</em> of the channel.  Only an operator can change these.<p>A channel that has a user <b>L</b>imit means that only that many users can be in the channel at any one time.  Some channels have a bot that sits in the channel and changes this automatically depending on how busy the channel is.</qt>"));
  
  connect(modeT,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeN,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeS,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeI,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeP,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeM,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeK,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeL,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));

  limit=new KLineEdit(modeBox);
  QToolTip::add(limit, i18n("Maximum users allowed in channel"));
  QWhatsThis::add(limit, i18n("<qt>This is the channel user limit - the maximum number of users that can be in the channel at a time.  If you are an operator, you can set this.  The channel mode <b>T</b>opic (button to left) will automatically be set if set this.</qt>"));
  connect(limit,SIGNAL (returnPressed()),this,SLOT (channelLimitChanged()) );
  connect(limit,SIGNAL (lostFocus()), this, SLOT(channelLimitChanged()) );
  limit->installEventFilter(this);

  topicLayout->addWidget(modeBox, 0, 2);
  topicLayout->setRowStretch(1, 10);
  topicLayout->setColStretch(1, 10);

  showTopic(KonversationApplication::preferences.getShowTopic());
  showModeButtons(KonversationApplication::preferences.getShowModeButtons());

  // (this) The main Box, holding the channel view/topic and the input line
  splitter = new QSplitter(m_vertSplitter);

#ifdef OPAQUE_CONF
  splitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
#else
  splitter->setOpaqueResize(true);
#endif

  setTextView(new IRCView(splitter, NULL));  // Server will be set later in setServer()
  connect(textView,SIGNAL(popupCommand(int)),this,SLOT(popupCommand(int)));
  // The box that holds the Nick List and the quick action buttons
  QVBox* nickListButtons = new QVBox(splitter);
  nickListButtons->setSpacing(spacing());

  nicknameListView=new NickListView(nickListButtons, this);
  nicknameListView->setSelectionModeExt(KListView::Extended);
  nicknameListView->setAllColumnsShowFocus(true);
  nicknameListView->setSorting(1,true);
  nicknameListView->addColumn(QString::null);
  nicknameListView->addColumn(QString::null);
  nicknameListView->setColumnWidthMode(1,KListView::Maximum);
  if (KonversationApplication::preferences.getAutoUserhost()) {
    nicknameListView->addColumn(QString::null);
    nicknameListView->setColumnWidthMode(2,KListView::Maximum);
  }

  nicknameListView->header()->hide();

  // setResizeMode must be called after all the columns are added
  nicknameListView->setResizeMode(KListView::LastColumn);

//  nicknameListView->header()->setStretchEnabled(true,2);

  // separate LED from Text a little more
  nicknameListView->setColumnWidth(0, 10);
  nicknameListView->setColumnAlignment(0, Qt::AlignHCenter);
  nicknameListView->installEventFilter(this);

  // the grid that holds the quick action buttons
  buttonsGrid = new QGrid(2, nickListButtons);
  // set hide() or show() on grid
  showQuickButtons(KonversationApplication::preferences.getShowQuickButtons());

  for(int index = 0; index < 8; index++)
  {
    // generate empty buttons first, text will be added by updateQuickButtons() later
    QuickButton* newQuickButton = new QuickButton(QString::null, QString::null, buttonsGrid);
    buttonList.append(newQuickButton);

    connect(newQuickButton, SIGNAL(clicked(const QString &)), this, SLOT(quickButtonClicked(const QString &)));
  }

  updateQuickButtons(KonversationApplication::preferences.getButtonList());

  // The box holding the Nickname button and Channel input
  commandLineBox = new QHBox(this);
  commandLineBox->setSpacing(spacing());

  nicknameCombobox = new QComboBox(commandLineBox);
  nicknameCombobox->setEditable(true);
  nicknameCombobox->insertStringList(KonversationApplication::preferences.getNicknameList());
  nicknameCombobox->installEventFilter(this);
  QWhatsThis::add(nicknameCombobox, i18n("<qt>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.<p>You can add change the alternative nicknames from the <em>Identities</em> option in the <em>File</em> menu.</qt>"));
  oldNick = nicknameCombobox->currentText();
  
  setShowNicknameBox(KonversationApplication::preferences.showNicknameBox());
  
  awayLabel = new QLabel(i18n("(away)"), commandLineBox);
  awayLabel->hide();
  channelInput = new IRCInput(commandLineBox);
  channelInput->installEventFilter(this);
  nicknameListView->installEventFilter(channelInput);

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
  // remember alternate background color
  abgCache=nicknameListView->alternateBackground().name();

  connect(channelInput,SIGNAL (submit()),this,SLOT (channelTextEntered()) );
  connect(channelInput,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
  connect(channelInput,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
  connect(channelInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );

  connect(getTextView(), SIGNAL(textPasted()), channelInput, SLOT(paste()));
  connect(getTextView(),SIGNAL (gotFocus()),channelInput,SLOT (setFocus()) );
  connect(getTextView(),SIGNAL (newText(const QString&,bool)),this,SLOT (newTextInView(const QString&,bool)) );
  connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
  connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendChannelText(const QString&)) );

  connect(nicknameListView,SIGNAL (popupCommand(int)),this,SLOT (popupCommand(int)) );
  connect(nicknameListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT (doubleClickCommand(QListViewItem*)) );
  connect(nicknameListView,SIGNAL (dropped(QDropEvent*,QListViewItem*)),this,SLOT (filesDropped(QDropEvent*)) );
  connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
 
  Q_ASSERT(nicknameCombobox->lineEdit());  //it should be editedable.  if we design it so it isn't, remove these lines.
  if(nicknameCombobox->lineEdit())
    connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

  nicknameList.setAutoDelete(true);     // delete items when they are removed

  updateFonts();
  setLog(KonversationApplication::preferences.getLog());

  connect(&userhostTimer,SIGNAL (timeout()),this,SLOT (autoUserhost()));
  connect(&KonversationApplication::preferences,SIGNAL (autoUserhostChanged(bool)),this,SLOT (autoUserhostChanged(bool)));

  // every few seconds try to get more userhosts
  autoUserhostChanged(KonversationApplication::preferences.getAutoUserhost());
  userhostTimer.start(10000);

  m_firstAutoWhoDone = false;
  connect(&m_whoTimer,SIGNAL (timeout()),this,SLOT (autoWho()));
  // re-schedule when the settings were changed
  connect(&KonversationApplication::preferences,SIGNAL (autoContinuousWhoChanged()),this,SLOT (scheduleAutoWho()));

  m_allowNotifications = true;


//FIXME JOHNFLUX
//  connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
//  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));
}

void Channel::setServer(Server *server) {
  ChatWindow::setServer(server);
  topicLine->setServer(server);
  refreshModeButtons();

  connect(server->getOutputFilter(),SIGNAL(cycleChannel()),this,SLOT(cycleChannel()));
}

Channel::~Channel()
{
  kdDebug() << "Channel::~Channel(" << getName() << ")" << endl;

  KonversationApplication::preferences.setChannelSplitter(splitter->sizes());
  KonversationApplication::preferences.setTopicSplitterSizes(m_vertSplitter->sizes());
  KConfig* config = kapp->config();
  config->setGroup("Appearance");
  config->writeEntry("TopicSplitterSizes", m_vertSplitter->sizes());

  // Purge nickname list
  purgeNicks();

  // Unlink this channel from channel list
  m_server->removeChannel(this);
}

ChannelNickPtr Channel::getOwnChannelNick() {
  return m_ownChannelNick;
}

ChannelNickPtr Channel::getChannelNick(const QString &ircnick) {
  return m_server->getChannelNick(getName(), ircnick);
}
void Channel::purgeNicks()
{
  // Purge nickname list
  nicknameList.clear();

  // clear stats counter
  nicks=0;
  ops=0;
}


void Channel::showOptionsDialog()
{
  (new Konversation::ChannelOptionsDialog(this))->show();
}

void Channel::filesDropped(QDropEvent* e)
{
  QPoint p(nicknameListView->contentsToViewport(e->pos()));
  NickListViewItem* it = dynamic_cast<NickListViewItem*>(nicknameListView->itemAt(p));
  if (!it) return;
  QStrList uris;
  if (QUriDrag::decode(e,uris)) m_server->sendURIs(uris, it->getNick()->getNickname());
}


void Channel::textPasted(const QString& text)
{
  if(m_server)
  {
    QStringList multiline=QStringList::split('\n',text);
    for(unsigned int index=0;index<multiline.count();index++)
    {
      QString line=multiline[index];
      QString cChar(KonversationApplication::preferences.getCommandChar());
      // make sure that lines starting with command char get escaped
      if(line.startsWith(cChar)) line=cChar+line;
      sendChannelText(line);
    }
  }
}

// Will be connected to NickListView::popupCommand(int) and IRCView::popupCommand(int)
void Channel::popupCommand(int id)
{
  QString pattern;
  QString cc=KonversationApplication::preferences.getCommandChar();
  bool raw=false;

  QString args;

  if(!textView->getContextNick().isEmpty())
    {
      nicknameListView->clearSelection();
      QListViewItem* item = nicknameListView->findItem(textView->getContextNick(),1);
      nicknameListView->setSelected(item,true);
      nicknameListView->ensureItemVisible(item);
      textView->clearContextNick();
    }

  switch(id)
  {
  case Konversation::AddressbookEdit:
      {
        ChannelNickList nickList=getSelectedChannelNicks();
	for(ChannelNickList::Iterator it=nickList.begin();it!=nickList.end();++it) {
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
	if(addressbook->getAndCheckTicket()) {
          for(ChannelNickList::Iterator it=nickList.begin();it!=nickList.end();++it) {
	    if(id == Konversation::AddressbookDelete) {
              KABC::Addressee addr = (*it)->getNickInfo()->getAddressee();
   	      addressbook->unassociateNick(addr, (*it)->getNickname(), m_server->getServerName(), m_server->getServerGroup());
	    } else {
	      //make new addressbook contact
              KABC::Addressee addr;
	      NickInfoPtr nickInfo = (*it)->getNickInfo();
	      if(nickInfo->getRealName().isEmpty())
		addr.setGivenName(nickInfo->getNickname());
	      else
		addr.setGivenName(nickInfo->getRealName());
	      addr.setNickName(nickInfo->getNickname());
	      addressbook->associateNickAndUnassociateFromEveryoneElse(addr, (*it)->getNickname(), m_server->getServerName(), m_server->getServerGroup());
	    }
          }
	  addressbook->saveTicket();  //This will refresh the nicks automatically for us. At least, if it doesn't, it's a bug :)
        }
        break;
      }
  case Konversation::AddressbookChange:
      {
        ChannelNickList nickList=getSelectedChannelNicks();
        for(ChannelNickList::Iterator it=nickList.begin();it!=nickList.end();++it) {
	  (*it)->getNickInfo()->showLinkAddressbookUI();
	}
        break;
      }
  case Konversation::SendEmail:
      {
        Konversation::Addressbook::self()->sendEmail(getSelectedChannelNicks());
        break;
      }
  case Konversation::AddressbookSub:
      kdDebug() << "sub called" << endl;
      break;
  case Konversation::GiveOp:
      pattern="MODE %c +o %u";
      raw=true;
      break;
  case Konversation::TakeOp:
      pattern="MODE %c -o %u";
      raw=true;
      break;
  case Konversation::GiveVoice:
      pattern="MODE %c +v %u";
      raw=true;
      break;
  case Konversation::TakeVoice:
      pattern="MODE %c -v %u";
      raw=true;
      break;
  case Konversation::Version:
      pattern="PRIVMSG %u :\x01VERSION\x01";
      raw=true;
      break;
  case Konversation::Whois:
      pattern="WHOIS %u %u";
      raw=true;
      break;
  case Konversation::Ping:
      {
        unsigned int time_t = QDateTime::currentDateTime().toTime_t();
        pattern=QString(KonversationApplication::preferences.getCommandChar()+"CTCP %u PING %1").arg(time_t);
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
      pattern=cc+"BAN -HOST %u\n"+
              cc+"KICK %u";
      break;
  case Konversation::KickBanDomain:
      pattern=cc+"BAN -DOMAIN %u\n"+
              cc+"KICK %u";
      break;
  case Konversation::KickBanUserHost:
      pattern=cc+"BAN -USERHOST %u\n"+
              cc+"KICK %u";
      break;
  case Konversation::KickBanUserDomain:
      pattern=cc+"BAN -USERDOMAIN %u\n"+
              cc+"KICK %u";
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
      pattern=cc+"IGNORE -ALL %u!*";
      break;
  } // switch

  if(!pattern.isEmpty())
  {
    pattern.replace("%c",getName());

    ChannelNickList nickList=getSelectedChannelNicks();

    QString command;
    for(ChannelNickList::Iterator it=nickList.begin();it!=nickList.end();++it)
    {
      QStringList patternList=QStringList::split('\n',pattern);

      for(unsigned int index=0;index<patternList.count();index++)
      {
        command=patternList[index];
        command.replace("%u",(*it)->getNickname());

        if(raw)
          m_server->queue(command);
        else
          sendChannelText(command);
      }
    }
  }
}

// Will be connected to NickListView::doubleClicked()
void Channel::doubleClickCommand(QListViewItem* item)
{
  if(item)
  {
    // TODO: put the quick button code in another function to make reusal more legitimate
    quickButtonClicked(KonversationApplication::preferences.getChannelDoubleClickAction());
  }
}

void Channel::completeNick()
{
  int pos; // = cursorPosition();
  int oldPos; // = cursorPosition();

  channelInput->getCursorPosition(&oldPos,&pos);  // oldPos is a dummy here, taking the paragraph parameter
  oldPos=channelInput->getOldCursorPosition();

  QString line=channelInput->text();
  QString newLine;
  // Check if completion position is out of range
  if(completionPosition>=nicknameList.count()) completionPosition=0;
  // Check, which completion mode is active
  char mode=channelInput->getCompletionMode();

  if(mode == 'c') {
    line.remove(oldPos,pos-oldPos);
    pos = oldPos;
  }

  // If the cursor is at beginning of line, insert last completion
  if(pos == 0 && !channelInput->lastCompletion().isEmpty())
  {
    QString addStart(KonversationApplication::preferences.getNickCompleteSuffixStart());
    newLine = channelInput->lastCompletion() + addStart;
    // New cursor position is behind nickname
    pos = newLine.length();
    // Add rest of the line
    newLine += line;
  }
  else
  {
    // set channel nicks completion mode
    channelInput->setCompletionMode('c');
    // remember old cursor position in input field
    channelInput->setOldCursorPosition(pos);
    // remember old cursor position locally
    oldPos=pos;
    // remember old nick completion position
    unsigned int oldCompletionPosition=completionPosition;
    // step back to last space or start of line
    while(pos && line[pos-1]!=' ') pos--;
    // copy search pattern (lowercase)
    QString pattern=line.mid(pos,oldPos-pos);
    // copy line to newLine-buffer
    newLine=line;
    // did we find any pattern?
    if(!pattern.isEmpty())
    {
      bool complete = false;
      QString foundNick;

      // try to find matching nickname in list of names
      if(KonversationApplication::preferences.getNickCompletionMode() == 1 ||
        KonversationApplication::preferences.getNickCompletionMode() == 2)
      { // Shell like completion
        QStringList found;
        foundNick = nicknameList.completeNick(pattern, complete, found,
          (KonversationApplication::preferences.getNickCompletionMode() == 2),
          KonversationApplication::preferences.nickCompletionCaseSensitive());

        if(!complete && !found.isEmpty()) {
          if(KonversationApplication::preferences.getNickCompletionMode() == 1) {
            QString nicksFound = found.join(" ");
            appendServerMessage(i18n("Completion"),i18n("Possible completions: %1.").arg(nicksFound));
          } else {
            channelInput->showCompletionList(found);
          }
        }
      } else if(KonversationApplication::preferences.getNickCompletionMode() == 0) { // Cycle completion
        complete = true;
        QString prefixCharacter = KonversationApplication::preferences.getPrefixCharacter();
        do
        {
          QString lookNick=nicknameList.at(completionPosition)->getNickname();

          if ( !prefixCharacter.isEmpty() && lookNick.contains(prefixCharacter) ) {
             lookNick = lookNick.section( prefixCharacter,1 );
          }

          if(lookNick.startsWith(pattern, KonversationApplication::preferences.nickCompletionCaseSensitive())) {
            foundNick=lookNick;
          }

          // increment search position
          completionPosition++;

          // wrap around
          if(completionPosition==nicknameList.count()) {
            completionPosition=0;
          }

          // the search ends when we either find a suitable nick or we end up at the
          // first search position
        } while(completionPosition!=oldCompletionPosition && foundNick.isEmpty());
      }

      // did we find a suitable nick?
      if(!foundNick.isEmpty())
      {
        // remove pattern from line
        newLine.remove(pos,pattern.length());

        // did we find the nick in the middle of the line?
        if(pos && complete)
        {
          channelInput->setLastCompletion(foundNick);
          QString addMiddle(KonversationApplication::preferences.getNickCompleteSuffixMiddle());
          newLine.insert(pos,foundNick+addMiddle);
          pos=pos+foundNick.length()+addMiddle.length();
        }
        // no, it was at the beginning
        else if(complete)
        {
          channelInput->setLastCompletion(foundNick);
          QString addStart(KonversationApplication::preferences.getNickCompleteSuffixStart());
          newLine.insert(pos,foundNick+addStart);
          pos=pos+foundNick.length()+addStart.length();
        }
        // the nick wasn't complete
        else
        {
          newLine.insert(pos,foundNick);
          pos=pos+foundNick.length();
        }
      }
      // no pattern found, so restore old cursor position
      else pos=oldPos;
    }
  }
  // Set new text and cursor position
  channelInput->setText(newLine);
  channelInput->setCursorPosition(0,pos);
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
  setLogfileName(newName.lower());
}

void Channel::setKey(const QString& newKey)
{
  key=newKey;
}

QString Channel::getKey()
{
  return key;
}

void Channel::sendFileMenu()
{
  emit sendFile();
}

void Channel::channelTextEntered()
{
  QString line = channelInput->text();
  channelInput->clear();

  if(line.lower()=="/clear")
    textView->clear();
  else
    if(!line.isEmpty()) sendChannelText(line);
}

void Channel::sendChannelText(const QString& sendLine)
{
  // create a work copy
  QString output(sendLine);
  // replace aliases and wildcards
  if(m_server->getOutputFilter()->replaceAliases(output)) {
    output = m_server->parseWildcards(output,m_server->getNickname(),getName(),getKey(),
      getSelectedNickList(),QString::null);
  }

  // encoding stuff is done in Server()
  Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(),output,getName());

  // Is there something we need to display for ourselves?
  if(!result.output.isEmpty())
  {
    if(result.type == Konversation::Action) appendAction(m_server->getNickname(), result.output);
    else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
    else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
    else if(result.type == Konversation::Query) appendQuery(result.typeString, result.output);
    else append(m_server->getNickname(), result.output);
  }
  // Send anything else to the server
  if(!result.toServer.isEmpty()) {
    m_server->queue(result.toServer);
  } else {
    m_server->queueList(result.toServerList);
  }
}

void Channel::newTextInView(const QString& highlightColor,bool important)
{
  emit newText(this,highlightColor,important);
}

void Channel::setNickname(const QString& newNickname)
{
  nicknameCombobox->setCurrentText(newNickname);
}
QStringList Channel::getSelectedNickList() {
  QStringList result;
  Nick* nick=nicknameList.first();

  while(nick)
  {
    if(nick->isSelected()) result.append(nick->getNickname());
    nick=nicknameList.next();
  }

  return result;

}

ChannelNickList Channel::getSelectedChannelNicks()
{
  ChannelNickList result;
  Nick* nick=nicknameList.first();

  while(nick)
  {
    if(nick->isSelected()) result.append(nick->getChannelNick());
    nick=nicknameList.next();
  }

  return result;

}

void Channel::channelLimitChanged()
{
  unsigned int lim=limit->text().toUInt();

  modeButtonClicked(7,lim>0);
}

void Channel::modeButtonClicked(int id,bool on)
{
  char mode[]={'t','n','s','i','p','m','k','l'};
  QString command("MODE %1 %2%3 %4");
  QString args;

  if(mode[id]=='k')
  {
    if(getKey().isEmpty())
    {
      QCString key;

      int result=KPasswordDialog::getPassword(key,i18n("Channel Password"));

      if(result==KPasswordDialog::Accepted && !key.isEmpty()) setKey(key);
    }
    args=getKey();
    if(!on) setKey(QString::null);
  }
  else if(mode[id]=='l')
  {
    if(limit->text().isEmpty() && on)
    {
      bool ok=false;
      // ask user how many nicks should be the limit
      args=KInputDialog::getText(i18n("Nick Limit"),
                                 i18n("Enter the new nick limit:"),
                                 limit->text(),                      // will be always "" but what the hell ;)
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
  QString out=m_server->parseWildcards(buttonText,m_server->getNickname(),getName(),getKey(),getSelectedNickList(),QString::null);
  // are there any newlines in the definition?
  if(out.find('\n')!=-1)
  {
    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',out);
    for(unsigned int index=0;index<outList.count();index++)
    {
      sendChannelText(outList[index]);
    }
  }
  // single line without newline needs to be copied into input line
  else channelInput->setText(out);
}
void Channel::addNickname(ChannelNickPtr channelnick)
{

  QString nickname = channelnick->loweredNickname();

  Nick* nick=0;
  Nick* lookNick;
  QPtrListIterator<Nick> it(nicknameList);

  while((lookNick = it.current()) != 0)
  {
    if(lookNick->loweredNickname() == nickname) {
      nick = lookNick;
      break;
    }

    ++it;
  }

  if(nick == 0)
  {
    fastAddNickname(channelnick);

    if(channelnick->isAdmin() || channelnick->isOwner() || channelnick->isOp() || channelnick->isHalfOp())
    {
      adjustOps(1);
    }

    adjustNicks(1);
    requestNickListSort();
  }
  else
  {
    Q_ASSERT(false); //We shouldn't be adding someone that is already in the channel.
  }
}

// Use with caution! Does not sort or check for duplicates!
void Channel::fastAddNickname(ChannelNickPtr channelnick) {
  Q_ASSERT(channelnick);
  if(!channelnick) return;
  Nick* nick = new Nick(nicknameListView, channelnick);
  // nicks get sorted later
  nicknameList.append(nick);
}

void Channel::nickRenamed(const QString &oldNick, const NickInfo& nickInfo) {
  Q_ASSERT(&nickInfo);
  Q_ASSERT(!oldNick.isEmpty());
  /* Did we change our nick name? */
  QString newNick = nickInfo.getNickname();
  if(oldNick==m_server->getNickname()) {
    setNickname(newNick);
    appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.").arg(newNick), false, true, true);
  } else {
    /* No, must've been someone else */
    appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.").arg(oldNick).arg(newNick),false);
  }

  nicknameListView->sort();

}
void Channel::joinNickname(ChannelNickPtr channelNick) {
  if(channelNick->getNickname() == m_server->getNickname())
  {
    appendCommandMessage(i18n("Join"),i18n("You have joined channel %1. (%2)").arg(getName()).arg(channelNick->getHostmask()),false, false);
    m_ownChannelNick = channelNick;
    connect(m_ownChannelNick, SIGNAL(channelNickChanged()), SLOT(refreshModeButtons()));
    refreshModeButtons();
    KWin::demandAttention(KonversationApplication::instance()->getMainWindow()->winId());

  } else {
    appendCommandMessage(i18n("Join"),i18n("%1 has joined this channel. (%2)").arg(channelNick->getNickname()).arg(channelNick->getHostmask()),false, false);
    addNickname(channelNick);
  }
}
void Channel::removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit) {
  if(channelNick->getNickname() == m_server->getNickname())
  {
    //If in the future we can leave a channel, but not close the window, refreshModeButtons() has to be called.
    if(quit) {
      appendCommandMessage(i18n("Quit"),i18n("You have left this server. (%1)").arg(reason),false);
    } else {
      appendCommandMessage(i18n("Part"),i18n("You have left channel %1. (%2)").arg(getName()).arg(reason),false);
    }

#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  } else {
    if(quit) {
      appendCommandMessage(i18n("Quit"),i18n("%1 has left this server. (%2)").arg(channelNick->getNickname()).arg(reason),false);
    } else {
      appendCommandMessage(i18n("Part"),i18n("%1 has left this channel. (%2)").arg(channelNick->getNickname()).arg(reason),false);
    }

    if(channelNick->isOp() || channelNick->isOwner() || channelNick->isAdmin() || channelNick->isHalfOp()) {
      adjustOps(-1);
    }

    adjustNicks(-1);
    Nick* nick = getNickByName(channelNick->getNickname());

    if(nick) {
      nicknameList.removeRef(nick);
    } else {
      kdWarning() << "Channel::kickNick(): Nickname " << channelNick->getNickname() << " not found!"<< endl;
    }
  }
}
void Channel::kickNick(ChannelNickPtr channelNick, const ChannelNick &kicker, const QString &reason) {
  if(channelNick->getNickname()==m_server->getNickname())
  {
    if(kicker.getNickname()==m_server->getNickname())
    {
      appendCommandMessage(i18n("Kick"),i18n("You have kicked yourself from the channel. (%1)").arg(reason));
      /* This message lets the user see what he has done after the channel window went away */
      m_server->appendStatusMessage(i18n("Kick"),i18n("You have kicked yourself from channel %1. (%2)").arg(getName()).arg(reason));
    }
    else
    {
      appendCommandMessage(i18n("Kick"),i18n("You have been kicked from the channel by %1. (%2)").arg(kicker.getNickname()).arg(reason));
      /* This message lets the user see what had happened after the channel window went away */
      m_server->appendStatusMessage(i18n("Kick"),i18n("You have been kicked from channel %1 by %2. (%3)").arg(getName()).arg(kicker.getNickname()).arg(reason));
      KonversationApplication* konv_app = static_cast<KonversationApplication*>(KApplication::kApplication());
      konv_app->notificationHandler()->kick(this,getName(),kicker.getNickname());
    }
#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  }
  else
  {
    if(kicker.getNickname()==m_server->getNickname())
      appendCommandMessage(i18n("Kick"),i18n("You have kicked %1 from the channel. (%2)").arg(channelNick->getNickname()).arg(reason));
    else
      appendCommandMessage(i18n("Kick"),i18n("%1 has been kicked from the channel by %2. (%3)").arg(channelNick->getNickname()).arg(kicker.getNickname()).arg(reason));

//TODO - Is this right??  Why not || isHalfop etc etc
    if(channelNick->isOp() || channelNick->isOwner() || channelNick->isAdmin() || channelNick->isHalfOp()) adjustOps(-1);
    adjustNicks(-1);

    Nick* nick=getNickByName(channelNick->getNickname());
    if(nick==0) kdWarning() << "Channel::kickNick(): Nickname " << channelNick->getNickname() << " not found!"<< endl;
    else
    {
      nicknameList.removeRef(nick);
    }
  }



}
Nick* Channel::getNickByName(const QString &lookname)
{
//FIXME I don't think this works
  QString lcLookname = lookname.lower();
  Nick* nick=nicknameList.first();
  while(nick)
  {
    if(nick->loweredNickname() == lcLookname) 
      return nick;
    nick=nicknameList.next();
  }
  return 0;
}
void Channel::adjustNicks(int value)
{
  if((nicks == 0) && (value <= 0)) {
    return;
  }
  
  nicks += value;

  if(nicks < 0) {
    nicks = 0;
  }
  
  emitUpdateInfo();
}

void Channel::adjustOps(int value)
{
  if((ops == 0) && (value <= 0)) {
    return;
  }

  ops += value;

  if(ops < 0) {
    ops = 0;
  }

  emitUpdateInfo();
}

void Channel::emitUpdateInfo()
{
  QString info = getName() + " - ";
  info += i18n("%n nick", "%n nicks", numberOfNicks());
  info += i18n(" (%n op)", " (%n ops)", numberOfOps());

  emit updateInfo(info);
}

void Channel::setTopic(const QString &newTopic)
{
  appendCommandMessage(i18n("Topic"), i18n("The channel topic is \"%1\".").arg(newTopic));

  if(m_topicHistory.first() != newTopic)
  {
    m_topicHistory.prepend(i18n("<unknown> %1").arg(newTopic));
    QString topic = Konversation::removeIrcMarkup(newTopic);
    topicLine->setText(topic);

    emit topicHistoryChanged();
  }
}

void Channel::setTopic(const QString &nickname, const QString &newTopic) // Overloaded
{
  if(nickname == m_server->getNickname()) {
    appendCommandMessage(i18n("Topic"), i18n("You set the channel topic to \"%1\".").arg(newTopic));
  } else {
    appendCommandMessage(i18n("Topic"), i18n("%1 sets the channel topic to \"%2\".").arg(nickname).arg(newTopic));
  }

  m_topicHistory.prepend("<" + nickname + "> " + newTopic);
  QString topic = Konversation::removeIrcMarkup(newTopic);
  topicLine->setText(topic);

  emit topicHistoryChanged();
}

QStringList Channel::getTopicHistory() {
  return m_topicHistory;
}

QString Channel::getTopic() {
  return m_topicHistory[0];
}
void Channel::setTopicAuthor(const QString& newAuthor)
{
  if(topicAuthorUnknown)
  {
    m_topicHistory[0] = "<" + newAuthor + "> " + m_topicHistory[0].section(' ', 1);
    topicAuthorUnknown = false;
  }
}
void Channel::updateMode(QString sourceNick, char mode, bool plus, const QString &parameter)
{
  //Note for future expansion: doing m_server->getChannelNick(getName(), sourceNick);  may not return a valid channelNickPtr if the
  //mode is updated by the server.


  QString message(QString::null);
  ChannelNickPtr parameterChannelNick;

  bool fromMe=false;
  bool toMe=false;

  if(sourceNick.lower()==m_server->loweredNickname()) 
    fromMe=true;
  if(parameter.lower()==m_server->loweredNickname()) 
    toMe=true;

  switch(mode)
  {
    case 'a':
      if(plus)
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You give channel owner privileges to yourself.");
          else
            message=i18n("You give channel owner privileges to %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 gives channel owner privileges to you.").arg(sourceNick);
          else
            message=i18n("%1 gives channel owner privileges to %2.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You take channel owner privileges from yourself.");
          else
            message=i18n("You take channel owner privileges from %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 takes channel owner privileges from you.").arg(sourceNick);
          else
            message=i18n("%1 takes channel owner privileges from %2.").arg(sourceNick).arg(parameter);
        }
      }
      parameterChannelNick=m_server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isOwner() && !parameterChannelNick->isOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isOwner() && !parameterChannelNick->isOp()) adjustOps(-1);
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
            message=i18n("You give channel operator privileges to %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 gives channel operator privileges to you.").arg(sourceNick);
          else
            message=i18n("%1 gives channel operator privileges to %2.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You take channel operator privileges from yourself.");
          else
            message=i18n("You take channel operator privileges from %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 takes channel operator privileges from you.").arg(sourceNick);
          else
            message=i18n("%1 takes channel operator privileges from %2.").arg(sourceNick).arg(parameter);
        }
      }
      parameterChannelNick=m_server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isOp()) adjustOps(-1);
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
            message=i18n("You give channel halfop privileges to %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 gives channel halfop privileges to you.").arg(sourceNick);
          else
            message=i18n("%1 gives channel halfop privileges to %2.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe)
            message=i18n("You take channel halfop privileges from yourself.");
          else
            message=i18n("You take channel halfop privileges from %1.").arg(parameter);
        }
        else
        {
          if(toMe)
            message=i18n("%1 takes channel halfop privileges from you.").arg(sourceNick);
          else
            message=i18n("%1 takes channel halfop privileges from %2.").arg(sourceNick).arg(parameter);
        }
      }
      parameterChannelNick=m_server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isHalfOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isHalfOp()) adjustOps(-1);
        parameterChannelNick->setHalfOp(plus);
        emitUpdateInfo();
        nicknameListView->sort();
      }
    break;

    case 'O': break;

    case 'v':
      if(plus)
      {
        if(fromMe)
        {
          if(toMe) message=i18n("You give yourself the permission to talk.");
          else     message=i18n("You give %1 the permission to talk.").arg(parameter);
        }
        else
        {
          if(toMe) message=i18n("%1 gives you the permission to talk.").arg(sourceNick);
          else     message=i18n("%1 gives %2 the permission to talk.").arg(sourceNick).arg(parameter);
        }
      }
      else
      {
        if(fromMe)
        {
          if(toMe) message=i18n("You take the permission to talk from yourself.");
          else     message=i18n("You take the permission to talk from %1.").arg(parameter);
        }
        else
        {
          if(toMe) message=i18n("%1 takes the permission to talk from you.").arg(sourceNick);
          else     message=i18n("%1 takes the permission to talk from %2.").arg(sourceNick).arg(parameter);
        }
      }
      parameterChannelNick=m_server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
	parameterChannelNick->setVoice(plus);
	nicknameListView->sort();
      }
    break;

    case 'c':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'no colors allowed'.");
        else message=i18n("%1 sets the channel mode to 'no colors allowed'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'allow color codes'.");
        else message=i18n("%1 sets the channel mode to 'allow color codes'.").arg(sourceNick);
      }
      modeM->setDown(plus);
    break;

    case 'i':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'invite only'.");
        else message=i18n("%1 sets the channel mode to 'invite only'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You remove the 'invite only' mode from the channel.");
        else message=i18n("%1 removes the 'invite only' mode from the channel.").arg(sourceNick);
      }
      modeI->setDown(plus);
    break;

    case 'm':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'moderated'.");
        else message=i18n("%1 sets the channel mode to 'moderated'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'unmoderated'.");
        else message=i18n("%1 sets the channel mode to 'unmoderated'.").arg(sourceNick);
      }
      modeM->setDown(plus);
    break;

    case 'n':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'no messages from outside'.");
        else message=i18n("%1 sets the channel mode to 'no messages from outside'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'allow messages from outside'.");
        else message=i18n("%1 sets the channel mode to 'allow messages from outside'.").arg(sourceNick);
      }
      modeN->setDown(plus);
    break;

    case 'q':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'quiet'.");
        else message=i18n("%1 sets the channel mode to 'quiet'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You remove the 'quiet' channel mode.");
        else message=i18n("%1 removes the 'quiet' channel mode.").arg(sourceNick);
      }

    break;

    case 'p':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'private'.");
        else message=i18n("%1 sets the channel mode to 'private'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'public'.");
        else message=i18n("%1 sets the channel mode to 'public'.").arg(sourceNick);
      }
      modeP->setDown(plus);
      if(plus) modeS->setDown(false);
    break;

    case 's':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel mode to 'secret'.");
        else message=i18n("%1 sets the channel mode to 'secret'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You set the channel mode to 'visible'.");
        else message=i18n("%1 sets the channel mode to 'visible'.").arg(sourceNick);
      }
      modeS->setDown(plus);
      if(plus) modeP->setDown(false);
    break;

    case 'r': break;

    case 't':
      if(plus)
      {
        if(fromMe) message=i18n("You switch on 'topic protection'.");
        else message=i18n("%1 switches on 'topic protection'.").arg(sourceNick);
      }
      else
      {
        if(fromMe) message=i18n("You switch off 'topic protection'.");
        else message=i18n("%1 switches off 'topic protection'.").arg(sourceNick);
      }
      modeT->setDown(plus);
    break;

    case 'k': break;

    case 'l':
      if(plus)
      {
        if(fromMe) message=i18n("You set the channel limit to %1 nicks.").arg(parameter);
        else message=i18n("%1 sets the channel limit to %2 nicks.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the channel limit.");
        else message=i18n("%1 removes the channel limit.").arg(sourceNick);
      }
      modeL->setDown(plus);
      if(plus) limit->setText(parameter);
      else limit->clear();
    break;

    case 'b':
      if(plus)
      {
        if(fromMe) message=i18n("You set a ban on %1.").arg(parameter);
        else message=i18n("%1 sets a ban on %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the ban on %1.").arg(parameter);
        else message=i18n("%1 removes the ban on %2.").arg(sourceNick).arg(parameter);
      }
    break;

    case 'e':
      if(plus)
      {
        if(fromMe) message=i18n("You set a ban exception on %1.").arg(parameter);
        else message=i18n("%1 sets a ban exception on %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the ban exception on %1.").arg(parameter);
        else message=i18n("%1 removes the ban exception on %2.").arg(sourceNick).arg(parameter);
      }
    break;

    case 'I':
      if(plus)
      {
        if(fromMe) message=i18n("You set invitation mask %1.").arg(parameter);
        else message=i18n("%1 sets invitation mask %2.").arg(sourceNick).arg(parameter);
      }
      else
      {
        if(fromMe) message=i18n("You remove the invitation mask %1.").arg(parameter);
        else message=i18n("%1 removes the invitation mask %2.").arg(sourceNick).arg(parameter);
      }
    break;
  }
  
  if(!message.isEmpty() && !KonversationApplication::preferences.getUseLiteralModes()) {
    appendCommandMessage(i18n("Mode"),message);
  }

  updateModeWidgets(mode,plus,parameter);
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

  if(plus) {
    m_modeList.append(QString(mode + parameter));
  } else {
    QStringList removable = m_modeList.grep(QRegExp(QString("^%1.*").arg(mode)));

    for(QStringList::iterator it = removable.begin(); it != removable.end(); ++it) {
      m_modeList.remove(m_modeList.find((*it)));
    }
  }
  emit modesChanged();
}

void Channel::updateQuickButtons(const QStringList &newButtonList)
{
  for(int index=0;index<8;index++)
  {
    // Get the button definition
    QString buttonText=newButtonList[index];
    // Extract button label
    QString buttonLabel=buttonText.section(',',0,0);
    // Extract button definition
    buttonText=buttonText.section(',',1);

    QuickButton* quickButton=buttonList.at(index);
    quickButton->setText(buttonLabel);
    quickButton->setDefinition(buttonText);

    // Update tool tips
    QToolTip::remove(quickButton);
    QString toolTip=buttonText.replace("&","&amp;").
                    replace("<","&lt;").
                    replace(">","&gt;");

    QToolTip::add(quickButton,toolTip);
  }
}

void Channel::showQuickButtons(bool show)
{
  // QT does not redraw the buttons properly when they are not on screen
  // while getting hidden, so we remember the "soon to be" state here.
  if(isHidden())
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
  // QT does not redraw the buttons properly when they are not on screen
  // while getting hidden, so we remember the "soon to be" state here.
  if(isHidden())
  {
    modeButtonsChanged=true;
    modeButtonsState=show;
  }
  else
  {
    if(show) {
      modeBox->show();
      modeBox->parentWidget()->show();
    } else {
      modeBox->hide();

      if(topicLine->isHidden()) {
        modeBox->parentWidget()->hide();
      }
    }
  }
}

void Channel::indicateAway(bool show)
{
  // QT does not redraw the label properly when they are not on screen
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
  if(splitterChanged)
  {
    splitterChanged=false;
    QValueList<int> sizes = KonversationApplication::preferences.getChannelSplitter();

    if(sizes.isEmpty()) {
      int listWidth = nicknameListView->columnWidth(0) + nicknameListView->columnWidth(1);
      sizes << (splitter->width() - listWidth) << listWidth;
    }
    
    splitter->setSizes(sizes);
    sizes = KonversationApplication::preferences.topicSplitterSizes();

    if(sizes.isEmpty()) {
      sizes << m_topicButton->height() << (m_vertSplitter->height() - m_topicButton->height());
    }

    m_vertSplitter->setSizes(sizes);
  }
  if(awayChanged)
  {
    awayChanged=false;
    indicateAway(awayState);
  }
}

void Channel::updateFonts()
{
  nicknameCombobox->setFont(KonversationApplication::preferences.getTextFont());

  QString fgString;
  QString bgString;
  QString abgString;

  if(KonversationApplication::preferences.getColorInputFields())
  {
    fgString="#"+KonversationApplication::preferences.getColor("ChannelMessage");
    bgString="#"+KonversationApplication::preferences.getColor("TextViewBackground");
    abgString="#"+KonversationApplication::preferences.getColor("AlternateBackground");
  }
  else
  {
    fgString=colorGroup().foreground().name();
    bgString=colorGroup().base().name();
    // get alternate background color from cache
    abgString=abgCache;
  }

  const QColor fg(fgString);
  const QColor bg(bgString);
  const QColor abg(abgString);

  channelInput->setPaletteForegroundColor(fg);
  channelInput->setPaletteBackgroundColor(bg);
  channelInput->setFont(KonversationApplication::preferences.getTextFont());

  limit->setPaletteForegroundColor(fg);
  limit->setPaletteBackgroundColor(bg);
  limit->setFont(KonversationApplication::preferences.getTextFont());

  //topicLine->lineEdit()->setPaletteForegroundColor(fg);
  //topicLine->lineEdit()->setPaletteBackgroundColor(bg);
  topicLine->setFont(KonversationApplication::preferences.getTextFont());

  getTextView()->setFont(KonversationApplication::preferences.getTextFont());

  if(KonversationApplication::preferences.getShowBackgroundImage()) {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                  KonversationApplication::preferences.getBackgroundImageName());
  } else {
    getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
      QString::null);
  }

  nicknameListView->sort();

  nicknameListView->setPaletteForegroundColor(fg);
  nicknameListView->setPaletteBackgroundColor(bg);
  nicknameListView->setAlternateBackground(abg);
  nicknameListView->setFont(KonversationApplication::preferences.getListFont());
}

void Channel::updateStyleSheet()
{
  getTextView()->updateStyleSheet();
}

void Channel::nicknameComboboxChanged()
{
  QString newNick=nicknameCombobox->currentText();
  oldNick=m_server->getNickname();
  if(oldNick == newNick) return; //nothing changed
  
  nicknameCombobox->setCurrentText(oldNick);
  m_server->queue("NICK "+newNick);
}

void Channel::changeNickname(const QString& newNickname)
{
  m_server->queue("NICK "+newNickname);
}

void Channel::addPendingNickList(const QStringList& pendingChannelNickList)
{
  if(!getPendingNicks())
  {
    purgeNicks();
    setPendingNicks(true);

    if(!m_processingTimer) {
      m_processingTimer = new QTimer(this);
      connect(m_processingTimer, SIGNAL(timeout()), this, SLOT(processPendingNicks()));
    }
  }

  m_pendingChannelNickLists.append(pendingChannelNickList);

  if(!m_processingTimer->isActive()) {
    nicknameListView->setUpdatesEnabled(false);
    m_processingTimer->start(0);
  }
}

void Channel::setPendingNicks(bool state)
{
  pendingNicks=state;
}

bool Channel::getPendingNicks()
{
  return pendingNicks;
}

QPtrList<Nick> Channel::getNickList()
{
  return nicknameList;
}
void Channel::childAdjustFocus()
{
  channelInput->setFocus();
  refreshModeButtons(); //not really needed i think
}

void Channel::refreshModeButtons() {
  bool enable = true;
  if(getOwnChannelNick()){
    enable=getOwnChannelNick()->isAnyTypeOfOp();
  }  //if not channel nick, then enable is true - fall back to assuming they are op
    
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
  
  QToolTip::add(modeT, i18n("Topic can be changed by channel operator only.  %1").arg(opOnly));
  QToolTip::add(modeN, i18n("No messages to channel from clients on the outside.  %1").arg(opOnly));
  QToolTip::add(modeS, i18n("Secret channel.  %1").arg(opOnly));
  QToolTip::add(modeI, i18n("Invite only channel.  %1").arg(opOnly));
  QToolTip::add(modeP, i18n("Private channel.  %1").arg(opOnly));
  QToolTip::add(modeM, i18n("Moderated channel.  %1").arg(opOnly));
  QToolTip::add(modeK, i18n("Protect channel with a password."));
  QToolTip::add(modeL, i18n("Set user limit to channel."));

}

void Channel::cycleChannel()
{
  closeYourself();
  m_server->sendJoinCommand(getName());
}

void Channel::autoUserhost()
{
  if(KonversationApplication::preferences.getAutoUserhost())
  {
    int limit = 5;

    QString nickString;
    QPtrList<Nick> nickList = getNickList();
    QPtrListIterator<Nick> it(nickList);
    Nick* nick;

    while((nick = it.current()) != 0)
    {
      if(nick->getHostmask().isEmpty())
      {
        if(limit--) nickString = nickString + nick->getNickname() + " ";
        else break;
      }

      ++it;
    }

    if(!nickString.isEmpty()) m_server->requestUserhost(nickString);
  }
}

void Channel::autoUserhostChanged(bool state)
{
  if(state)
  {
    // we can't have automatic resizing with three columns; the hostname column is too wide
    nicknameListView->setResizeMode(QListView::NoColumn);
    // shrink the first column and let it re-expand, otherwise it stays
    // maximum width, leaving the hostmask column off the screen
    nicknameListView->setColumnWidth(1,32);
    nicknameListView->setColumnWidthMode(1,KListView::Maximum);

    // restart userhost timer
    userhostTimer.start(10000);
    // if the column was actually gone (just to be sure) ...
    if(nicknameListView->columns()==2)
    {
      // re-add the hostmask column
      nicknameListView->addColumn(QString::null);

      // re-add already known hostmasks
      QListViewItem* item=nicknameListView->itemAtIndex(0);
      while(item)
      {
        Nick* lookNick=getNickByName(item->text(1));
        if(lookNick) item->setText(2,lookNick->getHostmask());
        item=item->itemBelow();
      } // while
    }
  }
  else
  {
    userhostTimer.stop();
    if(nicknameListView->columns()==3) nicknameListView->removeColumn(2);

    // make the nick column resize itself automatically to prevent horizontal scrollbar
    nicknameListView->setResizeMode(QListView::LastColumn);
  }
}

void Channel::scheduleAutoWho()  // slot
{
  if(!m_firstAutoWhoDone)  // abort if initialization hasn't done yet
    return;
  if(m_whoTimer.isActive())
    m_whoTimer.stop();
  if(KonversationApplication::preferences.getAutoWhoContinuousEnabled())
    m_whoTimer.start(KonversationApplication::preferences.getAutoWhoContinuousInterval()*1000, true);
}

void Channel::autoWho()
{
  // don't use auto /WHO when the number of nicks is too large, or get banned.
  if(nicks > KonversationApplication::preferences.getAutoWhoNicksLimit())
  {
    scheduleAutoWho();
    return;
  }
  if(m_server->getInputFilter()->isWhoRequestUnderProcess(getName()))
    return;
  m_server->requestWho(getName());
}

QString Channel::getTextInLine() { return channelInput->text(); }

bool Channel::canBeFrontView()        { return true; }
bool Channel::searchView()       { return true; }

void Channel::appendInputText(const QString& s)
{
  channelInput->setText(channelInput->text() + s);
}

bool Channel::closeYourself()
{
#ifndef USE_MDI
  m_server->closeChannel(getName());
  m_server->removeChannel(this);
  delete this;
  return true;
#endif
}

void Channel::closeYourself(ChatWindow* /* view */)
{
#ifdef USE_MDI
  m_server->closeChannel(getName());
#endif
}

void Channel::serverQuit(const QString& reason)
{
#ifdef USE_MDI
  ChannelNickPtr channelNick=m_server->getChannelNick(getName(),m_server->getNickname());
  if(channelNick)  removeNick(channelNick,reason,true);
#endif
}

//Used to disable functions when not connected
void Channel::serverOnline(bool online)
{
  nicknameCombobox->setEnabled(online);
}

void Channel::showTopic(bool show)
{
  if(show) {
    topicLine->show();
    m_topicButton->show();
    topicLine->parentWidget()->show();
  } else {
    topicLine->hide();
    m_topicButton->hide();

    if(modeBox->isHidden()) {
      topicLine->parentWidget()->hide();
    }
  }
}

void Channel::processPendingNicks()
{
  bool admin = false;
  bool owner = false;
  bool op = false;
  bool halfop = false;
  bool voice = false;
  QString nickname = m_pendingChannelNickLists.first()[m_currentIndex];

  // remove possible mode characters from nickname and store the resulting mode
  m_server->mangleNicknameWithModes(nickname,admin,owner,op,halfop,voice);

  // TODO: make these an enumeration in KApplication or somewhere, we can use them as well
  unsigned int mode=(admin  ? 16 : 0)+
                    (owner  ?  8 : 0)+
                    (op     ?  4 : 0)+
                    (halfop ?  2 : 0)+
                    (voice  ?  1 : 0);

  ChannelNickPtr nick = m_server->addNickToJoinedChannelsList(getName(), nickname);
  Q_ASSERT(nick);
  nick->setMode(mode);

  fastAddNickname(nick);

  if(nick->isAdmin() || nick->isOwner() ||
    nick->isOp() || nick->isHalfOp())
  {
    m_opsToAdd++;
  }

  m_currentIndex++;

  if(m_pendingChannelNickLists.first().count() == m_currentIndex) {
    adjustNicks(m_pendingChannelNickLists.first().count());
    adjustOps(m_opsToAdd);
    m_pendingChannelNickLists.pop_front();
    m_currentIndex = 0;
    m_opsToAdd = 0;
  }

  if(m_pendingChannelNickLists.isEmpty()) {
    m_processingTimer->stop();
    nicknameListView->sort();
    sortNickList();
    nicknameListView->setUpdatesEnabled(true);
    nicknameListView->triggerUpdate();
    if(!m_firstAutoWhoDone)
    {
      autoWho();
      m_firstAutoWhoDone = true;
    }
  }
}

void Channel::setChannelEncoding(const QString& encoding)  // virtual
{
  KonversationApplication::preferences.setChannelEncoding(m_server->getServerGroup(), getName(), encoding);
}

QString Channel::getChannelEncoding()  // virtual
{
  return KonversationApplication::preferences.getChannelEncoding(m_server->getServerGroup(), getName());
}

QString Channel::getChannelEncodingDefaultDesc()  // virtual
{
  return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
}

void Channel::setShowNicknameBox(bool show)
{
  if(show) {
    nicknameCombobox->show();
  } else {
    nicknameCombobox->hide();
  }
}

void Channel::requestNickListSort()
{
  if(!m_delayedSortTimer) {
    m_delayedSortTimer = new QTimer(this);
    connect(m_delayedSortTimer, SIGNAL(timeout()), this, SLOT(sortNickList()));
  }

  if(!m_delayedSortTimer->isActive()) {
    m_delayedSortTimer->start(1000, true);
  }
}

void Channel::sortNickList()
{
  nicknameList.sort();
  nicknameListView->resort();

  if(m_delayedSortTimer) {
    m_delayedSortTimer->stop();
  }
}


//
// NickList
//

int NickList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  return QString::compare(static_cast<Nick*>(item1)->getNickname(),
    static_cast<Nick*>(item2)->getNickname());
}

QString NickList::completeNick(const QString& pattern, bool& complete, QStringList& found,
                               bool skipNonAlfaNum, bool caseSensitive)
{
  found.clear();
  QString prefix = "^";
  QString newNick;
  QString prefixCharacter = KonversationApplication::preferences.getPrefixCharacter();


  if((pattern.find(QRegExp("^(\\d|\\w)")) != -1) && skipNonAlfaNum) {
    prefix = "^([^\\d\\w]|[\\_]){0,}";
  }

  QRegExp regexp(prefix + QRegExp::escape(pattern));
  regexp.setCaseSensitive(caseSensitive);
  QPtrListIterator<Nick> it(*this);

  while(it.current() != 0) {
    newNick = it.current()->getNickname();

    if ( !prefix.isEmpty() && newNick.contains(prefixCharacter) )
       newNick = newNick.section( prefixCharacter,1 );

    if(newNick.find(regexp) != -1) {
      found.append(newNick);
    }

    ++it;
  }

  if(found.count() > 1) {
    bool ok = true;
    unsigned int patternLength = pattern.length();
    QString firstNick = found[0];
    unsigned int firstNickLength = firstNick.length();
    unsigned int foundCount = found.count();

    while(ok && ((patternLength) < firstNickLength)) {
      ++patternLength;
      QStringList tmp = found.grep(firstNick.left(patternLength), caseSensitive);

      if(tmp.count() != foundCount) {
        ok = false;
        --patternLength;
      }
    }

    complete = false;
    return firstNick.left(patternLength);
  } else if(found.count() == 1) {
    complete = true;
    return found[0];
  }

  return QString::null;
}

#include "channel.moc"
