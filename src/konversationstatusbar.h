/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONSTATUSBAR_H
#define KONVERSATIONSTATUSBAR_H

#include <qobject.h>


class QLabel;

class KonversationMainWindow;
class KSqueezedTextLabel;
class SSLLabel;
class Server;

class KonversationStatusBar : public QObject
{
    Q_OBJECT

    public:
        explicit KonversationStatusBar(KonversationMainWindow* parent);
        ~KonversationStatusBar();

    public slots:
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
        void resetLagLabel();
        void setTooLongLag(Server* lagServer, int msec);

        void updateSSLLabel(Server* server);
        void removeSSLLabel();

    private:
        KonversationMainWindow* m_window;

        KSqueezedTextLabel* m_mainLabel;
        QLabel* m_infoLabel;
        QLabel* m_lagLabel;
        SSLLabel* m_sslLabel;

        QString m_oldMainLabelText;
        QString m_tempMainLabelText;
};

#endif
