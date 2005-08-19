/*
  Copyright 2005 İsmail Dönmez <ismail@kde.org.tr
  Licensed under GPLv2 or later at your option
*/

#ifndef BLOWFISH_H
#define BLOWFISH_H

class Server;

namespace Konversation
{

    int findOccurrence(const QCString& input, const QCString& seperator, int nth);
    void decrypt(const QString& recepient, QCString& cipher, Server* server);
    void decryptTopic(const QString& recepient, QCString& cipher, Server* server);
    void encrypt(const QString& recepient, QString& cipher, Server* server);
}
#endif
