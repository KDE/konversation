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
#include <qpushbutton.h>
#include <qlabel.h>
#include <qregexp.h>
#include <qspinbox.h>
#include <QTextCodec>
#include <QKeyEvent>

#include <kdialog.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kdebug.h>
#include <kio/copyjob.h>
#include <kio/jobclasses.h>
#include <KJobUiDelegate>


LogfileReader::LogfileReader(QWidget* parent, const QString& log) : ChatWindow(parent)
{
    setType(ChatWindow::LogFileReader);

    fileName = log;

    toolBar = new KToolBar(this, true, true);
    toolBar->setObjectName("logfile_toolbar");
    toolBar->addAction(KIcon("document-save-as"), i18n("Save As..."), this, SLOT(saveLog()));
    toolBar->addAction(KIcon("view-refresh"), i18n("Reload"), this, SLOT(updateView()));
    toolBar->addAction(KIcon("edit-delete"), i18n("Clear Logfile"), this, SLOT(clearLog()));

    toolBar->addWidget(new QLabel(i18n("Show last:"),toolBar));
    sizeSpin = new QSpinBox(toolBar);
    sizeSpin->setMinimum(10);
    sizeSpin->setMaximum(1000);
    sizeSpin->setSingleStep(10);
    sizeSpin->setObjectName("logfile_size_spinbox");
    sizeSpin->setWhatsThis(i18n("Use this box to set the maximum size of the log file. This setting does not take effect until you restart Konversation. Each log file may have a separate setting."));
    sizeSpin->setValue(Preferences::self()->logfileBufferSize());
    sizeSpin->setSuffix(i18n(" KB"));
    sizeSpin->installEventFilter(this);
    toolBar->addWidget(sizeSpin);

    IRCViewBox* ircBox = new IRCViewBox(this, 0);
    setTextView(ircBox->ircView());
    getTextView()->setWhatsThis(i18n("The messages in the log file are displayed here. The oldest messages are at the top and the most recent are at the bottom."));

    updateView();
    resize(Preferences::self()->logfileReaderSize());
    ircBox->ircView()->setFocusPolicy(Qt::StrongFocus);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(ircBox->ircView());

    connect(getTextView(), SIGNAL(gotFocus()), getTextView(), SLOT(setFocus()));
}

LogfileReader::~LogfileReader()
{
    Preferences::self()->setLogfileReaderSize(size());
    Preferences::self()->setLogfileBufferSize(sizeSpin->value());

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
    qint64 pos = Q_INT64_C(1024) * sizeSpin->value();
    getTextView()->clear();

    QFile file(fileName);

    if(file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForName("UTF-8"));
        stream.setAutoDetectUnicode(true);

        // Set file pointer to <pos> bytes from the end
        if(stream.device()->size()>pos)
            stream.device()->seek(stream.device()->size()-pos);
        // Skip first line, since it may be incomplete
        stream.readLine();
        QString str;

        while(!stream.atEnd())
        {
            str = Qt::escape(stream.readLine());
            getTextView()->appendRaw(str, true);
        }

        stream.setDevice(0);
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
        KIO::Job* job=KIO::copy(KUrl(fileName),
            KUrl(destination));

        connect(job,SIGNAL(result(KJob*)),this,SLOT(copyResult(KJob*)));
    }
}

void LogfileReader::copyResult(KJob* job)
{
    if(job->error()) job->uiDelegate()->showErrorMessage();

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
