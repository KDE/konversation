/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Sun Feb 24 2002
  copyright: (C) 2002 by Dario Abatianni
             in parts (C) by Trolltech
  email:     eisfuchs@tigress.com
*/

#include <qpainter.h>
#include <qstyle.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qregexp.h>

#include <kdebug.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include "channel.h"
#include "irccharsets.h"
#include "ledtabbar.h"
#include "ledtab.h"
#include "konversationapplication.h"
#include "chatwindow.h"
#include "server.h"
#include "linkaddressbookui.h"
#include "addressbook.h"
#include "query.h"

#define LABEL_OFFSET 16

#define POPUPID_ENCODING_OFFSET 100

// from kbarcode
static const char* remove_xpm[]=
{
  "16 16 15 1",
  " 	c None",
  ".	c #B7B7B7",
  "+	c #FFFFFF",
  "@	c #6E6E6E",
  "#	c #E9E9E9",
  "$	c #E4E4E4",
  "%	c #000000",
  "&	c #DEDEDE",
  "*	c #D9D9D9",
  "=	c #D4D4D4",
  "-	c #CECECE",
  ";	c #C9C9C9",
  ">	c #C3C3C3",
  ",	c #BEBEBE",
  "'	c #B9B9B9",

  "...............+",
  ".@@@@@@@@@@@@@@+",
  ".@+++++++++++.@+",
  ".@+          .@+",
  ".@+  %    %  .@+",
  ".@+ %%%  %%% .@+",
  ".@+  %%%%%%  .@+",
  ".@+   %%%%   .@+",
  ".@+   %%%%   .@+",
  ".@+  %%%%%%  .@+",
  ".@+ %%%  %%% .@+",
  ".@+  %    %  .@+",
  ".@+           @+",
  ".@............@+",
  ".@@@@@@@@@@@@@@+",
  "++++++++++++++++"
};

LedTabBar::LedTabBar(QWidget* parent,const char* name) :
           KTabBar(parent,name)
{
  m_popup=new KPopupMenu(this,"ledtabbar_context_menu");
  m_popupAddressbook = new KPopupMenu(this, "ledtabbar_addressbook_context_menu");

  if(m_popup)
  {
    // Encoding {
    m_popupEncoding=new KPopupMenu(this,"ledtabbar_context_menu_encoding");
    m_popupEncoding->setCheckable(true);
    QStringList encodingDescs=Konversation::IRCCharsets::self()->availableEncodingDescriptiveNames();
    unsigned int j=0;
    for(QStringList::ConstIterator it=encodingDescs.begin(); it!=encodingDescs.end(); ++it)
    {
      m_popupEncoding->insertItem(*it,POPUPID_ENCODING_OFFSET+j+1);
      ++j;
    }
    m_popupEncoding->insertSeparator();
    m_popupEncoding->insertItem(i18n("Default"),POPUPID_ENCODING_OFFSET+0);
    // }

    m_popup->insertItem(SmallIconSet("1leftarrow"), i18n("Move Left"),MoveLeft);
    m_popup->insertItem(SmallIconSet("1rightarrow"), i18n("Move Right"),MoveRight);
    m_popup->insertSeparator();
    m_popup->insertItem(SmallIconSet("charset"),i18n("Set Encoding"),m_popupEncoding,EncodingSub);
    m_popup->insertItem(SmallIconSet("mail_generic"),i18n("Send Email..."),SendEmail);
    m_popup->insertItem(i18n("Addressbook Associations"), m_popupAddressbook, AddressbookSub);

    m_popup->insertSeparator();
    m_popup->insertItem(i18n("Enable Notifications"), EnableNotifications);
    m_popup->insertSeparator();
    m_popup->insertItem(SmallIcon("tab_remove"),i18n("Close Tab"),CloseTab);
  }

  else kdWarning() << "LedTabBar::LedTabBar(): Could not create popup!" << endl;
  
   setTabReorderingEnabled(true);
#if KDE_IS_VERSION(3,3,0)
  setTabCloseActivatePrevious(true);
#endif
  m_closePixmap = new QPixmap(remove_xpm);
}

