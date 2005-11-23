/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include <qcombobox.h>
#include <klocale.h>

#include "ex_dcc_preferences.h"

DCC_Config_Ext::DCC_Config_Ext(QWidget *parent, const char* name) :
  DCC_Settings(parent,name)
{
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Network Interface"));
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Reply From IRC Server"));
    kcfg_DccMethodToGetOwnIp->insertItem(i18n("Specify Manually"));
}

DCC_Config_Ext::~DCC_Config_Ext()
{
}
