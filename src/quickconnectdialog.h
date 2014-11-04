/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Dialog for quick connection to an IRC network without adding a server in the Server List.
  begin:     Sat June 05 2004
  copyright: (C) 2004 by Michael Goettsche
  email:     mail@tuxipuxi.de
*/

#ifndef QUICKCONNECTDIALOG_H
#define QUICKCONNECTDIALOG_H

#include "common.h"

#include <QDialog>


class QCheckBox;
class KLineEdit;
class QPushButton;

class QuickConnectDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit QuickConnectDialog(QWidget* parent=0);
        ~QuickConnectDialog();

    Q_SIGNALS:
        void connectClicked(Konversation::ConnectionFlag flag,
                            const QString& hostName,
                            const QString& port,
                            const QString& password,
                            const QString& nick,
                            const QString& channel,
                            bool useSSL
            );

    protected Q_SLOTS:
        void slotOk();
    void slotServerNameChanged( const QString& );

    private:
        void delayedDestruct();
    protected:
        KLineEdit*      hostNameInput;
        KLineEdit*      portInput;
        KLineEdit*  passwordInput;
        KLineEdit*      nickInput;
        QCheckBox*      sslCheckBox;
        QPushButton*    mOkButton;
};
#endif
