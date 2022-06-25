/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2002 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2005-2008 Eike Hein <hein@kde.org>
*/

#ifndef QUERY_H
#define QUERY_H

#include "chatwindow.h"
#include "nickinfo.h"

#include <config-konversation.h>

#if HAVE_QCA2
#include "cipher.h"
#endif

#include <QString>

/* TODO: Idle counter to close query after XXX minutes of inactivity */
/* TODO: Use /USERHOST to check if queries are still valid */

class AwayLabel;
class IRCInput;

class QLabel;
class QSplitter;

class KSqueezedTextLabel;


class Query : public ChatWindow
{
    Q_OBJECT
    friend class Server;

    public:
        explicit Query(QWidget* parent, const QString& _name);
        void setServer(Server* newServer) override;

        ~Query() override;

        /** This will always be called soon after this object is created.
         *  @param nickInfo A nickinfo that must exist.
         */
        void setNickInfo(const NickInfoPtr & nickInfo);
        /** It seems that this does _not_ guaranttee to return non null.
         *  The problem is when you open a query to someone, then the go offline.
         *  This should be fixed maybe?  I don't know.
         */
        NickInfoPtr getNickInfo() const;
        bool closeYourself(bool askForConfirmation=true) override;
        bool canBeFrontView() const override;
        bool searchView() const override;

        void setChannelEncoding(const QString& encoding) override;
        QString getChannelEncoding() const override;
        QString getChannelEncodingDefaultDesc() const override;
        void emitUpdateInfo() override;

        /** call this when you see a nick quit from the server.
         *  @param reason The quit reason given by that user.
         */
        void quitNick(const QString& reason, const QHash<QString, QString> &messageTags);

        #if HAVE_QCA2
        Konversation::Cipher* getCipher() const;
        #endif

    Q_SIGNALS:
        void sendFile(const QString& recipient);
        void updateQueryChrome(ChatWindow*, const QString&);

    public Q_SLOTS:
        void sendText(const QString& text) override;
        void indicateAway(bool show) override;
        void setEncryptedOutput(bool);
        void connectionStateChanged(Server*, Konversation::ConnectionState);

    protected:
        void setName(const QString& newName) override;
        void showEvent(QShowEvent* event) override;
        /** Called from ChatWindow adjustFocus */
        void childAdjustFocus() override;

    private Q_SLOTS:
        void queryTextEntered();
        void queryPassthroughCommand();
        void sendFileMenu();
        void urlsDropped(const QList<QUrl>& urls);
        // connected to IRCInput::textPasted() - used to handle large/multiline pastes
        void textPasted(const QString& text);
        void nickInfoChanged();
        void updateNickInfo(Server* server, const NickInfoPtr &nickInfo);

    private:
        bool awayChanged;
        bool awayState;

        QString queryName;
        QString buffer;

        QSplitter* m_headerSplitter;
        KSqueezedTextLabel* queryHostmask;
        QLabel* blowfishLabel;
        AwayLabel* awayLabel;
        NickInfoPtr m_nickInfo;

        bool m_initialShow;

        #if HAVE_QCA2
        //FIXME: We might want to put this into the attendee object (i.e. NickInfo).
        mutable Konversation::Cipher *m_cipher;
        #endif

        Q_DISABLE_COPY(Query)
};

#endif
