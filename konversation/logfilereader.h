/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  logfilereader.h  -  Shows the content of a log file
  begin:     Fri Dec 5 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef LOGFILEREADER_H
#define LOGFILEREADER_H

#include <qframe.h>

#include <kio/job.h>

/*
  @author Dario Abatianni
*/

class KToolBar;
class KTextBrowser;
class QSpinBox;

class LogfileReader : public QFrame
{
  Q_OBJECT

  public:
    LogfileReader(QString caption,QString log);
    ~LogfileReader();

    void hide();
  
  protected slots:
    void updateView();
    void clearLog();
    void saveLog();
    void closeLog();
    void copyResult(KIO::Job* job);
  
  protected:
    int margin();
    int spacing();

    KToolBar* toolBar;
    KTextBrowser* view;
    QSpinBox* sizeSpin;
    QString fileName;
};

#endif
