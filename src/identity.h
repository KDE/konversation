/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class holds the various user identities
  begin:     Son Feb 9 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IDENTITY_H
#define IDENTITY_H

#include <ksharedptr.h>
#include <QStringList>


class QTextCodec;

class Identity : public KShared
{
    public:
        /// Create an Identity with a new id.
        Identity();
        /// Create a new Identity with a set id.
        explicit Identity(int id);
        /// Copy all of @param original including the id.
        Identity(const Identity& original);
        ~Identity();

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

        void setBot(const QString& bot);
        QString getBot() const;
        void setPassword(const QString& password);
        QString getPassword() const;

        void setNicknameList(const QStringList& newList);
        QStringList getNicknameList() const;

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

    protected:
        QString name;

        QString bot;
        QString password;

        QString realName;
        QString ident;

        QStringList nicknameList;

        QString partReason;
        QString quitReason;
        QString kickReason;

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

    private:
        int m_id;
        static int s_availableId;
        void init();
};

typedef KSharedPtr<Identity> IdentityPtr;
typedef QList<IdentityPtr> IdentityList;

#endif
