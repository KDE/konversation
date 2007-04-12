/*
    This file is part of the KDE libraries

    Copyright (C) 1999 Lars Knoll (knoll@mpi-hd.mpg.de)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef KHTMLDECODER_H
#define KHTMLDECODER_H

#include <qstring.h>

class QTextCodec;
class QTextDecoder;

class JapaneseCode;

/**
 * @internal
 */
class Decoder
{
public:
    enum EncodingType {
        DefaultEncoding,
        AutoDetectedEncoding,
        EncodingFromXMLHeader,
        EncodingFromMetaTag,
        EncodingFromHTTPHeader,
        UserChosenEncoding
    };

    Decoder();
    ~Decoder();

    void setEncoding(const char *encoding, EncodingType type);
    const char *encoding() const;

    QString decode(const char *data, int len);

    bool visuallyOrdered() const { return visualRTL; }

    const QTextCodec *codec() const { return m_codec; }

    QString flush() const;


    enum AutoDetectLanguage {
        SemiautomaticDetection,
        Arabic,
        Baltic,
        CentralEuropean,
        Chinese,
        Greek,
        Hebrew,
        Japanese,
        Korean,
        Russian,
        Thai,
        Turkish,
        Ukrainian,
        Unicode,
        WesternEuropean
    };

    void setAutoDetectLanguage( AutoDetectLanguage _language ) { m_autoDetectLanguage = _language; }
    AutoDetectLanguage autoDetectLanguage() { return m_autoDetectLanguage; }



private:
    QCString automaticDetectionForArabic( const unsigned char* str, int size );
    QCString automaticDetectionForBaltic( const unsigned char* str, int size );
    QCString automaticDetectionForCentralEuropean( const unsigned char* str, int size );
    QCString automaticDetectionForCyrillic( const unsigned char* str, int size, AutoDetectLanguage _language );
    QCString automaticDetectionForGreek( const unsigned char* str, int size );
    QCString automaticDetectionForHebrew( const unsigned char* str, int size );
    QCString automaticDetectionForJapanese( const unsigned char* str, int size );
    QCString automaticDetectionForTurkish( const unsigned char* str, int size );
    QCString automaticDetectionForWesternEuropean( const unsigned char* str, int size );

    // codec used for decoding. default is Latin1.
    QTextCodec *m_codec;
    QTextDecoder *m_decoder; // only used for utf16
    QCString enc;
    EncodingType m_type;

    QCString buffer;

    bool body;
    bool beginning;
    bool visualRTL;

    AutoDetectLanguage m_autoDetectLanguage;

    JapaneseCode *kc;
};

#endif
