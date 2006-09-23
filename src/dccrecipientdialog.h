/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  lets the user choose a nick from the list
  begin:     Sam Dez 7 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef DCCRECIPIENTDIALOG_H
#define DCCRECIPIENTDIALOG_H

#include <kdialogbase.h>

/*
  @author Dario Abatianni
*/

class KLineEdit;

class DccRecipientDialog : public KDialogBase
{
    Q_OBJECT

        public:
        DccRecipientDialog(QWidget* parent, const QStringList &list, const QSize &size);
        ~DccRecipientDialog();

        static QString getNickname(QWidget* parent, const QStringList& list);

    protected slots:
        void newNicknameSelected(QListBoxItem* item);
                                                  // KDE double click
        void newNicknameSelectedQuit(QListBoxItem* item);

        void slotOk();
        void slotCancel();

    protected:
        QString getSelectedNickname();
        static QString selectedNickname;

        KLineEdit* nicknameInput;
};
#endif
