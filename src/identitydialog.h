/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
    SPDX-FileCopyrightText: 2012 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONIDENTITYDIALOG_H
#define KONVERSATIONIDENTITYDIALOG_H

#include "identity.h"
#include "ui_identitydialog.h"

#include <QDialog>


class KMessageWidget;

namespace Konversation
{

    class IdentityDialog : public QDialog, private Ui::IdentityDialog
    {
        Q_OBJECT
        public:
            explicit IdentityDialog(QWidget *parent = nullptr);
            ~IdentityDialog() override {}
            void setCurrentIdentity(int index);
            IdentityPtr setCurrentIdentity(const IdentityPtr &identity);
            IdentityPtr currentIdentity() const;

        public Q_SLOTS:
            void accept() override;

        Q_SIGNALS:
            void identitiesChanged();

        private:
            bool checkCurrentIdentity();

        private Q_SLOTS:
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

            KMessageWidget* m_additionalAuthInfo;

            Q_DISABLE_COPY(IdentityDialog)
    };
}

#endif
