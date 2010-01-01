/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#include "whiteboardtoolbar.h"
#include <QHBoxLayout>
#include <QLayout>
#include <KPushButton>
#include <KIcon>

namespace Konversation
{
    namespace DCC
    {
        WhiteBoardToolBar::WhiteBoardToolBar(QWidget* parent)
            : QFrame(parent)
        {
            setupUi(this);

            m_newPushButton->setIcon(KIcon("document-edit"));
            m_newPushButton->setToolTip(i18n("Clear"));
            m_savePushButton->setIcon(KIcon("document-save"));
            m_savePushButton->setToolTip(i18n("Save As.."));

            m_pencilPushButton->setIcon(KIcon("draw-freehand"));
            m_pencilPushButton->setToolTip(i18n("Freehand Drawing"));
            m_toogleButtonList.append(m_pencilPushButton);

            m_linePushButton->setIcon(KIcon("draw-line"));
            m_linePushButton->setToolTip(i18n("Draw a straight line"));
            m_toogleButtonList.append(m_linePushButton);

            m_rectanglePushButton->setIcon(KIcon("draw-rectangle"));
            m_rectanglePushButton->setToolTip(i18n("Draw a rectangle"));
            m_toogleButtonList.append(m_rectanglePushButton);

            m_ellipsePushButton->setIcon(KIcon("draw-circle"));
            m_ellipsePushButton->setToolTip(i18n("Draw an ellipse"));
            m_toogleButtonList.append(m_ellipsePushButton);

            m_textPushButton->setIcon(KIcon("draw-text"));
            m_textPushButton->setToolTip(i18n("Draw text"));
            m_toogleButtonList.append(m_textPushButton);

            m_selectionPushButton->setIcon(KIcon("select-rectangular"));
            m_selectionPushButton->setToolTip(i18n("Selection"));
            m_toogleButtonList.append(m_selectionPushButton);

            m_eraserPushButton->setIcon(KIcon("draw-eraser"));
            m_eraserPushButton->setToolTip(i18n("Eraser"));
            m_toogleButtonList.append(m_eraserPushButton);

            m_fillPushButton->setIcon(KIcon("fill-color"));
            m_fillPushButton->setToolTip(i18n("Fill a contigours area of with a color"));
            m_toogleButtonList.append(m_fillPushButton);

            m_optionFrame->setVisible(false);
        }

        WhiteBoardToolBar::~WhiteBoardToolBar()
        {
        }

        void WhiteBoardToolBar::connectToogleButtons()
        {
            connect(m_pencilPushButton, SIGNAL(toogled(bool)), this, SLOT(pencilToggled(bool)));
            connect(m_linePushButton, SIGNAL(toogled(bool)), this, SLOT(lineToggled(bool)));
            connect(m_rectanglePushButton, SIGNAL(toogled(bool)), this, SLOT(rectangleToogled(bool)));
            connect(m_ellipsePushButton, SIGNAL(toogled(bool)), this, SLOT(ellipseToogled(bool)));
            connect(m_textPushButton, SIGNAL(toogled(bool)), this, SLOT(textToogled(bool)));
            connect(m_selectionPushButton, SIGNAL(toogled(bool)), this, SLOT(selectionToogled(bool)));
            connect(m_eraserPushButton, SIGNAL(toogled(bool)), this, SLOT(eraseToogled(bool)));
            connect(m_fillPushButton, SIGNAL(toogled(bool)), this, SLOT(fillToogled(bool)));
        }

        void WhiteBoardToolBar::disconnectToogleButtons()
        {
            disconnect(m_pencilPushButton, SIGNAL(toogled(bool)), this, SLOT(pencilToggled(bool)));
            disconnect(m_linePushButton, SIGNAL(toogled(bool)), this, SLOT(lineToggled(bool)));
            disconnect(m_rectanglePushButton, SIGNAL(toogled(bool)), this, SLOT(rectangleToogled(bool)));
            disconnect(m_ellipsePushButton, SIGNAL(toogled(bool)), this, SLOT(ellipseToogled(bool)));
            disconnect(m_textPushButton, SIGNAL(toogled(bool)), this, SLOT(textToogled(bool)));
            disconnect(m_selectionPushButton, SIGNAL(toogled(bool)), this, SLOT(selectionToogled(bool)));
            disconnect(m_eraserPushButton, SIGNAL(toogled(bool)), this, SLOT(eraseToogled(bool)));
            disconnect(m_fillPushButton, SIGNAL(toogled(bool)), this, SLOT(fillToogled(bool)));
        }

        void WhiteBoardToolBar::ellipseToogled(bool checked)
        {
            handleToogleButton(m_ellipsePushButton, checked);
        }

        void WhiteBoardToolBar::eraseToogled(bool checked)
        {
            handleToogleButton(m_eraserPushButton, checked);
        }

        void WhiteBoardToolBar::fillToogled(bool checked)
        {
            handleToogleButton(m_fillPushButton, checked);
        }

        void WhiteBoardToolBar::lineToogled(bool checked)
        {
            handleToogleButton(m_linePushButton, checked);
        }

        void WhiteBoardToolBar::pencilToggled(bool checked)
        {
            handleToogleButton(m_pencilPushButton, checked);
        }

        void WhiteBoardToolBar::rectangleToogled(bool checked)
        {
            handleToogleButton(m_rectanglePushButton, checked);
        }

        void WhiteBoardToolBar::selectionToogled(bool checked)
        {
            handleToogleButton(m_selectionPushButton, checked);
        }

        void WhiteBoardToolBar::textToogled(bool checked)
        {
            handleToogleButton(m_textPushButton, checked);
        }

        void WhiteBoardToolBar::handleToogleButton(KPushButton* button, bool checked)
        {
            disconnectToogleButtons();
            if (checked)
            {
                unCheckOtherButtons(button);
            }
            else
            {
                button->setCheckable(true);
            }
            connectToogleButtons();
        }

        void WhiteBoardToolBar::unCheckOtherButtons(KPushButton* button)
        {
            foreach(KPushButton* pushButton, m_toogleButtonList)
            {
                if (pushButton != button && pushButton->isChecked())
                {
                    pushButton->setCheckable(false);
                    return;
                }
            }
        }

    }
}
