/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  topiccombobox.cpp  -  description
  begin:     Don Nov 21 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <kdebug.h>

#include "topiccombobox.h"

TopicComboBox::TopicComboBox(QWidget* parent) :
                   KComboBox(parent)
{
  connect(this,SIGNAL(activated(int)),this,SLOT(topicActivated(int)));
  connect(this,SIGNAL(returnPressed(const QString&)),this,SLOT(topicActivated(const QString&)));
}

TopicComboBox::~TopicComboBox()
{
}

void TopicComboBox::topicActivated(const QString& newTopic)
{
  kdDebug() << "TopicComboBox::topicActivated(" << newTopic << ")" << endl;

  emit topicChanged(newTopic);
}

void TopicComboBox::topicActivated(int index)
{
  kdDebug() << "TopicComboBox::topicActivated(" << index << ")" << endl;

  emit topicChanged(text(index).section(' ',1));
}

void TopicComboBox::insertStringList(const QStringList& list)
{
  KComboBox::insertStringList(list);

  setEditText(list[0].section(' ',1));
}

void TopicComboBox::wheelEvent(QWheelEvent *ev)
{
  ev->ignore();
}

#include "topiccombobox.moc"
