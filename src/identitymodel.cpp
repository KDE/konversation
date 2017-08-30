/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2017 Eli MacKenzie <argonel@kde.org>
*/


#include "identitymodel.h"
#include <preferences.h>

#include <QMetaEnum>

IdentityModel::IdentityModel(QObject *parent): QAbstractListModel(parent)
{
}

IdentityModel::~IdentityModel()
{
}

QHash<int, QByteArray> IdentityModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("AdditionalRoles"));

    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant IdentityModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= Preferences::identityList().count()) {
        return QVariant();
    }

    IdentityPtr identity = Preferences::identityList().at(index.row());

    switch (role)
    {
        case Qt::DisplayRole:
        case Name:
            return identity->getName();

        case RealName:
            return identity->getRealName();

        case Ident:
            return identity->getIdent();

//         case Nickname:
//             return identity->getNickname(int index);

        case NicknameList:
            return identity->getNicknameList();

        case AuthType:
            return identity->getAuthType();

        case AuthPassword:
            return identity->getAuthPassword();

        case NickservNickname:
            return identity->getNickservNickname();

        case NickservCommand:
            return identity->getNickservCommand();

        case SaslAccount:
            return identity->getSaslAccount();

        case PemClientCertFile:
            return identity->getPemClientCertFile();

        case QuitReason:
            return identity->getQuitReason();

        case PartReason:
            return identity->getPartReason();

        case KickReason:
            return identity->getKickReason();

        case InsertRememberLineOnAway:
            return identity->getInsertRememberLineOnAway();

        case RunAwayCommands:
            return identity->getRunAwayCommands();

        case AwayCommand:
            return identity->getAwayCommand();

        case ReturnCommand:
            return identity->getReturnCommand();

        case AutomaticAway:
            return identity->getAutomaticAway();

        case AwayInactivity:
            return identity->getAwayInactivity();

        case AutomaticUnaway:
            return identity->getAutomaticUnaway();

        case ShellCommand:
            return identity->getShellCommand();

        case CodecName:
            return identity->getCodecName();

//         case Codec:
//             return identity->getCodec();

        case AwayMessage:
            return identity->getAwayMessage();

        case AwayNickname:
            return identity->getAwayNickname();

        case Id:
            return identity->id();

        default:
            break;
    }
    
    return QVariant();
}

int IdentityModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : Preferences::identityList().count();
}
