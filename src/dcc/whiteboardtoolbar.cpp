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
#include <QDir>
#include <QImageWriter>
#include <QPainter>
#include <QPushButton>
#include <QUrl>
#include <QIcon>
#include <QDebug>
#include <QFileDialog>

#include <KLocalizedString>

#include "whiteboardfontchooser.h"

namespace Konversation
{
    namespace DCC
    {
        WhiteBoardToolBar::WhiteBoardToolBar(QWidget* parent)
            : QWidget(parent),
              m_lineWidthPixmap(20, 20),
              m_textType(SimpleText),
              m_fontDialog(0)
        {
            setupUi(this);

            m_clearPushButton->setIcon(QIcon::fromTheme("document-edit"));
            m_clearPushButton->setToolTip(i18n("Clear Image"));
            m_savePushButton->setIcon(QIcon::fromTheme("document-save"));
            m_savePushButton->setToolTip(i18n("Save As..."));

            m_pencilPushButton->setIcon(QIcon::fromTheme("draw-freehand"));
            m_pencilPushButton->setToolTip(i18n("Freehand Drawing"));
            m_pencilPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Pencil, m_pencilPushButton);
            m_pencilPushButton->setChecked(true);

            m_linePushButton->setIcon(QIcon::fromTheme("draw-line"));
            m_linePushButton->setToolTip(i18n("Draw a straight line"));
            m_linePushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Line, m_linePushButton);

