// irccharsets.h
// A wrapper for KCharsets
// Copyright (C) 2004 Shintaro Matsuoka <shin@shoegazed.org>

/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifndef KONVERSATION_IRCCHARSETS_H
#define KONVERSATION_IRCCHARSETS_H

class IRCCharsets
{
  public:
    
    /**
     * Lists all available encoding names.
     * e.g. "utf8", "iso 8859-1"
     * You can get instances of QTextCodec from these names
     * like this: QTextCodec* codec= QTextCodec::codecForName( shortName.ascii() )
     * Encodings which don't work on IRC are excluded. (e.g. utf16)
     * @note It's guranteed that the order of this list is same with that of @ref availableEncodingDescriptiveNames() .
     */
    static QStringList availableEncodingShortNames();
    
    /**
     * Lists all available encoding descriptions.
     * e.g. "Unicode ( utf8 )", "Western European ( iso 8859-1 )"
     * Encodings which don't work on IRC are excluded. (e.g. utf16)
     */
    static QStringList availableEncodingDescriptiveNames();
    
    static int availableEncodingsCount();
    
    static QString descriptiveNameToShortName( const QString& descriptiveName );
    
    /**
     * Returns the encoding index in the short names list or the descriptions list.
     * If the encoding name is invalid, returns -1 .
     */
    static int shortNameToIndex( const QString& shortName );
    
    /**
     * Checks if the encoding name is in the short encoding names.
     * @see availableEncodingShortNames()
     */
    static bool isValidEncoding( const QString& shortName );
    
    /**
     * Returns the short name of the most suitable encoding for this locale.
     * @return a short encoding name
     */
    static QString encodingForLocale();
    
  private:
    static void private_init();
    static QStringList s_shortNames;
    static QStringList s_descriptiveNames;
};

#endif  // KONVERSATION_IRCCHARSETS_H
