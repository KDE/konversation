/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    channel.cpp  -  The class that controls a channel
    begin:     Wed Jan 23 2002
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com
*/

#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgrid.h>
#include <qsizepolicy.h>
#include <qheader.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <qsplitter.h>
#include <qcheckbox.h>
#include <qtimer.h>
#include <qcombobox.h>

#include <kprocess.h>

#include <klineedit.h>
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kdeversion.h>
#include <kmessagebox.h>

#include "konversationapplication.h"
#include "channel.h"
#include "server.h"
#include "nick.h"
#include "nicklistview.h"
#include "quickbutton.h"
#include "modebutton.h"
#include "topiccombobox.h"
#include "ircinput.h"
#include "ircview.h"
#include <kabc/addressbook.h>
#include <kabc/stdaddressbook.h>

#include "linkaddressbook/linkaddressbookui.h"
#include "linkaddressbook/addressbook.h"
#if QT_VERSION < 0x030100
#include "main.h"
#endif

#ifdef KDE_IS_VERSION
#if KDE_IS_VERSION(3,1,92)
#define OPAQUE_CONF
#endif
#endif

#ifdef USE_MDI
Channel::Channel(QString caption) : ChatWindow(caption)
#else
Channel::Channel(QWidget* parent) : ChatWindow(parent)
#endif
{
  // init variables
  nicks=0;
  ops=0;
  completionPosition=0;
  nickChangeDialog=0;
  topic=QString::null;

  quickButtonsChanged=false;
  quickButtonsState=false;

  splitterChanged=true;
  modeButtonsChanged=false;
  modeButtonsState=false;

  awayChanged=false;
  awayState=false;

  // no nicks pending from /names reply
  setPendingNicks(false);

  // flag for first seen topic
  topicAuthorUnknown=true;

  setType(ChatWindow::Channel);

  // Build some size policies for the widgets
  QSizePolicy hfixed=QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
  QSizePolicy hmodest=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
  QSizePolicy vmodest=QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  QSizePolicy vfixed=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
  QSizePolicy modest=QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
  QSizePolicy greedy=QSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

  // (this) The main Box, holding the channel view/topic and the input line
  splitter=new QSplitter(this);
#ifdef USE_MDI
  mainLayout->setStretchFactor(splitter,10);
#else
  setStretchFactor(splitter,10);
#endif

#ifdef OPAQUE_CONF
  splitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
#else
  splitter->setOpaqueResize(true);
#endif

  // The grid for the topic line and Channel View
  QVBox* topicViewNicksGrid=new QVBox(splitter);
  topicViewNicksGrid->setSpacing(spacing());

  // The box holding the Topic label/line, and the channel modes
  QHBox* topicBox=new QHBox(topicViewNicksGrid);
  topicBox->setSpacing(spacing());

  topicLabel=new QLabel(i18n("&Topic:"),topicBox);
  topicLine=new TopicComboBox(topicBox);
  topicLine->setEditable(true);
  topicLine->setAutoCompletion(false);
  topicLine->setInsertionPolicy(QComboBox::NoInsertion);
  topicLine->installEventFilter(this);

  // link topic label shortcut to topic line
  topicLabel->setBuddy(topicLine);

  // The box holding the channel modes
  modeBox=new QHBox(topicBox);
  modeBox->setSizePolicy(hfixed);
  modeT=new ModeButton("T",modeBox,0);
  modeN=new ModeButton("N",modeBox,1);
  modeS=new ModeButton("S",modeBox,2);
  modeI=new ModeButton("I",modeBox,3);
  modeP=new ModeButton("P",modeBox,4);
  modeM=new ModeButton("M",modeBox,5);
  modeK=new ModeButton("K",modeBox,6);
  modeL=new ModeButton("L",modeBox,7);

  // Tooltips for the ModeButtons
  QToolTip::add(modeT, i18n("Topic can be changed by channel operator only."));
  QToolTip::add(modeN, i18n("No messages to channel from clients on the outside."));
  QToolTip::add(modeS, i18n("Secret channel."));
  QToolTip::add(modeI, i18n("Invite only channel."));
  QToolTip::add(modeP, i18n("Private channel."));
  QToolTip::add(modeM, i18n("Moderated channel."));
  QToolTip::add(modeK, i18n("Protect channel with a password."));
  QToolTip::add(modeL, i18n("Set user limit to channel."));

  connect(modeT,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeN,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeS,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeI,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeP,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeM,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeK,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));
  connect(modeL,SIGNAL(clicked(int,bool)),this,SLOT(modeButtonClicked(int,bool)));

  limit=new KLineEdit(modeBox);
  connect(limit,SIGNAL (returnPressed()),this,SLOT (channelLimitChanged()) );
  limit->installEventFilter(this);

  showModeButtons(KonversationApplication::preferences.getShowModeButtons());

  setTextView(new IRCView(topicViewNicksGrid,NULL));  // Server will be set later in setServer()
  // The box that holds the Nick List and the quick action buttons
  QVBox* nickListButtons=new QVBox(splitter);
  nickListButtons->setSpacing(spacing());

  nicksOps=new QLabel(i18n("Nicks"),nickListButtons);
  nicksOps->setAlignment(AlignVCenter | AlignHCenter);

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
  nicknameListView->setColumnWidth(0,10);
  nicknameListView->setColumnAlignment(0,Qt::AlignHCenter);
  nicknameListView->installEventFilter(this);

  // the grid that holds the quick action buttons
  buttonsGrid=new QGrid(2,nickListButtons);
  // set hide() or show() on grid
  showQuickButtons(KonversationApplication::preferences.getShowQuickButtons());

  for(int index=0;index<8;index++)
  {
    // generate empty buttons first, text will be added by updateQuickButtons() later
    QuickButton* newQuickButton=new QuickButton(QString::null,QString::null,buttonsGrid);
    buttonList.append(newQuickButton);

    connect(newQuickButton,SIGNAL (clicked(const QString &)),this,SLOT (quickButtonClicked(const QString &)) );
  }

  updateQuickButtons(KonversationApplication::preferences.getButtonList());

  // The box holding the Nickname button and Channel input
  commandLineBox=new QHBox(this);
  commandLineBox->setSpacing(spacing());

  nicknameCombobox=new QComboBox(commandLineBox);
  nicknameCombobox->setEditable(true);
  nicknameCombobox->insertStringList(KonversationApplication::preferences.getNicknameList());
  nicknameCombobox->installEventFilter(this);
  oldNick = nicknameCombobox->currentText();
  awayLabel=new QLabel(i18n("(away)"),commandLineBox);
  awayLabel->hide();
  channelInput=new IRCInput(commandLineBox);
  channelInput->installEventFilter(this);

  // Set the widgets size policies
  topicBox->setSizePolicy(vmodest);
  topicLabel->setSizePolicy(hfixed);  // This should prevent the widget from growing too wide
  topicLine->setSizePolicy(vfixed);

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
  nicksOps->setSizePolicy(modest);
  nicknameListView->setSizePolicy(hmodest);
  // remember alternate background color
  abgCache=nicknameListView->alternateBackground().name();

  connect(channelInput,SIGNAL (submit()),this,SLOT (channelTextEntered()) );
  connect(channelInput,SIGNAL (nickCompletion()),this,SLOT (completeNick()) );
  connect(channelInput,SIGNAL (endCompletion()),this,SLOT (endCompleteNick()) );
  connect(channelInput,SIGNAL (textPasted(QString)),this,SLOT (textPasted(QString)) );

  connect(textView,SIGNAL (newText(const QString&,bool)),this,SLOT (newTextInView(const QString&,bool)) );
  connect(textView,SIGNAL (gotFocus()),this,SLOT (adjustFocus()) );
  connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
  connect(textView,SIGNAL (autoText(const QString&)),this,SLOT (sendChannelText(const QString&)) );

  connect(nicknameListView,SIGNAL (popupCommand(int)),this,SLOT (popupCommand(int)) );
  connect(nicknameListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT (doubleClickCommand(QListViewItem*)) );
  connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged(int)));

  connect(topicLine,SIGNAL (topicChanged(const QString&)),this,SLOT (requestNewTopic(const QString&)) );

  nicknameList.setAutoDelete(true);     // delete items when they are removed

  updateFonts();
  setLog(KonversationApplication::preferences.getLog());

  connect(&userhostTimer,SIGNAL (timeout()),this,SLOT (autoUserhost()));
  connect(&KonversationApplication::preferences,SIGNAL (autoUserhostChanged(bool)),this,SLOT (autoUserhostChanged(bool)));

  // every few seconds try to get more userhosts
  autoUserhostChanged(KonversationApplication::preferences.getAutoUserhost());
  userhostTimer.start(10000);

  m_allowNotifications = true;

