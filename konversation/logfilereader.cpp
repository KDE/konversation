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
#include <qregexp.h>
#include <qspinbox.h>

#include <kdialog.h>
#include <ktoolbar.h>
#include <ktextbrowser.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>

#include <kio/jobclasses.h>

#include "logfilereader.h"
#include "konversationapplication.h"

#ifdef USE_MDI
LogfileReader::LogfileReader(QString caption, QString log) : ChatWindow(caption)
#else
LogfileReader::LogfileReader(QWidget* parent, QString log) : ChatWindow(parent)
#endif
{
  //setName(i18n("Logfile of %1").arg(caption));
  setType(ChatWindow::LogFileReader);

  fileName = log;
  QDockArea* toolBarDock = new QDockArea(Qt::Horizontal,QDockArea::Normal,this,"logfile_toolbar_dock"); 
  toolBar = new KToolBar(toolBarDock,"logfile_toolbar",true,true);
  
  toolBar->insertButton("filesaveas",0,SIGNAL(clicked()),this,SLOT(saveLog()),true,i18n("Save As..."));
  
  new QLabel(i18n("Show last:"),toolBar,"logfile_size_label");
  sizeSpin = new QSpinBox(10,1000,10,toolBar,"logfile_size_spinbox");
  sizeSpin->setValue(KonversationApplication::preferences.getLogfileBufferSize());
  sizeSpin->setSuffix(i18n(" KB"));

  toolBar->insertButton("reload",0,SIGNAL(clicked()),this,SLOT(updateView()),true,i18n("Reload"));
  toolBar->insertButton("editdelete",0,SIGNAL(clicked()),this,SLOT(clearLog()),true,i18n("Clear Logfile"));
  
  view = new KTextBrowser(this);
  
  updateView();
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

void LogfileReader::clearLog()
{
  if(KMessageBox::questionYesNo(this,
                                i18n("Do you really want to permanently discard all log information of this file?"),
                                i18n("Clear Logfile"),
                                KStdGuiItem::yes(),
                                KStdGuiItem::no(),
                                "ClearLogfileQuestion")==KMessageBox::Yes)
  {
    QFile::remove(fileName);
    updateView();
  }
}

void LogfileReader::saveLog()
{
  KMessageBox::information(this,
                           i18n("Note: By saving the logfile you will save all data in the file, not only the part you can see in this viewer."),
                           i18n("Save Logfile"),
                           "SaveLogfileNote");
  
  QString destination=KFileDialog::getSaveFileName(fileName,
                                                   QString::null,
                                                   this,
                                                   i18n("Choose Destination Folder"));
  if(!destination.isEmpty())
  {
    // replace # with %25 to make it URL conforming
    KIO::Job* job=KIO::copy(KURL(fileName.replace(QRegExp("#"),"%23")),
                            KURL(destination),
                            true);
    
    connect(job,SIGNAL(result(KIO::Job*)),this,SLOT(copyResult(KIO::Job*)));
  }
}

void LogfileReader::copyResult(KIO::Job* job)
{
  if(job->error()) job->showErrorDialog(this);
 
  job->deleteLater();
}

void LogfileReader::closeLog()
{
  delete this;
}

void LogfileReader::childAdjustFocus()
{
}

int LogfileReader::margin() { return KDialog::marginHint(); }
int LogfileReader::spacing() { return KDialog::spacingHint(); }

#include "logfilereader.moc"
