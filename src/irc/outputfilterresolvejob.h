/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor appro-
  ved by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see http://www.gnu.org/licenses/.
*/

/*
  Copyright (C) 2011 Eike Hein <hein@kde.org>
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
        OutputFilterResolveJob(const Konversation::OutputFilterInput& input);
        ~OutputFilterResolveJob();


    private slots:
        void resolved(QHostInfo hostInfo);


    private:
        QString m_target;
        bool m_reverse;
        QPointer<ChatWindow> m_chatWindow;
};

#endif
