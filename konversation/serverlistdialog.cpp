/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#include "serverlistdialog.h"

#include <qpushbutton.h>
#include <qframe.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qstyle.h>
#include <qrect.h>
#include <qpoint.h>
#include <qsize.h>
#include <qheader.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qpainter.h>

#include <klocale.h>
#include <kdebug.h>
#include <kguiitem.h>

#include "preferences.h"
#include "konversationapplication.h"
#include "editserverdialog.h"

namespace Konversation {
  //
  // ServerListItem
  //
  
  ServerListItem::ServerListItem(QListViewItem* parent, int serverId, const QString& group, const QString& server, 
    const QString& port, const QString& identity, bool autoConnect)
    : KListViewItem(parent, server + ":" + port, identity)
  {
    m_serverId = serverId;
    m_autoConnect = autoConnect;
    m_group = group;
  }

  void ServerListItem::setAutoConnect(bool ac)
  {
    m_autoConnect = ac;
    repaint();
  }
  
  void ServerListItem::activate()
  {
    QPoint pos;
    
    if(activatedPos(pos)) {
      QListView* lv = listView();
      QRect r;
      //pos = pos + r.topLeft();
      int boxsize = lv->style().pixelMetric(QStyle::PM_CheckListButtonSize, lv);
      int marg = lv->itemMargin();
      int x = lv->header()->sectionPos(2) - (lv->treeStepSize() * (depth() + 1));
      x += ((lv->header()->sectionSize(2) - boxsize) / 2) + marg;
      r.setTopLeft(QPoint(x, 0));
      r.setWidth(boxsize);
      r.setHeight(boxsize);
      
      if(!r.contains(pos)) {
        return;
      }
    }
    
    setAutoConnect(!autoConnect());
  }

