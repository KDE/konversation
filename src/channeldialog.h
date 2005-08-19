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

#include <kdialogbase.h>

class QLineEdit;

namespace Konversation
{

    class ChannelSettings;

    class ChannelDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            ChannelDialog(const QString& title, QWidget *parent = 0, const char *name = 0);
            ~ChannelDialog();

            void setChannelSettings(const ChannelSettings& channel);
            ChannelSettings channelSettings();

        private:
            QLineEdit* m_channelEdit;
            QLineEdit* m_passwordEdit;

    };

}
#endif
