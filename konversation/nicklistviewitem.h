// nicklistviewitem.h
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

#ifndef NICKLISTVIEWITEM_H
#define NICKLISTVIEWITEM_H

#include <klistview.h>

#include <qiconset.h>
#include <qpixmap.h>
#include <qobject.h>

#include "nick.h"

/*
  @author Matthias Gierlings
  @author Dario Abatianni (sorting code)
*/

class NickListViewItem : public QObject, public KListViewItem
{
  Q_OBJECT
  public:
    NickListViewItem(KListView* parent,
                    const QString &passed_label,
                    const QString &passed_label2,
                    Nick *n);
    ~NickListViewItem();
    int getFlags() const;
    virtual int compare(QListViewItem* item,int col,bool ascending) const;
    Nick *getNick();

  protected slots:
    //We will refresh ourselves, so make it protected.
    void refresh();
  
  protected:
    Nick *nick;

    QString label;

    QString calculateLabel1();
    QString calculateLabel2();
    
    static void initializeIcons();
    
    static bool s_bIconsInitialized;
    static QPixmap *s_pIconNormal, *s_pIconNormalAway,
                   *s_pIconVoice,  *s_pIconVoiceAway,
                   *s_pIconHalfOp, *s_pIconHalfOpAway,
                   *s_pIconOp,     *s_pIconOpAway,
                   *s_pIconOwner,  *s_pIconOwnerAway,
                   *s_pIconAdmin,  *s_pIconAdminAway;
};

#endif
