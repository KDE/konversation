/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2005 Ismail Donmez <ismail@kde.org>
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 John Tapsell <johnflux@gmail.com>
  Copyright (C) 2007 Eike Hein <hein@kde.org>
*/

#include "theme_config.h"
#include "preferences_base.h"
#include "images.h"
#include "common.h"
#include "application.h"

#include <QStringList>

#include <QUrl>
#include <KMessageBox>
#include <QFileDialog>
#include <KTar>
#include <KZip>
#include <KDesktopFile>
#include <KIO/DeleteJob>
#include <KIO/CopyJob>

#include <unistd.h> // unlink()
#include <KSharedConfig>
#include <QStandardPaths>
#include <QDebug>
#include <QTemporaryFile>
#include <QMimeDatabase>


using namespace Konversation;

Theme_Config::Theme_Config(QWidget* parent, const char* name)
  : QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    m_defaultThemeIndex = -1;

    // load the current settings
    loadSettings();

    connect(iconThemeIndex, &QListWidget::currentRowChanged, this, &Theme_Config::updatePreview);
    connect(iconThemeIndex, &QListWidget::itemSelectionChanged, this, &Theme_Config::updateButtons);
    connect(iconThemeIndex, &QListWidget::itemSelectionChanged, this, &Theme_Config::modified);
    connect(installButton, &QPushButton::clicked, this, &Theme_Config::installTheme);
    connect(removeButton, &QPushButton::clicked, this, &Theme_Config::removeTheme);
}

Theme_Config::~Theme_Config()
{
}

void Theme_Config::loadSettings()
{
    // get list of theme dirs
    QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "konversation/themes/", QStandardPaths::LocateDirectory);
    m_dirs.clear();

    foreach(const QString& path, paths)
    {
        QDir dir(path);

        foreach(const QString& themedir, dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot))
        {
            QFileInfo file(path + themedir + "/index.desktop");

            if(file.exists())
            {
                m_dirs.append(file.absoluteFilePath());
            }
        }
    }

    // if we have any themes
    if (m_dirs.count() > 0)
    {
        m_dirs.sort();

        QString themeName, themeComment, themeDir;
        QString currentTheme = Preferences::self()->iconTheme();
        int currentThemeIndex = 0;

        // clear listview
        iconThemeIndex->clear();
        // initialize index counter
        int i = 0;
        // iterate through all found theme directories
        for(QStringList::ConstIterator it = m_dirs.constBegin(); it != m_dirs.constEnd(); ++it)
        {
            KDesktopFile themeRC(*it);
            // get the name and comment from the theme
            themeName = themeRC.readName();
            themeComment = themeRC.readComment();

            // extract folder name
            themeDir=(*it).section('/',-2,-2);
            // is this our currently used theme?
            if (themeDir==currentTheme)
            {
                // remember for hasChanged()
                m_oldTheme=themeDir;
                // remember for updatePreview()
                currentThemeIndex = i;
            }

            if (themeDir=="oxygen")
                m_defaultThemeIndex= i;

            // if there was a comment to the theme, add it to the listview entry string
            if(!themeComment.isEmpty())
                themeName = themeName+" ("+themeComment+')';

            // insert entry into the listview
            iconThemeIndex->addItem(themeName);

            // increment index counter
            ++i;
        }
        // highlight currently active theme and update preview box
        iconThemeIndex->setCurrentRow(currentThemeIndex);
    }

    // if there was no currently used theme found, use the default theme
    // If anyone knows how to get the default value from this, please change this!
    if(m_oldTheme.isEmpty())
        m_oldTheme = "oxygen";

    // update enabled/disabled state of buttons
    updateButtons();
}

bool Theme_Config::hasChanged()
{
  // return true if the theme selected is different from the saved theme
  return ( m_oldTheme != m_currentTheme );
}

void Theme_Config::saveSettings()
{
    // if there are any themes in the listview ...
    if(iconThemeIndex->count())
    {
        // and if anything has changed ...
        if(hasChanged())
        {
            // save icon theme name
            KSharedConfigPtr config = KSharedConfig::openConfig();
            KConfigGroup grp = config->group("Themes");
            grp.writeEntry("IconTheme",m_currentTheme);
            // set in-memory theme to the saved theme
            Preferences::self()->setIconTheme(m_currentTheme);
            // update theme on runtime
            Application::instance()->images()->initializeNickIcons();

            // remember current theme for hasChanged()
            m_oldTheme = m_currentTheme;
        }
    }
}

void Theme_Config::restorePageToDefaults()
{
    if (m_defaultThemeIndex != -1)
        iconThemeIndex->setCurrentRow(m_defaultThemeIndex);
}