            m_rectanglePushButton->setIcon(QIcon::fromTheme("draw-rectangle"));
            m_rectanglePushButton->setToolTip(i18n("Draw a rectangle"));
            m_rectanglePushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Rectangle, m_rectanglePushButton);
            m_toggleButtonHash.insert(WhiteBoardGlobals::FilledRectangle, m_rectanglePushButton);

            m_ellipsePushButton->setIcon(QIcon::fromTheme("draw-circle"));
            m_ellipsePushButton->setToolTip(i18n("Draw an ellipse"));
            m_ellipsePushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Ellipse, m_ellipsePushButton);
            m_toggleButtonHash.insert(WhiteBoardGlobals::FilledEllipse, m_ellipsePushButton);

            m_textPushButton->setIcon(QIcon::fromTheme("draw-text"));
            m_textPushButton->setToolTip(i18n("Draw text"));
            m_textPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Text, m_textPushButton);

            m_selectionPushButton->setEnabled(false); // it has no function in current whiteboard
            m_selectionPushButton->setIcon(QIcon::fromTheme("select-rectangular"));
            m_selectionPushButton->setToolTip(i18nc("dcc whiteboard selection tool", "Selection"));
            m_selectionPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Selection, m_selectionPushButton);

            m_eraserPushButton->setIcon(QIcon::fromTheme("draw-eraser"));
            m_eraserPushButton->setToolTip(i18n("Eraser"));
            m_eraserPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Eraser, m_eraserPushButton);

            m_fillPushButton->setIcon(QIcon::fromTheme("fill-color"));
            m_fillPushButton->setToolTip(i18n("Fill a contiguous area with the foreground color"));
            m_fillPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::FloodFill, m_fillPushButton);

            m_arrowPushButton->setIcon(QIcon::fromTheme("draw-arrow-forward"));
            m_arrowPushButton->setToolTip(i18n("Draw an arrow"));
            m_arrowPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::Arrow, m_arrowPushButton);

            m_colorPickerPushButton->setIcon(QIcon::fromTheme("color-picker"));
            m_colorPickerPushButton->setToolTip(i18n("Select a color from the image"));
            m_colorPickerPushButton->setFlat(true);
            m_toggleButtonHash.insert(WhiteBoardGlobals::ColorPicker, m_colorPickerPushButton);

            m_lineWidthSlider->setMaximum(WhiteBoardGlobals::MaxPenWidth);

            connectToggleButtons();

            //foreward colorchooser signals
            connect(m_colorChooser, &Konversation::DCC::WhiteBoardColorChooser::colorsSwapped, this, &WhiteBoardToolBar::colorsSwapped);
            connect(m_colorChooser, &Konversation::DCC::WhiteBoardColorChooser::foregroundColorChanged, this, &WhiteBoardToolBar::foregroundColorChanged);
            connect(m_colorChooser, &Konversation::DCC::WhiteBoardColorChooser::backgroundColorChanged, this, &WhiteBoardToolBar::backgroundColorChanged);

            connect(m_lineWidthSlider, &QSlider::valueChanged, this, &WhiteBoardToolBar::lineWidthChanged);
            connect(m_lineWidthSlider, &QSlider::valueChanged, this, &WhiteBoardToolBar::updateLineWidthPixmap);

            connect(m_clearPushButton, &QPushButton::clicked, this, &WhiteBoardToolBar::clearClicked);
            connect(m_savePushButton, &QPushButton::clicked, this, &WhiteBoardToolBar::saveClicked);

            setFormOptionVisible(false);
            setLineWidthVisible(true);
            updateLineWidthPixmap(1);
        }

        WhiteBoardToolBar::~WhiteBoardToolBar()
        {
            delete m_fontDialog;
        }

        QColor WhiteBoardToolBar::foregroundColor() const
        {
            return m_colorChooser->foregroundColor();
        }

        void WhiteBoardToolBar::setForegroundColor(const QColor& foregroundColor)
        {
            m_colorChooser->setForegroundColor(foregroundColor);
        }

        QColor WhiteBoardToolBar::backgroundColor() const
        {
            return m_colorChooser->backgroundColor();
        }

        void WhiteBoardToolBar::setBackgroundColor(const QColor& backgroundColor)
        {
            m_colorChooser->setBackgroundColor(backgroundColor);
        }

        void WhiteBoardToolBar::disableTool(WhiteBoardGlobals::WhiteBoardTool tool)
        {
            QPushButton* button = m_toggleButtonHash.value(tool);
            if (button)
            {
                button->setEnabled(false);
            }
            else
            {
                qDebug() << "unhandled tool:" << tool;
            }
        }

        void WhiteBoardToolBar::enableTool(WhiteBoardGlobals::WhiteBoardTool tool)
        {
            QPushButton* button = m_toggleButtonHash.value(tool);
            if (button)
            {
                button->setEnabled(true);
            }
            else
            {
                qDebug() << "unhandled tool:" << tool;
            }
        }

        void WhiteBoardToolBar::setSupportedTextType(WhiteBoardToolBar::TextType textType)
        {
            m_textType = textType;
            if (m_textType == WhiteBoardToolBar::ExtentedText)
            {
                if (m_fontDialog)
                {
                    return;
                }
                m_fontDialog = new WhiteBoardFontChooser(this);
                connect(m_fontDialog, &WhiteBoardFontChooser::fontChanged, this, &WhiteBoardToolBar::fontChanged);
            }
            else
            {
                if (m_fontDialog)
                {
                    disconnect(m_fontDialog, 0, 0, 0);
                    delete m_fontDialog;
                    m_fontDialog = 0;
                }
            }
        }

        WhiteBoardToolBar::TextType WhiteBoardToolBar::textType() const
        {
            return m_textType;
        }

        void WhiteBoardToolBar::connectToggleButtons()
        {
            qDebug();
            connect(m_pencilPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::pencilToggled);
            connect(m_linePushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::lineToggled);
            connect(m_rectanglePushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::rectangleToggled);
            connect(m_ellipsePushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::ellipseToggled);
            connect(m_textPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::textToggled);
            connect(m_selectionPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::selectionToggled);
            connect(m_eraserPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::eraseToggled);
            connect(m_fillPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::fillToggled);
            connect(m_arrowPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::arrowToggled);
            connect(m_colorPickerPushButton, &QPushButton::toggled, this, &WhiteBoardToolBar::colorPickerToggled);
        }

        void WhiteBoardToolBar::disconnectToggleButtons()
        {
            qDebug();
            disconnect(m_pencilPushButton, 0, 0, 0);
            disconnect(m_linePushButton, 0, 0, 0);
            disconnect(m_rectanglePushButton, 0, 0, 0);
            disconnect(m_ellipsePushButton, 0, 0, 0);
            disconnect(m_textPushButton, 0, 0, 0);
            disconnect(m_selectionPushButton, 0, 0, 0);
            disconnect(m_eraserPushButton, 0, 0, 0);
            disconnect(m_fillPushButton, 0, 0, 0);
            disconnect(m_arrowPushButton, 0, 0, 0);
            disconnect(m_colorPickerPushButton, 0, 0, 0);
        }

        void WhiteBoardToolBar::clearClicked()
        {
            //TODO ask for confirm
            emit clear();
        }

        void WhiteBoardToolBar::saveClicked()
        {
            QPointer<QFileDialog> fileDialog = new QFileDialog(this, i18n("Save Image"), QDir::homePath(), i18n("Images (*.png *.xpm *.jpg)"));
            fileDialog->setAcceptMode(QFileDialog::AcceptSave);
            fileDialog->setFileMode(QFileDialog::AnyFile);

            int ret = fileDialog->exec();

            if (ret == QDialog::Accepted && fileDialog)
            {
                QStringList saveList = fileDialog->selectedFiles();
                qDebug() << saveList;
                if (saveList.count() > 0)
                emit save(saveList.at(0));
            }
            delete fileDialog;
        }

        void WhiteBoardToolBar::colorPickerToggled(bool checked)
        {
            handleToggleButton(m_colorPickerPushButton, checked, WhiteBoardGlobals::ColorPicker);
            setLineWidthVisible(false);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::arrowToggled (bool checked)
        {
            handleToggleButton(m_arrowPushButton, checked, WhiteBoardGlobals::Arrow);
            setLineWidthVisible(true);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::ellipseToggled(bool checked)
        {
            handleToggleButton(m_ellipsePushButton, checked, WhiteBoardGlobals::Ellipse);
            setLineWidthVisible(true);
            setFormOptionVisible(true);
            fillFormOptionList(Ellipse);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::eraseToggled(bool checked)
        {
            handleToggleButton(m_eraserPushButton, checked, WhiteBoardGlobals::Eraser);
            setLineWidthVisible(true);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::fillToggled(bool checked)
        {
            handleToggleButton(m_fillPushButton, checked, WhiteBoardGlobals::FloodFill);
            setLineWidthVisible(false);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::lineToggled(bool checked)
        {
            handleToggleButton(m_linePushButton, checked, WhiteBoardGlobals::Line);
            setLineWidthVisible(true);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::pencilToggled(bool checked)
        {
            handleToggleButton(m_pencilPushButton, checked, WhiteBoardGlobals::Pencil);
            setLineWidthVisible(true);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::rectangleToggled(bool checked)
        {
            handleToggleButton(m_rectanglePushButton, checked, WhiteBoardGlobals::Rectangle);
            setLineWidthVisible(true);
            setFormOptionVisible(true);
            fillFormOptionList(Rectangle);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::selectionToggled(bool checked)
        {
            handleToggleButton(m_selectionPushButton, checked, WhiteBoardGlobals::Selection);
            setLineWidthVisible(false);
            setFormOptionVisible(false);
            setFontDialogVisible(false);
        }

        void WhiteBoardToolBar::textToggled(bool checked)
        {
            if (textType() == WhiteBoardToolBar::SimpleText)
            {
                handleToggleButton(m_textPushButton, checked, WhiteBoardGlobals::Text);
            }
            else
            {
                handleToggleButton(m_textPushButton, checked, WhiteBoardGlobals::TextExtended);
            }
            setLineWidthVisible(false);
            setFormOptionVisible(false);
            setFontDialogVisible(true);
        }

        void WhiteBoardToolBar::handleToggleButton(QPushButton* button, bool checked, Konversation::DCC::WhiteBoardGlobals::WhiteBoardTool tool)
        {
            disconnectToggleButtons();
            qDebug() << "tool:" << tool << "checked:" << checked;
            if (checked)
            {
                unCheckOtherButtons(button);
                emit toolChanged(tool);
            }
            else
            {
                // don't uncheck the button
                button->setChecked(true);
            }
            connectToggleButtons();
        }

        void WhiteBoardToolBar::unCheckOtherButtons(QPushButton* button)
        {
            foreach(QPushButton* pushButton, m_toggleButtonHash)
            {
                if (pushButton != button && pushButton->isChecked())
                {
                    pushButton->setChecked(false);
                    return;
                }
            }
        }

        void WhiteBoardToolBar::updateLineWidthPixmap(int lineWidth)
        {
            if (m_lineWidthLabel->width() != m_lineWidthPixmap.width()-2 ||
                m_lineWidthLabel->height() != m_lineWidthPixmap.height())
            {
                m_lineWidthPixmap = QPixmap(m_lineWidthLabel->width()-2, m_lineWidthLabel->height());
            }
            //hm.. really white? or is transparent better?
            m_lineWidthPixmap.fill(Qt::white);

            QPainter tPaint(&m_lineWidthPixmap);
            tPaint.setPen(QPen(Qt::black, lineWidth));
            tPaint.drawLine(0, m_lineWidthPixmap.height()/2, m_lineWidthPixmap.width(), m_lineWidthPixmap.height()/2);
            tPaint.end();
            m_lineWidthLabel->setPixmap(m_lineWidthPixmap);
        }

        void WhiteBoardToolBar::formSelectionChanged()
        {
            // qDebug();
            QList<QListWidgetItem *> selectList = m_formOptionListWidget->selectedItems();
            const int selectedRow = m_formOptionListWidget->row(selectList.first());
            if (selectedRow == 0)
            {
                if (m_rectanglePushButton->isChecked())
                {
                    qDebug() << "emit rectangle";
                    emit toolChanged(WhiteBoardGlobals::Rectangle);
                }
                else if (m_ellipsePushButton->isChecked())
                {
                    qDebug() << "emit ellipse";
                    emit toolChanged(WhiteBoardGlobals::Ellipse);
                }
            }
            else if (selectedRow == 1)
            {
                if (m_rectanglePushButton->isChecked())
                {
                    qDebug() << "emit filledrectangle";
                    emit toolChanged(WhiteBoardGlobals::FilledRectangle);
                }
                else if (m_ellipsePushButton->isChecked())
                {
                    qDebug() << "emit filledellipse";
                    emit toolChanged(WhiteBoardGlobals::FilledEllipse);
                }
            }
        }

        void WhiteBoardToolBar::setLineWidthVisible(bool visible)
        {
            m_lineWidthFrame->setVisible(visible);
        }

        void WhiteBoardToolBar::setFormOptionVisible(bool visible)
        {
            m_formOptionListWidget->setVisible(visible);
        }

        void WhiteBoardToolBar::setFontDialogVisible(bool visible)
        {
            if (m_textType == WhiteBoardToolBar::ExtentedText && m_fontDialog)
            {
                if (visible)
                    m_fontDialog->show();
                else
                    m_fontDialog->hide();
            }
        }

        void WhiteBoardToolBar::fillFormOptionList(FormOption form)
        {
            disconnect(m_formOptionListWidget, 0, 0, 0);
            m_formOptionListWidget->clear();
            const int width = m_formOptionListWidget->contentsRect().width() - m_formOptionListWidget->lineWidth()*4 - 3;
            const int drawHeight = 20 - 2;
            const int widthLayoutOffset = 2;
            const QSize sizeHint(width, 20);
            // qDebug() << "wanted width" << width;
            // qDebug() << "actual width" << m_formOptionListWidget->contentsRect().width();
            switch (form)
            {
                case Rectangle:
                {
                    if (m_rectanglePixmap.width() != width ||
                        m_rectanglePixmap.height() != 20)
                    {
                        m_rectanglePixmap = QPixmap(width, 20);
                        m_rectanglePixmap.fill(Qt::transparent);
                        m_filledRectanglePixmap = QPixmap(width, 20);
                        m_filledRectanglePixmap.fill(Qt::transparent);
                        QPainter tPaint(&m_rectanglePixmap);
                        tPaint.drawRect(0, 0, width-widthLayoutOffset, drawHeight);
                        tPaint.end();
                        tPaint.begin(&m_filledRectanglePixmap);
                        tPaint.setBrush(Qt::black);
                        tPaint.drawRect(0, 0, width-widthLayoutOffset, drawHeight);
                        tPaint.end();
                    }
                    QListWidgetItem *tRectangle = new QListWidgetItem(QString(), m_formOptionListWidget, QListWidgetItem::UserType +1);
                    tRectangle->setData(Qt::DecorationRole, QVariant(m_rectanglePixmap));
                    tRectangle->setSizeHint(sizeHint);

                    QListWidgetItem *tFilledRectangle = new QListWidgetItem(QString(), m_formOptionListWidget, QListWidgetItem::UserType +1);
                    tFilledRectangle->setData(Qt::DecorationRole, QVariant(m_filledRectanglePixmap));
                    tFilledRectangle->setSizeHint(sizeHint);

                    tRectangle->setSelected(true);
                    break;
                }
                case Ellipse:
                {
                    if (m_ellipsePixmap.width() != width ||
                        m_ellipsePixmap.height() != 20)
                    {
                        m_ellipsePixmap = QPixmap(width, 20);
                        m_ellipsePixmap.fill(Qt::transparent);
                        m_filledEllipsePixmap = QPixmap(width, 20);
                        m_filledEllipsePixmap.fill(Qt::transparent);
                        QPainter tPaint(&m_ellipsePixmap);
                        tPaint.drawEllipse(0, 0, width-widthLayoutOffset, drawHeight);
                        tPaint.end();
                        tPaint.begin(&m_filledEllipsePixmap);
                        tPaint.setBrush(Qt::black);
                        tPaint.drawEllipse(0, 0, width-widthLayoutOffset, drawHeight);
                        tPaint.end();
                    }
                    QListWidgetItem *tEllipse = new QListWidgetItem(QString(), m_formOptionListWidget, QListWidgetItem::UserType +1);
                    tEllipse->setData(Qt::DecorationRole, QVariant(m_ellipsePixmap));
                    tEllipse->setSizeHint(sizeHint);

                    QListWidgetItem *tFilledEllipse = new QListWidgetItem(QString(), m_formOptionListWidget, QListWidgetItem::UserType +1);
                    tFilledEllipse->setData(Qt::DecorationRole, QVariant(m_filledEllipsePixmap));
                    tFilledEllipse->setSizeHint(sizeHint);

                    tEllipse->setSelected(true);
                    break;
                }
            }
            connect(m_formOptionListWidget, &QListWidget::itemSelectionChanged, this, &WhiteBoardToolBar::formSelectionChanged);
        }
    }
}
