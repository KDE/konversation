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
#ifndef KONVERSATIONSERVERLISTDIALOG_H
#define KONVERSATIONSERVERLISTDIALOG_H

#include <kdialogbase.h>
#include <klistview.h>

class Preferences;
class QPushButton;

namespace Konversation {
  class ServerGroupSettings;
  
  class ServerListItem : public KListViewItem
  {
    public:
      ServerListItem(QListViewItem* parent, int serverId, const QString& group, const QString& serverGroup, 
        const QString& identity, bool autoConnect);
      ServerListItem(QListView* parent, int serverId, const QString& group, const QString& serverGroup, 
        const QString& identity, bool autoConnect);

      int serverId() const { return m_serverId; }
      QString group() const { return m_group; }
      bool autoConnect() const { return m_autoConnect; }
      void setAutoConnect(bool ac);
      virtual void paintCell(QPainter* p, const QColorGroup& cg, int column, int width, int align);
      
    protected:
      virtual void activate();
    
    private:
      int m_serverId;
      bool m_autoConnect;
      QString m_group;
  };
  
  class ServerListDialog : public KDialogBase
  {
    Q_OBJECT
    public:
      ServerListDialog(QWidget *parent = 0, const char *name = 0);
      ~ServerListDialog();

    signals:
      void connectToServer(int serverId);

    protected slots:
      virtual void slotOk();
      virtual void slotApply();

      void slotAdd();
      void slotEdit();
      void slotDelete();

      void updateButtons();

    protected:
      QListViewItem* findBranch(QString name, bool generate = true);
      void addServerGroup(const ServerGroupSettings& serverGroup);

    private:
      KListView* m_serverList;
      QPushButton* m_addButton;
      QPushButton* m_editButton;
      QPushButton* m_delButton;
      Preferences* m_preferences;
  };
};

#endif
