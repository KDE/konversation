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
*/


#ifndef LEDTAB_H
#define LEDTAB_H

#include <qobject.h>
#include <qtabbar.h>
#include <qtimer.h>

/*
  @author Dario Abatianni
*/

class LedTab : public QObject,public QTab
{
  Q_OBJECT

  public:
    LedTab(QWidget* newWidget,const QString& text,int newColor,bool on);
    ~LedTab();
    
    void setOn(bool on, bool important=true);
    void setLabelColor(const QString& newLabelColor);
    const QString& getLabelColor();

    QWidget* getWidget();
    int getColor();

    bool getOnline();
    void setOnline(bool state);

    QIconSet iconOn;
    QIconSet iconOff;
    QTimer blinkTimer;

  signals:
    void repaintTab(LedTab* myself);

  protected slots:
    void blinkTimeout();

  protected:
    enum StateType
    {
      Off=0,
      Slow,
      Fast
    };

    void setIconSet(const QIconSet& icon);

    int color;      // color of the LED
    StateType state;// if and how fast the LED should blink
    bool blinkOn;   // true, if blinking LED is on at this moment
    bool online;    // if false label should be crossed out

    QWidget* widget;
    QString labelColor;
};

#endif
