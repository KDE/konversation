/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004-2005 by Peter Simonsson
*/
#include "prefspagechatwinappearance.h"

#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

#include <klocale.h>
#include <kfontrequester.h>
#include <kurlrequester.h>

#include "preferences.h"

PrefsPageChatWinAppearance::PrefsPageChatWinAppearance(QWidget* newParent,Preferences* newPreferences)
 : ChatWindowAppearance_Config(newParent)
{
  preferences = newPreferences;

  kcfg_Timestamping->setChecked(preferences->getTimestamping());
  kcfg_ShowDate->setChecked(preferences->getShowDate());

  kcfg_TimestampFormat->insertItem("hh");
  kcfg_TimestampFormat->insertItem("hh:mm");
  kcfg_TimestampFormat->insertItem("hh:mm:ss");
  kcfg_TimestampFormat->insertItem("hh ap");
  kcfg_TimestampFormat->insertItem("hh:mm ap");
  kcfg_TimestampFormat->insertItem("hh:mm:ss ap");

  // find actual timestamp format
  for(int index=0; index < kcfg_TimestampFormat->count(); index++) {
    if(kcfg_TimestampFormat->text(index) == preferences->getTimestampFormat()) {
      kcfg_TimestampFormat->setCurrentItem(index);
    }
  }

  kcfg_ShowTopic->setChecked(preferences->getShowTopic());
  kcfg_ShowModeButtons->setChecked(preferences->getShowModeButtons());
  kcfg_ShowQuickButtons->setChecked(preferences->getShowQuickButtons());
  kcfg_AutoUserhost->setChecked(preferences->getAutoUserhost());
  kcfg_ShowNicknameBox->setChecked(preferences->showNicknameBox());

  kcfg_ShowBackgroundImage->setChecked(preferences->getShowBackgroundImage());

  kcfg_BackgroundImage->setCaption(i18n("Select Background Image"));
  kcfg_BackgroundImage->setURL(preferences->getBackgroundImageName());

  connect(kcfg_ShowBackgroundImage,SIGNAL(toggled(bool)),this, SLOT(setBackgroundImageConfig(bool)));
  connect(kcfg_BackgroundImage,SIGNAL(textChanged(const QString&)),this,SLOT(saveBackgroundImage(const QString&)));
}

// This really shouldn't be here...
void PrefsPageChatWinAppearance::setBackgroundImageConfig(bool state )
{
    if( !state )
        preferences->setShowBackgroundImage(FALSE );
    else {
        preferences->setBackgroundImageName(kcfg_BackgroundImage->url());
        preferences->setShowBackgroundImage(TRUE);
    }
}

void PrefsPageChatWinAppearance::saveBackgroundImage(const QString& url)
{
    preferences->setShowBackgroundImage(TRUE);
    preferences->setBackgroundImageName(url);
}

void PrefsPageChatWinAppearance::applyPreferences()
{
  preferences->setTimestamping(kcfg_Timestamping->isChecked());
  preferences->setShowDate(kcfg_ShowDate->isChecked());
  preferences->setTimestampFormat(kcfg_TimestampFormat->currentText());
  preferences->setShowQuickButtons(kcfg_ShowQuickButtons->isChecked());
  preferences->setShowModeButtons(kcfg_ShowModeButtons->isChecked());
  preferences->setAutoUserhost(kcfg_AutoUserhost->isChecked());
  preferences->setShowTopic(kcfg_ShowTopic->isChecked());
  preferences->setShowNicknameBox(kcfg_ShowNicknameBox->isChecked());
}

#include "prefspagechatwinappearance.moc"