//FIXME JOHNFLUX
//  connect( Konversation::Addressbook::self()->getAddressBook(), SIGNAL( addressBookChanged( AddressBook * ) ), this, SLOT( slotLoadAddressees() ) );
//  connect( Konversation::Addressbook::self(), SIGNAL(addresseesChanged()), this, SLOT(slotLoadAddressees()));
}

Channel::~Channel()
{
  kdDebug() << "Channel::~Channel(" << getName() << ")" << endl;

  KonversationApplication::preferences.setChannelSplitter(splitter->sizes());

  // Purge nickname list
  purgeNicks();

  // Unlink this channel from channel list
  server->removeChannel(this);
}

void Channel::purgeNicks()
{
  // Purge nickname list
  Nick* nick=nicknameList.first();
  while(nick)
  {
    // Remove the first element of the list
    nicknameList.removeRef(nick);
    // Again, get the first element in the list
    nick=nicknameList.first();
  }

  // clear stats counter
  nicks=0;
  ops=0;
}

void Channel::requestNewTopic(const QString& newTopic)
{
  topicLine->setCurrentText(topic);

  if(newTopic!=topic) sendChannelText(KonversationApplication::preferences.getCommandChar()+"TOPIC "+getName()+" "+newTopic);

  channelInput->setFocus();
}

