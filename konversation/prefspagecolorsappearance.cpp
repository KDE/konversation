/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2002 by Dario Abatianni
             (C) 2004 by Peter Simonsson
	     (C) 2004 by İsmail Dönmez
*/

#include <kcolorbutton.h>
#include <kdebug.h>

#include <qstringlist.h>
#include <qcheckbox.h>
#include <qgroupbox.h>

#include "prefspagecolorsappearance.h"
#include "colorsappearance_preferences.h"
#include "preferences.h"

PrefsPageColorsAppearance::PrefsPageColorsAppearance(QWidget* newParent,Preferences* newPreferences)
 : ColorsAppearance_Config(newParent)
{
  preferences = newPreferences;
  
  // Checkboxes
  kcfg_UseCustomColors->setChecked(preferences->getColorInputFields());
  coloredNicksGBox->setChecked(preferences->getUseColoredNicks());
  ircColorsGBox->setChecked(!preferences->getFilterColors());

  // Custom Colors
  kcfg_ActionColor->setColor("#"+preferences->getColor("ActionMessage"));
  kcfg_BacklogColor->setColor("#"+preferences->getColor("BacklogMessage"));
  kcfg_ChannelMessageColor->setColor("#"+preferences->getColor("ChannelMessage"));
  kcfg_CommandMessageColor->setColor("#"+preferences->getColor("CommandMessage"));
  kcfg_HyperlinkColor->setColor("#"+preferences->getColor("LinkMessage"));
  kcfg_QueryMessageColor->setColor("#"+preferences->getColor("QueryMessage"));
  kcfg_ServerMessageColor->setColor("#"+preferences->getColor("ServerMessage"));
  kcfg_TimestampColor->setColor("#"+preferences->getColor("Time"));
  kcfg_BackgroundColor->setColor("#"+preferences->getColor("TextViewBackground"));
  kcfg_AlternateBackgroundColor->setColor("#"+preferences->getColor("AlternateBackground"));

  // Nick colors
  QStringList colorList = preferences->getNickColorList();
  kcfg_NickColor1->setColor(colorList[0]);
  kcfg_NickColor2->setColor(colorList[1]);
  kcfg_NickColor3->setColor(colorList[2]);
  kcfg_NickColor4->setColor(colorList[3]);
  kcfg_NickColor5->setColor(colorList[4]);
  kcfg_NickColor6->setColor(colorList[5]);
  kcfg_NickColor7->setColor(colorList[6]);
  kcfg_NickColor8->setColor(colorList[7]);
  if(colorList[8].isEmpty())
    colorList[8]="#000000";
  kcfg_NickColor9->setColor(colorList[8]);

  // IRC Color Codes
  QStringList ircColorList = preferences->getIRCColorList();
  kcfg_IrcColorCode1->setColor(ircColorList[0]);
  kcfg_IrcColorCode2->setColor(ircColorList[1]);
  kcfg_IrcColorCode3->setColor(ircColorList[2]);
  kcfg_IrcColorCode4->setColor(ircColorList[3]);
  kcfg_IrcColorCode5->setColor(ircColorList[4]);
  kcfg_IrcColorCode6->setColor(ircColorList[5]);
  kcfg_IrcColorCode7->setColor(ircColorList[6]);
  kcfg_IrcColorCode8->setColor(ircColorList[7]);
  kcfg_IrcColorCode9->setColor(ircColorList[8]);
  kcfg_IrcColorCode10->setColor(ircColorList[9]);
  kcfg_IrcColorCode11->setColor(ircColorList[10]);
  kcfg_IrcColorCode12->setColor(ircColorList[11]);
  kcfg_IrcColorCode13->setColor(ircColorList[12]);
  kcfg_IrcColorCode14->setColor(ircColorList[13]);
  kcfg_IrcColorCode15->setColor(ircColorList[14]);
  kcfg_IrcColorCode16->setColor(ircColorList[15]);

}


PrefsPageColorsAppearance::~PrefsPageColorsAppearance()
{
}

void PrefsPageColorsAppearance::applyPreferences()
{
  // Custom Colors
  preferences->setColor("ActionMessage",kcfg_ActionColor->color().name().mid(1));
  preferences->setColor("BacklogMessage",kcfg_BacklogColor->color().name().mid(1));
  preferences->setColor("ChannelMessage",kcfg_ChannelMessageColor->color().name().mid(1));
  preferences->setColor("CommandMessage",kcfg_CommandMessageColor->color().name().mid(1));
  preferences->setColor("LinkMessage",kcfg_HyperlinkColor->color().name().mid(1));
  preferences->setColor("QueryMessage",kcfg_QueryMessageColor->color().name().mid(1));
  preferences->setColor("ServerMessage",kcfg_ServerMessageColor->color().name().mid(1));
  preferences->setColor("Time",kcfg_TimestampColor->color().name().mid(1));
  preferences->setColor("TextViewBackground",kcfg_BackgroundColor->color().name().mid(1));
  preferences->setColor("AlternateBackground",kcfg_AlternateBackgroundColor->color().name().mid(1));

  // Nick colors
  QStringList nickColorList;

  nickColorList.append(kcfg_NickColor1->color().name());
  nickColorList.append(kcfg_NickColor2->color().name());
  nickColorList.append(kcfg_NickColor3->color().name());
  nickColorList.append(kcfg_NickColor4->color().name());
  nickColorList.append(kcfg_NickColor5->color().name());
  nickColorList.append(kcfg_NickColor6->color().name());
  nickColorList.append(kcfg_NickColor7->color().name());
  nickColorList.append(kcfg_NickColor8->color().name());
  nickColorList.append(kcfg_NickColor9->color().name());
  preferences->setNickColorList(nickColorList);
  

  // IRC Colors
  QStringList ircColorList;
  
  ircColorList.append(kcfg_IrcColorCode1->color().name());
  ircColorList.append(kcfg_IrcColorCode2->color().name());
  ircColorList.append(kcfg_IrcColorCode3->color().name());
  ircColorList.append(kcfg_IrcColorCode4->color().name());
  ircColorList.append(kcfg_IrcColorCode5->color().name());
  ircColorList.append(kcfg_IrcColorCode6->color().name());
  ircColorList.append(kcfg_IrcColorCode7->color().name());
  ircColorList.append(kcfg_IrcColorCode8->color().name());
  ircColorList.append(kcfg_IrcColorCode9->color().name());
  ircColorList.append(kcfg_IrcColorCode10->color().name());
  ircColorList.append(kcfg_IrcColorCode11->color().name());
  ircColorList.append(kcfg_IrcColorCode12->color().name());
  ircColorList.append(kcfg_IrcColorCode13->color().name());
  ircColorList.append(kcfg_IrcColorCode14->color().name());
  ircColorList.append(kcfg_IrcColorCode15->color().name());
  ircColorList.append(kcfg_IrcColorCode16->color().name());
  preferences->setIRCColorList(ircColorList);

  // Checkboxes
  preferences->setColorInputFields(kcfg_UseCustomColors->isChecked());
  preferences->setUseColoredNicks(coloredNicksGBox->isChecked());
  preferences->setFilterColors(!ircColorsGBox->isChecked());
}

#include "prefspagecolorsappearance.moc"
