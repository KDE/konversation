/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 1997 Robey Pointer <robeypointer@gmail.com>
    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2009 Travis McHenry <tmchenryaz@cox.net>
*/

#ifndef CIPHER_H
#define CIPHER_H

#include <QIODevice>
#include <QtCrypto>

namespace Konversation
{
    class Cipher
    {
        public:
            Cipher();
            explicit Cipher(const QByteArray &key, const QString &cipherType=QStringLiteral("blowfish"));
            ~Cipher();

            QByteArray decrypt(QByteArray cipher);
            QByteArray decryptTopic(QByteArray cipher);
            bool encrypt(QByteArray& cipher);
            QByteArray initKeyExchange();
            QByteArray parseInitKeyX(const QByteArray &key);
            bool parseFinishKeyX(const QByteArray &key);
            bool setKey(const QByteArray &key);
            QByteArray key() const { return m_key; }
            bool setType(const QString &type);
            QString type() const { return m_type; }

            enum CipherFeature { DH, Blowfish };
            static bool isFeatureAvailable(CipherFeature feature);
            static QString runtimeError() { return m_runtimeError; }

        private:
            //direction is true for encrypt, false for decrypt
            QByteArray blowfishCBC(QByteArray cipherText, bool direction);
            QByteArray blowfishECB(QByteArray cipherText, bool direction);
            QByteArray b64ToByte(const QByteArray &text);
            QByteArray byteToB64(const QByteArray &text);

        private:
            QCA::Initializer init;
            QByteArray m_key;
            QCA::DHPrivateKey m_tempKey;
            QCA::BigInteger m_primeNum;
            QString m_type;
            bool m_cbc;

            static QString m_runtimeError;

            Q_DISABLE_COPY(Cipher)
    };
}

#endif
