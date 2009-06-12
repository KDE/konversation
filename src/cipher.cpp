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

#include "cipher.h"

#include <KDebug>


namespace Konversation
{
    Cipher::Cipher(QByteArray key, QString cipherType)
    {
        if(key.mid(0,4).toLower() == "ecb:")
        {
            m_cbc = false;
            m_key = key.mid(4);
        }
        //strip cbc: if included
        else if(key.mid(0,4).toLower() == "cbc:")
        {
            m_cbc = true;
            m_key = key.mid(4);
        }
        else
        {
            m_cbc = true;
            m_key = key;
        }
        m_cipher = cipherType;
    }

    Cipher::~Cipher()
    {
    }

    QByteArray Cipher::decrypt(QByteArray cipherText)
    {
        QByteArray pfx = "(e) ";
        bool error = false; // used to flag non cbc, seems like good practice not to parse w/o regard for set encryption type

        //if we get cbc
        if(cipherText.mid(0,5) == "+OK *")
        {
            //if we have cbc
            if(m_cbc)
                cipherText = cipherText.mid(5);
            //if we don't
            else
            {
                cipherText = cipherText.mid(5);
                pfx = "ERROR_NONECB: ";
                error = true;
            }
        }
        //if we get ecb
        else if(cipherText.mid(0,4) == "+OK " || cipherText.mid(0,5) == "mcps ")
        {
            //if we had cbc
            if(m_cbc)
            {
                cipherText = (cipherText.mid(0,4) == "+OK ") ? cipherText.mid(4) : cipherText.mid(5);
                pfx = "ERROR_NONCBC: ";
                error = true;
            }
            //if we don't
            else
            {
                if(cipherText.mid(0,4) == "+OK ")
                    cipherText = cipherText.mid(4);
                else
                    cipherText = cipherText.mid(5);
            }
        }
        //all other cases we fail
        else
            return "ERROR: "+cipherText;

        QByteArray temp;
        // (if cbc and no error we parse cbc) || (if ecb and error we parse cbc)
        if((m_cbc && !error) || (!m_cbc && error))
        {
            cipherText = cipherText;
            temp = blowfishCBC(cipherText, false);

            if(temp == cipherText)
            {
                kDebug("Decryption from CBC Failed");
                return "ERROR: "+cipherText+' '+'\n';
            }
            else
                cipherText = temp;
        }
        else
        {
            temp = blowfishECB(cipherText, false);

            if(temp == cipherText)
            {
                kDebug("Decryption from ECB Failed");
                return "ERROR: "+cipherText+' '+'\n';
            }
            else
                cipherText = temp;
        }
        // If it's a CTCP we don't want to have the (e) interfering with the processing
        // TODO FIXME the proper fix for this is to show encryption differently e.g. [nick] instead of <nick>
        // don't hate me for the mircryption reference there.
        if (cipherText.at(0) == 1)
            pfx = "\x0";
        cipherText = pfx+cipherText+' '+'\n'; // FIXME(??) why is there an added space here?
        return cipherText;
    }

    QByteArray Cipher::decryptTopic(QByteArray cipherText)
    {
        if(cipherText.mid(0,4) == "+OK ")// FiSH style topic
            cipherText = cipherText.mid(4);
        else if(cipherText.left(5) == "«m«")
            cipherText = cipherText.mid(5,cipherText.length()-10);
        else
            return "ERROR: "+cipherText;

        QByteArray temp;
        //TODO currently no backwards sanity checks for topic, it seems to use different standards
        //if somebody figures them out they can enable it and add "ERROR_NONECB/CBC" warnings
        if(m_cbc)
            temp = blowfishCBC(cipherText.mid(1), false);
        else
            temp = blowfishECB(cipherText, false);

        if(temp == cipherText)
        {
            return "ERROR: "+cipherText;
        }
        else
            cipherText = temp;

        if(cipherText.mid(0,2) == "@@")
            cipherText = cipherText.mid(2);

        cipherText = "(e) "+cipherText;
        return cipherText;
    }

