/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  logfilereader.cpp  -  Shows the content of a log file
  begin:     Fri Dec 5 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qlayout.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qspinbox.h>

#include <kdialog.h>
#include <ktextbrowser.h>
#include <klocale.h>
#include <kdebug.h>

#include "logfilereader.h"
#include "konversationapplication.h"

LogfileReader::LogfileReader(QString caption,QString log) :
                      QFrame(0)
{
  setCaption(i18n("Logfile of %1").arg(caption));

  QGridLayout* mainLayout=new QGridLayout(this,2,4,0,spacing());
  fileName=log;

  view=new KTextBrowser(this);
  QPushButton* saveButton=new QPushButton(i18n("Save Logfile ..."),this,"logfile_save_button");

  QLabel* sizeLabel=new QLabel(i18n("Show last"),this,"logfile_size_label");

  sizeSpin=new QSpinBox(10,1000,10,this,"logfile_size_spinbox");
  sizeSpin->setValue(102400/1024);  // KonversationApplication.preferences.getLogfileBufferSize();
  sizeSpin->setSuffix(i18n(" KB"));

  QPushButton* reloadButton=new QPushButton(i18n("Reload"),this,"logfile_reload_button");

  updateView();

  int row=0;

  mainLayout->addMultiCellWidget(view,row,row,0,3);

  row++;
  mainLayout->addWidget(saveButton,row,0);
  mainLayout->addWidget(sizeLabel,row,1);
  mainLayout->addWidget(sizeSpin,row,2);
  mainLayout->addWidget(reloadButton,row,3);
}

void LogfileReader::updateView()
{
  unsigned long pos=sizeSpin->value()*1024;
  view->clear();

  QFile file(fileName);

  if(file.open(IO_ReadOnly))
  {
    QTextStream stream(&file);
    stream.setEncoding(QTextStream::UnicodeUTF8);

    // Set file pointer to <pos> bytes from the end
    if(stream.device()->size()>pos)
      stream.device()->at(stream.device()->size()-pos);
    // Skip first line, since it may be incomplete
    stream.readLine();

    view->setText(stream.read());

    stream.unsetDevice();
    file.close();
  }
}

LogfileReader::~LogfileReader()
{
  delete view;
}

int LogfileReader::margin() { return KDialog::marginHint(); }
int LogfileReader::spacing() { return KDialog::spacingHint(); }

#include "logfilereader.moc"
