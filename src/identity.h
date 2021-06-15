/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef IDENTITY_H
#define IDENTITY_H

#include <QExplicitlySharedDataPointer>
#include <QUrl>

#include <QStringList>


class QTextCodec;

/**
 * This class holds the various user identities
 */
class Identity : public QSharedData
{
    public:
        /// Create an Identity with a new id.
        Identity();
        /// Create a new Identity with a set id.
        explicit Identity(int id);
        /// Copy all of @param original including the id.
        Identity(const Identity& original);
        ~Identity();

        Identity& operator=(const Identity& original) = delete;

        /// This function copies all of @param original but the id
        void copy(const Identity& original);

        void setName(const QString& name);        // the name of this identity
        QString getName() const;

        void setRealName(const QString& name);
        QString getRealName() const;
        void setIdent(const QString& ident);
        QString getIdent() const;

        void setNickname(uint index,const QString& nickname);
        QString getNickname(int index) const;

        void setNicknameList(const QStringList& newList);
        QStringList getNicknameList() const;

        void setAuthType(const QString& authType);
        QString getAuthType() const;
        void setAuthPassword(const QString& authPassword);
        QString getAuthPassword() const;
        void setNickservNickname(const QString& nickservNickname);
        QString getNickservNickname() const;
        void setNickservCommand(const QString& nickservCommand);
        QString getNickservCommand() const;
        void setSaslAccount(const QString& saslAccount);
        QString getSaslAccount() const;
        void setPemClientCertFile(const QUrl &url);
        QUrl getPemClientCertFile() const;

        void setQuitReason(const QString& reason);
        QString getQuitReason() const;
        void setPartReason(const QString& reason);
        QString getPartReason() const;
        void setKickReason(const QString& reason);
        QString getKickReason() const;

        void setInsertRememberLineOnAway(bool state);
        bool getInsertRememberLineOnAway() const;

        void setRunAwayCommands(bool run);
        bool getRunAwayCommands() const;
        void setAwayCommand(const QString& command);
        QString getAwayCommand() const;
        void setReturnCommand(const QString& command);
        QString getReturnCommand() const;

        void setAutomaticAway(bool automaticAway);
        bool getAutomaticAway() const;
        void setAwayInactivity(int awayInactivity);
        int getAwayInactivity() const;
        void setAutomaticUnaway(bool automaticUnaway);
        bool getAutomaticUnaway() const;

        void setShellCommand(const QString &command);
        QString getShellCommand() const;

        void setCodecName(const QString &newCodecName);
        QString getCodecName() const;
        QTextCodec* getCodec() const;

        void setAwayMessage(const QString& message);
        QString getAwayMessage() const;

        void setAwayNickname(const QString& nickname);
        QString getAwayNickname() const;

        int id() const { return m_id; }

    private:
        void init();

    private:
        QString name;

        QString realName;
        QString ident;

        QStringList nicknameList;

        QString partReason;
        QString quitReason;
        QString kickReason;

        QString m_authType;
        QString m_authPassword;
        QString m_nickservNickname;
        QString m_nickservCommand;
        QString m_saslAccount;
        QUrl m_pemClientCertFile;

        bool insertRememberLineOnAway;
        bool runAwayCommands;
        QString awayCommand;
        QString returnCommand;

        bool m_automaticAway;
        int m_awayInactivity;
        bool m_automaticUnaway;

        QString m_codecName;
        QTextCodec* m_codec;

        QString m_shellCommand;

        QString awayMessage;
        QString awayNickname;

        int m_id;

        static int s_availableId;
};

using IdentityPtr = QExplicitlySharedDataPointer<Identity>;
using IdentityList = QList<IdentityPtr>;

#endif
