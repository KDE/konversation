#ifndef EXOSDPREFERENCES_H
#define EXOSDPREFERENCES_H

#include "osd_preferencesui.h"
#include "konvisettingspage.h"

class OSDPreviewWidget;

class OSD_Config : public OSD_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

public:
    OSD_Config( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~OSD_Config();

    virtual void restorePageToDefaults();
    virtual void saveSettings();
    virtual void loadSettings();

    virtual bool hasChanged() { return false; }; // FIXME

protected slots:
    void slotOSDEnabledChanged(bool on);
    void slotCustomColorsChanged(bool on);
    void slotTextColorChanged(const QColor& color);
    void slotBackgroundColorChanged(const QColor& color);
    void slotScreenChanged(int index);
    void slotDrawShadowChanged(bool on);
    void slotUpdateFont(const QFont& font);
    void slotPositionChanged();

protected:
    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent* event);

private:
    OSDPreviewWidget* m_pOSDPreview;
};

#endif // EXOSDPREFERENCES_H
