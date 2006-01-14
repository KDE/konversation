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

#include "ex_theme_preferences.h"
#include "preferences_base.h"
#include "images.h"
#include "common.h"
#include "konversationapplication.h"

using namespace Konversation;

Theme_Config_Ext::Theme_Config_Ext(QWidget* parent, const char* name)
  : Theme_Config( parent, name)
{

    updateList();
    updateButtons();

    m_oldTheme = Preferences::iconThemeName();

    connect(kcfg_IconThemeIndex,SIGNAL(highlighted(int)),this,SLOT(updatePreview(int)));
    connect(kcfg_IconThemeIndex,SIGNAL(currentChanged(QListBoxItem*)),this,SLOT(updateButtons()));
    connect(installButton,SIGNAL(clicked()),this,SLOT(installTheme()));
    connect(removeButton,SIGNAL(clicked()),this,SLOT(removeTheme()));

    KConfigDialog *conf = static_cast<KConfigDialog *>(parent);
    connect( conf, SIGNAL(applyClicked()), this, SLOT(applyPreferences()));
    connect( conf, SIGNAL(okClicked()), this, SLOT(applyPreferences()));
}

Theme_Config_Ext::~Theme_Config_Ext()
{
}

void Theme_Config_Ext::applyPreferences()
{
    if(kcfg_IconThemeIndex->count())
    {
        QString theme;
        theme = m_dirs[kcfg_IconThemeIndex->currentItem()];
        theme = theme.section('/',-2,-2);
        if(m_oldTheme != theme)
        {
            kdDebug() << "New Theme :" << theme << endl;
	    KConfig* config = kapp->config();
	    config->setGroup("Themes");
	    config->writeEntry("IconThemeName",theme);
	    Preferences::setIconThemeName(theme);
            KonversationApplication::instance()->images()->initializeNickIcons();
            KonversationApplication::instance()->updateNickIcons();
            m_oldTheme = theme;
        }
    }
}

void Theme_Config_Ext::installTheme()
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

    updateList();
    updateButtons();
    KIO::NetAccess::removeTempFile(tmpThemeFile);

}

void Theme_Config_Ext::removeTheme()
{
    QString dir;
    QString themeName = kcfg_IconThemeIndex->currentText();

    dir = m_dirs[kcfg_IconThemeIndex->currentItem()];

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
        updateList();
        updateButtons();
    }
}

void Theme_Config_Ext::updatePreview(int id)
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

void Theme_Config_Ext::updateList()
{
    QString themeName, themeComment;
    QString currentTheme = Preferences::iconThemeName();
    int index = Preferences::iconThemeIndex();

    m_dirs = KGlobal::dirs()->findAllResources("data","konversation/themes/*/index.desktop");

    if(m_dirs.count() > 0)
    {
        kcfg_IconThemeIndex->clear();

        for(QStringList::ConstIterator it = m_dirs.begin(); it != m_dirs.end(); ++it)
        {
            KDesktopFile themeRC(*it);
            themeName = themeRC.readName();
            themeComment = themeRC.readComment();

            if(!themeComment.isEmpty())
                themeName = themeName+" ( "+themeComment+" )";

            kcfg_IconThemeIndex->insertItem(themeName);
        }

        kcfg_IconThemeIndex->setSelected(index, true);
        updatePreview(index);
    }
}

void Theme_Config_Ext::updateButtons()
{
    if(kcfg_IconThemeIndex->count() < 2)
    {
        removeButton->setEnabled(false);
        return;
    }

    QString dir = m_dirs[kcfg_IconThemeIndex->currentItem()];
    QFile themeRC(dir);

    if(!themeRC.open(IO_ReadOnly | IO_WriteOnly))
        removeButton->setEnabled(false);
    else
        removeButton->setEnabled(true);

    themeRC.close();
}

#include "ex_theme_preferences.moc"
