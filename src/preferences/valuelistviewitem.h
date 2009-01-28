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

#ifndef VALUELISTVIEWITEM_H
#define VALUELISTVIEWITEM_H

#include <k3listview.h>


class ValueListViewItem : public K3ListViewItem
{
    public:
        ValueListViewItem(int newValue, K3ListView* parent, const QString& label);
        ValueListViewItem(int newValue, K3ListView* parent, Q3ListViewItem* after, const QString& label);
        ~ValueListViewItem();

        int getValue() const;

    protected:
        int m_value;
};
#endif
