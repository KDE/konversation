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

#include <KBookmarkManager>
#include <KBookmarkOwner>


class QString;
class QMenu;
class MainWindow;
class KBookmarkMenu;
class QMenu;

class KonviBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

        public:
        explicit KonviBookmarkHandler(QMenu *menu, MainWindow* mainWindow);
        ~KonviBookmarkHandler() override;

        // KBookmarkOwner interface:
        void openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km) override;
        QUrl currentUrl() const override;
        QString currentTitle() const override;
        bool enableOption(BookmarkOption option) const override;
        bool supportsTabs() const override;
        QList<KBookmarkOwner::FutureBookmark> currentBookmarkList() const override;
        void openFolderinTabs(const KBookmarkGroup &group) override;

    private:
        MainWindow* m_mainWindow;
        KBookmarkMenu *m_bookmarkMenu;
        QString m_file;
};
#endif                                            // KONVIBOOKMARKHANDLER_H
