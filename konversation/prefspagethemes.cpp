/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004 by İsmail Dönmez
*/
#include "prefspagethemes.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qbitmap.h>
#include <qpainter.h>

#include <klistbox.h>
#include <kurl.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include "preferences.h"
#include "common.h"

using namespace Konversation;

PrefsPageThemes::PrefsPageThemes(QFrame* newParent,Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{

  QGridLayout* gridLayout = new QGridLayout(newParent,3,1,marginHint(),spacingHint());

  QLabel* selectLabel = new QLabel("Select Nicklist Icon Theme to Use",newParent,"selectLabel");
  themeList = new KListBox(newParent,"themeList");
  QLabel* previewLabel = new QLabel(newParent);
  previewLabel->setText("Preview :");
  
  QHBox* previewBox = new QHBox(newParent);

  for(int i=0; i <= 5; ++i)
    label[i] = new QLabel(previewBox);
  
  gridLayout->addWidget(selectLabel, 1, 0);
  gridLayout->addWidget(themeList, 2, 0);
  gridLayout->addWidget(previewLabel, 3, 0);
  gridLayout->addWidget(previewBox, 4, 0);
  
  
  dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/themerc");
  
  QString themeName,themeComment;
  QFile themeRC;
  QTextStream stream;

  for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it)
    {
      themeRC.setName( *it );
      themeRC.open( IO_ReadOnly );
      stream.setDevice( &themeRC );

      themeName = stream.readLine();
      themeName = themeName.section('=',1,1);

      themeComment = stream.readLine();
      themeComment = themeComment.section('=',1,1);

      themeList->insertItem( themeName );
      themeRC.close();
    }
  
  connect(themeList,SIGNAL(highlighted(int)),this,SLOT(updatePreview(int)));
  
}

PrefsPageThemes::~PrefsPageThemes()
{
}

void PrefsPageThemes::applyPreferences()
{
  QString theme;
  theme = dirs[themeList->currentItem()];
  theme = theme.section('/',-2,-2);
  kdDebug() << "Theme :" << theme << endl;
  preferences->setIconTheme( theme );
}

void PrefsPageThemes::updatePreview(int id)
{
  QString dir;
  dir = dirs[id];
  dir.remove("/themerc");
  QPixmap normal(dir+"/irc_normal.png");

  label[0]->setPixmap(normal);
  label[1]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_voice.png")));
  label[2]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_halfop.png")));
  label[3]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_op.png")));
  label[4]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_owner.png")));
  label[5]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_admin.png")));

  for(int i=0; i <= 5; ++i)
    label[i]->show();
}

#include "prefspagethemes.moc"
