/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 Peter Simonsson <psn@linux.se>
*/
#ifndef KONVERSATIONTOPICLABEL_H
#define KONVERSATIONTOPICLABEL_H

#include <kactivelabel.h>

namespace Konversation {

class TopicLabel : public KActiveLabel
{
  Q_OBJECT
  public:
    TopicLabel(QWidget *parent = 0, const char *name = 0);
    ~TopicLabel();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
};

};

#endif
