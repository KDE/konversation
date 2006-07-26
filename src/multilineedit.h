/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#ifndef MULTILINEEDIT_H
#define MULTILINEEDIT_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class QWidget;
class MultilineTextEdit;

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
        void slotUser1();

    protected:
        MultilineTextEdit* textEditor;
        static QString returnText;
};
#endif
