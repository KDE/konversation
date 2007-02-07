/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef MULTILINETEXTEDIT_H
#define MULTILINETEXTEDIT_H

#include <qtextedit.h>

/**
  @author Dario Abatianni <eisfuchs@tigress.com> and the kbabel Team
*/

class QPaintEvent;
class QWheelEvent;

class MultilineTextEdit : public QTextEdit
{
    Q_OBJECT

    public:
        explicit MultilineTextEdit(QWidget* parent=0,const char* name=0);
        ~MultilineTextEdit();

    protected slots:
        void drawWhitespaces();
        void cursorChanged(int,int);

    protected:
        // reimplemented
        virtual void drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph);

        // the stuff below is copied from kbabel. Thanks, guys!
        QRect mapToView(int paragraph,int index);
};

#endif
