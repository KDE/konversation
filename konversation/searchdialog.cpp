/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  searchdialog.cpp  -  The search dialog for text views
  begin:     Son Mär 16 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
 
  $Id$
*/

#include <klocale.h>
#include <kcombobox.h>
#include <kdebug.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qhbox.h>

#include "searchdialog.h"

QStringList SearchDialog::lastSearchPatterns;
bool SearchDialog::caseSensitive=false;
bool SearchDialog::wholeWords=false;
bool SearchDialog::forward=false;
bool SearchDialog::fromCursor=false;

SearchDialog::SearchDialog(QWidget* parent,QSize size) :
               KDialogBase(parent,"search_dialog",true,i18n("Search text"),
                           KDialogBase::Ok | KDialogBase::Cancel,KDialogBase::Ok,true)
{
  kdDebug() << "SearchDialog::SearchDialog()" << endl;

  // Create the top level widget
  QWidget* page=new QWidget(this);
  setMainWidget(page);
  
  // Add the layout to the widget
  QGridLayout* dialogLayout=new QGridLayout(page,2,2);
  dialogLayout->setSpacing(spacingHint());

  QHBox* searchBox=new QHBox(page);
  
  QLabel* searchLabel=new QLabel(i18n("Search text"),searchBox,"search_label");
  searchPattern=new KComboBox(searchBox);
  searchPattern->setEditable(true);
  searchPattern->insertStringList(lastSearchPatterns);

  searchBox->setStretchFactor(searchPattern,10);
  searchBox->setSpacing(spacingHint());

  caseSensitiveCheck=new QCheckBox(i18n("Case sensitive"),page,"case_sensitive_check");
  caseSensitiveCheck->setChecked(caseSensitive);
  
  wholeWordsCheck=new QCheckBox(i18n("Whole words"),page,"whole_words_check");
  wholeWordsCheck->setChecked(wholeWords);
  
  forwardCheck=new QCheckBox(i18n("Search forward"),page,"forward_check");
  forwardCheck->setChecked(forward);

  fromCursorCheck=new QCheckBox(i18n("From cursor"),page,"from_cursor_check");
  fromCursorCheck->setChecked(fromCursor);

  int row=0;
  
  dialogLayout->addMultiCellWidget(searchBox,row,0,row,1);
  row++;
  dialogLayout->addWidget(caseSensitiveCheck,row,0);
  dialogLayout->addWidget(wholeWordsCheck,row,1);
  row++;
  dialogLayout->addWidget(forwardCheck,row,0);
  dialogLayout->addWidget(fromCursorCheck,row,1);

  connect(searchPattern,SIGNAL (returnPressed(const QString&)),this,SLOT (newPattern(const QString&)));
  connect(caseSensitiveCheck,SIGNAL (stateChanged(int)),this,SLOT (caseSensitiveChanged(int)));
  connect(wholeWordsCheck,SIGNAL (stateChanged(int)),this,SLOT (wholeWordsChanged(int)));
  connect(forwardCheck,SIGNAL (stateChanged(int)),this,SLOT (forwardChanged(int)));
  connect(fromCursorCheck,SIGNAL (stateChanged(int)),this,SLOT (fromCursorChanged(int)));

  setButtonOKText(i18n("OK"),i18n("Search for text"));
  setButtonCancelText(i18n("Cancel"),i18n("Close the window"));

  setInitialSize(size);
  searchPattern->setFocus();
  show();
}

SearchDialog::~SearchDialog()
{
}

QString SearchDialog::getSearchText()
{
  return searchPattern->currentText();
}

QString SearchDialog::search(QWidget* parent,bool* cs,bool* wo,bool* fw,bool* fc)
{
  kdDebug() << "SearchDialog::search()" << endl;

  QSize size; // TODO: get it from KonversationApplication::preferences
  SearchDialog dlg(parent,size);
  dlg.exec();

  *cs=caseSensitive;
  *wo=wholeWords;
  *fw=forward;
  *fc=fromCursor;
    
  return dlg.getSearchText();
}

void SearchDialog::newPattern(const QString& pattern)
{
  lastSearchPatterns.prepend(pattern);
}

void SearchDialog::caseSensitiveChanged(int state)
{
  caseSensitive=(state==2);
}

void SearchDialog::wholeWordsChanged(int state)
{
  wholeWords=(state==2);
}

void SearchDialog::forwardChanged(int state)
{
  forward=(state==2);
}

void SearchDialog::fromCursorChanged(int state)
{
  fromCursor=(state==2);
}

#include "searchdialog.moc"