LedTabBar::~LedTabBar()
{
    delete m_closePixmap;
}
void LedTabBar::insertAssociationSubMenu(const NickInfoPtr &nickinfo) {

  KABC::Addressee addr = nickinfo->getAddressee();
  m_popupAddressbook->clear();
  if(!addr.isEmpty()) {
    m_popupAddressbook->insertItem(SmallIcon("contents"), i18n("Edit &Contact..."), AddressbookEdit);
    m_popupAddressbook->insertSeparator();
    m_popupAddressbook->insertItem(i18n("&Change Association..."), AddressbookChange);
    m_popupAddressbook->insertItem(SmallIcon("editdelete"), i18n("&Delete Association"), AddressbookDelete);
    if(addr.preferredEmail().isEmpty()) {
      m_popup->setItemEnabled(SendEmail, false);
      m_popup->setWhatsThis(SendEmail, "Sends an email to this contact using the preferred email address set in the contact's addressbook association.  This is currently disabled because the associated contact does not have any preferred email address set.");
    } else {
      m_popup->setItemEnabled(SendEmail, true);
      m_popup->setWhatsThis(SendEmail, i18n("Sends an email to this contact using the preferred email address (%1) set in the contact's addressbook association").arg(addr.preferredEmail()));
    }
  } else {
    m_popupAddressbook->insertItem(i18n("Choose &Contact..."), AddressbookChange);
    m_popupAddressbook->insertItem(i18n("Create &New Contact..."), AddressbookNew);
    m_popup->setItemEnabled(SendEmail, false);
    m_popup->setWhatsThis(SendEmail, i18n("Sends an email to this contact using the preferred email address set in the contact's addressbook association.  This is currently disabled because this nick does not have any association with any contacts in your addressbook."));
  }
}
LedTab* LedTabBar::tab(QWidget* widget)
{
  QPtrList<QTab>* list=tabList();

  // These casts can't be helped, templates don't like casting
  LedTab* tab=static_cast<LedTab*>(list->first());
  while(tab)
  {
    if(tab->getWidget()==widget) return tab;
    tab=static_cast<LedTab*>(list->next());
  }

  return 0;
}

// reimplemented to avoid casts in active code
LedTab* LedTabBar::tab(int id)
{
  return static_cast<LedTab*>(QTabBar::tab(id));
}

// repaint only the needed tab region to avoid flickering
void LedTabBar::repaintLED(LedTab* tab)
{
  repaint(tab->rect(),false);
}

// original code by Trolltech, adapted for close pixmap
void LedTabBar::paint( QPainter * p, QTab * t, bool selected ) const
{
  QRect r( t->rect() );
  int iw = 0;
  int ih = 0;
  if ( t->iconSet() != 0 ) {
      iw = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).width() + 4;
      ih = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).height();
  }

  // do we want close widgets on the tabs?
  if(KonversationApplication::preferences.getCloseButtonsOnTabs())
  {
    QStyle::SFlags flags = QStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= QStyle::Style_Enabled;
    if ( selected )
        flags |= QStyle::Style_Selected;
    else if(t == tabAt(indexOf(currentTab())))
        flags |= QStyle::Style_Sunken;

    //selection flags
    if(t->rect().contains(mapFromGlobal(QCursor::pos())))
        flags |= QStyle::Style_MouseOver;
    style().drawControl( QStyle::CE_TabBarTab, p, this, t->rect(),
                         colorGroup(), flags, QStyleOption(t) );

    p->setFont( font() );

    // add size of close pixmap
    iw+=LABEL_OFFSET;

    QFontMetrics fm = p->fontMetrics();
    int fw = fm.width( t->text() );
    fw -= t->text().contains('&') * fm.width('&');
    fw += t->text().contains("&&") * fm.width('&');
    int w = iw + fw + 4;
    int h = QMAX(fm.height() + 4, ih );

    paintLabel( p, QRect( r.left() + (r.width()-w)/2 - 3,
                          r.top() + (r.height()-h)/2,
                          w, h ), t, t->identifier() == keyboardFocusTab() );
  }
  // otherwise call original code
  else QTabBar::paint( p, t, selected );

  // cross out offline tabs
  if(!static_cast<LedTab*>(t)->getOnline())
  {
    // set default values
    int x=r.left()+6+iw;
    int y=r.top()+r.height()/2;
    int x2=r.right()-4-iw;

    if(KonversationApplication::preferences.getCloseButtonsOnTabs())
    {
      if(KonversationApplication::preferences.getCloseButtonsAlignRight())
      {
        x-=(LABEL_OFFSET/2+2);
        x2+=(LABEL_OFFSET/2-2);
      }
      else
      {
        x+=LABEL_OFFSET/2;
        x2=r.right()-LABEL_OFFSET/2;
      }
    }

    p->setPen(foregroundColor());
    p->drawLine(x,y,x2,y);
  }
}

