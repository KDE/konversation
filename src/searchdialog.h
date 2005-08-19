/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The search dialog for text views
  begin:     Son Mär 16 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <kdialogbase.h>

/*
 Dario Abatianni
*/

class QCheckBox;

class KComboBox;

class SearchDialog : public KDialogBase
{
    Q_OBJECT

        public:
        SearchDialog(QWidget* parent,QSize size);
        ~SearchDialog();

        static QString search(QWidget* parent,bool* cs,bool* wo,bool* fw,bool* fc);

    protected slots:
        void caseSensitiveChanged(int cs);
        void wholeWordsChanged(int wo);
        void forwardChanged(int fw);
        void fromCursorChanged(int fw);

    protected:
        QString getSearchText();

        static QStringList lastSearchPatterns;
        static bool caseSensitive;
        static bool wholeWords;
        static bool forward;
        static bool fromCursor;

        KComboBox* searchPattern;

        QCheckBox* caseSensitiveCheck;
        QCheckBox* wholeWordsCheck;
        QCheckBox* forwardCheck;
        QCheckBox* fromCursorCheck;
};
#endif
