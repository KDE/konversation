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

#include "urlcatcher.h"
#include "channel.h"
#include "server.h"
#include "application.h"
#include "viewcontainer.h"

#include <QPushButton>
#include <QClipboard>
#include <QTreeWidget>
#include <QLayout>

#include <KFileDialog>
#include <KTreeWidgetSearchLine>


UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
    setName(i18n("URL Catcher"));
    setType(ChatWindow::UrlCatcher);

    urlListView=new QTreeWidget(this);
    urlListView->setObjectName("url_list_view");
    urlListView->setRootIsDecorated(false);
    urlListView->setHeaderLabels(QStringList() << i18n("Nick") << i18n("URL"));
    urlListView->setAllColumnsShowFocus(true);
    QString urlListViewWT = i18n(
        "List of Uniform Resource Locators mentioned in any of the Konversation windows "
        "during this session.");
    urlListView->setWhatsThis(urlListViewWT);

    searchWidget = new KTreeWidgetSearchLineWidget(this, urlListView);
    searchWidget->setObjectName("search_line");
    searchWidget->setEnabled(false);

    KHBox* buttonBox=new KHBox(this);
    buttonBox->setSpacing(spacing());

    openUrlButton = new QPushButton(i18n("&Open URL"), buttonBox);
    openUrlButton->setObjectName("open_url_button");
    QString openUrlButtonWT = i18n(
        "<p>Select a <b>URL</b> above, then click this button to launch the "
        "application associated with the mimetype of the URL.</p>"
        "<p>In the <b>Settings</b>, under <b>Behavior</b> | <b>General</b>, "
        "you can specify a custom web browser for web URLs.</p>");
    openUrlButton->setWhatsThis(openUrlButtonWT);
    copyUrlButton = new QPushButton(i18n("&Copy URL"), buttonBox);
    copyUrlButton->setObjectName("copy_url_button");
    QString copyUrlButtonWT = i18n(
        "Select a <b>URL</b> above, then click this button to copy the URL to the clipboard.");
    copyUrlButton->setWhatsThis(copyUrlButtonWT);
    deleteUrlButton = new QPushButton(i18n("&Delete URL"), buttonBox);
    deleteUrlButton->setObjectName("delete_url_button");
    QString deleteUrlButtonWT = i18n(
        "Select a <b>URL</b> above, then click this button to delete the URL from the list.");
    deleteUrlButton->setWhatsThis(deleteUrlButtonWT);
    saveListButton = new QPushButton(i18n("Sa&ve List..."), buttonBox);
    saveListButton->setObjectName("save_list_button");
    QString saveListButtonWT = i18n(
        "Click to save the entire list to a file.");
    saveListButton->setWhatsThis(saveListButtonWT);
    clearListButton = new QPushButton(i18n("C&lear List"), buttonBox);
    clearListButton->setObjectName("clear_list_button");
    QString clearListButtonWT = i18n(
        "Click to erase the entire list.");
    clearListButton->setWhatsThis(clearListButtonWT);

    connect(urlListView,SIGNAL (itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT (openUrl(QTreeWidgetItem*)) );
    connect(urlListView,SIGNAL (itemSelectionChanged()),this,SLOT (urlSelected()) );

    connect(openUrlButton,SIGNAL (clicked()),this,SLOT (openUrlClicked()) );
    connect(copyUrlButton,SIGNAL (clicked()),this,SLOT (copyUrlClicked()) );
    connect(deleteUrlButton,SIGNAL (clicked()),this,SLOT (deleteUrlClicked()) );
    connect(saveListButton,SIGNAL (clicked()),this,SLOT (saveListClicked()) );
    connect(clearListButton,SIGNAL (clicked()),this,SLOT (clearListClicked()) );

    saveListButton->setEnabled(false);
    clearListButton->setEnabled(false);

    layout()->addWidget(searchWidget);
    layout()->addWidget(urlListView);
    layout()->addWidget(buttonBox);

    urlSelected();
}


UrlCatcher::~UrlCatcher()
{
}

void UrlCatcher::urlSelected()
{
    QTreeWidgetItem* item=urlListView->currentItem();
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
    new QTreeWidgetItem(urlListView, QStringList() << who << url);
    clearListButton->setEnabled(true);
    saveListButton->setEnabled(true);
    searchWidget->setEnabled(true);
}

void UrlCatcher::openUrl(QTreeWidgetItem* item)
{
    QString url = item->text(1);

    Application::openUrl(url);
}

void UrlCatcher::openUrlClicked()
{
    QTreeWidgetItem* item=urlListView->currentItem();
    if(item) openUrl(item);
}

void UrlCatcher::copyUrlClicked()
{
    QTreeWidgetItem* item=urlListView->currentItem();
    if(item)
    {
        QClipboard *cb = qApp->clipboard();
        cb->setText(item->text(1),QClipboard::Selection);
        cb->setText(item->text(1),QClipboard::Clipboard);
    }
}

void UrlCatcher::deleteUrlClicked()
{
    QTreeWidgetItem* item=urlListView->currentItem();
    if(item)
    {
        emit deleteUrl(item->text(0),item->text(1));
        delete item;
        // select next item
        item=urlListView->currentItem();
        if(item) urlListView->setCurrentItem(item);
        else
        {
            saveListButton->setEnabled(false);
            clearListButton->setEnabled(false);
            searchWidget->setEnabled(false);
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
        QString(),
        QString(),
        this,
        i18n("Save URL List"));

    if(!fileName.isEmpty())
    {
        // now save the list to disk
        QFile listFile(fileName);
        listFile.open(QIODevice::WriteOnly);
        // wrap the file into a stream
        QTextStream stream(&listFile);
        QTreeWidgetItem* item=urlListView->topLevelItem(0);
        while(item)
        {
            stream << item->text(0) << ": " << item->text(1) << endl;
            item=urlListView->itemBelow(item);
        }                                         // while
    }
}

void UrlCatcher::childAdjustFocus()
{
}

#include "urlcatcher.moc"
