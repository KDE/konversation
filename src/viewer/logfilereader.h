/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef LOGFILEREADER_H
#define LOGFILEREADER_H

#include "chatwindow.h"

#include <KIO/Job>

class QSpinBox;

class KToolBar;

/**
 * Shows the content of a log file
 */
class LogfileReader : public ChatWindow
{
    Q_OBJECT

        public:
        LogfileReader(QWidget* parent, const QString& log, const QString& caption);
        ~LogfileReader() override;

        using ChatWindow::closeYourself;
        virtual bool closeYourself() { closeLog(); return true; }
        bool searchView() const override;

        bool eventFilter(QObject* watched, QEvent* e) override;

    protected:
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;

    private Q_SLOTS:
        void updateView();
        void storeBufferSize(int kb);
        void clearLog();
        void saveLog();
        void closeLog();
        void copyResult(KJob* job);

    private:
        KToolBar* toolBar;
        QSpinBox* sizeSpin;
        QString fileName;

        Q_DISABLE_COPY(LogfileReader)
};

#endif
