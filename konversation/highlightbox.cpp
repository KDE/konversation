/***************************************************************************
                          highlightbox.cpp  -  description
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

#include "highlightbox.h"
#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>

HighLightBox::HighLightBox(QStringList passed_highLightList, QSize passed_windowSize)
{
  highLightList = passed_highLightList;
  windowSize = passed_windowSize;

  GroupBox = new QVGroupBox(i18n("Highlight List"), this);
  HighLightListEditBox = new QVBox(GroupBox);
  InputLineBox = new QHBox(GroupBox);
  InputLineBox->setSpacing(KDialog::spacingHint());
  ButtonsBox = new QHBox(this);
  ButtonsBox->setSpacing(KDialog::spacingHint());

  HighLightListEdit = new KListBox(HighLightListEditBox);
  InputLineLabel = new QLabel(i18n("Pattern:"), InputLineBox);
  InputLine = new KLineEdit(InputLineBox);
  RemoveButton = new KPushButton(i18n("Remove"), InputLineBox);
  OkayButton = new KPushButton(i18n("Okay"), ButtonsBox);
  CancelButton = new KPushButton(i18n("Cancel"), ButtonsBox);

  connect(HighLightListEdit, SIGNAL(executed(QListBoxItem*)), this, SLOT(updateInputLine(QListBoxItem*)));
  connect(RemoveButton, SIGNAL(clicked()), this, SLOT(removeItemFromList()));
  connect(OkayButton, SIGNAL(clicked()), this, SLOT(closeWindowReturnChanges()));
  connect(CancelButton, SIGNAL(clicked()), this, SLOT(closeWindowDiscardingChanges()));
  connect(InputLine, SIGNAL(textChanged(const QString &)), this, SLOT(updateListItem(const QString &)));
  connect(InputLine, SIGNAL(returnPressed()), this, SLOT(addItemToList()));

  GroupBox->setAlignment(Qt::AlignHCenter);

  this->setMargin(KDialog::marginHint());
  this->setSpacing(KDialog::spacingHint());
  this->setCaption(i18n("Edit Highlight List"));
  this->resize(windowSize);
  this->show();

  HighLightListEdit->insertStringList(highLightList);
}

HighLightBox::~HighLightBox()
{
}


void HighLightBox::updateInputLine(QListBoxItem* selectedItem)
{
  if(HighLightListEdit->isSelected(HighLightListEdit->currentItem()))
  {
    HighLightListEdit->setCurrentItem(selectedItem);
    InputLine->setText(HighLightListEdit->currentText());
    InputLine->setFocus();
  }
}

void HighLightBox::addItemToList()
{
  if((!(HighLightListEdit->isSelected(HighLightListEdit->currentItem()))) && (!(InputLine->text() == "")) && ((HighLightListEdit->findItem(InputLine->text())) == 0))
  {
    HighLightListEdit->insertItem(InputLine->text(), 0);
    HighLightListEdit->setCurrentItem(HighLightListEdit->count());
    InputLine->setText("");
  }
  HighLightListEdit->setSelected(HighLightListEdit->currentItem(), false);
  InputLine->setText("");
}

void HighLightBox::updateListItem(const QString &passed_inputText)
{
  if(HighLightListEdit->isSelected(HighLightListEdit->currentItem()))
  {
    inputText = passed_inputText;
    HighLightListEdit->changeItem(inputText, (HighLightListEdit->currentItem()));
  }
}

void HighLightBox::removeItemFromList()
{
  if(HighLightListEdit->isSelected(HighLightListEdit->currentItem()))
  {
    HighLightListEdit->removeItem(HighLightListEdit->currentItem());
    InputLine->setText("");
  }
}

void HighLightBox::closeWindowDiscardingChanges()
{
  emit highLightListClose(this->size());
}

void HighLightBox::closeWindowReturnChanges()
{
  unsigned int i;

  highLightList.clear();

  for(i = 0; i < (HighLightListEdit->count()); i++)
  {
    highLightList.append(HighLightListEdit->text(i));
  }
  emit highLightListChange(highLightList);
  emit highLightListClose(this->size());
}

void HighLightBox::closeEvent(QCloseEvent *ev)
{
  ev->ignore();
  emit highLightListClose(this->size());
}
