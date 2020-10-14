/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Peter Simonsson <psn@linux.se>
*/

#ifndef IRCCOLORCHOOSER_H
#define IRCCOLORCHOOSER_H

#include <QDialog>

#include "ui_irccolorchooserui.h"

class IRCColorChooserUI;
class KComboBox;

/**
 * Dialog used to add irc colors to your messages
 */
class IRCColorChooser : public QDialog
{
    Q_OBJECT
    public:
        explicit IRCColorChooser(QWidget* parent);
        QString color() const;

    private Q_SLOTS:
        void updatePreview();

    private:
        void initColors(KComboBox* combo);

    private:
        Ui::IRCColorChooserUI m_ui;
};
#endif
