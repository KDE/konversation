#ifndef PREFSPAGEOSD_H
#define PREFSPAGEOSD_H

#include "osd_preferences.h"

class OSDPreviewWidget;

class OSD_Config_Ext : public OSD_Config
{
    Q_OBJECT

public:
    OSD_Config_Ext( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~OSD_Config_Ext();

protected slots:
    void slotOSDEnabledChanged(bool on);
    void slotCustomColorsChanged(bool on);
    void slotTextColorChanged(const QColor& color);
    void slotBackgroundColorChanged(const QColor& color);
    void slotScreenChanged(int index);
    void slotDrawShadowChanged(bool on);
    void slotUpdateFont(const QFont& font);
    void slotPositionChanged();
    void slotApply();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private:
    OSDPreviewWidget* m_pOSDPreview;
};

#endif // PREFSPAGEOSD_H
