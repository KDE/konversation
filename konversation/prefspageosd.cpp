/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Configuration tab for the OSD Widget
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qgroupbox.h>

#include <klocale.h>
#include <klistview.h>
#include <klineeditdlg.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <knuminput.h>
#include <kfontdialog.h>
#include <kcolorcombo.h>
#include <kfontrequester.h>

#include "prefspageosd.h"
#include "preferences.h"
#include "konversationapplication.h"
#include "osd.h"

PrefsPageOSD::PrefsPageOSD(QWidget* newParent,Preferences* newPreferences) :
  OSD_Config( newParent )
{
  showingPage = false;
  preferences = newPreferences;
  
  // set up the preview OSD widget
  m_pOSDPreview = new OSDPreviewWidget("Konversation", newParent);
  connect(m_pOSDPreview, SIGNAL(positionChanged()), this, SLOT(slotPositionChanged()));

  // General
  kcfg_UseOsdDisplay->setChecked(preferences->getOSDUsage());

  kcfg_OsdNickAppear->setChecked(preferences->getOSDShowOwnNick());
  kcfg_OsdChannelMessage->setChecked(preferences->getOSDShowChannel());
  kcfg_OsdQueryActivity->setChecked(preferences->getOSDShowQuery());
  kcfg_OsdJoinPart->setChecked(preferences->getOSDShowChannelEvent());

  // Font settings
   osdFont = preferences->getOSDFont();
   connect(kcfg_OsdFont,SIGNAL(fontSelected(const QFont&)), this, SLOT(osdFontClicked()));

  kcfg_OsdDrawShadow->setChecked(preferences->getOSDDrawShadow());

  //color box
  kcfg_OsdCustomColor->setChecked(preferences->getOSDUseCustomColors());
  kcfg_OsdTextColor->setColor(preferences->getOSDTextColor());
  kcfg_OsdBackgroundColor->setColor(preferences->getOSDBackgroundColor());

   //others box
  kcfg_OsdDuration->setValue(preferences->getOSDDuration());

  const int numScreens = QApplication::desktop()->numScreens();
    for( int i = 0; i < numScreens; i++ )
      kcfg_OsdScreen->insertItem( QString::number( i ) );
  
  kcfg_OsdScreen->setCurrentItem(preferences->getOSDScreen());

  // update previews
  m_pOSDPreview->setAlignment((OSDWidget::Alignment)preferences->getOSDAlignment());
  m_pOSDPreview->setOffset(preferences->getOSDOffsetX(), preferences->getOSDOffsetY());
  slotOSDEnabledChanged(preferences->getOSDUsage());
  slotCustomColorsChanged(preferences->getOSDUseCustomColors());
  slotTextColorChanged(preferences->getOSDTextColor());
  slotBackgroundColorChanged(preferences->getOSDBackgroundColor());
  slotScreenChanged(preferences->getOSDScreen());
  slotDrawShadowChanged(preferences->getOSDDrawShadow());
  updateFonts();
  
  connect(kcfg_UseOsdDisplay, SIGNAL(toggled(bool)), this, SLOT(slotOSDEnabledChanged(bool)));
  connect(kcfg_OsdCustomColor, SIGNAL(toggled(bool)), this, SLOT(slotCustomColorsChanged(bool)));
  connect(kcfg_OsdTextColor, SIGNAL(activated(const QColor&)), this, SLOT(slotTextColorChanged(const QColor&)));
  connect(kcfg_OsdBackgroundColor, SIGNAL(activated(const QColor&)), this, SLOT(slotBackgroundColorChanged(const QColor&)));
  connect(kcfg_OsdScreen, SIGNAL(activated(int)), this, SLOT(slotScreenChanged(int)));
  connect(kcfg_OsdDrawShadow, SIGNAL(toggled(bool)), this, SLOT(slotDrawShadowChanged(bool)));
}

PrefsPageOSD::~PrefsPageOSD()
{
}

