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
class KPopupMenu;
class KonversationMainWindow;
class KonviBookmarkMenu;

class KonviBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

        public:
        explicit KonviBookmarkHandler(KonversationMainWindow *mainWindow);
        ~KonviBookmarkHandler();

        KPopupMenu* popupMenu();

        // KBookmarkOwner interface:
        virtual void openBookmarkURL(const QString& url, const QString& title);
        virtual QString currentURL() const;
        virtual QString currentTitle() const;

    private slots:
        void slotBookmarksChanged(const QString &, const QString & caller);
        void slotEditBookmarks();


    private:
        KonversationMainWindow* m_mainWindow;
        KPopupMenu *m_menu;
        KonviBookmarkMenu *m_bookmarkMenu;
        QString m_file;
};
#endif                                            // KONVIBOOKMARKHANDLER_H