// original code by Trolltech, adapted for close pixmap
void LedTabBar::paintLabel( QPainter* p, const QRect& br, QTab* tab, bool has_focus ) const
{
  LedTab* t=static_cast<LedTab*>(tab);

  QRect r = br;
  bool selected = currentTab() == t->identifier();
  if ( t->iconSet()) {
      // the tab has an iconset, draw it in the right mode
      QIconSet::Mode mode = (t->isEnabled() && isEnabled())
          ? QIconSet::Normal : QIconSet::Disabled;
      if ( mode == QIconSet::Normal && has_focus )
          mode = QIconSet::Active;
      QPixmap pixmap = t->iconSet()->pixmap( QIconSet::Small, mode );

      int pixw = pixmap.width();
      int pixh = pixmap.height();
      int close_pixh = m_closePixmap->height();

      r.setLeft( r.left() + pixw);
      r.setRight( r.right() + 2);

      // do we want close widgets on the tabs?
      if(KonversationApplication::preferences.getCloseButtonsOnTabs())
      {
        if(!KonversationApplication::preferences.getCloseButtonsAlignRight())
        {
          // Shift the text to the right
          r.setLeft( r.left() + LABEL_OFFSET);
          r.setRight( r.right() + LABEL_OFFSET);

          // Draw iconset on the left side
          p->drawPixmap( br.left() + 6 + LABEL_OFFSET /* + ((selected == TRUE) ? 0 : 1) */,
                         br.center().y()-pixh/2 + ((selected == TRUE) ? 0 : 1),
                         pixmap );

          // Draw close button on the left side
          p->drawPixmap( br.left(),
                         br.center().y() - close_pixh/2,
                         *m_closePixmap );
        }
        else
        {
          // Shift the text to the left
          r.setLeft( r.left() - 6);

          // Draw iconset on the left side
          p->drawPixmap( br.left() + 2,
                         br.center().y()-pixh/2 + ((selected == TRUE) ? 0 : 1),
                         pixmap );

          // Draw close button on the right side
          p->drawPixmap( br.right() - 7,
                         br.center().y() - close_pixh/2,
                         *m_closePixmap );
        }
      }
      else
      {
        // ### the pixmap shift should probably not be hardcoded..
        p->drawPixmap( br.left() + 2 /* + ((selected == TRUE) ? 0 : 1) */,
                       br.center().y()-pixh/2 + ((selected == TRUE) ? 0 : 1),
                       pixmap );
      }
    }

    QStyle::SFlags flags = QStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= QStyle::Style_Enabled;
    if (has_focus)
        flags |= QStyle::Style_HasFocus;

    // set new label color if there is one
    QColorGroup  myColorGroup(colorGroup());
    if(!t->getLabelColor().isEmpty())
    {
      myColorGroup.setColor(QColorGroup::Foreground,t->getLabelColor());
      // most styles use Foreground, dotNET up to 1.4 uses Mid  ...
      myColorGroup.setColor(QColorGroup::Mid,t->getLabelColor());
    }

    style().drawControl( QStyle::CE_TabBarLabel, p, this, r,
                         t->isEnabled() ? myColorGroup : palette().disabled(),
                         flags, QStyleOption(t) );
}

