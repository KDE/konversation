/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

    SPDX-FileCopyrightText: 2011 Eike Hein <hein@kde.org>
*/

#ifndef OUTPUTFILTERRESOLVEJOB_H
#define OUTPUTFILTERRESOLVEJOB_H

#include <QObject>
#include <QHostInfo>

#include "outputfilter.h"


class OutputFilterResolveJob : public QObject
{
    Q_OBJECT

    public:
        explicit OutputFilterResolveJob(const Konversation::OutputFilterInput& input);
        ~OutputFilterResolveJob() override;

    private Q_SLOTS:
        void resolved(const QHostInfo &hostInfo);

    private:
        QString m_target;
        bool m_reverse;
        QPointer<ChatWindow> m_chatWindow;

        Q_DISABLE_COPY(OutputFilterResolveJob)
};

#endif
