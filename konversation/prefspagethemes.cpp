/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004 by İsmail Dönmez ( Resistence is Futile. Turn god damn unicode on! )
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qtooltip.h>

#include <klistbox.h>
#include <kurl.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <ktar.h>
#include <kdesktopfile.h>

#include <unistd.h> // unlink()

#include "prefspagethemes.h"
#include "preferences.h"
#include "images.h"
#include "common.h"
#include "konversationapplication.h"

using namespace Konversation;

PrefsPageThemes::PrefsPageThemes(QWidget* newParent,Preferences* newPreferences)
  : Theme_Config( newParent )
{

  preferences = newPreferences;

  updateList();
  updateButtons();

  m_oldTheme = KonversationApplication::preferences.getIconTheme();

  connect(themeList,SIGNAL(highlighted(int)),this,SLOT(updatePreview(int)));
  connect(themeList,SIGNAL(currentChanged(QListBoxItem*)),this,SLOT(updateButtons()));
  connect(installButton,SIGNAL(clicked()),this,SLOT(installTheme()));
  connect(removeButton,SIGNAL(clicked()),this,SLOT(removeTheme()));
}

PrefsPageThemes::~PrefsPageThemes()
{
}

void PrefsPageThemes::applyPreferences()
{
  if(themeList->count())
    {
      QString theme;
      theme = m_dirs[themeList->currentItem()];
      theme = theme.section('/',-2,-2);
      if(m_oldTheme != theme)
	{
	  kdDebug() << "New Theme :" << theme << endl;
	  preferences->setIconTheme(theme);
	  KonversationApplication::instance()->images()->initializeNickIcons();
	  KonversationApplication::instance()->updateNickIcons();
	}
    }
}

void PrefsPageThemes::installTheme()
{
  KURL themeURL = KFileDialog::getOpenURL(QString::null,
					  i18n("*.tar.gz *.tar.bz2 *.tar *.zip|Konversation Themes"),
                                           NULL,
					  i18n("Select Theme Package")
					   );

  if(themeURL.isEmpty())
    return;

  QString themesDir(locateLocal("data", "konversation/themes/"));
  QString tmpThemeFile;

  if(!KIO::NetAccess::download(themeURL, tmpThemeFile, NULL))
    {
      KMessageBox::error(0L,
			 KIO::NetAccess::lastErrorString(),
			 i18n("Failed to Download Theme"),
			 KMessageBox::Notify
			 );
      return;
    }

  QDir themeInstallDir(tmpThemeFile);

  if(themeInstallDir.exists()) // We got a directory not a file
    {
      if(themeInstallDir.exists("index.desktop"))
	  KIO::NetAccess::dircopy(KURL(tmpThemeFile),KURL(themesDir),0L);
      else
	{
	  KMessageBox::error(0L,
                             i18n("Theme archive is invalid."),
                             i18n("Cannot Install Theme"),
                             KMessageBox::Notify
                             );
	}
    }
  else // we got a file
    {

      KTar themeArchive(tmpThemeFile);
      themeArchive.open(IO_ReadOnly);
      kapp->processEvents();

      const KArchiveDirectory* themeDir = themeArchive.directory();;
      QStringList allEntries = themeDir->entries();

      for(QStringList::ConstIterator it=allEntries.begin(); it != allEntries.end(); ++it)
	{
	  if(themeDir->entry(*it+"/index.desktop") == NULL)
	    {
	      KMessageBox::error(0L,
				 i18n("Theme archive is invalid."),
				 i18n("Cannot Install Theme"),
				 KMessageBox::Notify
				 );
	      break;
	    }
	  else
	    themeDir->copyTo(themesDir);

	}
      themeArchive.close();
    }

  updateList();
  updateButtons();
  KIO::NetAccess::removeTempFile(tmpThemeFile);

}

void PrefsPageThemes::removeTheme()
{
  QString dir;
  QString themeName = themeList->currentText();

  dir = m_dirs[themeList->currentItem()];

  int remove = KMessageBox::warningContinueCancel(0L,
						  i18n("Do you want to remove %1 ?").arg(themeName),
						  i18n("Remove Theme"),
						  KStdGuiItem::del(),
						  "warningRemoveTheme"
						  );

  if(remove == KMessageBox::Continue)
    {
      unlink(QFile::encodeName(dir));
      KIO::del(KURL(dir.remove("index.desktop")));
      updateList();
      updateButtons();
    }
}

void PrefsPageThemes::updatePreview(int id)
{
  QString dir;
  dir = m_dirs[id];
  dir.remove("/index.desktop");
  QPixmap normal(dir+"/irc_normal.png");

  previewLabel1->setPixmap(normal);
  previewLabel2->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_away.png")));
  previewLabel3->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_voice.png")));
  previewLabel4->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_halfop.png")));
  previewLabel5->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_op.png")));
  previewLabel6->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_owner.png")));
  previewLabel7->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_admin.png")));
}

void PrefsPageThemes::updateList()
{
  QString themeName,themeComment;
  QString currentTheme = KonversationApplication::preferences.getIconTheme();
  int index = 0;
  bool found = false;

  if(!KGlobal::dirs()->findAllResources("data","konversation/themes/"+currentTheme+"/index.desktop").count())
    {
      m_oldTheme=currentTheme;
      currentTheme = "default";
      KonversationApplication::preferences.setIconTheme("default");
    }

  m_dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/index.desktop");

  if(m_dirs.count() > 0)
    {

      themeList->clear();

      for(QStringList::ConstIterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
	{
	  if(!found)
	    {
	      if((*it).section('/',-2,-2) == currentTheme)
		found = true;
	      else
		++index;
	    }

	  KDesktopFile themeRC(*it);
	  themeName = themeRC.readName();
	  themeComment = themeRC.readComment();

	  if(!themeComment.isEmpty())
	    themeName = themeName+" ( "+themeComment+" )";

	  themeList->insertItem(themeName);
	}

      themeList->setSelected(index,TRUE);
      updatePreview(index);
    }
}

void PrefsPageThemes::updateButtons()
{
  if(themeList->count() < 2)
    {
      removeButton->setEnabled(false);
      return;
    }

  QString dir = m_dirs[themeList->currentItem()];
  QFile themeRC(dir);

  if(!themeRC.open(IO_ReadOnly | IO_WriteOnly))
    removeButton->setEnabled(false);
  else
    removeButton->setEnabled(true);

  themeRC.close();
}

#include "prefspagethemes.moc"
