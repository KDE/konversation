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

#include <QWidget>
#include <QLabel>

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
                    TextExtended
                };

                enum WhiteBoardCommand
                {
                    BLT, // Copies a rectangular region from one area of the canvas to another
                    CAN, // Indicates that the sender supports the named option
                    CANT, // Indicates that the sender does not support the named option
                    CLIP, // Sets the canvas's clipping region to the specified rectangle
                    CLS, // Clears the canvas to all white
                    DO, // Indicates that the sender supports the named option, and wants the recipient to use it
                    DONT, // Indicates that the sender does not support the named option, and does not want the recipient to use it
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
                    TXTEX, // Writes text using the specified font and style information
                };
        };
    }
}

#endif // WHITEBOARDGLOBALS_H
