/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  List view item that carries an arbitrary value
  begin:     Fre Apr 25 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com

*/

#include "valuelistviewitem.h"


ValueListViewItem::ValueListViewItem(int newValue, QTreeWidget* parent, const QString& label)
: QTreeWidgetItem(parent, QStringList() << label)
{
    m_value=newValue;
//     enforceSortOrder();
}

ValueListViewItem::ValueListViewItem(int newValue, QTreeWidget* parent, QTreeWidgetItem* after, const QString& label)
: QTreeWidgetItem(parent,after)
{
    setText(0, label);
    m_value=newValue;
//     enforceSortOrder();
}

ValueListViewItem::~ValueListViewItem()
{
}

int ValueListViewItem::getValue() const
{
    return m_value;
}
