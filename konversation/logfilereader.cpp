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
#include <qdockarea.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qspinbox.h>

#include <kdialog.h>
#include <ktoolbar.h>
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
  QDockArea* toolBarDock=new QDockArea(Qt::Horizontal,QDockArea::Normal,this,"logfile_toolbar_dock"); 
  toolBar=new KToolBar(toolBarDock,"logfile_toolbar",true,true);
  
  toolBar->insertButton("filesaveas",0,SIGNAL(clicked()),this,SLOT(saveLog()),true,i18n("Save as..."));
  
  new QLabel(i18n("Show last"),toolBar,"logfile_size_label");
  sizeSpin=new QSpinBox(10,1000,10,toolBar,"logfile_size_spinbox");
  sizeSpin->setValue(KonversationApplication::preferences.getLogfileBufferSize());
  sizeSpin->setSuffix(i18n(" KB"));

  toolBar->insertButton("reload",0,SIGNAL(clicked()),this,SLOT(updateView()),true,i18n("Reload"));
  toolBar->insertButton("fileclose",0,SIGNAL(clicked()),this,SLOT(closeLog()),true,i18n("Close"));
  
  view=new KTextBrowser(this);
  
  updateView();

  int row=0;

  mainLayout->addMultiCellWidget(toolBarDock,row,row,0,3);
  
  row++;
  mainLayout->addMultiCellWidget(view,row,row,0,3);

  resize(KonversationApplication::preferences.getLogfileReaderSize());
}

LogfileReader::~LogfileReader()
{
  KonversationApplication::preferences.setLogfileReaderSize(size());
  KonversationApplication::preferences.setLogfileBufferSize(sizeSpin->value());
  
  delete view;
  delete toolBar;
}

void LogfileReader::updateView()
{
  // get maximum size of logfile to display
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

// make sure that the widget gets closed when user presses the window manager's [x] button
void LogfileReader::hide()
{
  closeLog();
}

void LogfileReader::closeLog()
{
  delete this;
}

int LogfileReader::margin() { return KDialog::marginHint(); }
int LogfileReader::spacing() { return KDialog::spacingHint(); }

#include "logfilereader.moc"
