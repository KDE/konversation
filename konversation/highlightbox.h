/***************************************************************************
                          highlightbox.h  -  description
                             -------------------
    begin                : Tue May 28 2002
    copyright            : (C) 2002 by Matthias Gierlings
    email                : gismore@users.sourcefoge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef HIGHLIGHTBOX_H
#define HIGHLIGHTTBOX_H

#include <qvbox.h>
#include <qhbox.h>
#include <klistbox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <qvgroupbox.h>
#include <qsize.h>

class HighLightBox : public QVBox
{
  Q_OBJECT

  private:
    QVBox           *HighLightListEditBox;
    QHBox           *InputLineBox, *ButtonsBox;
    KListBox        *HighLightListEdit;
    KLineEdit       *InputLine;
    KPushButton     *OkayButton, *CancelButton, *RemoveButton, *UndoButton;
    QLabel          *InputLineLabel;
    QVGroupBox      *GroupBox;
    QListBoxItem    *selectedItem;
    QStringList     highLightList;
    QString         OkayButtonLabel, CancelButtonLabel, inputText;
    QSize           windowSize;
    int             selectedItemsIndex, i;

  protected:
    void closeEvent(QCloseEvent *ev);

  public:
    HighLightBox(QStringList passed_HighLightList, QSize passed_windowSize);
    ~HighLightBox();


  signals:
    void highLightListChange(QStringList HighLightList);
    void highLightListClose(QSize);

  protected slots:
    void updateInputLine(QListBoxItem* selectedItem);
    void updateListItem(const QString &inputText);
    void addItemToList();
    void removeItemFromList();

  public slots:
    void closeWindowDiscardingChanges();
    void closeWindowReturnChanges();
};

#endif
