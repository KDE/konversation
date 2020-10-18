/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 İsmail Dönmez <ismail@kde.org.tr>
    Based on the code by :
    SPDX-FileCopyrightText: 2002 Carsten Pfeiffer <pfeiffer@kde.org>
*/

#ifndef KONVIBOOKMARKHANDLER_H
#define KONVIBOOKMARKHANDLER_H

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

        Q_DISABLE_COPY(KonviBookmarkHandler)
};

#endif                                            // KONVIBOOKMARKHANDLER_H
