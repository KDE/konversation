/*
    This file is part of libkabc.
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef KABC_ADDRESSEEDIALOG_H
#define KABC_ADDRESSEEDIALOG_H

#include <q3dict.h>

#include <klineedit.h>
#include <k3listview.h>

#include <kabc/addressbook.h>

/**
  @short Special ListViewItem
*/
class AddresseeItem : public K3ListViewItem
{
    public:

        /**
          Type of column
          @li @p Name -  Name in Addressee
          @li @p Email - Email in Addressee
        */
        enum columns { Photo =0, Name = 1, Email = 2 };

        /**
          Constructor.

          @param parent    The parent listview.
          @param addressee The associated addressee.
        */
        AddresseeItem( Q3ListView *parent, const KABC::Addressee &addressee );

        /**
          Returns the addressee.
        */
        KABC::Addressee addressee() const { return mAddressee; }

        /**
          Method used by QListView to sort the items.
        */
        virtual QString key( int column, bool ascending ) const;

    private:
        KABC::Addressee mAddressee;
};
#endif
