/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@ivor.org>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2006 Peter Simonsson <psn@linux.se>
*/

#include "osd_config.h"
#include "preferences.h"
#include "osd.h"
#include "application.h"

#include <KColorButton>
#include <QCheckBox>
#include <QScreen>
#include <KFontRequester>


OSD_Config::OSD_Config( QWidget* parent, const char* name, Qt::WindowFlags fl )
    : QWidget(parent, fl)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    bool enableScreenChooser = false;
    QRect screenRect;

    for(int i = 0; i < QApplication::screens().count(); ++i) {
        kcfg_OSDScreen->addItem(QString::number(i));
        screenRect = QApplication::screens()[i]->geometry();

        //Check if we're using xinerama or not
        if(screenRect.left() != 0 || screenRect.top() != 0) {
            enableScreenChooser = true;
        }
    }

    kcfg_OSDScreen->setEnabled(enableScreenChooser);

    m_pOSDPreview = new OSDPreviewWidget(QStringLiteral("Konversation"));
    connect(m_pOSDPreview, &OSDPreviewWidget::positionChanged, this, &OSD_Config::slotPositionChanged);

    connect(kcfg_OSDFont, &KFontRequester::fontSelected, this, &OSD_Config::slotUpdateFont);

    slotOSDEnabledChanged(kcfg_UseOSD->isChecked());
    slotCustomColorsChanged(kcfg_OSDUseCustomColors->isChecked());
    slotScreenChanged(Preferences::self()->oSDScreen());
    slotDrawShadowChanged( kcfg_OSDDrawShadow->isChecked());
    slotUpdateFont(Preferences::self()->oSDFont());

    kcfg_OSDOffsetX->hide();
    kcfg_OSDOffsetY->hide();
    kcfg_OSDAlignment->hide();

    //Connect config page entries to control the OSDPreview
    connect(kcfg_UseOSD, &QCheckBox::toggled, this, &OSD_Config::slotOSDEnabledChanged);
    connect(kcfg_OSDUseCustomColors, &QGroupBox::toggled, this, &OSD_Config::slotCustomColorsChanged);
    connect(kcfg_OSDTextColor, &KColorButton::changed, this, &OSD_Config::slotTextColorChanged);
    connect(kcfg_OSDBackgroundColor, &KColorButton::changed, this, &OSD_Config::slotBackgroundColorChanged);
    connect(kcfg_OSDScreen, QOverload<int>::of(&QComboBox::activated), this, &OSD_Config::slotScreenChanged);
    connect(kcfg_OSDDrawShadow, &QCheckBox::toggled, this, &OSD_Config::slotDrawShadowChanged);
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
    OSDWidget *osd = Application::instance()->osd();

    osd->setEnabled(kcfg_UseOSD->isChecked());
    if (kcfg_UseOSD->isChecked())
    {
        osd->setFont(Preferences::self()->oSDFont());
        if(kcfg_OSDUseCustomColors->isChecked())
        {
            osd->setTextColor(kcfg_OSDTextColor->color());
            QPalette p = osd->palette();
            p.setColor(osd->backgroundRole(), kcfg_OSDBackgroundColor->color());
            osd->setPalette(p);
        }
        else
        {
            osd->unsetColors();
        }

        osd->setDuration(kcfg_OSDDuration->value());
        osd->setScreen(kcfg_OSDScreen->currentIndex());
        osd->setShadow(kcfg_OSDDrawShadow->isChecked());

        //x is ignored anyway, but leave incase we use in future
        osd->setOffset(kcfg_OSDOffsetX->value(), kcfg_OSDOffsetY->value());
        osd->setAlignment((OSDWidget::Alignment)kcfg_OSDAlignment->value());
    }

}

void OSD_Config::showEvent(QShowEvent*)
{
    //Update the preview
    m_pOSDPreview->setAlignment((OSDWidget::Alignment)( kcfg_OSDAlignment->value() ) );
    m_pOSDPreview->setOffset(kcfg_OSDOffsetX->value(),kcfg_OSDOffsetY->value());

    if (kcfg_UseOSD->isChecked()) m_pOSDPreview->show();
}

void OSD_Config::hideEvent(QHideEvent*)
{
    m_pOSDPreview->setVisible(false);
}

bool OSD_Config::hasChanged()
{
  // follow the interface, no Non-KConfigXT settings here, so none have changed
  return false;
}

void OSD_Config::slotOSDEnabledChanged(bool on)
{
    if ( isVisible() )
        on ? m_pOSDPreview->show() : m_pOSDPreview->setVisible(false);
}

void OSD_Config::slotPositionChanged()
{
    kcfg_OSDScreen->setCurrentIndex(m_pOSDPreview->screen());

    kcfg_OSDAlignment->setValue( m_pOSDPreview->alignment() );
    kcfg_OSDOffsetX->setValue( m_pOSDPreview->x());
    kcfg_OSDOffsetY->setValue( m_pOSDPreview->y());
}


void OSD_Config::slotCustomColorsChanged(bool on)
{
    if(on)
    {
        m_pOSDPreview->setTextColor(kcfg_OSDTextColor->color());
        QPalette p = m_pOSDPreview->palette();
        p.setColor(m_pOSDPreview->backgroundRole(), kcfg_OSDBackgroundColor->color());
        m_pOSDPreview->setPalette(p);
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
    if(kcfg_OSDUseCustomColors->isChecked()) {
        QPalette p = m_pOSDPreview->palette();
        p.setColor(m_pOSDPreview->backgroundRole(), color);
        m_pOSDPreview->setPalette(p);
    }
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

#include "moc_osd_config.cpp"
