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
#include <qsize.h>

#include <klocale.h>
#include <kdebug.h>
#include <kguiitem.h>
#include <kmessagebox.h>

#include "preferences.h"
#include "konversationapplication.h"
#include "servergroupdialog.h"
#include "servergroupsettings.h"

namespace Konversation {
  //
  // ServerListItem
  //
  
  ServerListItem::ServerListItem(QListViewItem* parent, int serverId, const QString& serverGroup, 
                                 const QString& identity, const QString& channels, bool autoConnect)
    : KListViewItem(parent, serverGroup, identity, channels)
  {
    m_serverId = serverId;
    m_autoConnect = autoConnect;
  }

  ServerListItem::ServerListItem(QListView* parent, int serverId, const QString& serverGroup, 
                                 const QString& identity, const QString& channels, bool autoConnect)
    : KListViewItem(parent, serverGroup, identity, channels)
  {
    m_serverId = serverId;
    m_autoConnect = autoConnect;
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

/*  void ServerListItem::paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align)
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
    
//    lv->style().drawPrimitive(QStyle::PE_CheckListIndicator, p,
//      QRect(x, y, boxsize, fm.height() + 2 + marg), _cg, styleflags, QStyleOption(this));
    p->drawRect(x, y, boxsize, boxsize);
  
    if(autoConnect()) {
      p->drawLine(x, y, x + boxsize, y + boxsize);
      p->drawLine(x, y + boxsize, x + boxsize, y);
    }
  }
*/
  
  //
  // ServerListDialog
  //
  
  ServerListDialog::ServerListDialog(QWidget *parent, const char *name)
    : KDialogBase(Plain, i18n("Server List"), Ok|/*Apply|*/Cancel, Ok, parent, name, false)
  {
    m_preferences = &KonversationApplication::preferences;
    setButtonOK(KGuiItem(i18n("C&onnect"), "connect_creating", i18n("Connect to the server")));
    
    QFrame* mainWidget = plainPage();
    
    m_serverList = new KListView(mainWidget);
    m_serverList->setAllColumnsShowFocus(true);
    m_serverList->setRootIsDecorated(true);
    m_serverList->setResizeMode(QListView::AllColumns);
    m_serverList->addColumn(i18n("Network"));
    m_serverList->addColumn(i18n("Identity"));
    m_serverList->addColumn(i18n("Channels"));
    m_serverList->setSelectionMode(QListView::Multi);
    //m_serverList->addColumn(i18n("Auto Connect"));
    
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
    updateServerGroupList();
    
    connect(m_serverList, SIGNAL(doubleClicked(QListViewItem *, const QPoint&, int)), this, SLOT(slotOk()));
    connect(m_serverList, SIGNAL(selectionChanged()), this, SLOT(updateButtons()));
    connect(m_addButton, SIGNAL(clicked()), this, SLOT(slotAdd()));
    connect(m_editButton, SIGNAL(clicked()), this, SLOT(slotEdit()));
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(slotDelete()));
    
    updateButtons();
  
    KConfig* config = kapp->config();
    config->setGroup("ServerListDialog");
    QSize newSize = size();
    newSize = config->readSizeEntry("Size", &newSize);
    resize(newSize);
  }
  
  ServerListDialog::~ServerListDialog()
  {
    KConfig* config = kapp->config();
    config->setGroup("ServerListDialog");
    config->writeEntry("Size", size());
  }

  QListViewItem* ServerListDialog::findBranch(QString name, bool generate)
  {
    if(name.isEmpty()) {
      return 0;
    }

    QListViewItem* branch = m_serverList->firstChild();
    bool found = false;
    
    while(branch && !found)
    {
      if((branch->rtti() != 10001) && (branch->text(0) == name)) {
        found = true;
      } else {
        branch = branch->nextSibling();
      }
    }

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
/*    QListViewItem* branch = m_serverList->firstChild();
    ServerListItem* item;
    
    while(branch) {
      item = static_cast<ServerListItem*>(branch->firstChild());
      
      while(item) {
        m_preferences->changeServerProperty(item->serverId(), 6, item->autoConnect() ? "1" : "0");
        item = static_cast<ServerListItem*>(item->nextSibling());
      }
      
      branch = branch->nextSibling();
  }*/
  }
  
