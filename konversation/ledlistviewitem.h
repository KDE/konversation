/***************************************************************************
                          ledlistviewitem.h  -  A list view with led indicator
                             -------------------
    begin                : Thu Jul 25 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LEDLISTVIEWITEM_H
#define LEDLISTVIEWITEM_H

#include <klistview.h>

#include <qiconset.h>
#include <qpixmap.h>

#include "images.h"
#include "nick.h"

/*
  @author Matthias Gierlings
  @author Dario Abatianni (sorting code)
*/

class LedListViewItem : public KListViewItem
{
  public:
    LedListViewItem(KListView* parent,
                    const QString &passed_label,
                    const QString &passed_label2,
                    Nick *n);
    ~LedListViewItem();
    int getFlags() const;
    virtual int compare(QListViewItem* item,int col,bool ascending) const;
    Nick *getNick();
    void refresh();
  protected:
    Nick *nick;
    QPixmap adminLedOn;
    QPixmap ownerLedOff;
    QPixmap opLedOn;
    QPixmap opLedOff;
    QPixmap voiceLedOn;
    QPixmap voiceLedOff;

    QIconSet currentLeds;
    QString label;

    Images leds;

};

#endif
