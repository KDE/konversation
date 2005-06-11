/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Shows the content of a log file
  begin:     Fri Dec 5 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef LOGFILEREADER_H
#define LOGFILEREADER_H

#include "chatwindow.h"

#include <kio/job.h>

/*
  @author Dario Abatianni
*/

class KToolBar;
class QSpinBox;

class LogfileReader : public ChatWindow
{
  Q_OBJECT

  public:
    LogfileReader(QWidget* parent, QString log);
    ~LogfileReader();

    virtual bool closeYourself() { closeLog(); return true; }
    virtual bool searchView();
  
  protected slots:
    void updateView();
    void clearLog();
    void saveLog();
    void closeLog();
    void copyResult(KIO::Job* job);
  
  protected:
    int margin();
    int spacing();

    /** Called from ChatWindow adjustFocus */
    virtual void childAdjustFocus();

    KToolBar* toolBar;
    QSpinBox* sizeSpin;
    QString fileName;
};

#endif
