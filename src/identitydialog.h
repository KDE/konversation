/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004, 2009 by Peter Simonsson
  email:     peter.simonsson@gmail.com
  Copyright (C) 2012 Eike Hein <hein@kde.org>
*/
#ifndef KONVERSATIONIDENTITYDIALOG_H
#define KONVERSATIONIDENTITYDIALOG_H

#include "identity.h"
#include "ui_identitydialog.h"

#include <QDialog>


class KEditListWidget;
class KMessageWidget;

namespace Konversation
{

    class IdentityDialog : public QDialog, private Ui::IdentityDialog
    {
        Q_OBJECT
        public:
            explicit IdentityDialog(QWidget *parent = 0);
            ~IdentityDialog() {}
            void setCurrentIdentity(int index);
            IdentityPtr setCurrentIdentity(IdentityPtr identity);
            IdentityPtr currentIdentity() const;

        public Q_SLOTS:
            virtual void accept();

        Q_SIGNALS:
            void identitiesChanged();

        protected:
            bool checkCurrentIdentity();

        protected Q_SLOTS:
            void updateIdentity(int index);
            void refreshCurrentIdentity();

            void newIdentity();
            void renameIdentity();
            void deleteIdentity();
            void copyIdentity();

            void authTypeChanged(int index);

        private:
            IdentityList m_identityList;
            IdentityPtr m_currentIdentity;

            KEditListWidget* m_nicknameLBox;

            KMessageWidget* m_additionalAuthInfo;
    };

}
#endif
