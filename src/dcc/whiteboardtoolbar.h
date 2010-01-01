/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef WBOARDTOOLBAR_H
#define WBOARDTOOLBAR_H

#include <QFrame>
#include <QList>

#include "ui_whiteboardtoolbarui.h"

class KPushButton;

namespace Konversation
{
    namespace DCC
    {
        class WhiteBoardToolBar : public QFrame, public Ui::WhiteBoardToolBarUi
        {
            Q_OBJECT

            public:
                WhiteBoardToolBar(QWidget* parent);
                ~WhiteBoardToolBar();

            public slots:
                //void toolChanged();

            protected slots:
                void pencilToggled(bool checked);
                void lineToogled(bool checked);
                void rectangleToogled(bool checked);
                void ellipseToogled(bool checked);
                void textToogled(bool checked);
                void selectionToogled(bool checked);
                void eraseToogled(bool checked);
                void fillToogled(bool checked);

            private:
                 inline void connectToogleButtons();
                 inline void disconnectToogleButtons();

                 inline void handleToogleButton(KPushButton* button, bool checked);
                 inline void unCheckOtherButtons(KPushButton* button);

                 QList<KPushButton*> m_toogleButtonList;
        };
    }
}

#endif // WBOARDTOOLBAR_H
