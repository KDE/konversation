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
#include <qtooltip.h>

#include <klistbox.h>
#include <kurl.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>

#include "preferences.h"
#include "common.h"

using namespace Konversation;

PrefsPageThemes::PrefsPageThemes(QFrame* newParent,Preferences* newPreferences)
 : PrefsPage(newParent, newPreferences)
{

  QGridLayout* gridLayout = new QGridLayout(newParent,3,1,marginHint(),spacingHint());

  QLabel* selectLabel = new QLabel(i18n("Select Nicklist Icon Theme to Use"),newParent,"selectLabel");
  themeList = new KListBox(newParent,"themeList");
  QLabel* previewLabel = new QLabel(newParent);
  previewLabel->setText("Preview :");
  
  QFrame* previewFrame = new QFrame(newParent);
  
  QHBoxLayout *previewLayout=new QHBoxLayout( previewFrame );
  
  for(int i=0; i <= 5; ++i) {

    if(i == 0) 
      previewLayout->addStretch(10);
    else
      previewLayout->addStretch(1);

    label[i] = new QLabel(previewFrame);
    previewLayout->addWidget(label[i]);

    if(i == 5)
      previewLayout->addStretch(10);
  }
  
  gridLayout->addWidget(selectLabel, 1, 0);
  gridLayout->addWidget(themeList, 2, 0);
  gridLayout->addWidget(previewLabel, 3, 0);
  gridLayout->addWidget(previewFrame, 4, 0);
  
  
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
  QToolTip::add(label[0],i18n("Icon For Normal Users"));
  label[1]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_voice.png")));
  QToolTip::add(label[1],i18n("Icon For Users With Voice"));
  label[2]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_halfop.png")));
  QToolTip::add(label[2],i18n("Icon For Users With Half-Operator Priviliges"));
  label[3]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_op.png")));
  QToolTip::add(label[3],i18n("Icon For Users With Operator Priviliges"));
  label[4]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_owner.png")));
  QToolTip::add(label[4],i18n("Icon For Users With Owner privileges"));
  label[5]->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_admin.png")));
  QToolTip::add(label[5],i18n("Icon For Users With Admin privileges"));

  for(int i=0; i <= 5; ++i)
    label[i]->show();
}

#include "prefspagethemes.moc"
