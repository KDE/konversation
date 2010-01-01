/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboard.h"

#include "whiteboardtoolbar.h"

namespace Konversation
{
    namespace DCC
    {
        WhiteBoard::WhiteBoard(QWidget* parent)
            : QWidget(parent)
        {
            setLayoutDirection(Qt::LeftToRight);

            m_toolbar = new WhiteBoardToolBar(this);
            m_label = new QLabel(this);
        }

        WhiteBoard::~WhiteBoard()
        {
        }

        void WhiteBoard::receivedWhiteBoardLine (const QString& line)
        {

        }
    }
}
