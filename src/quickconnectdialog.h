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

#include <kdialog.h>


class QCheckBox;
class KLineEdit;

class QuickConnectDialog : public KDialog
{
    Q_OBJECT

    public:
        explicit QuickConnectDialog(QWidget* parent=0);
        ~QuickConnectDialog();

    signals:
        void connectClicked(Konversation::ConnectionFlag flag,
                            const QString& hostName,
                            const QString& port,
                            const QString& password,
                            const QString& nick,
                            const QString& channel,
                            bool useSSL
            );

    protected slots:
        void slotOk();
    void slotServerNameChanged( const QString& );
    protected:
        KLineEdit*      hostNameInput;
        KLineEdit*      portInput;
        KLineEdit*  passwordInput;
        KLineEdit*      nickInput;
        QCheckBox*      sslCheckBox;
};
#endif
