/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  osd.cpp  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Fre Sep 26 2003
  copyright: (C) 2003 by Christian Muehlhaeuser
  email:     muesli@chareit.net
*/

#include "osd.h"

#include <qtimer.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qpixmap.h>
#include <qbitmap.h>

#include <kcolorcombo.h>

OSDWidget::OSDWidget() : QWidget(NULL, "osd",
                                 WType_TopLevel | WStyle_StaysOnTop |
                                 WStyle_Customize | WStyle_NoBorder |
                                 WStyle_Tool | WRepaintNoErase | WX11BypassWM)
{
  // Get desktop dimensions
  QWidget *d = QApplication::desktop();
  int desktop_w = d->width();

  // Currently fixed to the top border of desktop, full width
  // This should be configurable, already on my TODO
  move(5, 5);
  resize(desktop_w - 10, 120);
  setFocusPolicy(NoFocus);
  timer = new QTimer(this);
}

void OSDWidget::showOSD(const QString &text)
{
  if (isEnabled())
  {
    if (timer->isActive()) timer->stop();

    // Strip HTML tags, expand basic HTML entities
    QString plaintext = text.copy();
    plaintext.replace(QRegExp("</?(?:font|a|b|i)\\b[^>]*>"), QString(""));
    plaintext.replace(QString("&lt;"), QString("<"));
    plaintext.replace(QString("&gt;"), QString(">"));
    plaintext.replace(QString("&amp;"), QString("&"));

    this->text = plaintext;

    // Repaint the QWidget and get it on top
    QWidget::show();
    repaint();
    raise();

    // let it disappear via a QTimer
    connect(timer, SIGNAL(timeout()), this, SLOT(removeOSD()));
    timer->start(5000, TRUE);
  }
}

void OSDWidget::setFont(QFont newfont)
{
  font = newfont;
}

void OSDWidget::setColor(QColor newcolor)
{
  color = newcolor;
}

void OSDWidget::removeOSD()
{
  // hide() and show() prevents flickering
  hide();
  this->text = "";
}

void OSDWidget::paintEvent(QPaintEvent*)
{
  QPainter paint;
  QPixmap *buffer = new QPixmap(width(), height());
  QColor bg(0, 0, 0);
  QColor fg(255, 255, 255);

  qApp->syncX();

  // Draw the OnScreenMessage
  QPainter paintBuffer(buffer, this);
  paintBuffer.setFont(font);

  // Draw the border around the text
  paintBuffer.setPen(black);
  paintBuffer.drawText(0, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paintBuffer.drawText(2, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paintBuffer.drawText(0, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paintBuffer.drawText(2, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text);

  // Draw the text
  paintBuffer.setPen(color);
  paintBuffer.drawText(1, 1, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paintBuffer.end();

  // Masking for transparency
  QBitmap bm(size());
  bm.fill(bg);
  paint.begin(&bm, this);
  paint.setPen(Qt::color0);
  paint.setFont(font);
  paint.drawText(0, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paint.drawText(1, 1, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paint.drawText(2, 0, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paint.drawText(0, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paint.drawText(2, 2, width()-1, height()-1, AlignLeft | WordBreak, this->text);
  paint.end();

  // Let's make it real, flush the buffers
  bitBlt(this, 0, 0, buffer);
  setMask(bm);

  delete buffer;
}

#include "osd.moc"
