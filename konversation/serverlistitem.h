/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.h  -  Holds the list items inside the server list preferences panel
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com

*/


#ifndef SERVERLISTITEM_H
#define SERVERLISTITEM_H

#include <qvalidator.h>
#include <qstring.h>

/*
  @author Dario Abatianni
*/

#include <klistview.h>
#include <klineedit.h>

#include "konvidebug.h"

class KListViewLineEdit : public KLineEdit
{
Q_OBJECT
public:
	KListViewLineEdit(KListView *parent);
	~KListViewLineEdit();

	QListViewItem *currentItem() const;

signals:
	void done(QListViewItem*, int);

public slots:
	void terminate();
	void load(QListViewItem *i, int c);

protected:
	virtual void focusOutEvent(QFocusEvent *);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void paintEvent(QPaintEvent *e);
	virtual bool event (QEvent *pe);

	/// @since 3.1
	void selectNextCell (QListViewItem *pi, int column, bool forward);
	void terminate(bool commit);
	QListViewItem *item;
	int col;
	KListView *p;

protected slots:
	void slotSelectionChanged();

};


class ServerListItem : public QObject, public QCheckListItem
{
  Q_OBJECT

  public:
    ServerListItem(QListViewItem* parent,
                   int newId,
                   QString arg0,
                   QString arg1=QString::null,
                   QString arg2=QString::null,
                   QString arg3=QString::null,
                   QString arg4=QString::null,
                   QString arg5=QString::null,
                   QString arg6=QString::null,
                   QString arg7=QString::null);
    ~ServerListItem();

    int getId() const;
    QString getGroup() const;

  signals:
    void stateChanged(ServerListItem* myself,bool state);

  public slots:
    void done(QListViewItem*, int);
  
  protected:
    void stateChange(bool state);
    int id;
    QString group;
    
  private:
    virtual void startRename( int col );
    bool m_eventFilterInstalled;
    KListViewLineEdit *m_klvle;
    bool eventFilter(QObject *obj, QEvent *event); //Because KListViewLine edit does not call fixup for the validator
};


class ChannelListValidator: public QValidator
{
public:
  ChannelListValidator(QObject *parent, const char *name = 0): QValidator(parent, name) { KX << "validator created" << endl;}
  
  virtual QValidator::State validate(QString & input, int & pos) const  {
    KX << "validate" << endl;
    if (input.contains(','))
      return Intermediate;
    return Acceptable;
  }
  
  virtual void fixup(QString & input) const {
    KX << "fixup" << endl;
    if (input.contains(',')) {
      input.replace(','," ");
      input.simplifyWhiteSpace();
    }
  }
};

#endif
