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
#ifndef KONVERSATIONCHANNELDIALOG_H
#define KONVERSATIONCHANNELDIALOG_H

#include <kdialog.h>


class KLineEdit;

namespace Konversation
{

    class ChannelSettings;

    class ChannelDialog : public KDialog
    {
        Q_OBJECT

        public:
            explicit ChannelDialog(const QString& title, QWidget *parent = 0);
            ~ChannelDialog();

            void setChannelSettings(const ChannelSettings& channel);
            ChannelSettings channelSettings();

        protected slots:
            void slotOk();
        void slotServerNameChanged( const QString& );
        private:
            KLineEdit* m_channelEdit;
            KLineEdit* m_passwordEdit;

    };

}
#endif
