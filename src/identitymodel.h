/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eli MacKenzie <argonel@kde.org>
*/

#ifndef IDENTITYMODEL_H
#define IDENTITYMODEL_H

#include "identity.h"

#include <QAbstractListModel>

class IdentityModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum AdditionalRoles {
        Name = Qt::UserRole + 1,
        RealName,
        Ident,
        Nickname,
        NicknameList,
        AuthType,
        AuthPassword,
        NickservNickname,
        NickservCommand,
        SaslAccount,
        PemClientCertFile,
        QuitReason,
        PartReason,
        KickReason,
        InsertRememberLineOnAway,
        RunAwayCommands,
        AwayCommand,
        ReturnCommand,
        AutomaticAway,
        AwayInactivity,
        AutomaticUnaway,
        ShellCommand,
        CodecName,
//        Codec,
        AwayMessage,
        AwayNickname,
        Id
    };
    Q_ENUM(AdditionalRoles)

    explicit IdentityModel(QObject *parent = 0);
    virtual ~IdentityModel();

    QHash<int, QByteArray> roleNames() const override;

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

private:

};

#endif // IDENTITYMODEL_H
