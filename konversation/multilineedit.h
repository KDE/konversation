//
// C++ Interface: multilineedit
//
// Description: Multi line edit widget for pasting into line edit
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef MULTILINEEDIT_H
#define MULTILINEEDIT_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class QWidget;
class QTextEdit;

class MultilineEdit : public KDialogBase
{
  Q_OBJECT

  public:
    MultilineEdit(QWidget* parent,QString text);
    ~MultilineEdit();

    static QString edit(QWidget* parent,QString text);

  protected slots:
    void slotOk();
    void slotCancel();

  protected:
    QTextEdit* textEditor;
    static QString returnText;
};

#endif
