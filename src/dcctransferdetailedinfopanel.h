/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#ifndef DCCTRANSFERDETAILEDINFOPANEL_H
#define DCCTRANSFERDETAILEDINFOPANEL_H

#include "dcctransferdetailedinfopanelui.h"

class DccTransferDetailedInfoPanel : public DccTransferDetailedInfoPanelUI
{
    Q_OBJECT

    public:
        DccTransferDetailedInfoPanel( QWidget* parent = 0, const char* name = 0 );
        virtual ~DccTransferDetailedInfoPanel();
};

#endif  // DCCTRANSFERDETAILEDINFOPANEL_H
