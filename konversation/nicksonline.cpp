/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  nicksonline.cpp  -  description
  begin:     Sam Aug 31 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

  $Id$
*/

#include <qlayout.h>
#include <qstringlist.h>
#include <qhbox.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>

#include "nicksonline.h"

NicksOnline::NicksOnline(const QSize& newSize)
{
  kdDebug() << "NicksOnline::NicksOnline()" << endl;
  nickListView=new KListView(this);
  nickListView->addColumn(i18n("Nickname"));
  setMargin(KDialog::marginHint());
  setSpacing(KDialog::spacingHint());

  QHBox* buttonBox=new QHBox(this);
  buttonBox->setSpacing(KDialog::spacingHint());

  QPushButton* editButton=new QPushButton(i18n("Edit"),buttonBox,"edit_notify_button");
  QPushButton* closeButton=new QPushButton(i18n("Close"),buttonBox,"close_nicksonline_window");

  connect(editButton,SIGNAL (clicked()),SIGNAL (editClicked()) );
  connect(closeButton,SIGNAL (clicked()),this,SLOT (closeButton()) );
  connect(nickListView,SIGNAL (doubleClicked(QListViewItem*)),this,SLOT(processDoubleClick(QListViewItem*)));

  resize(newSize);
}

NicksOnline::~NicksOnline()
{
  kdDebug() << "NicksOnline::~NicksOnline()" << endl;

  delete nickListView;
}

void NicksOnline::setOnlineList(QStringList list)
{
  nickListView->clear();
  for(unsigned int i=list.count();i!=0;i--)
    new KListViewItem(nickListView,list[i-1]);
}

void NicksOnline::closeEvent(QCloseEvent* ce)
{
  ce->accept();
  closeButton();
}

void NicksOnline::closeButton()
{
  emit closeClicked(size());
}

void NicksOnline::processDoubleClick(QListViewItem* item)
{
  emit doubleClicked(item);
}

#include "nicksonline.moc"
