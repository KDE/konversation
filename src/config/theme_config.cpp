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

#include <KUrl>
#include <KMessageBox>
#include <KStandardDirs>
#include <KFileDialog>
#include <KTar>
#include <KDesktopFile>
#include <KIO/DeleteJob>
#include <KIO/NetAccess>

#include <unistd.h> // unlink()


using namespace Konversation;

Theme_Config::Theme_Config(QWidget* parent, const char* name)
  : QWidget(parent)
{
    setObjectName(QString::fromLatin1(name));
    setupUi(this);

    m_defaultThemeIndex = -1;

    // load the current settings
    loadSettings();

    connect(iconThemeIndex,SIGNAL(currentRowChanged(int)),this,SLOT(updatePreview(int)));
    connect(iconThemeIndex,SIGNAL(itemSelectionChanged()),this,SLOT(updateButtons()));
    connect(iconThemeIndex,SIGNAL(itemSelectionChanged()),this,SIGNAL(modified()));
    connect(installButton,SIGNAL(clicked()),this,SLOT(installTheme()));
    connect(removeButton,SIGNAL(clicked()),this,SLOT(removeTheme()));
}

Theme_Config::~Theme_Config()
{
}

void Theme_Config::loadSettings()
{
    // get list of theme dirs
    m_dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/index.desktop");

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
            KSharedConfigPtr config = KGlobal::config();
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
    KUrl themeURL = KFileDialog::getOpenUrl(QString(),
        i18n("*.tar.gz *.tar.bz2 *.tar *.zip|Konversation Themes"),
        NULL,
        i18n("Select Theme Package")
        );

    if(themeURL.isEmpty())
        return;

    QString themesDir(KStandardDirs::locateLocal("data", "konversation/themes/"));
    QString tmpThemeFile;

    if(!KIO::NetAccess::download(themeURL, tmpThemeFile, NULL))
    {
        KMessageBox::error(0L,
            KIO::NetAccess::lastErrorString(),
            i18n("Failed to Download Theme"),
            KMessageBox::Notify
            );
        return;
    }

    QDir themeInstallDir(tmpThemeFile);

    if(themeInstallDir.exists()) // We got a directory not a file
    {
        if(themeInstallDir.exists("index.desktop"))
            KIO::NetAccess::dircopy(KUrl(tmpThemeFile),KUrl(themesDir),0L);
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

        KTar themeArchive(tmpThemeFile);
        themeArchive.open(QIODevice::ReadOnly);
        kapp->processEvents();

        const KArchiveDirectory* themeDir = themeArchive.directory();;
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
        themeArchive.close();
    }

    loadSettings();
    KIO::NetAccess::removeTempFile(tmpThemeFile);

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

        unlink(QFile::encodeName(dir));
        KIO::DeleteJob* job = KIO::del(KUrl(dir.remove("index.desktop")));
        connect(job, SIGNAL(result(KJob*)), this, SLOT(postRemoveTheme(KJob*)));
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

#include "theme_config.moc"
