#ifndef KONVIBOOKMARKHANDLER_H
#define KONVIBOOKMARKHANDLER_H

#include <kbookmarkmanager.h>
#include "konversationmainwindow.h"
#include "konvibookmarkmenu.h"

class QTextStream;
class KPopupMenu;
class KonviBookmarkMenu;
class KBookmarkManager;

class KonviBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

public:
  KonviBookmarkHandler(KonversationMainWindow *mainWindow);
  ~KonviBookmarkHandler();

  QPopupMenu * popupMenu();

  // KBookmarkOwner interface:
  virtual void openBookmarkURL(const QString& url, const QString& title);
  KPopupMenu *menu() const;

private slots:
    void slotBookmarksChanged(const QString &, const QString & caller);
    void slotEditBookmarks();

signals:
    void openURL(const QString& url, const QString& title);

private:
    KonversationMainWindow* m_mainWindow;
    KPopupMenu *m_menu;
    KonviBookmarkMenu *m_bookmarkMenu;
    QString m_file;
};


#endif // KONVIBOOKMARKHANDLER_H