void PrefsPageOSD::aboutToShow()
{
  m_pOSDPreview->setShown(kcfg_UseOsdDisplay->isChecked());
  showingPage = true;
}

void PrefsPageOSD::aboutToHide()
{
  m_pOSDPreview->setShown(false);
  showingPage = false;
}

void PrefsPageOSD::slotOSDEnabledChanged(bool on)
{
  if(showingPage)
    m_pOSDPreview->setShown(on);
}

void PrefsPageOSD::slotCustomColorsChanged(bool on)
{
  if(on)
  {
    m_pOSDPreview->setTextColor(kcfg_OsdTextColor->color());
    m_pOSDPreview->setBackgroundColor(kcfg_OsdBackgroundColor->color());
  }
  else
    m_pOSDPreview->unsetColors();
}

void PrefsPageOSD::slotTextColorChanged(const QColor& color)
{
  if(kcfg_OsdCustomColor->isChecked())
    m_pOSDPreview->setTextColor(color);
}

void PrefsPageOSD::slotBackgroundColorChanged(const QColor& color)
{
  if(kcfg_OsdCustomColor->isChecked())
    m_pOSDPreview->setBackgroundColor(color);
}

void PrefsPageOSD::slotScreenChanged(int index)
{
  m_pOSDPreview->setScreen(index);
}

void PrefsPageOSD::slotDrawShadowChanged(bool on)
{
  m_pOSDPreview->setShadow(on);
}

void PrefsPageOSD::osdFontClicked()
{
  osdFont = kcfg_OsdFont->font();
  updateFonts();
}

void PrefsPageOSD::slotPositionChanged()
{
  kcfg_OsdScreen->setCurrentItem(m_pOSDPreview->screen());
}

void PrefsPageOSD::applyPreferences()
{
  preferences->setOSDUsage(kcfg_UseOsdDisplay->isChecked());
  preferences->setOSDShowOwnNick(kcfg_OsdNickAppear->isChecked());
  preferences->setOSDShowChannel(kcfg_OsdChannelMessage->isChecked());
  preferences->setOSDShowQuery(kcfg_OsdQueryActivity->isChecked());
  preferences->setOSDShowChannelEvent(kcfg_OsdJoinPart->isChecked());
  preferences->setOSDFont(osdFont);
  preferences->setOSDUseCustomColors(kcfg_OsdCustomColor->isChecked());
  preferences->setOSDTextColor(kcfg_OsdTextColor->color().name());
  preferences->setOSDBackgroundColor(kcfg_OsdBackgroundColor->color().name());
  preferences->setOSDDuration(kcfg_OsdDuration->value());
  preferences->setOSDScreen(kcfg_OsdScreen->currentText().toUInt());
  preferences->setOSDDrawShadow(kcfg_OsdDrawShadow->isChecked());
  preferences->setOSDOffsetY(m_pOSDPreview->y());
  preferences->setOSDAlignment(m_pOSDPreview->alignment());

  KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
  konvApp->osd->setEnabled(kcfg_UseOsdDisplay->isChecked());
  if (preferences->getOSDUsage())
  {
    konvApp->osd->setFont(osdFont);
    if(kcfg_OsdCustomColor->isChecked())
    {
      konvApp->osd->setTextColor(kcfg_OsdTextColor->color());
      konvApp->osd->setBackgroundColor(kcfg_OsdBackgroundColor->color());
    }
    else
    {
      konvApp->osd->unsetColors();
    }

    konvApp->osd->setDuration(kcfg_OsdDuration->value());
    konvApp->osd->setScreen(kcfg_OsdScreen->currentText().toUInt());
    konvApp->osd->setShadow(kcfg_OsdDrawShadow->isChecked());
    konvApp->osd->setOffset(preferences->getOSDOffsetX(),m_pOSDPreview->y());
    konvApp->osd->setAlignment((OSDWidget::Alignment)m_pOSDPreview->alignment());
  }

}

void PrefsPageOSD::updateFonts()
{
  m_pOSDPreview->setFont(osdFont);
}


#include "prefspageosd.moc"
