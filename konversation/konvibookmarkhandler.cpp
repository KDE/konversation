#include <kbookmarkimporter.h>
#include <kbookmarkdombuilder.h>
#include <kmimetype.h>
#include <kpopupmenu.h>
#include <ksavefile.h>
#include <kstandarddirs.h>

#include "konversationapplication.h"
#include "konvibookmarkmenu.h"
#include "konvibookmarkhandler.h"

KonviBookmarkHandler::KonviBookmarkHandler(KonversationMainWindow* mainWindow)
    : QObject( mainWindow, "KonviBookmarkHandler" ),
      KBookmarkOwner(),
      m_mainWindow(mainWindow)
{
    m_menu = new KPopupMenu( mainWindow, "bookmark menu" );

    m_file = locate( "data", "konversation/bookmarks.xml" );
    if ( m_file.isEmpty() )
        m_file = locateLocal( "data", "konversation/bookmarks.xml" );

    KBookmarkManager *manager = KBookmarkManager::managerForFile( m_file, false);
    manager->setEditorOptions(kapp->caption(), false);
    manager->setUpdate( true );
    manager->setShowNSBookmarks( false );
    
    connect( manager, SIGNAL( changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &, const QString &) ) );

    m_bookmarkMenu = new KonviBookmarkMenu( manager, this, m_menu,  mainWindow->actionCollection(), true );
}

KonviBookmarkHandler::~KonviBookmarkHandler()
{
    delete m_bookmarkMenu;
}

void KonviBookmarkHandler::slotEditBookmarks()
{
    KProcess proc;
    proc << QString::fromLatin1("keditbookmarks");
    proc << "--nobrowser";
    proc << "--caption" << i18n("Konversation Bookmarks Editor");
    proc << m_file;
    proc.start(KProcess::DontCare);
}

void KonviBookmarkHandler::slotBookmarksChanged( const QString &,
						 const QString &)
{
    // This is called when someone changes bookmarks in konversation
    m_bookmarkMenu->slotBookmarksChanged("");
}

void KonviBookmarkHandler::openBookmarkURL(const QString& url, const QString& title)
{
  emit openURL(url,title);
}

KPopupMenu* KonviBookmarkHandler::menu() const
{
  return m_menu;
}

#include "konvibookmarkhandler.moc"
