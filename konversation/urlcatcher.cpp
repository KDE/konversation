/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  urlcatcher.cpp  -  shows all URLs found by the client
  begin:     Die Mai 27 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qhbox.h>
#include <qpushbutton.h>
#include <qclipboard.h>

#include <kapp.h>
#include <klocale.h>
#include <kdebug.h>
#include <klistview.h>
#include <krun.h>
#include <kfiledialog.h>
#include <kshell.h>
#include <kprocess.h>

#include "urlcatcher.h"
#include "konversationapplication.h"

UrlCatcher::UrlCatcher(QWidget* parent) : ChatWindow(parent)
{
  setName(i18n("URL Catcher"));
  setType(ChatWindow::UrlCatcher);

  urlListView=new KListView(this,"url_list_view");
  urlListView->addColumn(i18n("Nick"));
  urlListView->addColumn(i18n("URL"));
  urlListView->setFullWidth(true);
  urlListView->setAllColumnsShowFocus(true);

  QHBox* buttonBox=new QHBox(this);
  buttonBox->setSpacing(spacing());

  openUrlButton=new QPushButton(i18n("&Open URL"),buttonBox,"open_url_button");
  copyUrlButton=new QPushButton(i18n("&Copy URL"),buttonBox,"copy_url_button");
  deleteUrlButton=new QPushButton(i18n("&Delete URL"),buttonBox,"delete_url_button");
  saveListButton=new QPushButton(i18n("Sa&ve List..."),buttonBox,"save_list_button");
  clearListButton=new QPushButton(i18n("C&lear List"),buttonBox,"clear_list_button");

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
  if (KonversationApplication::preferences.getWebBrowserUseKdeDefault())
    new KRun(item->text(1));
  else
  {
    QString url = item->text(1);
    QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
    cmd.replace("%u", url);
    KProcess *proc = new KProcess;
    QStringList cmdAndArgs = KShell::splitArgs(cmd);
    kdDebug() << "UrlCatcher::openUrl(): cmd = " << cmdAndArgs << endl;
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
#if QT_VERSION >= 0x030100
    cb->setText(item->text(1),QClipboard::Selection);
#else
    cb->setSelectionMode(true);
    cb->setText(item->text(1));
#endif
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
    } // while
  }
}

void UrlCatcher::adjustFocus()
{
}

#include "urlcatcher.moc"