// reimplemented for close pixmap
void LedTabBar::layoutTabs()
{
  if(count()==0) return;

  // at first let QT layout our tabs
  QTabBar::layoutTabs();

  // do we want close widgets on the tabs?
  if(KonversationApplication::preferences.getCloseButtonsOnTabs())
  {
    // make necessary modifications
    int offset=0;
    for(int index=0;index<count();index++)
    {
      QTab* ltab=tabAt(index);
      QRect r=ltab->rect();
      r.setWidth(r.width()+LABEL_OFFSET);

#if QT_VERSION >= 0x030300
      r.moveLeft(offset);
      offset+=r.width();
#else
      r.moveBy(offset,0);
      offset+=LABEL_OFFSET;
#endif

      ltab->setRect(r);
    } // endfor
  }
}

void LedTabBar::mouseReleaseEvent(QMouseEvent* e)
{
  // do we have close widgets on the tabs?
  if(KonversationApplication::preferences.getCloseButtonsOnTabs())
  {
    if(e->button()==LeftButton)
    {
      LedTab* t=tab(currentTab());

      // get physical position of QTab* t
      QRect target(t->rect());

      // move target area to aprop. place
      if(KonversationApplication::preferences.getCloseButtonsAlignRight())
        target.moveBy(target.width() - 20,4);
      else
        target.moveBy(8,4);

      // set size of target area
      target.setWidth(16);
      target.setHeight(16);

      if(target.contains(e->pos())) emit closeTab(t->identifier());
    }
  }
  if (e->button()==MidButton) KTabBar::mouseReleaseEvent(e);
}

void LedTabBar::updateTabs()
{
  layoutTabs();
  update();
}