    bool Cipher::encrypt(QByteArray& cipherText)
    {
        if (cipherText.left(3) == "+p ") //don't encode if...?
            cipherText = cipherText.mid(3);
        else
        {
            if(m_cbc) //encode in ecb or cbc decide how to determine later
            {
                QByteArray temp = blowfishCBC(cipherText, true);

                if(temp == cipherText)
                {
                    kDebug("CBC Encoding Failed");
                    return false;
                }

                cipherText = "+OK *" + temp;
            }
            else
            {
                QByteArray temp = blowfishECB(cipherText, true);

                if(temp == cipherText)
                {
                    kDebug("ECB Encoding Failed");
                    return false;
                }

                cipherText = "+OK " + temp;
            }
        }
        return true;
    }
    //THE BELOW WORKS AKA DO NOT TOUCH UNLESS YOU KNOW WHAT YOU'RE DOING
    QByteArray Cipher::blowfishCBC(QByteArray cipherText, bool direction)
    {
        QByteArray temp = cipherText;
        QCA::Initializer init;
        if(direction)
        {
            // make sure cipherText is an interval of 8 bits. We do this before so that we
            // know there's at least 8 bytes to en/decryption this ensures QCA doesn't fail
            int length = temp.length();
            if ((length % 8) != 0)
                length += 8 - (length % 8);
            temp.resize(length);

            QCA::InitializationVector iv(8);
            temp.prepend(iv.toByteArray()); // prefix with 8bits of IV for mircryptions *CUSTOM* cbc implementation
        }
        else
        {
            temp = QByteArray::fromBase64(temp);
            //supposedly nescessary if we get a truncated message also allows for decryption of 'crazy'
            //en/decoding clients that use STANDARDIZED PADDING TECHNIQUES
            int length = temp.length();
            if ((length % 8) != 0)
                length += 8 - (length % 8);
            temp.resize(length);
        }

        QCA::Direction dir = (direction) ? QCA::Encode : QCA::Decode;
        QCA::Cipher cipher(m_cipher, QCA::Cipher::CBC, QCA::Cipher::NoPadding, dir, QCA::SymmetricKey(m_key), QCA::InitializationVector(QByteArray("0")));
        QByteArray temp2 = cipher.update(QCA::MemoryRegion(temp)).toByteArray();
        temp2 += cipher.final().toByteArray();

        if(!cipher.ok())
            return cipherText;

        if(direction) //send in base64
            temp2 = temp2.toBase64();
        else //cut off the 8bits of IV
            temp2 = temp2.remove(0,8);

        return temp2;
    }

    QByteArray Cipher::blowfishECB(QByteArray cipherText, bool direction)
    {
        QByteArray temp = cipherText;

        //do padding ourselves
        if(direction)
        {
            int length = temp.length();
            if ((length % 8) != 0)
                length += 8 - (length % 8);
            temp.resize(length);
        }
        else
        {
            temp = b64ToByte(temp);
            int length = temp.length();
            if ((length % 8) != 0)
                length += 8 - (length % 8);
            temp.resize(length);
        }

        QCA::Direction dir = (direction) ? QCA::Encode : QCA::Decode;
        QCA::Initializer init;
        QCA::Cipher cipher(m_cipher, QCA::Cipher::ECB, QCA::Cipher::NoPadding, dir, QCA::SymmetricKey(m_key));
        QByteArray temp2 = cipher.update(QCA::MemoryRegion(temp)).toByteArray();
        temp2 += cipher.final().toByteArray();

        if(!cipher.ok())
            return cipherText;

        if(direction)
            temp2 = byteToB64(temp2);

        return temp2;
    }

    //Custom non RFC 2045 compliant Base64 enc/dec code for mircryption / FiSH compatibility
    QByteArray Cipher::byteToB64(QByteArray text)
    {
        int left = 0;
        int right = 0;
        int k = -1;
        int v;
        QString base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        QByteArray encoded;
        while (k < (text.length() - 1)) {
            k++;
            v=text.at(k); if (v<0) v+=256;
            left = v << 24;
            k++;
            v=text.at(k); if (v<0) v+=256;
            left += v << 16;
            k++;
            v=text.at(k); if (v<0) v+=256;
            left += v << 8;
            k++;
            v=text.at(k); if (v<0) v+=256;
            left += v;

            k++;
            v=text.at(k); if (v<0) v+=256;
            right = v << 24;
            k++;
            v=text.at(k); if (v<0) v+=256;
            right += v << 16;
            k++;
            v=text.at(k); if (v<0) v+=256;
            right += v << 8;
            k++;
            v=text.at(k); if (v<0) v+=256;
            right += v;

            for (int i = 0; i < 6; i++) {
                encoded.append(base64.at(right & 0x3F).toAscii());
                right = right >> 6;
            }
            //TODO make sure the .toascii doesn't break anything

            for (int i = 0; i < 6; i++) {
                encoded.append(base64.at(left & 0x3F).toAscii());
                left = left >> 6;
            }
        }
        return encoded;
    }

    QByteArray Cipher::b64ToByte(QByteArray text)
    {
        QString base64 = "./0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        QByteArray decoded;
        int k = -1;
        while (k < (text.length() - 1)) {
            int right = 0;
            int left = 0;
            int v = 0;
            int w = 0;
            int z = 0;

            for (int i = 0; i < 6; i++) {
                k++;
                v = base64.indexOf(text.at(k));
                right |= v << (i * 6);
            }

            for (int i = 0; i < 6; i++) {
                k++;
                v = base64.indexOf(text.at(k));
                left |= v << (i * 6);
            }

            for (int i = 0; i < 4; i++) {
                w = ((left & (0xFF << ((3 - i) * 8))));
                z = w >> ((3 - i) * 8);
                if(z < 0) {z = z + 256;}
                decoded.append(z);
            }

            for (int i = 0; i < 4; i++) {
                w = ((right & (0xFF << ((3 - i) * 8))));
                z = w >> ((3 - i) * 8);
                if(z < 0) {z = z + 256;}
                decoded.append(z);
            }
        }
        return decoded;
    }
}
