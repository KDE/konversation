/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Die Jun 25 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#ifndef IGNORELISTVIEWITEM_H
#define IGNORELISTVIEWITEM_H

#include <QString>

#include <QTreeWidget>


class IgnoreListViewItem : public QTreeWidgetItem
{
    public:
        IgnoreListViewItem(QTreeWidget* parent, const QString& name, int flags);
        ~IgnoreListViewItem() override;

        void setFlag(int flag,bool active);
        bool getFlag(int flag) const { return m_flags & flag; };
        QString getName() const { return text(0); };
        void setName(const QString &name) { setText(0, name); };
        int getFlags() const { return m_flags; };

        void setFlags(int flags);
    protected:
        int m_flags;
};

#endif
