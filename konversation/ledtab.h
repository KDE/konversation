/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ledtab.h  -  The tabs with LEDs on them
  begin:     Fri Feb 22 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/


#ifndef LEDTAB_H
#define LEDTAB_H

#include <qobject.h>
#include <qtabbar.h>
#include <qtimer.h>

#include "images.h"

/*
  @author Dario Abatianni
*/

class LedTab : public QObject,public QTab
{
  Q_OBJECT

  public:
    LedTab(QWidget* newWidget,const QString& text,int newColor,bool state);
    ~LedTab();

    void setOn(bool state);
    void setLabelColor(const QString& newLabelColor);
    const QString& getLabelColor();

    QWidget* getWidget();

    QIconSet iconOn;
    QIconSet iconOff;
    QTimer blinkTimer;

  signals:
    void repaintTab(LedTab* myself);

  protected slots:
    void blinkTimeout();

  protected:
    void setIconSet(const QIconSet& icon);

    int color;      // color of the LED
    bool on;        // true, if LED should indicate "on" status
    bool blinkOn;   // true, if blinking LED is on at this moment

    QWidget* widget;
    Images images;
    QString labelColor;

};

#endif
