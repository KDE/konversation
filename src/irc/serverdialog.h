/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONSERVERDIALOG_H
#define KONVERSATIONSERVERDIALOG_H

#include <kdialog.h>

class KLineEdit;

class QSpinBox;
class QCheckBox;

namespace Konversation
{

    class ServerSettings;

    class ServerDialog : public KDialog
    {
        Q_OBJECT

        public:
            explicit ServerDialog(const QString& title, QWidget *parent = 0);
            ~ServerDialog();

            void setServerSettings(const ServerSettings& server);
            ServerSettings serverSettings();

        protected slots:
            void slotOk();
        void slotServerNameChanged( const QString& );
        private:
            KLineEdit* m_serverEdit;
            QSpinBox* m_portSBox;
            KLineEdit* m_passwordEdit;
            QCheckBox* m_sslChBox;
    };

}
#endif
