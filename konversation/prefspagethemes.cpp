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
#include <qpushbutton.h>

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

#include <unistd.h> // unlink()

#include "prefspagethemes.h"
#include "preferences.h"
#include "common.h"
#include "konversationapplication.h"

using namespace Konversation;

PrefsPageThemes::PrefsPageThemes(QFrame* newParent,Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{

  QGridLayout* gridLayout = new QGridLayout(newParent,3,1,marginHint(),spacingHint());

  QLabel* selectLabel = new QLabel(i18n("Select Nicklist Icon Theme to Use"),newParent,"selectLabel");
  m_themeList = new KListBox(newParent,"themeList");
  m_themeList->setSelectionMode(QListBox::Single);
  QLabel* previewLabel = new QLabel(newParent);
  previewLabel->setText("Preview :");
  
  
  QFrame* previewFrame = new QFrame(newParent);
  QHBoxLayout *previewLayout=new QHBoxLayout(previewFrame);

  QFrame* buttonFrame = new QFrame(newParent);
  QHBoxLayout *buttonLayout=new QHBoxLayout(buttonFrame,spacingHint());
  
  QPushButton* installButton = new QPushButton(buttonFrame,"installButton");
  m_removeButton = new QPushButton(buttonFrame,"removeButton");
  
  installButton->setText(i18n("I&nstall Theme"));
  m_removeButton->setText(i18n("&Remove Theme"));
  
  buttonLayout->addWidget(installButton);
  buttonLayout->addWidget(m_removeButton);
  
  previewLayout->addStretch(9); 
  
  for(int i=0; i <= 5; ++i) {
    
    previewLayout->addStretch(1);

    m_label[i] = new QLabel(previewFrame);
    previewLayout->addWidget(m_label[i]);

  }

  previewLayout->addStretch(10);

  gridLayout->addWidget(selectLabel, 1, 0);
  gridLayout->addWidget(m_themeList, 2, 0);
  gridLayout->addWidget(previewLabel, 3, 0);
  gridLayout->addWidget(previewFrame, 4, 0);
  gridLayout->addWidget(buttonFrame, 5, 0);
  
  updateList();
  updateButtons();
  
  connect(m_themeList,SIGNAL(highlighted(int)),this,SLOT(updatePreview(int)));
  connect(m_themeList,SIGNAL(currentChanged(QListBoxItem*)),this,SLOT(updateButtons()));
  connect(installButton,SIGNAL(clicked()),this,SLOT(installTheme()));
  connect(m_removeButton,SIGNAL(clicked()),this,SLOT(removeTheme()));
}

PrefsPageThemes::~PrefsPageThemes()
{
}

void PrefsPageThemes::applyPreferences()
{
  QString theme;
  theme = m_dirs[m_themeList->currentItem()];
  theme = theme.section('/',-2,-2);
  kdDebug() << "Theme :" << theme << endl;
  preferences->setIconTheme(theme);
}

void PrefsPageThemes::installTheme()
{
  KURL themeURL = KFileDialog::getOpenURL(QString::null, 
					  i18n("*.tar.gz *.tar.bz2 *.tar *.zip|Konversation Themes"),
                                           NULL,
					  i18n("Select Theme Package")
					   );
  
  QString tmpThemeFile;
  if(!KIO::NetAccess::download(themeURL, tmpThemeFile, NULL))
    {
      KMessageBox::error(NULL,
			 KIO::NetAccess::lastErrorString(),
			 i18n("Failed to download theme"),
			 KMessageBox::Notify
			 );
      return;
    }
  
  KTar themeArchieve(tmpThemeFile);
  themeArchieve.open(IO_ReadOnly);
  kapp->processEvents();
  
  const KArchiveDirectory* themeDir = themeArchieve.directory();;
  QStringList allEntries = themeDir->entries();

  for(QStringList::Iterator it=allEntries.begin(); it != allEntries.end(); ++it)
    {
      if(themeDir->entry(*it+"/themerc") == NULL)
	{
	  
	  KMessageBox::error(NULL,
			     i18n("Invalid Theme Archieve"),
			     i18n("Cannot install theme"),
			     KMessageBox::Notify
			     );

	  KIO::NetAccess::removeTempFile(tmpThemeFile);
	  themeArchieve.close();
	  return;
	}
      else
	{
	  QString themesDir(locateLocal("data", "konversation/themes/"));
	  
	  themeDir->copyTo(themesDir);
	  KIO::NetAccess::removeTempFile(tmpThemeFile);
	  themeArchieve.close();
	  
	  updateList();
	  updateButtons();
	  
	  return;
	}
    }
}

void PrefsPageThemes::removeTheme()
{
  QString dir;
  QString themeName = m_themeList->currentText();

  dir = m_dirs[m_themeList->currentItem()];

  int remove = KMessageBox::warningContinueCancel(0L,
						  i18n("Do you want to remove %1 ?").arg(themeName),
						  i18n("Remove Theme"),
						  KStdGuiItem::cont(),
						  "warningRemoveTheme"
						  );

  if(remove == KMessageBox::Continue) 
    {
      unlink(QFile::encodeName(dir));
      KIO::del(KURL(dir.remove("themerc")));
      updateList();
    }
}

void PrefsPageThemes::updatePreview(int id)
{
  QString dir;
  dir = m_dirs[id];
  dir.remove("/themerc");
  QPixmap normal(dir+"/irc_normal.png");

  m_label[0]->setPixmap(normal);
  QToolTip::add(m_label[0],i18n("Icon For Normal Users"));
  m_label[1]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_voice.png")));
  QToolTip::add(m_label[1],i18n("Icon For Users With Voice"));
  m_label[2]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_halfop.png")));
  QToolTip::add(m_label[2],i18n("Icon For Users With Half-Operator Priviliges"));
  m_label[3]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_op.png")));
  QToolTip::add(m_label[3],i18n("Icon For Users With Operator Priviliges"));
  m_label[4]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_owner.png")));
  QToolTip::add(m_label[4],i18n("Icon For Users With Owner privileges"));
  m_label[5]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_admin.png")));
  QToolTip::add(m_label[5],i18n("Icon For Users With Admin privileges"));

  for(int i=0; i <= 5; ++i)
    m_label[i]->show();
}

void PrefsPageThemes::updateList()
{
  QString themeName,themeComment;
  QFile themeRC;
  QTextStream stream;
  QString currentTheme = KonversationApplication::preferences.getIconTheme();
  int index = 0;
  bool found = false;

  m_dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/themerc");
  m_themeList->clear();

  for(QStringList::Iterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
    {
      if(!found)
	{
	  if((*it).section('/',-2,-2) == currentTheme)
	    found = true;
	  else
	    ++index;
	}

      themeRC.setName(*it);
      themeRC.open(IO_ReadOnly);
      stream.setDevice(&themeRC);

      themeName = stream.readLine();
      themeName = themeName.section('=',1,1);

      themeComment = stream.readLine();
      themeComment = themeComment.section('=',1,1);
      
      if(!themeComment.isEmpty())
	themeName = themeName+" ( "+themeComment+" )";

      m_themeList->insertItem(themeName);
      themeRC.close();
    }

  m_themeList->setSelected(index,TRUE);
  updatePreview(index);
}

void PrefsPageThemes::updateButtons()
{
  if(m_themeList->count() == 1)
    return;

  QString dir = m_dirs[m_themeList->currentItem()];
  QFile themeRC(dir);

  if(!themeRC.open(IO_ReadOnly | IO_WriteOnly))
    m_removeButton->setEnabled(false);
  else
    m_removeButton->setEnabled(true);

  themeRC.close();
}

#include "prefspagethemes.moc"
