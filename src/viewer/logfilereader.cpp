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
#include "application.h" ////// header renamed
#include "ircview.h"
#include "ircviewbox.h"

#include <qlayout.h>
#include <qfile.h>
#include <q3textstream.h>
#include <q3dockarea.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <q3stylesheet.h>
#include <q3whatsthis.h>
//Added by qt3to4:
#include <QEvent>
#include <QKeyEvent>

#include <kdialog.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/copyjob.h>
#include <kio/jobclasses.h>
#include <QTextDocument>


LogfileReader::LogfileReader(QWidget* parent, const QString& log) : ChatWindow(parent)
{
    setType(ChatWindow::LogFileReader);

    fileName = log;
    Q3DockArea* toolBarDock = new Q3DockArea(Qt::Horizontal,Q3DockArea::Normal,this,"logfile_toolbar_dock");
    toolBar = new KToolBar(toolBarDock, true, true);
    toolBar->setObjectName("logfile_toolbar");
    toolBar->addAction(KIcon("filesaveas"), i18n("Save As..."), this, SLOT(saveLog()));

    new QLabel(i18n("Show last:"),toolBar,"logfile_size_label");
    sizeSpin = new QSpinBox(10,1000,10,toolBar,"logfile_size_spinbox");
    Q3WhatsThis::add(sizeSpin, i18n("Use this box to set the maximum size of the log file. This setting does not take effect until you restart Konversation. Each log file may have a separate setting."));
    sizeSpin->setValue(Preferences::logfileBufferSize());
    sizeSpin->setSuffix(i18n(" KB"));
    sizeSpin->installEventFilter(this);

    toolBar->addAction(KIcon("reload"), i18n("Reload"), this, SLOT(updateView()));
    toolBar->addAction(KIcon("editdelete"), i18n("Clear Logfile"), this, SLOT(clearLog()));

    IRCViewBox* ircBox = new IRCViewBox(this, 0);
    setTextView(ircBox->ircView());
    Q3WhatsThis::add(getTextView(), i18n("The messages in the log file are displayed here. The oldest messages are at the top and the most recent are at the bottom."));

    updateView();
    resize(Preferences::logfileReaderSize());
    ircBox->ircView()->setFocusPolicy(Qt::StrongFocus);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(ircBox->ircView());

    connect(getTextView(), SIGNAL(gotFocus()), getTextView(), SLOT(setFocus()));
}

LogfileReader::~LogfileReader()
{
    Preferences::setLogfileReaderSize(size());
    Preferences::setLogfileBufferSize(sizeSpin->value());

    delete toolBar;
}

bool LogfileReader::eventFilter(QObject* /* watched */, QEvent* e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
        {
            updateView();

            return true;
        }
        else
            return false;
    }

    return false;
}

void LogfileReader::updateView()
{
    // get maximum size of logfile to display
    unsigned long pos=sizeSpin->value()*1024;
    getTextView()->clear();

    QFile file(fileName);

    if(file.open(QIODevice::ReadOnly))
    {
        Q3TextStream stream(&file);
        stream.setEncoding(Q3TextStream::UnicodeUTF8);

        // Set file pointer to <pos> bytes from the end
        if(stream.device()->size()>pos)
            stream.device()->at(stream.device()->size()-pos);
        // Skip first line, since it may be incomplete
        stream.readLine();
        QString str;

        while(!stream.eof())
        {
            str = Qt::escape(stream.readLine());
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
        KStandardGuiItem::del(),
        KStandardGuiItem::cancel(),
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
        KIO::Job* job=KIO::copy(KUrl(fileName.replace("#","%23")),
            KUrl(destination));

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

// #include "./viewer/logfilereader.moc"
