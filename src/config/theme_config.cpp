/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2005 Ismail Donmez <ismail@kde.org>
    SPDX-FileCopyrightText: 2006 Dario Abatianni <eisfuchs@tigress.com>
    SPDX-FileCopyrightText: 2006 John Tapsell <johnflux@gmail.com>
    SPDX-FileCopyrightText: 2007 Eike Hein <hein@kde.org>
*/

#include "theme_config.h"

#include "preferences_base.h"
#include "images.h"
#include "nickiconset.h"
#include "common.h"
#include "application.h"

#include <KMessageBox>
#include <KTar>
#include <KZip>
#include <KDesktopFile>
#include <KIO/DeleteJob>
#include <KIO/CopyJob>
#include <KSharedConfig>

#include <QStringList>
#include <QUrl>
#include <QFileDialog>
#include <QStandardPaths>
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

    getButton->setConfigFile(QStringLiteral("konversation_nicklist_theme.knsrc"));

    connect(iconThemeIndex, &QListWidget::currentRowChanged, this, &Theme_Config::updatePreview);
    connect(iconThemeIndex, &QListWidget::itemSelectionChanged, this, &Theme_Config::updateButtons);
    connect(iconThemeIndex, &QListWidget::itemSelectionChanged, this, &Theme_Config::modified);
    connect(getButton, &KNSWidgets::Button::dialogFinished, this, &Theme_Config::gotNewSchemes);
    connect(installButton, &QPushButton::clicked, this, &Theme_Config::installTheme);
    connect(removeButton, &QPushButton::clicked, this, &Theme_Config::removeTheme);
}

Theme_Config::~Theme_Config()
{
}

void Theme_Config::loadSettings()
{
    // get list of theme dirs
    const QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("konversation/themes/"), QStandardPaths::LocateDirectory);
    m_dirs.clear();

    for (const QString& path : paths) {
        QDir dir(path);

        const auto themeDirs = dir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        for (const QString& themedir : themeDirs) {
            QFileInfo file(path + themedir + QLatin1String("/index.desktop"));

            if(file.exists())
            {
                m_dirs.append(file.absoluteFilePath());
            }
        }
    }

    // if we have any themes
    if (!m_dirs.isEmpty()) {
        m_dirs.sort();

        QString themeName, themeComment, themeDir;
        QString currentTheme = Preferences::self()->iconTheme();
        int currentThemeIndex = 0;

        // clear listview
        iconThemeIndex->clear();
        // initialize index counter
        int i = 0;
        // iterate through all found theme directories
        for (const QString& dir : std::as_const(m_dirs)) {
            KDesktopFile themeRC(dir);
            // get the name and comment from the theme
            themeName = themeRC.readName();
            themeComment = themeRC.readComment();

            // extract folder name
            themeDir = dir.section(QLatin1Char('/'),-2,-2);
            // is this our currently used theme?
            if (themeDir==currentTheme)
            {
                // remember for hasChanged()
                m_oldTheme=themeDir;
                // remember for updatePreview()
                currentThemeIndex = i;
            }

            if (themeDir==QLatin1String("oxygen"))
                m_defaultThemeIndex= i;

            // if there was a comment to the theme, add it to the listview entry string
            if(!themeComment.isEmpty())
                themeName = themeName + QLatin1String(" (") + themeComment + QLatin1Char(')');

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
        m_oldTheme = QStringLiteral("oxygen");

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
            KConfigGroup grp = config->group(QStringLiteral("Themes"));
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

void Theme_Config::gotNewSchemes()
{
    loadSettings();
}

void Theme_Config::installTheme()
{
    QUrl themeURL = QFileDialog::getOpenFileUrl(this,
        i18n("Select Theme Package"), QUrl (),
        i18n("Konversation Themes (*.tar.gz *.tar.bz2 *.tar *.zip)")
        );

    if(themeURL.isEmpty())
        return;

    QString themesDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konversation/themes/"));
    QString tmpThemeFile;

    QTemporaryFile tmpFile; // file automatically deleted when object is destroyed

    if (!themeURL.isLocalFile())
    {
        tmpFile.open(); // create the file, and thus create tmpFile.fileName
        tmpFile.close(); // no need to keep the file open, it isn't deleted until the destructor is called

        QUrl tmpUrl = QUrl::fromLocalFile(tmpFile.fileName());
        KIO::CopyJob *fileCopyJob = KIO::copy(themeURL, tmpUrl, KIO::Overwrite);
        if (!fileCopyJob->exec())
        {
            int errorCode = fileCopyJob->error();
            QString errorString;

            if (errorCode != 0)
                errorString = fileCopyJob->errorString();
            else
                errorString = i18n("Unknown error (0)");

            KMessageBox::error(nullptr,
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
        if(themeInstallDir.exists(QStringLiteral("index.desktop")))
        {
            KIO::CopyJob* job = KIO::copy(QUrl(tmpThemeFile), QUrl(themesDir));
            job->exec(); //FIXME error handling
        }
        else
        {
            KMessageBox::error(nullptr,
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
        const QStringList allEntries = themeDir->entries();

        for (const QString& entry : allEntries) {
            if (themeDir->entry(entry + QLatin1String("/index.desktop")) == nullptr) {
                KMessageBox::error(nullptr,
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

    int remove = KMessageBox::warningContinueCancel(nullptr,
        i18n("Do you want to remove %1?", themeName),
        i18n("Remove Theme"),
        KStandardGuiItem::del(),KStandardGuiItem::cancel(),
        QStringLiteral("warningRemoveTheme")
        );

    if (remove != KMessageBox::Continue) {
        return;
    }

    dir.chop(QLatin1String("index.desktop").size());
    // delete the files, the RemoveDeadEntries entry in the
    // knsrc file takes care of marking the KNS entry is as deleted
    KIO::DeleteJob* job = KIO::del(QUrl::fromLocalFile(dir));
    connect(job, &KIO::DeleteJob::result, this, &Theme_Config::postRemoveTheme);
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
    dir.remove(QStringLiteral("/index.desktop"));
    NickIconSet nickIconSet;
    nickIconSet.load(dir);

    // TODO: use QIcon::paint-based display, to catch icon changing on environment changes like color palette
    previewLabel1->setPixmap(nickIconSet.nickIcon(Images::Normal).pixmap(16));
    previewLabel2->setPixmap(nickIconSet.nickIcon(Images::Normal, NickIconSet::UserAway).pixmap(16));
    previewLabel3->setPixmap(nickIconSet.nickIcon(Images::Voice).pixmap(16));
    previewLabel4->setPixmap(nickIconSet.nickIcon(Images::HalfOp).pixmap(16));
    previewLabel5->setPixmap(nickIconSet.nickIcon(Images::Op).pixmap(16));
    previewLabel6->setPixmap(nickIconSet.nickIcon(Images::Admin).pixmap(16));
    previewLabel7->setPixmap(nickIconSet.nickIcon(Images::Owner).pixmap(16));
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
    m_currentTheme = dir.section(QLatin1Char('/'),-2,-2);

    // allow delete action only for themes that have been installed by the user
    if(!themeRC.open(QIODevice::ReadOnly | QIODevice::WriteOnly))
        removeButton->setEnabled(false);
    else
        removeButton->setEnabled(true);

    themeRC.close();
}

#include "moc_theme_config.cpp"
