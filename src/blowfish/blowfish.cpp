// -*- mode: c++; c-file-style: "stroustrup"; c-basic-offset: 4; tabs-width: 4; indent-tabs-mode: nil -*-
/*
  Copyright (c) 2005 by İsmail Dönmez <ismail.donmez@boun.edu.tr>
  Licensed under GNU GPLv2 or later at your option

*/

#include <qcstring.h>
#include <qstringlist.h>

#include "blowfish.h"
#include "mc_blowfish.h"
#include "server.h"
#include "channel.h"

namespace Konversation
{
    // Find n'th occurrence of seperator in input and return the index
    int findOccurrence(const QCString& input, const QCString& seperator, int nth)
    {
        int j=1;
        uint i;

        for(i=0; i < input.length(); ++i)
        {
            if((input.mid(i,1) == seperator))
            {
                if (j == nth)
                    return i;
                else
                    ++j;
            }
        }
        return i;
    }

    void decrypt(const QString& recepient, QCString& cipher, Server* server)
    {
        QCString key = server->getKeyForRecepient(recepient);

        if(!key.isEmpty())
        {
            int index = findOccurrence(cipher, ":", 2);
            QCString backup = cipher.mid(0,index+1);
            QCString tmp = cipher.mid(index+1);
            char* tmp2;

            if( !(tmp.mid(0,4) == "+OK ") && !(tmp.mid(0,5) == "mcps ") )
                return;
            else
                cipher = tmp;

            if(cipher.mid(0,5) == "mcps ")
                cipher = cipher.mid(5);
            else
                cipher = cipher.mid(4);

            QCString ckey( key.length()+2 );
            QCString result( cipher.length()+1 );
            qstrncpy(result.data(), cipher.data(), cipher.length());
            qstrncpy(ckey.data(), key.data(), key.length()+1);
            tmp2 = decrypt_string(ckey.data(),result.data());
            cipher = backup+"(e) "+tmp2+" "+"\n";
            free(tmp2);
        }
    }

    void decryptTopic(const QString& recepient, QCString& cipher, Server* server)
    {
        QCString key = server->getKeyForRecepient(recepient);

        if(!key.isEmpty())
        {
            int index = findOccurrence(cipher, ":", 2);
            QCString backup = cipher.mid(0,index+1);
            QCString tmp = cipher.mid(index+1);
            char* tmp2;

            if(tmp.mid(0,4) == "+OK ")            // FiSH style topic
                cipher = tmp.mid(4);
            else if(tmp.left(5) == "«m«")
                cipher = tmp.mid(5,tmp.length()-10);
            else
                return;

            QCString result( cipher.length()+1 );
            QCString ckey( key.length()+2 );
            qstrncpy(ckey.data(), key.data(), key.length()+1);
            qstrncpy(result.data(), cipher.data(), cipher.length());
            tmp2 = decrypt_string(ckey.data(),result.data());
            cipher = tmp2;

            cipher = backup+"(e) "+cipher;
            free(tmp2);
        }
    }

    void encrypt(const QString& recepient, QString& cipher, Server* server)
    {
        QString key = server->getKeyForRecepient(recepient);

        if(!key.isEmpty())
        {
            if(cipher.startsWith("+p "))
            {
                cipher = cipher.mid(3);
                return;
            }

            QString backup = cipher.section(":",0,0)+":";
            cipher = cipher.section(":",1).remove("\n");

            char* tmp;
            int size = cipher.utf8().length();
            QCString encrypted( size+1 );
            QCString ckey( key.length()+1 );

            strcpy(ckey.data(),key.local8Bit());
            strcpy(encrypted.data(),cipher.utf8());
            tmp = encrypt_string(ckey.data(),encrypted.data());
            cipher = backup +"+OK " + tmp +"\n";
            free(tmp);
        }
    }
}
