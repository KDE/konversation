/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  RawLog.h  -  provides a view to the raw protocol
  begin:     Die Mär 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef _RAWLOG_H_
#define _RAWLOG_H_

#include "chatwindow.h"

/*
  Dario Abatianni
*/

class RawLog : public ChatWindow
{
  Q_OBJECT

  public:
#ifdef USE_MDI
    RawLog(QString caption);
#else
    RawLog(QWidget* parent);
#endif
    ~RawLog();

    virtual void closeYourself();
    virtual bool searchView();

  public slots:
    void updateFonts();

  protected slots:
    void serverQuit(const QString& reasoon); // USE_MDI

#ifdef USE_MDI
  protected:
    virtual void closeYourself(ChatWindow* view);
#endif
    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();
};

#endif
