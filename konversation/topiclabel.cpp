/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 Peter Simonsson <psn@linux.se>
*/
#include "topiclabel.h"

namespace Konversation {

TopicLabel::TopicLabel(QWidget *parent, const char *name)
 : KActiveLabel(parent, name)
{
}

TopicLabel::~TopicLabel()
{
}

QSize TopicLabel::minimumSizeHint() const
{
  return QSize(0, 0);
}

QSize TopicLabel::sizeHint() const
{
  return QSize(0, 0);
}

}

#include "topiclabel.moc"
