/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  This class manages everything DCC transfer related
  begin:     Mon Apr 21 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include "dcctransferhandler.h"
#include "konversationmainwindow.h"

DccTransferHandler::DccTransferHandler(KonversationMainWindow* newMainWindow)
{
    mainWindow=newMainWindow;
}

DccTransferHandler::~DccTransferHandler()
{
}

#include "dcctransferhandler.moc"
