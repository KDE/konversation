/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  shows all URLs found by the client
  begin:     Die Mai 27 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qhbox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qclipboard.h>
#include <qwhatsthis.h>

#include <kapplication.h>
#include <kactionclasses.h>
#include <klocale.h>
#include <kdebug.h>
#include <klistview.h>
#include <krun.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kdeversion.h>
#include <kshell.h>

#include "urlcatcher.h"
#include "channel.h"
#include "server.h"
#include "konversationapplication.h"
#include "viewcontainer.h"

UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("URL Catcher"));
    setType(ChatWindow::UrlCatcher);

    urlListView=new KListView(this,"url_list_view");
    urlListView->addColumn(i18n("Nick"));
    urlListView->addColumn(i18n("URL"));
    urlListView->setFullWidth(true);
    urlListView->setAllColumnsShowFocus(true);
    QString urlListViewWT = i18n(
        "List of Uniform Resource Locators mentioned in any of the Konversation windows "
        "during this session.");
    QWhatsThis::add(urlListView, urlListViewWT);

    QHBox* buttonBox=new QHBox(this);
    buttonBox->setSpacing(spacing());

    openUrlButton=new QPushButton(i18n("&Open URL"),buttonBox,"open_url_button");
    QString openUrlButtonWT = i18n(
        "<p>Select a <b>URL</b> above, then click this button to launch the "
        "application associated with the mimetype of the URL.</p>"
        "<p>In the <b>Settings</b>, under <b>Behavior</b> | <b>General</b>, "
        "you can specify a custom web browser for web URLs.</p>");
    QWhatsThis::add(openUrlButton, openUrlButtonWT);
    copyUrlButton=new QPushButton(i18n("&Copy URL"),buttonBox,"copy_url_button");
    QString copyUrlButtonWT = i18n(
        "Select a <b>URL</b> above, then click this button to copy the URL to the clipboard.");
    QWhatsThis::add(copyUrlButton, copyUrlButtonWT);
    deleteUrlButton=new QPushButton(i18n("&Delete URL"),buttonBox,"delete_url_button");
    QString deleteUrlButtonWT = i18n(
        "Select a <b>URL</b> above, then click this button to delete the URL from the list.");
    QWhatsThis::add(deleteUrlButton, deleteUrlButtonWT);
    saveListButton=new QPushButton(i18n("Sa&ve List..."),buttonBox,"save_list_button");
    QString saveListButtonWT = i18n(
        "Click to save the entire list to a file.");
    QWhatsThis::add(saveListButton, saveListButtonWT);
    clearListButton=new QPushButton(i18n("C&lear List"),buttonBox,"clear_list_button");
    QString clearListButtonWT = i18n(
        "Click to erase the entire list.");
    QWhatsThis::add(clearListButton, clearListButtonWT);

    connect(urlListView,SIGNAL (executed(QListViewItem*)),this,SLOT (openUrl(QListViewItem*)) );
    connect(urlListView,SIGNAL (selectionChanged()),this,SLOT (urlSelected()) );

    connect(openUrlButton,SIGNAL (clicked()),this,SLOT (openUrlClicked()) );
    connect(copyUrlButton,SIGNAL (clicked()),this,SLOT (copyUrlClicked()) );
    connect(deleteUrlButton,SIGNAL (clicked()),this,SLOT (deleteUrlClicked()) );
    connect(saveListButton,SIGNAL (clicked()),this,SLOT (saveListClicked()) );
    connect(clearListButton,SIGNAL (clicked()),this,SLOT (clearListClicked()) );

    saveListButton->setEnabled(false);
    clearListButton->setEnabled(false);

    urlSelected();
}


UrlCatcher::~UrlCatcher()
{
}

void UrlCatcher::urlSelected()
{
    QListViewItem* item=urlListView->selectedItem();
    if(item)
    {
        openUrlButton->setEnabled(true);
        copyUrlButton->setEnabled(true);
        deleteUrlButton->setEnabled(true);
    }
    else
    {
        openUrlButton->setEnabled(false);
        copyUrlButton->setEnabled(false);
        deleteUrlButton->setEnabled(false);
    }
}

void UrlCatcher::addUrl(const QString& who,const QString& url)
{
    new KListViewItem(urlListView,who,url);
    clearListButton->setEnabled(true);
    saveListButton->setEnabled(true);
}

void UrlCatcher::openUrl(QListViewItem* item)
{
    QString url = item->text(1);
    if (!Preferences::useCustomBrowser() || url.lower().startsWith("mailto:") )
    {
        new KRun(KURL(url));
    }
    else
    {
        QString cmd = Preferences::webBrowserCmd();
        cmd.replace("%u", url);
        KProcess *proc = new KProcess;
        QStringList cmdAndArgs = KShell::splitArgs(cmd);
        *proc << cmdAndArgs;
        //    This code will also work, but starts an extra shell process.
        //    kdDebug() << "UrlCatcher::openUrl(): cmd = " << cmd << endl;
        //    *proc << cmd;
        //    proc->setUseShell(true);
        proc->start(KProcess::DontCare);
        delete proc;
    }
}

void UrlCatcher::openUrlClicked()
{
    QListViewItem* item=urlListView->selectedItem();
    if(item) openUrl(item);
}

void UrlCatcher::copyUrlClicked()
{
    QListViewItem* item=urlListView->selectedItem();
    if(item)
    {
        QClipboard *cb=KApplication::kApplication()->clipboard();
        cb->setText(item->text(1),QClipboard::Selection);
    }
}

void UrlCatcher::deleteUrlClicked()
{
    QListViewItem* item=urlListView->selectedItem();
    if(item)
    {
        emit deleteUrl(item->text(0),item->text(1));
        delete item;
        // select next item
        item=urlListView->currentItem();
        if(item) urlListView->setSelected(item,true);
        else
        {
            saveListButton->setEnabled(false);
            clearListButton->setEnabled(false);
        }
    }
}

void UrlCatcher::clearListClicked()
{
    urlListView->clear();
    saveListButton->setEnabled(false);
    clearListButton->setEnabled(false);
    urlSelected();
    emit clearUrlList();
}

void UrlCatcher::saveListClicked()
{
    // Ask user for file name
    QString fileName=KFileDialog::getSaveFileName(
        QString::null,
        QString::null,
        this,
        i18n("Save URL List"));

    if(!fileName.isEmpty())
    {
        // now save the list to disk
        QFile listFile(fileName);
        listFile.open(IO_WriteOnly);
        // wrap the file into a stream
        QTextStream stream(&listFile);
        QListViewItem* item=urlListView->itemAtIndex(0);
        while(item)
        {
            stream << item->text(0) << ": " << item->text(1) << endl;
            item=item->itemBelow();
        }                                         // while
    }
}

void UrlCatcher::childAdjustFocus()
{
}

#include "urlcatcher.moc"