  void ServerListItem::paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align)
  {
    if(column != 2) {
      KListViewItem::paintCell(p, cg, column, width, align);
      return;
    }
    
    QListView* lv = listView();
    
    if(!lv) return;
    
    // Copied from KListViewItem::paintCell()
    QColorGroup _cg = cg;
    const QPixmap *pm = lv->viewport()->backgroundPixmap();
    
    if (pm && !pm->isNull()) {
      _cg.setBrush(QColorGroup::Base, QBrush(backgroundColor(), *pm));
      QPoint o = p->brushOrigin();
      p->setBrushOrigin( o.x()-lv->contentsX(), o.y()-lv->contentsY() );
    } else if (isAlternate()) {
      if (lv->viewport()->backgroundMode() == Qt::FixedColor) {
        _cg.setColor(QColorGroup::Background, static_cast<KListView*>(lv)->alternateBackground());
      } else {
        _cg.setColor(QColorGroup::Base, static_cast<KListView*>(lv)->alternateBackground());
      }
    }
    // End copy
    
    // Copied from QCheckListItem::paintCell()
    const BackgroundMode bgmode = lv->viewport()->backgroundMode();
    const QColorGroup::ColorRole crole = QPalette::backgroundRoleFromMode(bgmode);    
    p->fillRect(0, 0, width, height(), _cg.brush(crole));
    
    QFontMetrics fm(lv->fontMetrics());
    int boxsize = lv->style().pixelMetric(QStyle::PM_CheckListButtonSize, lv);
    int marg = lv->itemMargin();
    
    int styleflags = QStyle::Style_Default;
    
    if(autoConnect()) {
      styleflags |= QStyle::Style_On;
    } else {
      styleflags |= QStyle::Style_Off;
    }
    
    if(isSelected()) {
      styleflags |= QStyle::Style_Selected;
    }
    
    if(isEnabled() && lv->isEnabled()) {
      styleflags |= QStyle::Style_Enabled;
    }
    
    int y = ((height() - boxsize) / 2) + marg;
    // End copy

    int x = ((width - boxsize) / 2) + marg;
    
/*    lv->style().drawPrimitive(QStyle::PE_CheckListIndicator, p,
      QRect(x, y, boxsize, fm.height() + 2 + marg), _cg, styleflags, QStyleOption(this));*/
    p->drawRect(x, y, boxsize, boxsize);
  
    if(autoConnect()) {
      p->drawLine(x, y, x + boxsize, y + boxsize);
      p->drawLine(x, y + boxsize, x + boxsize, y);
    }
  }
  
  //
  // ServerListDialog
  //
  
  ServerListDialog::ServerListDialog(QWidget *parent, const char *name)
    : KDialogBase(Plain, i18n("Server List"), Ok|Apply|Cancel, Ok, parent, name)
  {
    m_preferences = &KonversationApplication::preferences;
    setButtonOK(KGuiItem(i18n("C&onnect"), "connect_creating", i18n("Connect to the server")));
    
    QFrame* mainWidget = plainPage();
    
    m_serverList = new KListView(mainWidget);
    m_serverList->setAllColumnsShowFocus(true);
    m_serverList->setRootIsDecorated(true);
    m_serverList->addColumn(i18n("Server"));
    m_serverList->addColumn(i18n("Identity"));
    m_serverList->addColumn(i18n("Auto Connect"));
    
    m_addButton = new QPushButton(i18n("A&dd..."), mainWidget);
    m_editButton = new QPushButton(i18n("&Edit..."), mainWidget);
    m_delButton = new QPushButton(i18n("&Delete"), mainWidget);
    
    QGridLayout* layout = new QGridLayout(mainWidget, 1, 2, 0, spacingHint());
    
    layout->addMultiCellWidget(m_serverList, 0, 3, 0, 0);
    layout->addWidget(m_addButton, 0, 1);
    layout->addWidget(m_editButton, 1, 1);
    layout->addWidget(m_delButton, 2, 1);
    layout->setRowStretch(3, 10);
  
    // Load server list
    int index = 0;
    QString serverString = m_preferences->getServerByIndex(index);
    
    while(!serverString.isEmpty())
    {
      int id = m_preferences->getServerIdByIndex(index);
      QStringList serverEntry = QStringList::split(',', serverString, true);
  
      QListViewItem* branch = findBranch(serverEntry[0]);
      new ServerListItem(branch, id,
                        serverEntry[0],
                        serverEntry[1],
                        serverEntry[2],
                        serverEntry[7],
                        serverEntry[6] == "1");
  
      serverString = m_preferences->getServerByIndex(++index);
    }
    
    connect(m_serverList, SIGNAL(doubleClicked(QListViewItem *, const QPoint&, int)), this, SLOT(slotOk()));
    connect(m_serverList, SIGNAL(selectionChanged()), this, SLOT(updateButtons()));
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(m_editButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(slotDelete()));
    
    updateButtons();
  }
  
  ServerListDialog::~ServerListDialog()
  {
  }

  QListViewItem* ServerListDialog::findBranch(QString name, bool generate)
  {
    QListViewItem* branch = m_serverList->findItem(name, 0);
    
    if(branch == 0 && generate == true)
    {
      branch = new KListViewItem(m_serverList, name);
      branch->setOpen(true);
      branch->setSelectable(false);
    }
  
    return branch;
  }

  void ServerListDialog::slotOk()
  {
    QPtrList<QListViewItem> selected = m_serverList->selectedItems();
    ServerListItem * server = static_cast<ServerListItem*>(selected.first());
    
    while(server) {
      emit connectToServer(server->serverId());
      
      server = static_cast<ServerListItem*>(selected.next());
    }
    
    slotApply();
    accept();
  }

  void ServerListDialog::slotApply()
  {
    QListViewItem* branch = m_serverList->firstChild();
    ServerListItem* item;
    
    while(branch) {
      item = static_cast<ServerListItem*>(branch->firstChild());
      
      while(item) {
        m_preferences->changeServerProperty(item->serverId(), 6, item->autoConnect() ? "1" : "0");
        item = static_cast<ServerListItem*>(item->nextSibling());
      }
      
      branch = branch->nextSibling();
    }
  }
  
  void ServerListDialog::slotAdd()
  {
    EditServerDialog editServerDialog(this, i18n("New"), QString::null, "6667", QString::null, QString::null,
      QString::null, QString::null, QString::null);
  
    connect(&editServerDialog, SIGNAL(serverChanged(const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&)),
                            this, SLOT(createServer(const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&,
                                                    const QString&)));
    
    editServerDialog.exec();
  }
  
  void ServerListDialog::slotEdit()
  {
    ServerListItem* item = static_cast<ServerListItem*>(m_serverList->selectedItems().first());
    
    if(item)
    {
      QString server = m_preferences->getServerById(item->serverId());
      
      if(!server.isEmpty()) {
        QStringList properties = QStringList::split(',', server, true);
        EditServerDialog editServerDialog(this, properties[0], properties[1], properties[2], properties[3],
          properties[4], properties[5], properties[7], properties[8]);
  
        connect(&editServerDialog, SIGNAL(serverChanged(const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&)),
                                this, SLOT(updateServer(const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&,
                                                        const QString&)));
        
        editServerDialog.exec();
      }
    }
  }
  
  void ServerListDialog::slotDelete()
  {
    QPtrList<QListViewItem> selected = m_serverList->selectedItems();
    ServerListItem * server = static_cast<ServerListItem*>(selected.first());
    
    while(server) {
      // find branch this item belongs to
      QListViewItem* branch = findBranch(server->group());
      // remove server from preferences
      m_preferences->removeServer(server->serverId());
      // remove item from view
      delete server;
      // if the branch has no other items, remove it
      if(branch->childCount() == 0) {
        delete branch;
      }
      
      server = static_cast<ServerListItem*>(selected.next());
    }
  }

  void ServerListDialog::createServer(const QString& groupName, const QString& serverName,
    const QString& serverPort, const QString& serverKey, const QString& channelName, const QString& channelKey,
    const QString& identity, const QString& connectCommands)
  {
    
    int id = m_preferences->addServer(groupName + "," + serverName + "," + serverPort + "," + serverKey + "," +
      channelName + "," + channelKey + ",0," + identity + "," + connectCommands);
    
    // find branch to insert the new item into
    QListViewItem* branch = findBranch(groupName);
  
    ServerListItem* item = new ServerListItem(branch, id, groupName, serverName, serverPort, identity, false);
  
    m_serverList->setSelected(item, true);
    m_serverList->ensureItemVisible(item);
  }

  void ServerListDialog::updateServer(const QString& groupName, const QString& serverName, const QString& serverPort,
    const QString& serverKey, const QString& channelName, const QString& channelKey, const QString& identity,
    const QString& connectCommands)
  {
    ServerListItem* item = static_cast<ServerListItem*>(m_serverList->selectedItems().first());
    // save state of autoconnect checkbox
    bool autoConnect = item->autoConnect();
    // find branch the old item resides in
    QListViewItem* branch = findBranch(item->group());
    // save server id of the old item
    int id = item->serverId();
    // remove item from the list
    delete item;
    
    // if the branch is empty, remove it
    if(branch->childCount() == 0) {
      delete branch;
    }
  
    // find branch to insert the new item into
    branch = findBranch(groupName);
  
    item = new ServerListItem(branch, id, groupName, serverName, serverPort, identity, autoConnect);
  
    m_preferences->updateServer(id, groupName + "," + serverName + "," + serverPort + "," + serverKey + "," +
      channelName + "," + channelKey + "," + (autoConnect ? "1" : "0") + "," + identity + "," + connectCommands);
  
    m_serverList->setSelected(item, true);
    m_serverList->ensureItemVisible(item);
  }

  void ServerListDialog::updateButtons()
  {
    bool enable = m_serverList->selectedItems().count() > 0;
    
    enableButtonOK(enable);
    m_editButton->setEnabled(enable);
    m_delButton->setEnabled(enable);
  }
}

#include "serverlistdialog.moc"
