/*
  This program is free software; you can redistribute it and/or modifydvancedModes
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONCHANNELOPTIONSDIALOG_H
#define KONVERSATIONCHANNELOPTIONSDIALOG_H

#include <kdialogbase.h>

#include <qstringlist.h>
#include "channel.h"

namespace Konversation
{

    class ChannelOptionsUI;

    class ChannelOptionsDialog : public KDialogBase
    {
        Q_OBJECT
            public:
            ChannelOptionsDialog(Channel *channel);
            ~ChannelOptionsDialog();

            QString topic();
            QStringList modes();

        public slots:
            void refreshTopicHistory();
            void refreshAllowedChannelModes();
            void refreshModes();
            void refreshEnableModes();
            void toggleAdvancedModes();

            void closeOptionsDialog();
            void changeOptions();

        protected slots:
            void topicHistoryItemClicked(QListViewItem* item);
            void topicBeingEdited(bool state);

        protected:
            bool m_editingTopic;

        private:
            ChannelOptionsUI* m_widget;
            Channel *m_channel;
    };

}
#endif
