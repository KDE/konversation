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

#include <kdialogbase.h>

/*
    @author Michael Goettsche
*/

class KLineEdit;

class QuickConnectDialog : public KDialogBase
{
    Q_OBJECT

    public:
        explicit QuickConnectDialog(QWidget* parent=0);
        ~QuickConnectDialog();

    signals:
        void connectClicked(const QString& hostName,
            const QString& port,
            const QString& channel,
            const QString& nick,
            const QString& password,
            const bool& useSSL
            );

    protected slots:
        void slotOk();

    protected:
        KLineEdit*  hostNameInput;
        KLineEdit*  portInput;
        KLineEdit*  passwordInput;
        KLineEdit*  nickInput;
        QCheckBox*      sslCheckBox;
};
#endif
