#ifndef KONVIBOOKMARKMENU_H
#define KONVIBOOKMARKMENU_H

/*
  Copyright (c) 2005 by İsmail Dönmez <ismail@kde.org.tr>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************

Based on the code by:
Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

*/

#include <qobject.h>
#include <kbookmarkmenu.h>

class QString;
class KBookmark;
class KAction;
class KActionMenu;
class KActionCollection;
class KBookmarkOwner;
class KBookmarkMenu;
class KPopupMenu;
class KonviBookmarkHandler;
class KonviBookmarkMenu;
class KonviBookmarkMenuPrivate;

class KonviBookmarkMenu : public KBookmarkMenu
{
    Q_OBJECT

        public:
        KonviBookmarkMenu( KBookmarkManager* mgr,
            KonviBookmarkHandler * _owner, KPopupMenu * _parentMenu,
            KActionCollection *collec, bool _isRoot,
            bool _add = true, const QString & parentAddress = "");

        void fillBookmarkMenu();

    public slots:

    private:

    protected slots:
        void slotAboutToShow2();
        void slotBookmarkSelected();

    protected:
        void refill();

    private:
        KonviBookmarkHandler * m_kOwner;
        KonviBookmarkMenuPrivate *d;
};
#endif                                            // KONVIBOOKMARKMENU_H
