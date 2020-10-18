/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2006 Eike Hein <hein@kde.org>
*/

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QObject>


class QLabel;

class MainWindow;
class KSqueezedTextLabel;
class SSLLabel;
class Server;

namespace Konversation
{

    class StatusBar : public QObject
    {
        Q_OBJECT

        public:
            explicit StatusBar(MainWindow* parent);
            ~StatusBar() override;

        public Q_SLOTS:
            void updateAppearance();

            void resetStatusBar();

            void setMainLabelText(const QString& text);

            void setMainLabelTempText(const QString& text);
            void clearMainLabelTempText();

            void setInfoLabelShown(bool shown);
            void updateInfoLabel(const QString& text);
            void clearInfoLabel();

            void setLagLabelShown(bool shown);
            void updateLagLabel(Server* lagServer, int msec);
            void resetLagLabel(Server* lagServer = nullptr);
            void setTooLongLag(Server* lagServer, int msec);

            void updateSSLLabel(Server* server);
            void removeSSLLabel();

        private:
            MainWindow* m_window;

            KSqueezedTextLabel* m_mainLabel;
            QLabel* m_infoLabel;
            QLabel* m_lagLabel;
            SSLLabel* m_sslLabel;

            QString m_oldMainLabelText;
            QString m_tempMainLabelText;

            Q_DISABLE_COPY(StatusBar)
    };

}

#endif