  void ServerListDialog::slotAdd()
  {
    ServerGroupDialog dlg(i18n("Add Network"), this);
    QStringList groups = createGroupList();
    dlg.setAvailableGroups(groups);

    if(dlg.exec() == KDialog::Accepted) {
      addServerGroup(dlg.serverGroupSettings());
      
      if(dlg.identitiesNeedsUpdate()) {
        updateServerGroupList();
      }
    }
  }

  void ServerListDialog::slotEdit()
  {
    ServerListItem* item = static_cast<ServerListItem*>(m_serverList->selectedItems().first());

    if(item)
    {
      Konversation::ServerGroupSettings serverGroup = m_preferences->serverGroupById(item->serverId());

      if(!serverGroup.name().isEmpty()) {
        ServerGroupDialog dlg(i18n("Edit Network"), this);
        QStringList groups = createGroupList();
        dlg.setAvailableGroups(groups);
        dlg.setServerGroupSettings(serverGroup);

        if(dlg.exec() == KDialog::Accepted) {
          // find branch the old item resides in
          QListViewItem* branch = item->parent();
          // remove item from the list
          delete item;

          // if the branch is empty, remove it
          if(branch && branch->childCount() == 0) {
            delete branch;
          }

          ServerGroupSettings serverGroup = dlg.serverGroupSettings();
          m_preferences->removeServerGroup(serverGroup.id());
          addServerGroup(serverGroup);

          if(dlg.identitiesNeedsUpdate()) {
            updateServerGroupList();
          }
        }
      }
    }
  }

  void ServerListDialog::slotDelete()
  {
    if(KMessageBox::warningContinueCancel(this, i18n("Do you really want to delete the selected networks?"))
       == KMessageBox::Cancel)
    {
      return;
    }

    QPtrList<QListViewItem> selected = m_serverList->selectedItems();
    ServerListItem * server = static_cast<ServerListItem*>(selected.first());

    while(server) {
      // find branch this item belongs to
      QListViewItem* branch = server->parent();
      // remove server from preferences
      m_preferences->removeServerGroup(server->serverId());
      // remove item from view
      delete server;
      // if the branch has no other items, remove it
      if(branch && branch->childCount() == 0) {
        delete branch;
      }

      server = static_cast<ServerListItem*>(selected.next());
    }
  }

  void ServerListDialog::updateButtons()
  {
    int count = m_serverList->selectedItems().count();
    bool enable = (count > 0);

    enableButtonOK(enable);
    m_delButton->setEnabled(enable);

    enable = (count == 1);
    m_editButton->setEnabled(enable);
  }

  void ServerListDialog::addServerGroup(const ServerGroupSettings& serverGroup)
  {
    m_preferences->addServerGroup(serverGroup);
    QListViewItem* item = addListItem(serverGroup);
    m_serverList->clearSelection();
    m_serverList->setSelected(item, true);
    m_serverList->ensureItemVisible(item);
  }

  QStringList ServerListDialog::createGroupList()
  {
    QStringList groups;
    QListViewItem* branch = m_serverList->firstChild();

    while(branch) {
      if(branch->rtti() != 10001) {
        groups.append(branch->text(0));
      }

      branch = branch->nextSibling();
    }
    
    return groups;
  }

  void ServerListDialog::updateServerGroupList()
  {
    m_serverList->clear();
    Konversation::ServerGroupList serverGroups = m_preferences->serverGroupList();
    Konversation::ServerGroupList::iterator it;

    for(it = serverGroups.begin(); it != serverGroups.end(); ++it) {
      addListItem((*it));
    }
  }

  QListViewItem* ServerListDialog::addListItem(const ServerGroupSettings& serverGroup)
  {
    QListViewItem* branch = findBranch(serverGroup.group());
    Konversation::ChannelList tmpList = serverGroup.channelList();
    Konversation::ChannelList::iterator it;
    Konversation::ChannelList::iterator begin = tmpList.begin();
    QString channels;

    for(it = begin; it != tmpList.end(); ++it) {
      if(it != begin) {
        channels += ',';
      }

      channels += (*it).name();
    }

    QListViewItem* item = 0;

    if(branch) {
      item = new ServerListItem(branch, serverGroup.id(),
                         serverGroup.name(),
                         serverGroup.identity()->getName(),
                         channels,
                         serverGroup.autoConnectEnabled());
    } else {
      item = new ServerListItem(m_serverList, serverGroup.id(),
                         serverGroup.name(),
                         serverGroup.identity()->getName(),
                         channels,
                         serverGroup.autoConnectEnabled());
    }
    
    return item;
  }

}

#include "serverlistdialog.moc"
