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

#include "logfilereader.h"
#include "konversationapplication.h"
#include "ircview.h"
#include "ircviewbox.h"

#include <qlayout.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qdockarea.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <qstylesheet.h>
#include <qwhatsthis.h>

#include <kdialog.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/jobclasses.h>


LogfileReader::LogfileReader(QWidget* parent, const QString& log) : ChatWindow(parent)
{
    setType(ChatWindow::LogFileReader);

    fileName = log;
    QDockArea* toolBarDock = new QDockArea(Qt::Horizontal,QDockArea::Normal,this,"logfile_toolbar_dock");
    toolBar = new KToolBar(toolBarDock,"logfile_toolbar",true,true);

    toolBar->insertButton("filesaveas",0,SIGNAL(clicked()),this,SLOT(saveLog()),true,i18n("Save As..."));

    new QLabel(i18n("Show last:"),toolBar,"logfile_size_label");
    sizeSpin = new QSpinBox(10,1000,10,toolBar,"logfile_size_spinbox");
    QWhatsThis::add(sizeSpin, i18n("Use this box to set the maximum size of the log file. This setting does not take effect until you restart Konversation. Each log file may have a separate setting."));
    sizeSpin->setValue(Preferences::logfileBufferSize());
    sizeSpin->setSuffix(i18n(" KB"));

    toolBar->insertButton("reload",0,SIGNAL(clicked()),this,SLOT(updateView()),true,i18n("Reload"));
    toolBar->insertButton("editdelete",0,SIGNAL(clicked()),this,SLOT(clearLog()),true,i18n("Clear Logfile"));

    IRCViewBox* ircBox = new IRCViewBox(this, 0);
    setTextView(ircBox->ircView());
    QWhatsThis::add(getTextView(), i18n("The messages in the log file are displayed here. The oldest messages are at the top and the most recent are at the bottom."));

    updateView();
    resize(Preferences::logfileReaderSize());
    ircBox->ircView()->setFocusPolicy(QWidget::StrongFocus);
    setFocusPolicy(QWidget::StrongFocus);
    setFocusProxy(ircBox->ircView());
}

LogfileReader::~LogfileReader()
{
    Preferences::setLogfileReaderSize(size());
    Preferences::setLogfileBufferSize(sizeSpin->value());

    delete toolBar;
}

void LogfileReader::updateView()
{
    // get maximum size of logfile to display
    unsigned long pos=sizeSpin->value()*1024;
    getTextView()->clear();

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
        QString str;

        while(!stream.eof())
        {
            str = QStyleSheet::escape(stream.readLine());
            getTextView()->appendRaw(str, true);
        }

        stream.unsetDevice();
        file.close();
    }
}

void LogfileReader::clearLog()
{
    if(KMessageBox::warningContinueCancel(this,
        i18n("Do you really want to permanently discard all log information of this file?"),
        i18n("Clear Logfile"),
        KStdGuiItem::del(),
        "ClearLogfileQuestion")==KMessageBox::Continue)
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
        QString(),
        this,
        i18n("Choose Destination Folder"));
    if(!destination.isEmpty())
    {
        // replace # with %25 to make it URL conforming
        KIO::Job* job=KIO::copy(KURL(fileName.replace("#","%23")),
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
  getTextView()->setFocus();
}

int LogfileReader::margin() { return KDialog::marginHint(); }
int LogfileReader::spacing() { return KDialog::spacingHint(); }
bool LogfileReader::searchView() { return true; }

#include "logfilereader.moc"
