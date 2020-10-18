/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ivor Hewitt <ivor@ivor.org>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2006 Peter Simonsson <psn@linux.se>
*/

#ifndef OSD_CONFIG_H
#define OSD_CONFIG_H

#include "ui_osd_configui.h"
#include "settingspage.h"

class OSDPreviewWidget;


class OSD_Config : public QWidget, public KonviSettingsPage, private Ui::OSD_ConfigUI
{
    Q_OBJECT

    public:
        explicit OSD_Config( QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags fl = {} );
        ~OSD_Config() override;

        void restorePageToDefaults() override;
        void saveSettings() override;
        void loadSettings() override;

        bool hasChanged() override;  // implement the interface, will not be used here, though

    protected:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private Q_SLOTS:
        void slotOSDEnabledChanged(bool on);
        void slotCustomColorsChanged(bool on);
        void slotTextColorChanged(const QColor& color);
        void slotBackgroundColorChanged(const QColor& color);
        void slotScreenChanged(int index);
        void slotDrawShadowChanged(bool on);
        void slotUpdateFont(const QFont& font);
        void slotPositionChanged();

    private:
        OSDPreviewWidget* m_pOSDPreview;

        Q_DISABLE_COPY(OSD_Config)
};

#endif // OSD_CONFIG_H
