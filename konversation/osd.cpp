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

    this->text = text;

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

void OSDWidget::removeOSD()
{
  // hide() and show() prevents flickering
  hide();
  this->text = "";
}

void OSDWidget::paintEvent(QPaintEvent*)
{
  QPainter paint;
  QColor bg(0, 0, 0);
  QColor fg(255, 255, 255);

  qApp->syncX();
  QBitmap bm(size());
  bm.fill(bg);
  paint.begin(&bm, this);

  // Draw the text. Coloring doesn't work right now. It's on my TODO
  paint.setBrush(fg);
  paint.setPen(fg);
  paint.setFont(font);
  paint.drawText(rect(), AlignLeft | WordBreak, this->text);

  paint.end();
  setMask(bm);
}

#include "osd.moc"
