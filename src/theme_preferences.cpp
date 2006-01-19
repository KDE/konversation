/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  (C) 2004,2005 by İsmail Dönmez ( Resistence is Futile. Turn god damn unicode on! )
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qbitmap.h>
#include <qpainter.h>
#include <qtooltip.h>

#include <klistbox.h>
#include <kurl.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <ktar.h>
#include <kdesktopfile.h>
#include <kconfigdialog.h>

#include <unistd.h> // unlink()

#include "theme_preferences.h"
#include "preferences_base.h"
#include "images.h"
#include "common.h"
#include "konversationapplication.h"

using namespace Konversation;

Theme_Config::Theme_Config(QWidget* parent, const char* name)
  : Theme_ConfigUI( parent, name)
{
    m_oldTheme = Preferences::iconTheme();

    loadSettings();

    connect(iconThemeIndex,SIGNAL(highlighted(int)),this,SLOT(updatePreview(int)));
    connect(iconThemeIndex,SIGNAL(currentChanged(QListBoxItem*)),this,SLOT(updateButtons()));
    connect(installButton,SIGNAL(clicked()),this,SLOT(installTheme()));
    connect(removeButton,SIGNAL(clicked()),this,SLOT(removeTheme()));
}

Theme_Config::~Theme_Config()
{
}

void Theme_Config::loadSettings()
{
    QString themeName, themeComment;
    QString currentTheme = Preferences::iconTheme();
    int currentThemeIndex = 0;

    m_dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/index.desktop");

    if(m_dirs.count() > 0)
    {
        iconThemeIndex->clear();
        int i = 0;

        for(QStringList::ConstIterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
        {
            KDesktopFile themeRC(*it);
            themeName = themeRC.readName();
            themeComment = themeRC.readComment();

            if ((*it).section('/',-2,-2)==currentTheme)
                currentThemeIndex = i;

            if(!themeComment.isEmpty())
                themeName = themeName+" ("+themeComment+")";

            iconThemeIndex->insertItem(themeName);

            ++i;
        }

        iconThemeIndex->setSelected(currentThemeIndex, true);
        updatePreview(currentThemeIndex);
    }

    updateButtons();
}

bool Theme_Config::hasChanged()
{
  return ( m_oldTheme != m_currentTheme );
}

void Theme_Config::saveSettings()
{
    if(iconThemeIndex->count())
    {
        if(hasChanged())
        {
            KConfig* config = kapp->config();
            config->setGroup("Themes");
            config->writeEntry("IconTheme",m_currentTheme);
            Preferences::setIconTheme(m_currentTheme);
            KonversationApplication::instance()->images()->initializeNickIcons();
            KonversationApplication::instance()->updateNickIcons();
            m_oldTheme = m_currentTheme;
        }
    }
}

void Theme_Config::restorePageToDefaults()
{
    // FIXME!
}

void Theme_Config::installTheme()
{
    KURL themeURL = KFileDialog::getOpenURL(QString::null,
        i18n("*.tar.gz *.tar.bz2 *.tar *.zip|Konversation Themes"),
        NULL,
        i18n("Select Theme Package")
        );

    if(themeURL.isEmpty())
        return;

    QString themesDir(locateLocal("data", "konversation/themes/"));
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
            KIO::NetAccess::dircopy(KURL(tmpThemeFile),KURL(themesDir),0L);
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
        themeArchive.open(IO_ReadOnly);
        kapp->processEvents();

        const KArchiveDirectory* themeDir = themeArchive.directory();;
        QStringList allEntries = themeDir->entries();

        for(QStringList::ConstIterator it=allEntries.begin(); it != allEntries.end(); ++it)
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
    QString themeName = iconThemeIndex->currentText();

    dir = m_dirs[iconThemeIndex->currentItem()];

    int remove = KMessageBox::warningContinueCancel(0L,
        i18n("Do you want to remove %1 ?").arg(themeName),
        i18n("Remove Theme"),
        KStdGuiItem::del(),
        "warningRemoveTheme"
        );

    if(remove == KMessageBox::Continue)
    {
        unlink(QFile::encodeName(dir));
        KIO::del(KURL(dir.remove("index.desktop")));
        loadSettings();
    }
}

void Theme_Config::updatePreview(int id)
{
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
    if(iconThemeIndex->count() < 2)
    {
        removeButton->setEnabled(false);
        return;
    }

    QString dir = m_dirs[iconThemeIndex->currentItem()];
    QFile themeRC(dir);
    m_currentTheme = dir.section('/',-2,-2);

    if(!themeRC.open(IO_ReadOnly | IO_WriteOnly))
        removeButton->setEnabled(false);
    else
        removeButton->setEnabled(true);

    themeRC.close();
}

#include "theme_preferences.moc"
