/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004, 2009 by Peter Simonsson
  email:     peter.simonsson@gmail.com
*/
#ifndef KONVERSATIONIDENTITYDIALOG_H
#define KONVERSATIONIDENTITYDIALOG_H

#include "identity.h"
#include "ui_identitydialog.h"

#include <KDialog>
#include <kdeversion.h>

#if KDE_IS_VERSION(4, 6, 0)
class KEditListWidget;
#else
class KEditListBox;
#endif

namespace Konversation
{

    class IdentityDialog : public KDialog, private Ui::IdentityDialog
    {
        Q_OBJECT
        public:
            IdentityDialog(QWidget *parent = 0);
            ~IdentityDialog() {}
            void setCurrentIdentity(int index);
            IdentityPtr setCurrentIdentity(IdentityPtr identity);
            IdentityPtr currentIdentity() const;

        public slots:
            virtual void accept();

        signals:
            void identitiesChanged();

        protected:
            bool checkCurrentIdentity();

        protected slots:
            void updateIdentity(int index);

            void refreshCurrentIdentity();

            void newIdentity();
            void renameIdentity();
            void deleteIdentity();
            void copyIdentity();
        private:
            IdentityList m_identityList;
            IdentityPtr m_currentIdentity;
#if KDE_IS_VERSION(4, 6, 0)
            KEditListWidget* m_nicknameLBox;
#else
            KEditListBox* m_nicknameLBox;
#endif
    };

}
#endif
