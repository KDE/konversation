
#include "ex_osd_preferences.h"
#include "preferences.h"
#include "osd.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <kcombobox.h>
#include <kcolorcombo.h>
#include <qcheckbox.h>
#include <kfontrequester.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>


OSD_Config_Ext::OSD_Config_Ext( QWidget* parent, const char* name, WFlags fl )
    : OSD_Config( parent, name, fl )
{
    m_pOSDPreview = new OSDPreviewWidget("Konversation");
    connect(m_pOSDPreview, SIGNAL(positionChanged()), this, SLOT(slotPositionChanged()));

    slotOSDEnabledChanged(kcfg_UseOSD->isChecked());
    slotCustomColorsChanged(kcfg_OSDUseCustomColors->isChecked());
    slotScreenChanged(kcfg_OSDScreen->currentItem());
    slotDrawShadowChanged( kcfg_OSDDrawShadow->isChecked());
    updateFonts();

    kcfg_OSDOffsetX->hide();
    kcfg_OSDOffsetY->hide();
    kcfg_OSDAlignment->hide();

    //Connect config page entries to control the OSDPreview
    connect ( kcfg_UseOSD,  SIGNAL( toggled( bool ) ), this, SLOT( slotOSDEnabledChanged(bool) ) );
    connect ( kcfg_OSDUseCustomColors, SIGNAL(toggled(bool)), this, SLOT(slotCustomColorsChanged(bool)));
    connect ( kcfg_OSDTextColor, SIGNAL(activated(const QColor&)), this, SLOT(slotTextColorChanged(const QColor&)));
    connect ( kcfg_OSDBackgroundColor, SIGNAL(activated(const QColor&)), this, SLOT(slotBackgroundColorChanged(const QColor&)));
    connect ( kcfg_OSDScreen, SIGNAL(activated(int)), this, SLOT(slotScreenChanged(int)));
    connect ( kcfg_OSDDrawShadow, SIGNAL(toggled(bool)), this, SLOT(slotDrawShadowChanged(bool)));
}

OSD_Config_Ext::~OSD_Config_Ext()
{
    delete m_pOSDPreview;
}

void OSD_Config_Ext::showEvent(QShowEvent*)
{
    //Update the preview
    m_pOSDPreview->setAlignment((OSDWidget::Alignment)( kcfg_OSDAlignment->currentItem()+1 ) );
    m_pOSDPreview->setOffset(kcfg_OSDOffsetX->value(),kcfg_OSDOffsetY->value());

    m_pOSDPreview->setShown(true);
}

void OSD_Config_Ext::hideEvent(QHideEvent*)
{
    m_pOSDPreview->setShown(false);
}


void OSD_Config_Ext::slotOSDEnabledChanged(bool on)
{
    if ( isVisible() )
        m_pOSDPreview->setShown(on);
}

void OSD_Config_Ext::slotPositionChanged()
{
    kcfg_OSDScreen->setCurrentItem(m_pOSDPreview->screen());

    kcfg_OSDAlignment->setCurrentItem( m_pOSDPreview->alignment()-1 );
    kcfg_OSDOffsetX->setValue( m_pOSDPreview->x());
    kcfg_OSDOffsetY->setValue( m_pOSDPreview->y());
}


void OSD_Config_Ext::slotCustomColorsChanged(bool on)
{
    if(on)
    {
        m_pOSDPreview->setTextColor(kcfg_OSDTextColor->color());
        m_pOSDPreview->setBackgroundColor(kcfg_OSDBackgroundColor->color());
    }
    else
        m_pOSDPreview->unsetColors();
}
void OSD_Config_Ext::slotTextColorChanged(const QColor& color)
{
    if(kcfg_OSDUseCustomColors->isChecked())
        m_pOSDPreview->setTextColor(color);
}

void OSD_Config_Ext::slotBackgroundColorChanged(const QColor& color)
{
    if(kcfg_OSDUseCustomColors->isChecked())
        m_pOSDPreview->setBackgroundColor(color);
}

void OSD_Config_Ext::slotScreenChanged(int index)
{
    m_pOSDPreview->setScreen(index);
}

void OSD_Config_Ext::slotDrawShadowChanged(bool on)
{
    m_pOSDPreview->setShadow(on);
}

void OSD_Config_Ext::osdFontClicked()
{
    osdFont = kcfg_OSDFont->font();
    updateFonts();
}

void OSD_Config_Ext::updateFonts()
{
    m_pOSDPreview->setFont(osdFont);
}
