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
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qpainter.h>
#include <qstyle.h>
#include <qcursor.h>
#include <qapp.h>

#include <kdebug.h>

#include "ledtabbar.h"

LedTabBar::LedTabBar(QWidget* parent,const char* name) :
           QTabBar(parent,name)
{
}

LedTabBar::~LedTabBar()
{
}

LedTab* LedTabBar::tab(QWidget* widget)
{
  QPtrList<QTab>* list=tabList();

  // These casts can't be helped, templates don't like casting
  LedTab* tab=(LedTab*) list->first();
  while(tab)
  {
    if(tab->getWidget()==widget) return tab;
    tab=(LedTab*) list->next();
  }

  return 0;
}

// reimplemented to avoid casts in active code
LedTab* LedTabBar::tab(int id)
{
  return (LedTab*) QTabBar::tab(id);
}

// repaint only the needed tab region to avoid flickering
void LedTabBar::repaintLED(LedTab* tab)
{
  repaint(tab->rect(),false);
}












void LedTabBar::paint( QPainter * p, QTab * t, bool selected ) const
{
    QStyle::SFlags flags = QStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= QStyle::Style_Enabled;
    if ( selected )
        flags |= QStyle::Style_Selected;
/*    else if(t == d->pressed)
        flags |= QStyle::Style_Sunken;
*/
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

/*!
    Paints the label of tab \a t centered in rectangle \a br using
    painter \a p. A focus indication is drawn if \a has_focus is TRUE.
*/

void LedTabBar::paintLabel( QPainter* p, const QRect& br,
                          QTab* t, bool has_focus ) const
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
        int pixw = pixmap.width();
        int pixh = pixmap.height();
        r.setLeft( r.left() + pixw + 4 );
        r.setRight( r.right() + 2 );
        // ### the pixmap shift should probably not be hardcoded..
        p->drawPixmap( br.left() + 2 + 8 + ((selected == TRUE) ? 0 : 2),
                       br.center().y()-pixh/2 + ((selected == TRUE) ? 0 : 2),
                       pixmap );
    }

    QStyle::SFlags flags = QStyle::Style_Default;

    if (isEnabled() && t->isEnabled())
        flags |= QStyle::Style_Enabled;
    if (has_focus)
        flags |= QStyle::Style_HasFocus;

    r.moveBy(8,0);
    style().drawControl( QStyle::CE_TabBarLabel, p, this, r,
                         t->isEnabled() ? colorGroup(): palette().disabled(),
                         flags, QStyleOption(t) );
}


void LedTabBar::layoutTabs()
{
  if(count()==0) return;

  QTabBar::layoutTabs();
  int offset=0;

  for(int index=0;index<count();index++)
  {
    LedTab* ltab=tab(index);
    QRect r=ltab->rect();
    r.setWidth(r.width()+8);
    r.setLeft(r.left()+offset);
//    offset+=50;
    ltab->setRect(r);
  }
/*

    if ( count()==0 )
        return;

    int hframe, vframe, overlap;
    hframe  = style().pixelMetric( QStyle::PM_TabBarTabHSpace, this );
    vframe  = style().pixelMetric( QStyle::PM_TabBarTabVSpace, this );
    overlap = style().pixelMetric( QStyle::PM_TabBarTabOverlap, this );

    QFontMetrics fm = fontMetrics();
    int x = 0;
    QRect r;
    LedTab *t;
    int index;
    int increment;
    int max;
    bool reverse = QApplication::reverseLayout();

    if ( reverse )
    {
        index=count()-1;
        increment=-1;
        max=-1;
    }
    else
    {
        index=0;
        increment=1;
        max=count();
    }

    t=tab(index);


    for(;index!=max;index+=increment)
    {
        int lw = fm.width( t->text() );
        lw -= t->text().contains('&') * fm.width('&');
        lw += t->text().contains("&&") * fm.width('&');
        int iw = 0;
        int ih = 0;
        if ( t->iconSet() != 0 ) {
            iw = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).width() + 4;
            ih = t->iconSet()->pixmap( QIconSet::Small, QIconSet::Normal ).height();
        }
        int h = QMAX( fm.height(), ih );
        h = QMAX( h, QApplication::globalStrut().height() );

        h += vframe;
        t->setRect( QRect(QPoint(x, 0), style().sizeFromContents(QStyle::CT_TabBarTab, this,
                     QSize( QMAX( lw + hframe + iw, QApplication::globalStrut().width() ), h ),
                     QStyleOption(t) )));
        x += t->rect().width() - overlap;
        r = r.unite( t->rect() );

        if ( reverse )
            t = lstatic->prev();
        else
            t = lstatic->next();

    }

    for ( t = lstatic->first(); t; t = lstatic->next() )
        t->r.setHeight( r.height() ); */
}




#include "ledtabbar.moc"