void LedTabBar::contextMenuEvent(QContextMenuEvent* ce)
{
  for(int index=0;index<count();index++)
  {
    QTab* lookTab=tabAt(index);
    if(lookTab->rect().contains(ce->pos()))
    {
      m_popup->setItemEnabled(EncodingSub, false);
      ChatWindow* win = dynamic_cast<ChatWindow*>(static_cast<LedTab*>(lookTab)->getWidget());

      if(win) {
        m_popup->setItemEnabled(MoveRight, index!=(count()-1));
        m_popup->setItemEnabled(MoveLeft, index!=0);
        ChatWindow::WindowType viewType = win->getType();
        if(viewType == ChatWindow::Channel || viewType == ChatWindow::Query || viewType == ChatWindow::Status) {
          m_popup->setItemEnabled(EnableNotifications, true);
          m_popup->setItemChecked(EnableNotifications, win->notificationsEnabled());
        } else {
          m_popup->setItemEnabled(EnableNotifications, false);
        }
	if(viewType == ChatWindow::Query) {
          m_popup->setItemVisible(AddressbookSub, true);
	  m_popup->setItemVisible(SendEmail, true);

	  Query *query = static_cast<Query*>(win);
	  NickInfoPtr nickinfo = query->getNickInfo();
	  if(nickinfo) {
            insertAssociationSubMenu(nickinfo);
	    m_popup->setItemEnabled(AddressbookSub, true);
	    if(!nickinfo->getAddressee().preferredEmail().isEmpty())
	      m_popup->setItemEnabled(SendEmail, true);
	    else
	      m_popup->setItemEnabled(SendEmail, false);
	  } else {
            m_popup->setItemEnabled(AddressbookSub, false); //This _does_ happen when the user goes offline!
	    m_popup->setItemEnabled(SendEmail, false);
	    m_popup->changeTitle(Label, i18n( "%1 - Offline" ).arg( lookTab->text() ) );
	  }
	} else {
	  m_popup->setItemVisible(AddressbookSub, false);
	  m_popup->setItemVisible(SendEmail, false);
	}
        if(win->isChannelEncodingSupported())
        {
          m_popup->setItemEnabled(EncodingSub, true);
          m_popupEncoding->changeItem(POPUPID_ENCODING_OFFSET+0,win->getChannelEncodingDefaultDesc());
          QString encoding=win->getChannelEncoding();
          int encodingIndex=Konversation::IRCCharsets::self()->shortNameToIndex(encoding);
          m_popupEncoding->setItemChecked(POPUPID_ENCODING_OFFSET+0, (encoding.isEmpty())); // identity default
          for(int i=0; i<Konversation::IRCCharsets::self()->availableEncodingsCount(); ++i)
            m_popupEncoding->setItemChecked(POPUPID_ENCODING_OFFSET+i+1, (encodingIndex == i));
        }
      } else {
        m_popup->setItemVisible(EnableNotifications, false);
      }

      int r=m_popup->exec(ce->globalPos());
      if(r==CloseTab)
      {
        emit closeTab(lookTab->identifier());
      }
      else if(r==MoveLeft)
      {
        emit moveTabLeft(lookTab->identifier());
      }
      else if(r==MoveRight)
      {
        emit moveTabRight(lookTab->identifier());
      }
      else if(r == EnableNotifications)
      {
        if(win) {
          win->setNotificationsEnabled(!win->notificationsEnabled());
        }
      }
      else if(r == SendEmail) {
	win->getServer()->obtainNickInfo(win->getName())->sendEmail();
      }
      else if(r == AddressbookEdit) {
        win->getServer()->obtainNickInfo( win->getName() )->editAddressee();
      }
      else if(r == AddressbookChange) {
	win->getServer()->obtainNickInfo( win->getName() )->showLinkAddressbookUI();
      }
      else if(r == AddressbookDelete || r == AddressbookNew) {
	Server *server = win->getServer();
        KABC::Addressee addressee = server->getNickInfo(win->getName())->getAddressee();
        Konversation::Addressbook *addressbook = Konversation::Addressbook::self();
	QString nickname = win->getName();
        if(addressbook->getAndCheckTicket())
        {
          if(r == AddressbookDelete) {
            if (addressee.isEmpty()) return;
            addressbook->unassociateNick(addressee, nickname, server->getServerName(), server->getServerGroup());
          } else {
            addressee.setGivenName(nickname);
            addressee.setNickName(nickname);
            addressbook->associateNickAndUnassociateFromEveryoneElse(addressee, nickname, server->getServerName(), server->getServerGroup());
          }
          if(addressbook->saveTicket())
          {
            //saveTicket will refresh the addressees for us.
            if(r == AddressbookNew)
              Konversation::Addressbook::self()->editAddressee(addressee.uid());
          }
        }
      }
      else if(POPUPID_ENCODING_OFFSET <= r && r <= POPUPID_ENCODING_OFFSET+Konversation::IRCCharsets::self()->availableEncodingsCount()+1)
      {
        if(win)
        {
          if(POPUPID_ENCODING_OFFSET == r)
            win->setChannelEncoding(QString::null);
          else
            win->setChannelEncoding(Konversation::IRCCharsets::self()->availableEncodingShortNames()[r-POPUPID_ENCODING_OFFSET-1]);
        }
      }
    }
  } // endfor
}

// reimplemented to avoid casts in active code
LedTab* LedTabBar::tabAt (int index) const
{
  return static_cast<LedTab*>(QTabBar::tabAt(index));
}

#ifndef QT_NO_WHEELEVENT
void LedTabBar::wheelEvent( QWheelEvent *e )
{
	emit(wheel(e));
}
#endif

#include "ledtabbar.moc"
