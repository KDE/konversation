/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.h  -  shows a user tree of friends per server
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef NICKSONLINE_H
#define NICKSONLINE_H

#include <qvbox.h>
#include <qiconset.h>

#include "nickinfo.h"
#include "chatwindow.h"

/*
  @author Dario Abatianni
*/

class KListView;

class NicksOnline : public ChatWindow
{
  Q_OBJECT

  public:
    // Columns of the NickListView when using NickInfo.
    enum NickListViewColumn {
      nlvcServerNickChannel = 0,
      nlvcKabc = 1,
      nlvcAdditionalInfo = 2
    };
    
#ifdef USE_MDI
    NicksOnline(QString caption);
#else
    NicksOnline(QWidget* parent);
#endif
    ~NicksOnline();

  signals:
    void editClicked();
    void doubleClicked(const QString& server,const QString& nick);

  public slots:
    void setOnlineList(const QString& serverName,const QStringList& list,bool changed);
    virtual void adjustFocus();

  protected slots:
    void processDoubleClick(QListViewItem* item);
    void timerFired();

  protected:
    void updateServerOnlineList(Server* server, bool changed);
    void refreshAllServerOnlineLists();
    QString getNickAdditionalInfo(NickInfoPtr nickInfo);
#ifdef USE_MDI
    virtual void closeYourself(ChatWindow*);
#endif
    // Returns the named child of parent item in KListView.
    QListViewItem* findItemChild(const QListViewItem* parent, const QString& name);

    KListView* m_nickListView;
    QTimer* m_timer;
    QIconSet m_kabcIconSet;
};

#endif
