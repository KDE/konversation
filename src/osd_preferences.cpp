
#include "osd_preferences.h"
#include "config/preferences.h"
#include "osd.h"
#include "konversationapplication.h"

#include <qgroupbox.h>
#include <qspinbox.h>
#include <kcombobox.h>
#include <kcolorbutton.h>
#include <qcheckbox.h>
#include <kfontrequester.h>

#include <kconfigdialog.h>

OSD_Config::OSD_Config( QWidget* parent, const char* name, WFlags fl )
    : OSD_ConfigUI( parent, name, fl )
{
    for(int i = 0; i < QApplication::desktop()->numScreens(); ++i) {
        kcfg_OSDScreen->insertItem(QString::number(i));
    }

    m_pOSDPreview = new OSDPreviewWidget("Konversation");
    connect(m_pOSDPreview, SIGNAL(positionChanged()), this, SLOT(slotPositionChanged()));

    connect( kcfg_OSDFont, SIGNAL(fontSelected(const QFont&)), this, SLOT(slotUpdateFont(const QFont&)));

    slotOSDEnabledChanged(kcfg_UseOSD->isChecked());
    slotCustomColorsChanged(kcfg_OSDUseCustomColors->isChecked());
    slotScreenChanged(Preferences::oSDScreen());
    slotDrawShadowChanged( kcfg_OSDDrawShadow->isChecked());
    slotUpdateFont(Preferences::oSDFont());

    kcfg_OSDOffsetX->hide();
    kcfg_OSDOffsetY->hide();
    kcfg_OSDAlignment->hide();

    //Connect config page entries to control the OSDPreview
    connect ( kcfg_UseOSD,  SIGNAL( toggled( bool ) ), this, SLOT( slotOSDEnabledChanged(bool) ) );
    connect ( kcfg_OSDUseCustomColors, SIGNAL(toggled(bool)), this, SLOT(slotCustomColorsChanged(bool)));
    connect ( kcfg_OSDTextColor, SIGNAL(changed(const QColor&)), this, SLOT(slotTextColorChanged(const QColor&)));
    connect ( kcfg_OSDBackgroundColor, SIGNAL(changed(const QColor&)), this, SLOT(slotBackgroundColorChanged(const QColor&)));
    connect ( kcfg_OSDScreen, SIGNAL(activated(int)), this, SLOT(slotScreenChanged(int)));
    connect ( kcfg_OSDDrawShadow, SIGNAL(toggled(bool)), this, SLOT(slotDrawShadowChanged(bool)));
}

OSD_Config::~OSD_Config()
{
    delete m_pOSDPreview;
}

void OSD_Config::loadSettings()
{
}

void OSD_Config::restorePageToDefaults()
{
}

void OSD_Config::saveSettings()
{
    //Update the current OSD.
    KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());

    konvApp->osd->setEnabled(kcfg_UseOSD->isChecked());
    if (kcfg_UseOSD->isChecked())
    {
        konvApp->osd->setFont(Preferences::oSDFont());
	if(kcfg_OSDUseCustomColors->isChecked())
        {
            konvApp->osd->setTextColor(kcfg_OSDTextColor->color());
            konvApp->osd->setBackgroundColor(kcfg_OSDBackgroundColor->color());
        }
    else
        {
            konvApp->osd->unsetColors();
        }

        konvApp->osd->setDuration(kcfg_OSDDuration->value());
        konvApp->osd->setScreen(kcfg_OSDScreen->currentItem());
        konvApp->osd->setShadow(kcfg_OSDDrawShadow->isChecked());

        //x is ignored anyway, but leave incase we use in future
        konvApp->osd->setOffset(m_pOSDPreview->x(),m_pOSDPreview->y());
        konvApp->osd->setAlignment((OSDWidget::Alignment)m_pOSDPreview->alignment());
    }

}

void OSD_Config::showEvent(QShowEvent*)
{
    //Update the preview
    m_pOSDPreview->setAlignment((OSDWidget::Alignment)( kcfg_OSDAlignment->value() ) );
    m_pOSDPreview->setOffset(kcfg_OSDOffsetX->value(),kcfg_OSDOffsetY->value());

    m_pOSDPreview->setShown(kcfg_UseOSD->isChecked());
}

void OSD_Config::hideEvent(QHideEvent*)
{
    m_pOSDPreview->setShown(false);
}

bool OSD_Config::hasChanged()
{
  // follow the interface, no Non-KConfigXT settings here, so none have changed
  return false;
}

void OSD_Config::slotOSDEnabledChanged(bool on)
{
    if ( isVisible() )
        m_pOSDPreview->setShown(on);
}

void OSD_Config::slotPositionChanged()
{
    kcfg_OSDScreen->setCurrentItem(m_pOSDPreview->screen());

    kcfg_OSDAlignment->setValue( m_pOSDPreview->alignment() );
    kcfg_OSDOffsetX->setValue( m_pOSDPreview->x());
    kcfg_OSDOffsetY->setValue( m_pOSDPreview->y());
}


void OSD_Config::slotCustomColorsChanged(bool on)
{
    if(on)
    {
        m_pOSDPreview->setTextColor(kcfg_OSDTextColor->color());
        m_pOSDPreview->setBackgroundColor(kcfg_OSDBackgroundColor->color());
    }
    else
        m_pOSDPreview->unsetColors();
}
void OSD_Config::slotTextColorChanged(const QColor& color)
{
    if(kcfg_OSDUseCustomColors->isChecked())
        m_pOSDPreview->setTextColor(color);
}

void OSD_Config::slotBackgroundColorChanged(const QColor& color)
{
    if(kcfg_OSDUseCustomColors->isChecked())
        m_pOSDPreview->setBackgroundColor(color);
}

void OSD_Config::slotScreenChanged(int index)
{
    m_pOSDPreview->setScreen(index);
}

void OSD_Config::slotDrawShadowChanged(bool on)
{
    m_pOSDPreview->setShadow(on);
}

void OSD_Config::slotUpdateFont(const QFont& font)
{
    m_pOSDPreview->setFont(font);
}

#include "osd_preferences.moc"

