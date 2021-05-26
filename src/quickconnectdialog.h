/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004 Michael Goettsche <mail@tuxipuxi.de>
*/

#ifndef QUICKCONNECTDIALOG_H
#define QUICKCONNECTDIALOG_H

#include "common.h"

#include <QDialog>

class KPasswordLineEdit;
class QCheckBox;
class KLineEdit;
class QPushButton;

/**
 * Dialog for quick connection to an IRC network without adding a server in the Server List.
 */
class QuickConnectDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit QuickConnectDialog(QWidget* parent = nullptr);
        ~QuickConnectDialog() override;

    Q_SIGNALS:
        void connectClicked(Konversation::ConnectionFlag flag,
                            const QString& hostName,
                            const QString& port,
                            const QString& password,
                            const QString& nick,
                            const QString& channel,
                            bool useSSL
            );

    private Q_SLOTS:
        void slotOk();
        void slotServerNameChanged(const QString&);

    private:
        void delayedDestruct();

    private:
        KLineEdit*      hostNameInput;
        KLineEdit*      portInput;
        KPasswordLineEdit* passwordInput;
        KLineEdit*      nickInput;
        QCheckBox*      sslCheckBox;
        QPushButton*    mOkButton;

        Q_DISABLE_COPY(QuickConnectDialog)
};
#endif
