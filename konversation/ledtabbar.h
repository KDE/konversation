/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtabbar.h  -  description
  begin:     Sun Feb 24 2002
  copyright: (C) 2002 by Dario Abatianni
             in parts (C) by Trolltech
  email:     eisfuchs@tigress.com

  $Id$
*/

#ifndef LEDTABBAR_H
#define LEDTABBAR_H

#include <qtabbar.h>

#include "ledtab.h"

/*
  @author Dario Abatianni
*/

class LedTabBar : public QTabBar
{
  Q_OBJECT

  public:
    LedTabBar(QWidget* parent,const char* name);
    ~LedTabBar();

    LedTab* tab(int id);
    LedTab* tab(QWidget* widget);

    virtual void layoutTabs();
    void updateTabs();

  signals:
    void closeTab(int id);

  public slots:
    void repaintLED(LedTab* tab);

  protected:
    // these two come from the original QT source
    virtual void paint( QPainter *, QTab *, bool ) const; // ### not const
    virtual void paintLabel( QPainter*, const QRect&, QTab*, bool ) const;

  protected:
    void mouseReleaseEvent(QMouseEvent* e);
};

#endif
