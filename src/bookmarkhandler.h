#ifndef KONVIBOOKMARKHANDLER_H
#define KONVIBOOKMARKHANDLER_H

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

Based on the code by :
Copyright (C) 2002 Carsten Pfeiffer <pfeiffer@kde.org>

*/

#include <kbookmarkmanager.h>


class QString;
class KMenu;
class KonversationMainWindow;
class KBookmarkMenu;

class KonviBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

        public:
        explicit KonviBookmarkHandler(KonversationMainWindow *mainWindow);
        ~KonviBookmarkHandler();

        KMenu* popupMenu();

        // KBookmarkOwner interface:
        virtual void openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km);
        virtual QString currentUrl() const;
        virtual QString currentTitle() const;
        virtual bool enableOption(BookmarkOption option) const;

    private slots:
        void slotBookmarksChanged(const QString &, const QString & caller);


    private:
        KonversationMainWindow* m_mainWindow;
        KMenu *m_menu;
        KBookmarkMenu *m_bookmarkMenu;
        QString m_file;
};
#endif                                            // KONVIBOOKMARKHANDLER_H
