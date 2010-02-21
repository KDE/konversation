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
/*
  Copyright (C) 2009 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef RECIPIENTDIALOG_H
#define RECIPIENTDIALOG_H

#include <QModelIndex>

#include <KDialog>

class KLineEdit;

namespace Konversation
{
    namespace DCC
    {
        class RecipientDialog : public KDialog
        {
            Q_OBJECT

            public:
                RecipientDialog(QWidget* parent, QAbstractListModel* model);
                ~RecipientDialog();

                static QString getNickname(QWidget* parent, QAbstractListModel* model);

            protected slots:
                void newNicknameSelected(const QModelIndex& index);
                                                          // KDE double click
                void newNicknameSelectedQuit(const QModelIndex& index);

                void slotOk();
                void slotCancel();

            protected:
                QString getSelectedNickname();
                static QString selectedNickname;

                KLineEdit* nicknameInput;
        };
    }
}

#endif
