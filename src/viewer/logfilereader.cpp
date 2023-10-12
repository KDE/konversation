/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2003 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "logfilereader.h"
#include "application.h"
#include "ircview.h"
#include "ircviewbox.h"

#include <QFile>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QTextCodec>
#include <QKeyEvent>

#include <KToolBar>
#include <KMessageBox>
#include <QFileDialog>
#include <KLocalizedString>
#include <KIO/CopyJob>
#include <KJobUiDelegate>


LogfileReader::LogfileReader(QWidget* parent, const QString& log, const QString& caption) : ChatWindow(parent)
{
    setType(ChatWindow::LogFileReader);
    setName(i18n("Logfile of %1", caption));

    fileName = log;

    setSpacing(0);

    toolBar = new KToolBar(this, true, true);
    toolBar->setObjectName(QStringLiteral("logfile_toolbar"));
    toolBar->addAction(QIcon::fromTheme(QStringLiteral("document-save-as")), i18n("Save As..."), this, &LogfileReader::saveLog);
    toolBar->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Reload"), this, &LogfileReader::updateView);
    toolBar->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Clear Logfile"), this, &LogfileReader::clearLog);

    toolBar->addWidget(new QLabel(i18n("Show last:"),toolBar));
    sizeSpin = new QSpinBox(toolBar);
    sizeSpin->setMinimum(10);
    sizeSpin->setMaximum(1000);
    sizeSpin->setSingleStep(10);
    sizeSpin->setObjectName(QStringLiteral("logfile_size_spinbox"));
    sizeSpin->setWhatsThis(i18n("Use this box to set the maximum size of the log file. This setting does not take effect until you restart Konversation. Each log file may have a separate setting."));
    sizeSpin->setValue(Preferences::self()->logfileBufferSize());
    sizeSpin->setSuffix(i18n(" KB"));
    sizeSpin->installEventFilter(this);
    toolBar->addWidget(sizeSpin);
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LogfileReader::storeBufferSize);

    auto* ircBox = new IRCViewBox(this);
    setTextView(ircBox->ircView());
    getTextView()->setWhatsThis(i18n("The messages in the log file are displayed here. The oldest messages are at the top and the most recent are at the bottom."));

    updateView();
    ircBox->ircView()->setFocusPolicy(Qt::StrongFocus);
    setFocusPolicy(Qt::StrongFocus);
    setFocusProxy(ircBox->ircView());

    updateAppearance();

    connect(getTextView(), &IRCView::gotFocus, getTextView(), QOverload<>::of(&IRCView::setFocus));
}

LogfileReader::~LogfileReader()
{
}

bool LogfileReader::eventFilter(QObject* watched, QEvent* e)
{
    if (e->type() == QEvent::KeyPress)
    {
        auto* ke = static_cast<QKeyEvent*>(e);

        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
        {
            updateView();

            return true;
        }
        else
            return false;
    }

    return ChatWindow::eventFilter(watched, e);
}

void LogfileReader::storeBufferSize(int kb)
{
    Preferences::self()->setLogfileBufferSize(kb);
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
        stream.setAutoDetectUnicode(true);

        // Set file pointer to <pos> bytes from the end
        if(stream.device()->size() > pos)
        {
            stream.device()->seek(stream.device()->size() - pos);
        }
        // Skip first line, since it may be incomplete
        // NOTE: we always skip the first line(in the log), but the
        //       first line is just a '\n', so it's ok
        stream.readLine();
        QString str;

        while(!stream.atEnd())
        {
            str = stream.readLine().toHtmlEscaped();
            getTextView()->appendLog(str);
        }

        stream.setDevice(nullptr);
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
        QStringLiteral("ClearLogfileQuestion"))==KMessageBox::Continue)
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
        QStringLiteral("SaveLogfileNote"));

    QUrl logUrl = QUrl::fromLocalFile(fileName);
    QUrl destination = QFileDialog::getSaveFileUrl(this, i18n("Choose Destination Folder"), logUrl);
    if(!destination.isEmpty())
    {
        KIO::Job* job = KIO::copy(logUrl, destination);

        connect(job, &KIO::Job::result, this, &LogfileReader::copyResult);
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

bool LogfileReader::searchView() const { return true; }

#include "moc_logfilereader.cpp"
