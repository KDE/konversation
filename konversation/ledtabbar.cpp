/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabbar.cpp  -  description
  begin:     Sun Feb 24 2002
  copyright: (C) 2002 by Dario Abatianni
             in parts (C) by Trolltech
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qpainter.h>
#include <qstyle.h>
#include <qcursor.h>
#include <qapp.h>

#include <kdebug.h>

#include "ledtabbar.h"
#include "konversationapplication.h"

// const char* LedTabBar::remove_xpm[]; // static

#define LABEL_OFFSET 16

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
/*
  "...............+",
  ".@@@@@@@@@@@@@@+",
  ".@+++++++++++.@+",
  ".@+##########.@+",
  ".@+$$%$$$$%$$.@+",
  ".@+&%%%&&%%%&.@+",
  ".@+**%%%%%%**.@+",
  ".@+===%%%%===.@+",
  ".@+---%%%%---.@+",
  ".@+;;%%%%%%;;.@+",
  ".@+>%%%>>%%%>.@+",
  ".@+,,%,,,,%,,.@+",
  ".@+''''''''''.@+",
  ".@............@+",
  ".@@@@@@@@@@@@@@+",
  "++++++++++++++++"
*/
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
           QTabBar(parent,name)
{
  popup=new KPopupMenu(this,"ledtabbar_context_menu");

  if(popup)
  {
    popup->insertTitle(i18n("Tab"),Label);
//    popup->insertSeparator();
    popup->insertItem(i18n("Close Tab"),CloseTab);
  }
  else kdWarning() << "LedTabBar::LedTabBar(): Could not create popup!" << endl;
}

LedTabBar::~LedTabBar()
{
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

    QRect r( t->rect() );
    p->setFont( font() );

    int iw = 0;
    int ih = 0;
    if ( t->iconSet() != 0 ) {
        iw = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).width() + 4;
        ih = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).height();
    }

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
}

// original code by Trolltech, adapted for close pixmap
void LedTabBar::paintLabel( QPainter* p, const QRect& br, QTab* tab, bool has_focus ) const
{
  LedTab* t=static_cast<LedTab*>(tab);

  // do we want close widgets on the tabs?
  if(KonversationApplication::preferences.getCloseButtonsOnTabs())
  {
    QRect r = br;
    bool selected = currentTab() == t->identifier();
    if ( t->iconSet()) {
        // the tab has an iconset, draw it in the right mode
        QIconSet::Mode mode = (t->isEnabled() && isEnabled())
            ? QIconSet::Normal : QIconSet::Disabled;
        if ( mode == QIconSet::Normal && has_focus )
            mode = QIconSet::Active;
        QPixmap pixmap = t->iconSet()->pixmap( QIconSet::Small, mode );
        QPixmap close_pixmap(remove_xpm);
        int pixw = pixmap.width();
        int pixh = pixmap.height();
        int close_pixh = close_pixmap.height();
        r.setLeft( r.left() + pixw + LABEL_OFFSET);
        r.setRight( r.right() + 2 + LABEL_OFFSET);
        // ### the pixmap shift should probably not be hardcoded..
        p->drawPixmap( br.left() + 6 + LABEL_OFFSET /* + ((selected == TRUE) ? 0 : 1) */,
                       br.center().y()-pixh/2 + ((selected == TRUE) ? 0 : 1),
                       pixmap );

        p->drawPixmap( br.left(),
                       br.center().y()-close_pixh/2,
                       close_pixmap );
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
  // otherwise call original code
  else QTabBar::paintLabel( p, br, t, has_focus );
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
    // make neccessary modifications
    int offset=0;
    for(int index=0;index<count();index++)
    {
      QTab* ltab=tabAt(index);
      QRect r=ltab->rect();
      r.setWidth(r.width()+LABEL_OFFSET);
      r.moveBy(offset,0);
      offset+=LABEL_OFFSET;

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
      // set size of target area
      target.setWidth(16);
      target.setHeight(16);
      // move target area to final place
      target.moveBy(8,4);

      if(target.contains(e->pos())) emit closeTab(t->identifier());
    }
  }
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
      popup->changeTitle(Label,lookTab->text());
      int r=popup->exec(ce->globalPos());
      if(r==CloseTab)
      {
        emit closeTab(lookTab->identifier());
      }
    }
  } // endfor
}

#include "ledtabbar.moc"
