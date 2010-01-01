/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WBOARD_H
#define WBOARD_H

#include <QWidget>
#include <QLabel>

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardToolBar;

        class WhiteBoard : public QWidget
        {
            Q_OBJECT

            public:
                WhiteBoard(QWidget* parent);
                ~WhiteBoard();

            public slots:
                void receivedWhiteBoardLine(const QString& line);

            signals:
                void rawWhiteBoardCommand(const QString& command);

            private:
                WhiteBoardToolBar* m_toolbar;
                //TODO STUB!
                QLabel* m_label;
        };
    }
}

#endif // WBOARD_H
