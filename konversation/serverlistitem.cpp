/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  serverlistitem.cpp  -  Holds the list items inside the server list preferences panel
  begin:     Sun Feb 10 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <konvidebug.h>
#include <klineedit.h>
#include <klistview.h>
#include <qevent.h>

#include "serverlistitem.h"

ServerListItem::ServerListItem(QListViewItem* parent,
                               int newId,
                               QString arg0,
                               QString arg1,
                               QString arg2,
                               QString arg3,
                               QString arg4,
                               QString arg5,
                               QString arg6,
                               QString arg7) :
                QCheckListItem(parent,QString::null,QCheckListItem::CheckBox)
{
  id=newId;
  setText(1,arg1);
  setText(2,arg2);
  setText(3,arg3);
  setText(4,arg4);
  setText(5,arg5);
  setText(6,arg6);
  setText(7,arg7);

  group=arg0;
  
  m_eventFilterInstalled=FALSE;
}

ServerListItem::~ServerListItem()
{
}

void ServerListItem::stateChange(bool state)
{
  emit stateChanged(this,state);
}

int ServerListItem::getId() const        { return id; }
QString ServerListItem::getGroup() const { return group; }


// The massive workaround because the FIXUP method of the QValidator doesn't get called
// from KListViewLineEdit::terminate, and the QLineEdit::keyPressEvent is bypassed for
// the terminate call.

void ServerListItem::startRename( int col ) 
{
  QListViewItem::startRename(col);
  KListView *lv=dynamic_cast<KListView*>(listView());
  m_klvle=dynamic_cast<KListViewLineEdit *>( lv->renameLineEdit() );
  if (listView()->columnText(col) == "Channel") {
    //m_klvle->setCompletionMode(KGlobalSettings::CompletionNone);
    m_klvle->setValidator( new ChannelListValidator(m_klvle) );
    m_klvle->installEventFilter(this);
    m_eventFilterInstalled=TRUE;
    connect(m_klvle,SIGNAL(done(QListViewItem*, int)),this,SLOT(done(QListViewItem*, int)));
  }
}

bool ServerListItem::eventFilter(QObject *obj, QEvent *event)
{ 
    //This shall only be set for the correct column
    if (m_eventFilterInstalled && event->type()==QEvent::KeyPress) {
        QKeyEvent* keyEvent=(QKeyEvent*) event;
        if(keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter ) {
            KListViewLineEdit *edit=dynamic_cast<KListViewLineEdit*>(obj);
            if (edit != m_klvle)
              KX << "<Robin> holy fuck, Batman! It broke already!" << endl;
            const QValidator * v = edit->validator();
            QString text=edit->text();
            int pos = edit->cursorPosition();
            if ( !v || v->validate( text,pos ) == QValidator::Acceptable ) {
                return FALSE;
            } else {
                QString vstr = edit->text();
                v->fixup( vstr );
                if ( vstr != edit->text() ) {
                    edit->setText( vstr );
                    text=edit->text();
                    pos=edit->cursorPosition();
                    if ( v->validate(text, pos) == QValidator::Acceptable )
                        return FALSE;
                }
                return TRUE; // XXX silently swallow the enter key-press!
            }
        }
    }
    return FALSE;
}

void ServerListItem::done(QListViewItem *item, int col)
{
  if (m_eventFilterInstalled) {
      m_klvle->clearValidator();
      m_klvle->removeEventFilter(this);
      m_eventFilterInstalled=FALSE;
  }
}


#include "serverlistitem.moc"
