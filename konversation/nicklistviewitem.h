/***************************************************************************
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

class Nick;

/*
  @author Matthias Gierlings
  @author Dario Abatianni (sorting code)
*/

class NickListViewItem : public QObject, public KListViewItem
{
  Q_OBJECT
  public:
    NickListViewItem(KListView* parent,
		     QListViewItem *after,
                    const QString &passed_label,
                    const QString &passed_label2,
                    Nick *n);
    ~NickListViewItem();
    int getFlags() const;
    virtual int compare(QListViewItem* item,int col,bool ascending) const;
    Nick *getNick();

  public slots:
    void refresh();
  
  signals:
    void refreshed();

  protected:
    Nick *nick;

    QString label;

    QString calculateLabel1();
    QString calculateLabel2();
    int m_height;
    int m_flags;
    bool m_away;
};

#endif
