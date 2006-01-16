//
// C++ Interface: multilinetextedit
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com> and kbabel Team, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

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
    MultilineTextEdit(QWidget* parent=0,const char* name=0);
    ~MultilineTextEdit();

  protected slots:
    void drawWhitespaces();
    void cursorChanged(int,int);

  protected:
    virtual void drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph);

    // the stuff below is copied from kbabel. Thanks, guys!
    QRect mapToView(int paragraph,int index);
};

#endif
