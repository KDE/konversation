/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 1997 Robey Pointer <robeypointer@gmail.com>
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2009 Travis McHenry <tmchenryaz@cox.net>
*/

#ifndef CIPHER_H
#define CIPHER_H

#include <QtCrypto/qca_basic.h>

namespace Konversation
{
    class Cipher
    {
        public:
            Cipher(QByteArray key, QString cipherType=QString("blowfish"));
            ~Cipher();
            QByteArray decrypt(QByteArray cipher);
            QByteArray decryptTopic(QByteArray cipher);
            bool encrypt(QByteArray& cipher);

        private:
            //direction is true for encrypt, false for decrypt
            QByteArray blowfishCBC(QByteArray cipherText, bool direction);
            QByteArray blowfishECB(QByteArray cipherText, bool direction);
            QByteArray b64ToByte(QByteArray text);
            QByteArray byteToB64(QByteArray text);

        protected:
            QByteArray m_key;
            QString m_cipher;
            bool m_cbc;
    };
}
#endif
