/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ivor Hewitt <ivor@ivor.org>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2006 Peter Simonsson <psn@linux.se>
*/

#ifndef EXOSDPREFERENCES_H
#define EXOSDPREFERENCES_H

#include "osd_preferencesui.h"
#include "settingspage.h" ////// header renamed
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>


class OSDPreviewWidget;

class OSD_Config : public OSD_ConfigUI, public KonviSettingsPage
{
    Q_OBJECT

    public:
        explicit OSD_Config( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
        ~OSD_Config();

        virtual void restorePageToDefaults();
        virtual void saveSettings();
        virtual void loadSettings();

        virtual bool hasChanged();  // implement the interface, will not be used here, though

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
