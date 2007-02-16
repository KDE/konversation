/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include <qlabel.h>

#include "dcctransferdetailedinfopanel.h"

DccTransferDetailedInfoPanel::DccTransferDetailedInfoPanel( QWidget* parent, const char* name )
    : DccTransferDetailedInfoPanelUI( parent, name )
{
}

DccTransferDetailedInfoPanel::~DccTransferDetailedInfoPanel()
{
}

#include "dcctransferdetailedinfopanel.moc"
