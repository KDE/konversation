#ifndef KONVIBOOKMARKMENU_H
#define KONVIBOOKMARKMENU_H

#include <qptrlist.h>
#include <qptrstack.h>
#include <qobject.h>
#include <sys/types.h>
#include <kbookmark.h>
#include <kbookmarkmenu.h>

#include "konvibookmarkhandler.h"


class QString;
class KBookmark;
class KAction;
class KActionMenu;
class KActionCollection;
class KBookmarkOwner;
class KBookmarkMenu;
class KPopupMenu;
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

#endif // KONVIBOOKMARKMENU_H