void Theme_Config::installTheme()
{
    QUrl themeURL = QFileDialog::getOpenFileUrl(this,
        i18n("Select Theme Package"), QUrl (),
        i18n("Konversation Themes (*.tar.gz *.tar.bz2 *.tar *.zip)")
        );

    if(themeURL.isEmpty())
        return;

    QString themesDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "konversation/themes/");
    QString tmpThemeFile;

    QTemporaryFile tmpFile; // file automatically deleted when object is destroyed

    if (!themeURL.isLocalFile())
    {
        tmpFile.open(); // create the file, and thus create tmpFile.fileName
        tmpFile.close(); // no need to keep the file open, it isn't deleted until the destructor is called

        QUrl tmpUrl = QUrl::fromLocalFile(tmpFile.fileName());
        KIO::FileCopyJob *fileCopyJob = KIO::file_copy(themeURL, tmpUrl, -1, KIO::Overwrite);
        if (!fileCopyJob->exec())
        {
            int errorCode = fileCopyJob->error();
            QString errorString;

            if (errorCode != 0)
                errorString = fileCopyJob->errorString();
            else
                errorString = i18n("Unknown error (0)");

            KMessageBox::error(0L,
                errorString,
                i18n("Failed to Download Theme"),
                KMessageBox::Notify
                );
            return;

        }
        tmpThemeFile = tmpUrl.toLocalFile();
    }
    else
    {
        tmpThemeFile = themeURL.toLocalFile();
    }

    QDir themeInstallDir(tmpThemeFile);

    if(themeInstallDir.exists()) // We got a directory not a file
    {
        if(themeInstallDir.exists("index.desktop"))
        {
            KIO::CopyJob* job = KIO::copy(QUrl(tmpThemeFile), QUrl(themesDir));
            job->exec(); //FIXME error handling
        }
        else
        {
            KMessageBox::error(0L,
                i18n("Theme archive is invalid."),
                i18n("Cannot Install Theme"),
                KMessageBox::Notify
                );
        }
    }
    else // we got a file
    {
        QMimeDatabase db;
        QMimeType mimeType = db.mimeTypeForFile(tmpThemeFile);
        KArchive *themeArchive;

        if (mimeType.inherits(QStringLiteral("application/zip")))
        {
            themeArchive = new KZip(tmpThemeFile);
        }
        else
        {
            themeArchive = new KTar(tmpThemeFile);
        }

        themeArchive->open(QIODevice::ReadOnly);
        qApp->processEvents();

        const KArchiveDirectory* themeDir = themeArchive->directory();
        QStringList allEntries = themeDir->entries();

        for(QStringList::ConstIterator it=allEntries.constBegin(); it != allEntries.constEnd(); ++it)
        {
            if(themeDir->entry(*it+"/index.desktop") == NULL)
            {
                KMessageBox::error(0L,
                    i18n("Theme archive is invalid."),
                    i18n("Cannot Install Theme"),
                    KMessageBox::Notify
                    );
                break;
            }
            else
                themeDir->copyTo(themesDir);

        }
        themeArchive->close();
        delete themeArchive;
    }

    loadSettings();
}

void Theme_Config::removeTheme()
{
    QString dir;
    QString themeName = iconThemeIndex->currentItem() ? iconThemeIndex->currentItem()->text() : QString();

    dir = m_dirs[iconThemeIndex->currentRow()];

    int remove = KMessageBox::warningContinueCancel(0L,
        i18n("Do you want to remove %1?", themeName),
        i18n("Remove Theme"),
        KStandardGuiItem::del(),KStandardGuiItem::cancel(),
        "warningRemoveTheme"
        );

    if(remove == KMessageBox::Continue)
    {
        QByteArray encoded = QFile::encodeName(dir);
        unlink(encoded.data());
        KIO::DeleteJob* job = KIO::del(QUrl(dir.remove("index.desktop")));
        connect(job, &KIO::DeleteJob::result, this, &Theme_Config::postRemoveTheme);
    }
}

void Theme_Config::postRemoveTheme(KJob* /* delete_job */)
{
    loadSettings();
}

void Theme_Config::updatePreview(int id)
{
    if (id < 0)
        return;
    QString dir;
    dir = m_dirs[id];
    dir.remove("/index.desktop");
    QPixmap normal(dir+"/irc_normal.png");

    previewLabel1->setPixmap(normal);
    previewLabel2->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_away.png")));
    previewLabel3->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_voice.png")));
    previewLabel4->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_halfop.png")));
    previewLabel5->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_op.png")));
    previewLabel6->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_admin.png")));
    previewLabel7->setPixmap(overlayPixmaps(normal,QPixmap(dir+"/irc_owner.png")));
}

void Theme_Config::updateButtons()
{
    // don't allow clicking "remove" if there is only one or even no theme installed
    if(iconThemeIndex->count() < 2 || m_dirs.count() - 1 < iconThemeIndex->currentRow())
    {
        removeButton->setEnabled(false);
        return;
    }

    // get directory of current theme
    QString dir = m_dirs[iconThemeIndex->currentRow()];
    QFile themeRC(dir);
    // get name for directory
    m_currentTheme = dir.section('/',-2,-2);

    // allow delete action only for themes that have been installed by the user
    if(!themeRC.open(QIODevice::ReadOnly | QIODevice::WriteOnly))
        removeButton->setEnabled(false);
    else
        removeButton->setEnabled(true);

    themeRC.close();
}