void Channel::textPasted(QString text)
{
  if(server)
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

bool Channel::editAddressee(const QString &uid)
{
  Q_ASSERT(!uid.isEmpty());
  KProcess *proc = new KProcess;
  *proc << "kaddressbook";
  *proc << "--editor-only" << "--uid" << uid;
  kdDebug() << "running kaddressbook --editor-only --uid " << uid << endl;
  if(!proc->start()) {
    KMessageBox::error(this, "Could not run your addressbook program (kaddressbook).  This is most likely because it isn't installed.  Please install the 'kdepim' packages.");
    return false;
  }
  return true;
}

// Will be connected to NickListView::popupCommand(int)
void Channel::popupCommand(int id)
{
  QString pattern;
  QString cc=KonversationApplication::preferences.getCommandChar();
  bool raw=false;

  QString args;

  switch(id)
  {
/*    case NickListView::AddressbookEdit:
      {
        QStringList nickList=getSelectedNicksList();
	for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator) {
	  Nick *nick = getNickByName(*nickIterator);
	  if(!editAddressee(nick->getAddressee().uid())) break;
	}
	break;
      }
    case NickListView::AddressbookNew:
    case NickListView::AddressbookDelete:
      {
	Konversation::Addressbook *addressbook = Konversation::Addressbook::self();
        QStringList nickList=getSelectedNicksList();
	//Handle all the selected nicks in one go.  Either they all save, or none do.
	if(addressbook->getAndCheckTicket()) {
          for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator) {
	    if(id == NickListView::AddressbookDelete) {
              KABC::Addressee addr = addressbook->getKABCAddresseeFromNick(*nickIterator);
   	      addressbook->unassociateNick(addr, *nickIterator);
	    } else {
              KABC::Addressee addr;
	      addr.setGivenName(*nickIterator);
	      addr.setNickName(*nickIterator);
	      addressbook->associateNickAndUnassociateFromEveryoneElse(addr, *nickIterator);
	    }
          }
	  if(addressbook->saveTicket()) {
            //Nicks have changed.  Refresh.
	    for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator) {
	      Nick *nick = getNickByName(*nickIterator);
	      nick->refreshAddressee();
	      if(id == NickListView::AddressbookNew || NickListView::AddressbookEdit) {
		if(!editAddressee(nick->getAddressee().uid())) break;
	      }
	    }
	  }
        }
        break;
      }
    case NickListView::AddressbookChange:
      {
        QStringList nickList=getSelectedNicksList();
        for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator) {
	  LinkAddressbookUI *linkaddressbookui = new LinkAddressbookUI(this, NULL, *nickIterator);
	  linkaddressbookui->show();
	  getNickByName(*nickIterator)->refreshAddressee();
	}
        break;
      }
    case NickListView::AddressbookSub:
      kdDebug() << "sub called" << endl;
      break;*/
    case NickListView::GiveOp:
      pattern="MODE %c +o %u";
      raw=true;
      break;
    case NickListView::TakeOp:
      pattern="MODE %c -o %u";
      raw=true;
      break;
    case NickListView::GiveVoice:
      pattern="MODE %c +v %u";
      raw=true;
      break;
    case NickListView::TakeVoice:
      pattern="MODE %c -v %u";
      raw=true;
      break;
    case NickListView::Version:
      pattern="PRIVMSG %u :\x01VERSION\x01";
      raw=true;
      break;
    case NickListView::Whois:
      pattern="WHOIS %u";
      raw=true;
      break;
    case NickListView::Ping:
      {
#if QT_VERSION < 0x030100
        unsigned int time_t=toTime_t(QDateTime::currentDateTime());
#else
        unsigned int time_t=QDateTime::currentDateTime().toTime_t();
#endif
        pattern=QString(KonversationApplication::preferences.getCommandChar()+"CTCP %u PING %1").arg(time_t);
      }
      break;
    case NickListView::Kick:
      pattern=cc+"KICK %u";
      break;
    case NickListView::KickBan:
      pattern=cc+"BAN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::BanNick:
      pattern=cc+"BAN %u";
      break;
    case NickListView::BanHost:
      pattern=cc+"BAN -HOST %u";
      break;
    case NickListView::BanDomain:
      pattern=cc+"BAN -DOMAIN %u";
      break;
    case NickListView::BanUserHost:
      pattern=cc+"BAN -USERHOST %u";
      break;
    case NickListView::BanUserDomain:
      pattern=cc+"BAN -USERDOMAIN %u";
      break;
    case NickListView::KickBanHost:
      pattern=cc+"BAN -HOST %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanDomain:
      pattern=cc+"BAN -DOMAIN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanUserHost:
      pattern=cc+"BAN -USERHOST %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::KickBanUserDomain:
      pattern=cc+"BAN -USERDOMAIN %u\n"+
              cc+"KICK %u";
      break;
    case NickListView::Query:
      pattern=cc+"QUERY %u";
      break;
    case NickListView::DccSend:
      pattern=cc+"DCC SEND %u";
      break;
    case NickListView::Ignore:
      pattern=cc+"IGNORE -ALL %u!*";
      break;
  } // switch

  if(!pattern.isEmpty())
  {
    pattern.replace(QRegExp("%c"),getName());

    QStringList nickList=getSelectedNicksList();

    QString command;
    for(QStringList::Iterator nickIterator=nickList.begin();nickIterator!=nickList.end();++nickIterator)
    {
      QStringList patternList=QStringList::split('\n',pattern);

      for(unsigned int index=0;index<patternList.count();index++)
      {
        command=patternList[index];
        command.replace(QRegExp("%u"),*nickIterator);

        if(raw)
          server->queue(command);
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
  // Are we in query mode?
  if(mode=='q')
  {
    // Remove /msg <nick> part from string
    line=line.mid(pos);
    // Set cursor to beginning to restart query completion
    pos=0;
  }
  // Or maybe in channel mode?
  else if(mode=='c')
  {
    line.remove(oldPos,pos-oldPos);
    pos=oldPos;
  }
  // If the cursor is at beginning of line, insert /msq <query>
  if(pos==0)
  {
    // Find next query in list
    QString queryName=server->getNextQueryName();
    // Did we find any queries?
    if(!queryName.isEmpty())
    {
      // Prepend /msg name
      newLine="/msg "+queryName+" ";
      // New cursor position is behind nickname
      pos=newLine.length();
      // Add rest of the line
      newLine+=line;
      // Set query completion mode
      channelInput->setCompletionMode('q');
    }
    // No queries at all, so ignore this TAB
    else newLine=line;
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
    QString pattern=line.mid(pos,oldPos-pos).lower();
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
          (KonversationApplication::preferences.getNickCompletionMode() == 2));

        if(!complete && !found.isEmpty()) {
          if(KonversationApplication::preferences.getNickCompletionMode() == 1) {
            QString nicks = found.join(" ");
            appendServerMessage(i18n("Completion"),i18n("Possible completions: %1.").arg(nicks));
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

          if ( prefixCharacter != QString::null && lookNick.contains(prefixCharacter) )
             lookNick = lookNick.section( prefixCharacter,1 );

          if(lookNick.lower().startsWith(pattern)) foundNick=lookNick;
          // increment search position
          completionPosition++;
          // wrap around
          if(completionPosition==nicknameList.count()) completionPosition=0;
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
          QString addMiddle(KonversationApplication::preferences.getNickCompleteSuffixMiddle());
          newLine.insert(pos,foundNick+addMiddle);
          pos=pos+foundNick.length()+addMiddle.length();
        }
        // no, it was at the beginning
        else if(complete)
        {
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
  if(server->getOutputFilter()->replaceAliases(output)) {
    output = server->parseWildcards(output,server->getNickname(),getName(),getKey(),
      getSelectedNicksList(),QString::null);
  }

  // encoding stuff is done in Server()
  Konversation::OutputFilterResult result = server->getOutputFilter()->parse(server->getNickname(),output,getName());

  // Is there something we need to display for ourselves?
  if(!result.output.isEmpty())
  {
    if(result.type == Konversation::Action) appendAction(server->getNickname(), result.output);
    else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
    else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
    else if(result.type == Konversation::Query) appendQuery(result.typeString, result.output);
    else append(server->getNickname(), result.output);
  }
  // Send anything else to the server
  if(!result.toServer.isEmpty()) {
    server->queue(result.toServer);
  } else {
    server->queueList(result.toServerList);
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

QStringList Channel::getSelectedNicksList()
{
  selectedNicksList.clear();
  Nick* nick=nicknameList.first();

  while(nick)
  {
    if(nick->isSelected()) selectedNicksList.append(nick->getNickname());
    nick=nicknameList.next();
  }

  return selectedNicksList;
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
  server->queue(command.arg(getName()).arg((on) ? "+" : "-").arg(mode[id]).arg(args));
}

void Channel::quickButtonClicked(const QString &buttonText)
{
  // parse wildcards (toParse,nickname,channelName,nickList,queryName,parameter)
  QString out=server->parseWildcards(buttonText,server->getNickname(),getName(),getKey(),getSelectedNicksList(),QString::null);
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
#ifdef USE_NICKINFO
void Channel::addNickname(ChannelNickPtr channelnick)
#else
void Channel::addNickname(const QString& nickname,const QString& hostmask,
                          bool admin,bool owner,bool op,bool halfop,bool voice)
#endif
{

#ifdef USE_NICKINFO
  QString nickname = channelnick->getNickname();

#endif
  Nick* nick=0;
  Nick* lookNick=nicknameList.first();
  while(lookNick && (nick==0))
  {
    if(lookNick->getNickname().lower()==nickname.lower()) nick=lookNick;
    lookNick=nicknameList.next();
  }

  if(nick==0)
  {
#ifdef USE_NICKINFO
    fastAddNickname(channelnick);
    if(channelnick->isAdmin() || channelnick->isOwner() || channelnick->isOp() || channelnick->isHalfOp())
      adjustOps(1);
#else
    fastAddNickname(nickname,hostmask,admin,owner,op,halfop,voice);
    if(admin || owner || op || halfop) adjustOps(1);
#endif
    nicknameList.sort();

    adjustNicks(1);
  }
  else
  {
#ifdef USE_NICKINFO
    Q_ASSERT(false); //We shouldn't be adding someone that is already in the channel.
#else
    nick->setHostmask(hostmask);
    nick->setVoice(voice);

    if(nick->isOp() && (op==false))
    {
      adjustOps(-1);
    }

    nick->setAdmin(admin);
    nick->setOwner(owner);
    nick->setOp(op);
    nick->setHalfop(halfop);
    nick->setVoice(voice);

    // add all modes higher than voice to ops
    if((nick->isOp()==false) && (admin || owner || op || halfop))
    {
      adjustOps(1);
    }
#endif
  }
}

#ifdef USE_NICKINFO
// Use with caution! Does not sort or check for duplicates!
void Channel::fastAddNickname(ChannelNickPtr channelnick) {
  Q_ASSERT(channelnick);
  if(!channelnick) return;
  Nick* nick=new Nick(nicknameListView, channelnick);
  // nicks get sorted later
  nicknameList.append(nick);
}
#else
// Use with caution! Does not sort or check for duplicates!
void Channel::fastAddNickname(const QString& nickname,const QString& hostmask,
                              bool admin,bool owner,bool op,bool halfop,bool voice)
{
  Nick* nick=new Nick(nicknameListView,nickname,hostmask,admin,owner,op,halfop,voice);

  // nicks get sorted later
  nicknameList.append(nick);
}
#endif

#ifdef USE_NICKINFO
void Channel::nickRenamed(const QString &oldNick, const NickInfo& nickInfo) {
  Q_ASSERT(&nickInfo);
  Q_ASSERT(!oldNick.isEmpty());
  /* Did we change our nick name? */
  QString newNick = nickInfo.getNickname();
  if(oldNick==server->getNickname())
  {
    setNickname(newNick);
    appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.").arg(newNick),false);
  }
  /* No, must've been someone else */
   else appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.").arg(oldNick).arg(newNick),false);

}
#else
void Channel::renameNick(const QString& nickname,const QString& newNick)
{
  /* Did we change our nick name? */
  if(nickname==server->getNickname())
  {
    setNickname(newNick);
    appendCommandMessage(i18n("Nick"),i18n("You are now known as %1.").arg(newNick),false);
  }
  /* No, must've been someone else */
  else appendCommandMessage(i18n("Nick"),i18n("%1 is now known as %2.").arg(nickname).arg(newNick),false);

  /* Update the nick list */
  Nick* nick=getNickByName(nickname);
  if(nick==0) kdWarning() << "Channel::renameNick(): Nickname " << nickname << " not found!" << endl;
  else nick->setNickname(newNick);

  nicknameListView->sort();
  nicknameList.sort();


}
#endif
#ifdef USE_NICKINFO
void Channel::joinNickname(ChannelNickPtr channelNick) {
  if(channelNick->getNickname() == server->getNickname())
  {
    appendCommandMessage(i18n("Join"),i18n("You have joined channel %1. (%2)").arg(getName()).arg(channelNick->getHostmask()),false, false);
  } else {
    appendCommandMessage(i18n("Join"),i18n("%1 has joined this channel. (%2)").arg(channelNick->getNickname()).arg(channelNick->getHostmask()),false, false);
    addNickname(channelNick);
  }
}
void Channel::removeNick(ChannelNickPtr channelNick, const QString &reason, bool quit) {
  if(channelNick->getNickname() == server->getNickname())
  {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("You have left this server. (%1)").arg(reason),false);
    else appendCommandMessage(i18n("Part"),i18n("You have left channel %1. (%2)").arg(getName()).arg(reason),false);
#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  } else {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("%1 has left this server. (%2)").arg(channelNick->getNickname()).arg(reason),false);
    else appendCommandMessage(i18n("Part"),i18n("%1 has left this channel. (%2)").arg(channelNick->getNickname()).arg(reason),false);

    if(channelNick->isOp() || channelNick->isOwner() || channelNick->isAdmin() || channelNick->isHalfOp()) adjustOps(-1);
    adjustNicks(-1);
    Nick* nick=getNickByName(channelNick->getNickname());
    if(nick==0) kdWarning() << "Channel::kickNick(): Nickname " << channelNick->getNickname() << " not found!"<< endl;
    else {
      nicknameList.removeRef(nick);
    }

  }
}
void Channel::kickNick(ChannelNickPtr channelNick, const ChannelNick &kicker, const QString &reason) {
  if(channelNick->getNickname()==server->getNickname())
  {
    if(kicker.getNickname()==server->getNickname())
    {
      appendCommandMessage(i18n("Kick"),i18n("You have kicked yourself from the channel. (%1)").arg(reason));
      /* This message lets the user see what he has done after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have kicked yourself from channel %1. (%2)").arg(getName()).arg(reason));
    }
    else
    {
      appendCommandMessage(i18n("Kick"),i18n("You have been kicked from the channel by %1. (%2)").arg(kicker.getNickname()).arg(reason));
      /* This message lets the user see what had happened after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have been kicked from channel %1 by %2. (%3)").arg(getName()).arg(kicker.getNickname()).arg(reason));
    }
#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  }
  else
  {
    if(kicker.getNickname()==server->getNickname())
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
#else //was USE_NICKINFO

void Channel::joinNickname(const QString& nickname,const QString& hostmask)
{
  /* Did we join this channel ourselves? */
  if(nickname==server->getNickname())
  {
    appendCommandMessage(i18n("Join"),i18n("You have joined channel %1. (%2)").arg(getName()).arg(hostmask),false, false);
  }
  /* No, it was somebody else */
  else
  {
    appendCommandMessage(i18n("Join"),i18n("%1 has joined this channel. (%2)").arg(nickname).arg(hostmask),false, false);
    addNickname(nickname,hostmask,false,false,false,false,false);
  }
}

void Channel::removeNick(const QString &nickname, const QString &reason, bool quit)
{
  if(nickname==server->getNickname())
  {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("You have left this server. (%1)").arg(reason),false);
    else appendCommandMessage(i18n("Part"),i18n("You have left channel %1. (%2)").arg(getName()).arg(reason),false);

#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  }
  else
  {
    if(quit) appendCommandMessage(i18n("Quit"),i18n("%1 has left this server. (%2)").arg(nickname).arg(reason),false);
    else appendCommandMessage(i18n("Part"),i18n("%1 has left this channel. (%2)").arg(nickname).arg(reason),false);

    Nick* nick=getNickByName(nickname);
    if(nick==0) kdWarning() << "Channel::removeNick(): Nickname " << nickname << " not found!" << endl;
    else
    {
#ifdef USE_NICKINFO
      if(nick->isOp() || nick->isOwner() || nick->isAdmin() || nick->isHalfOp()) adjustOps(-1);
#else
      if(nick->isOp() || nick->isOwner() || nick->isAdmin() || nick->isHalfop()) adjustOps(-1);
#endif
      adjustNicks(-1);

      nicknameList.removeRef(nick);
    }

  }
}

void Channel::kickNick(const QString &nickname, const QString &kicker, const QString &reason)
{
  if(nickname==server->getNickname())
  {
    if(kicker==server->getNickname())
    {
      appendCommandMessage(i18n("Kick"),i18n("You have kicked yourself from the channel. (%1)").arg(reason));
      /* This message lets the user see what he has done after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have kicked yourself from channel %1. (%2)").arg(getName()).arg(reason));
    }
    else
    {
      appendCommandMessage(i18n("Kick"),i18n("You have been kicked from the channel by %1. (%2)").arg(kicker).arg(reason));
      /* This message lets the user see what had happened after the channel window went away */
      server->appendStatusMessage(i18n("Kick"),i18n("You have been kicked from channel %1 by %2. (%3)").arg(getName()).arg(kicker).arg(reason));
    }
#ifdef USE_MDI
    emit chatWindowCloseRequest(this);
#else
    delete this;
#endif
  }
  else
  {
    if(kicker==server->getNickname())
      appendCommandMessage(i18n("Kick"),i18n("You have kicked %1 from the channel. (%2)").arg(nickname).arg(reason));
    else
      appendCommandMessage(i18n("Kick"),i18n("%1 has been kicked from the channel by %2. (%3)").arg(nickname).arg(kicker).arg(reason));

    Nick* nick=getNickByName(nickname);
    if(nick==0) kdWarning() << "Channel::kickNick(): Nickname " << nickname << " not found!" << endl;
    else
    {
      if(nick->isOp()) adjustOps(-1);
      adjustNicks(-1);

      nicknameList.removeRef(nick);
    }
  }

}

#endif //USE_NICKINFO
Nick* Channel::getNickByName(const QString &lookname)
{
//FIXME I don't think this works
  QString lcLookname = lookname.lower();
  Nick* nick=nicknameList.first();
  while(nick)
  {
    if(nick->getNickname().lower()==lcLookname) return nick;
    nick=nicknameList.next();
  }
  return 0;
}
void Channel::adjustNicks(int value)
{
  nicks+=value;
  updateNicksOps();
}

void Channel::adjustOps(int value)
{
  ops+=value;
  updateNicksOps();
}

void Channel::updateNicksOps()
{
  /* %1 %2 / %3 %4 = 5 Nicks / 3 Ops */
  nicksOps->setText(i18n("%1 %2 / %3 %4").arg(nicks).arg((nicks==1) ? i18n("Nick") : i18n("Nicks")).arg(ops).arg((ops==1) ? i18n("Op") : i18n("Ops")));
}

void Channel::setTopic(const QString &newTopic)
{
  appendCommandMessage(i18n("Topic"),i18n("The channel topic is \"%1\".").arg(newTopic));
  if(topic!=newTopic)
  {
    topicHistory.prepend(i18n("<unknown> %1").arg(newTopic));
    topicLine->clear();
    topicLine->insertStringList(topicHistory);
    topic=newTopic;
    // Add a tool tip to the topic line if it gets too long
    QToolTip::remove(topicLine);

    if(newTopic.length()>80)
    {
      QString toolTip=newTopic;
      toolTip.replace(QRegExp("&",false,true),"&amp;").
                               replace(QRegExp("<",false,true),"&lt;").
                               replace(QRegExp(">",false,true),"&gt;");

      QToolTip::add(topicLine,"<qt>"+toolTip+"</qt>");
    }
  }
}

void Channel::setTopic(const QString &nickname, const QString &newTopic) // Overloaded
{
  if(nickname==server->getNickname())
    appendCommandMessage(i18n("Topic"),i18n("You set the channel topic to \"%1\".").arg(newTopic));
  else
    appendCommandMessage(i18n("Topic"),i18n("%1 sets the channel topic to \"%2\".").arg(nickname).arg(newTopic));

  topicHistory.prepend("<"+nickname+"> "+newTopic);
  topicLine->clear();
  topicLine->insertStringList(topicHistory);
  topic=newTopic;
  // Add a tool tip to the topic line if it gets too long
  QToolTip::remove(topicLine);
  if(newTopic.length()>80)
  {
    QString toolTip=newTopic;
    toolTip.replace(QRegExp("&",false,true),"&amp;").
                             replace(QRegExp("<",false,true),"&lt;").
                             replace(QRegExp(">",false,true),"&gt;");

    QToolTip::add(topicLine,"<qt>"+toolTip+"</qt>");
  }
}

void Channel::setTopicAuthor(const QString& newAuthor)
{
  if(topicAuthorUnknown)
  {
    topicHistory[0]="<"+newAuthor+"> "+topicHistory[0].section(' ',1);
    topicLine->clear();
    topicLine->insertStringList(topicHistory);
    topicAuthorUnknown=false;
  }
}
#ifdef USE_NICKINFO
void Channel::updateMode(ChannelNickPtr sourceChannelNick, char mode, bool plus, const QString &parameter)
#else
void Channel::updateMode(const QString &sourceNick, char mode, bool plus, const QString &parameter)
#endif
{
  QString message(QString::null);
#ifdef USE_NICKINFO
  ChannelNickPtr parameterChannelNick;
  QString sourceNick = sourceChannelNick->getNickname();
#else
  Nick* nick;
#endif

  bool fromMe=false;
  bool toMe=false;

  if(sourceNick.lower()==server->getNickname().lower()) fromMe=true;
  if(parameter.lower()==server->getNickname().lower()) toMe=true;

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
#ifdef USE_NICKINFO
      parameterChannelNick=server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isOwner() && !parameterChannelNick->isOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isOwner() && !parameterChannelNick->isOp()) adjustOps(-1);
        parameterChannelNick->setOwner(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#else
      nick=getNickByName(parameter);
      if(nick)
      {
        // Only update counter if something has actually changed
        if(plus && !nick->isOwner() && !nick->isOp()) adjustOps(1);
        else if(!plus && nick->isOwner() && !nick->isOp()) adjustOps(-1);
        nick->setOwner(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#endif
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
#ifdef USE_NICKINFO
      parameterChannelNick=server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isOp()) adjustOps(-1);
        parameterChannelNick->setOp(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#else
      nick=getNickByName(parameter);
      if(nick)
      {
        // Only update counter if something has actually changed
        if(plus && !nick->isOp()) adjustOps(1);
        else if(!plus && nick->isOp()) adjustOps(-1);
        nick->setOp(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#endif
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
#ifdef USE_NICKINFO
      parameterChannelNick=server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
        if(plus && !parameterChannelNick->isHalfOp()) adjustOps(1);
        else if(!plus && parameterChannelNick->isHalfOp()) adjustOps(-1);
        parameterChannelNick->setHalfOp(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#else
      nick=getNickByName(parameter);
      if(nick)
      {
        // Only update counter if something has actually changed
        if(plus && !nick->isHalfop()) adjustOps(1);
        else if(!plus && nick->isHalfop()) adjustOps(-1);
        nick->setHalfop(plus);
        updateNicksOps();
        nicknameListView->sort();
      }
#endif
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
#ifdef USE_NICKINFO
      parameterChannelNick=server->getChannelNick(getName(), parameter);
      if(parameterChannelNick) {
	parameterChannelNick->setVoice(plus);
	nicknameListView->sort();
      }
#else
      nick=getNickByName(parameter);
      if(nick)
      {
        nick->setVoice(plus);
        nicknameListView->sort();
      }
#endif
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
  if(!message.isEmpty()) appendCommandMessage(i18n("Mode"),message);
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
}

void Channel::updateQuickButtons(QStringList newButtonList)
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
    QString toolTip=buttonText.replace(QRegExp("&",false,true),"&amp;").
                               replace(QRegExp("<",false,true),"&lt;").
                               replace(QRegExp(">",false,true),"&gt;");

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
    if(show)
      modeBox->show();
    else
      modeBox->hide();
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
    splitter->setSizes(KonversationApplication::preferences.getChannelSplitter());
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

  topicLine->lineEdit()->setPaletteForegroundColor(fg);
  topicLine->lineEdit()->setPaletteBackgroundColor(bg);
  topicLine->setFont(KonversationApplication::preferences.getTextFont());

  getTextView()->setFont(KonversationApplication::preferences.getTextFont());

  if(KonversationApplication::preferences.getShowBackgroundImage())
      getTextView()->setViewBackground(KonversationApplication::preferences.getColor("TextViewBackground"),
                                   KonversationApplication::preferences.getBackgroundImageName());

  nicksOps->setFont(KonversationApplication::preferences.getListFont());

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

void Channel::nicknameComboboxChanged(int /*index*/)
{
  QString newNick=nicknameCombobox->currentText();
  oldNick=server->getNickname();
  nicknameCombobox->setCurrentText(oldNick);
  server->queue("NICK "+newNick);
}

void Channel::changeNickname(const QString& newNickname)
{
  server->queue("NICK "+newNickname);
}

#ifdef USE_NICKINFO
void Channel::addPendingNickList(ChannelNickList pendingChannelNickList)
#else
void Channel::addPendingNickList(const QStringList& newNickList)
#endif
{
  if(!getPendingNicks())
  {
    purgeNicks();
    setPendingNicks(true);
  }

  int opsToAdd=0;

  nicknameListView->setUpdatesEnabled(false);
#ifdef USE_NICKINFO
  ChannelNickList::iterator it;
  int count = 0;
  for( it = pendingChannelNickList.begin(); it != pendingChannelNickList.end(); it++,count++)
  {
    if(count % 50 == 0) nicknameListView->setUpdatesEnabled(true);
    fastAddNickname(*it);
    if(count % 50 == 0) {
      qApp->processEvents();
      nicknameListView->setUpdatesEnabled(false);
    }
    if((*it)->isAdmin() || (*it)->isOwner() || (*it)->isOp() || (*it)->isHalfOp())
      opsToAdd++;
  }

  adjustNicks(pendingChannelNickList.count());
#else

  unsigned int mode;
  for(unsigned int i=0;i<newNickList.count();i++)
  {
    QString nick=newNickList[i].section(" ",0,0);
    mode=newNickList[i].section(" ",1,1).toInt();

    // refresh visual if index is 50, 100, 150 ... or if it's the last nick in the list
    if((i % 50==0) || i==newNickList.count()-1) nicknameListView->setUpdatesEnabled(true);

    // TODO: make these an enumeration in KApplication or somewhere, we can use them from inputfilter.cpp as well
    fastAddNickname(nick,QString::null,(mode & 16),(mode & 8),(mode & 4),(mode & 2),(mode & 1));

    // stop refresh of visual if index is 50, 100, 150 ...
    if(i % 50==0)
    {
      // care about pending events befor continuing
      qApp->processEvents();
      nicknameListView->setUpdatesEnabled(false);
    }

    if((mode & 16) || (mode & 8) || (mode & 4) || (mode & 2)) opsToAdd++;
  } // endfor

  adjustNicks(newNickList.count());
#endif
  // should have been done already, but you never know ...
  nicknameListView->setUpdatesEnabled(true);

  nicknameListView->sort();
  nicknameList.sort();
  adjustOps(opsToAdd);
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

void Channel::adjustFocus()
{
  channelInput->setFocus();
}

void Channel::autoUserhost()
{
  if(KonversationApplication::preferences.getAutoUserhost())
  {
    int limit=5;

    QString nickString;
    QPtrList<Nick> nicks=getNickList();

    for(unsigned int index=0;index<nicks.count();index++)
    {
      if(nicks.at(index)->getHostmask().isEmpty())
      {
        if(limit--) nickString=nickString+nicks.at(index)->getNickname()+" ";
        else break;
      }
    }
    if(!nickString.isEmpty()) server->requestUserhost(nickString);
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

QString Channel::getTextInLine() { return channelInput->text(); }

bool Channel::frontView()        { return true; }
bool Channel::searchView()       { return true; }

void Channel::appendInputText(const QString& s)
{
  channelInput->setText(channelInput->text() + s);
}

void Channel::closeYourself()
{
#ifndef USE_MDI
  server->closeChannel(getName());
  server->removeChannel(this);
  delete this;
#endif
}

void Channel::closeYourself(ChatWindow* /* view */)
{
#ifdef USE_MDI
  server->closeChannel(getName());
#endif
}

void Channel::serverQuit(const QString& reason)
{
#ifdef USE_MDI
#ifdef USE_NICKINFO
  ChannelNickPtr channelNick=server->getChannelNick(getName(),server->getNickname());
  if(channelNick)  removeNick(channelNick,reason,true);
#else
  removeNick(server->getNickname(),reason,true);
#endif
#endif
}

void Channel::showTopic(bool show)
{
  if(show) {
    topicLine->show();
    topicLabel->show();
  } else {
    topicLine->hide();
    topicLabel->hide();
  }
}

//
// NickList
//

int NickList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  return QString::localeAwareCompare(static_cast<Nick*>(item1)->getNickname(),
    static_cast<Nick*>(item2)->getNickname());
}

QString NickList::completeNick(const QString& pattern, bool& complete, QStringList& found, bool skipNonAlfaNum)
{
  found.clear();
  QString prefix = "^";
  QString newNick;
  QString prefixCharacter = KonversationApplication::preferences.getPrefixCharacter();


  if((pattern.find(QRegExp("^(\\d|\\w)")) != -1) && skipNonAlfaNum) {
    prefix = "^([^\\d\\w]|[\\_]){0,}";
  }

  QRegExp regexp(prefix + QRegExp::escape(pattern.lower()));

  for(Nick* n = first(); n; n = next()) {
    newNick = n->getNickname();

    if ( prefix != QString::null && newNick.contains(prefixCharacter) )
       newNick = newNick.section( prefixCharacter,1 );

    if(newNick.lower().find(regexp) != -1) {
      found.append(newNick);
    }
  }

  if(found.count() > 1) {
    bool ok = true;
    int i = 0;

    while(ok && ((pattern.length() + i) < found[0].length())) {
      i++;
      QStringList tmp = found.grep(found[0].left(pattern.length() + i), false);
      if(tmp.count() != found.count()) {
        ok = false;
        i -= 1;
      }
    }

    complete = false;
    return found[0].left(pattern.length() + i);
  } else if(found.count() == 1) {
    complete = true;
    return found[0];
  }

  return QString::null;
}

#include "channel.moc"
