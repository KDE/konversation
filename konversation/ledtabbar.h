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
*/

#ifndef LEDTABBAR_H
#define LEDTABBAR_H

#include <ktabbar.h>
#include "nickinfo.h"
/*
  @author Dario Abatianni
*/

class KPopupMenu;
class LedTab;


class LedTabBar : public KTabBar
{
  Q_OBJECT

  public:
    LedTabBar(QWidget* parent,const char* name);
    ~LedTabBar();

    LedTab* tab(int id);
    LedTab* tab(QWidget* widget);

    virtual void layoutTabs();
    void updateTabs();
    LedTab* tabAt(int index) const;

  signals:
    void moveTabLeft(int id);
    void moveTabRight(int id);
    void closeTab(int id);
    
#ifndef QT_NO_WHEELEVENT
    void wheel(QWheelEvent *e);
#endif
    
  public slots:
    void repaintLED(LedTab* tab);

  protected:
    enum PopupIDs
    {
      Label=0,
      MoveLeft,
      MoveRight,
      CloseTab,
      EnableNotifications,
      EncodingSub,
      AddressbookSub,
      AddressbookEdit,
      AddressbookChange,
      AddressbookDelete,
      AddressbookNew,
      SendEmail
    };
    // these two come from the original QT source
    virtual void paint( QPainter *, QTab *, bool ) const; // ### not const
    virtual void paintLabel( QPainter*, const QRect&, QTab*, bool ) const;
    void insertAssociationSubMenu(const NickInfoPtr &nickinfo);
    QPixmap* m_closePixmap;
    
    void contextMenuEvent(QContextMenuEvent* ce);
    void mouseReleaseEvent(QMouseEvent* e);

    KPopupMenu* m_popup;
    KPopupMenu* m_popupEncoding;
    KPopupMenu* m_popupAddressbook;
    
    QStringList m_encodingsList;

#ifndef QT_NO_WHEELEVENT
    void wheelEvent( QWheelEvent *e );
#endif
};

#endif
