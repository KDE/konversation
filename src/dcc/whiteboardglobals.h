/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WHITEBOARDGLOBALS_H
#define WHITEBOARDGLOBALS_H

#include <QHash>

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardGlobals
        {
            // wboard specs, for more info see
            // http://www.visualirc.net/tech-wboard.php
            public:
                enum WhiteBoardTool
                {
                    Pencil          = 0,
                    Line            = 1,
                    /*Not defined = 2*/
                    Rectangle       = 3,
                    Ellipse         = 4,
                    FilledRectangle = 5,
                    FilledEllipse   = 6,
                    /*Not defined = 7*/
                    Eraser          = 8,
                    FloodFill       = 9,
                    Arrow           = 10,

                    /*User Internal, not sended*/
                    Selection       = 64,
                    Text,
                    TextExtended,
                    Stamp
                };

                enum WhiteBoardCommand
                {
                    BLT, // Copies a rectangular region from one area of the canvas to another
                    CAN, // Indicates that the sender supports the named option
                    CANT, // Indicates that the sender does not support the named option
                    CLIP, // Sets the canvas's clipping region to the specified rectangle
                    CLS, // Clears the canvas to all white
                    DO, // Indicates that the sender supports the named option, and wants the recipient to use it
                    DONT, //krazy:exclude=spelling // Indicates that the sender does not support the named option, and does not want the recipient to use it
                    DR, // Draws with the specified tool
                    ENTRY, // Prompts the user to enter text
                    IMG, // IMG and IMGDATA will work together to transfer small bitmap images over the DCC Whiteboard connection
                    IMGDATA, // IMG and IMGDATA will work together to transfer small bitmap images over the DCC Whiteboard connection
                    MAKEIMG, // Copies a rectangular region from the canvas
                    NOCLIP, // Cancels a previous CLIP command, allowing the user to draw anywhere on the canvas
                    SETSTAMP, // Selects the rubber stamp tool and the stamp image
                    SETTOOL, // Selects the tool identified
                    STAMP, // Draws the stamp image
                    TXT, // Writes text
                    TXTEX // Writes text using the specified font and style information
                };

                enum TextStyle
                {
                    Bold =      1 << 0,
                    Italic =    1 << 1,
                    Underline = 1 << 2,
                    Strikeout = 1 << 3
                };

                static QHash<QString, WhiteBoardCommand> wboardCommandHash()
                {
                    static QHash<QString, WhiteBoardCommand> wboardCommands;
                    if (!wboardCommands.isEmpty())
                    {
                        return wboardCommands;
                    }
                    wboardCommands.insert("BLT", BLT);
                    wboardCommands.insert("CAN", CAN);
                    wboardCommands.insert("CANT", CANT);
                    wboardCommands.insert("CLIP", CLIP);
                    wboardCommands.insert("CLS", CLS);
                    wboardCommands.insert("DR", DR);
                    wboardCommands.insert("DO", DO);
                    wboardCommands.insert("DONT", DONT); //krazy:exclude=spelling
                    wboardCommands.insert("ENTRY", ENTRY);
                    wboardCommands.insert("IMG", IMG);
                    wboardCommands.insert("IMGDATE", IMGDATA);
                    wboardCommands.insert("MAKEIMG", MAKEIMG);
                    wboardCommands.insert("NOCLIP", NOCLIP);
                    wboardCommands.insert("SETSTAMP", SETSTAMP);
                    wboardCommands.insert("SETTOOL", SETTOOL);
                    wboardCommands.insert("STAMP", STAMP);
                    wboardCommands.insert("TXT", TXT);
                    wboardCommands.insert("TXTEX", TXTEX);
                    return wboardCommands;
                }

                // no limit in specs, but vIRC 2.0 uses it as max value
                static const int MaxPenWidth;

                // there is no limit in the wboard specs, but we have enough
                // "broken" driver who don't support bigger pixmaps
                static const int MaxImageSize;
        };
    }
}

#endif // WHITEBOARDGLOBALS_H
