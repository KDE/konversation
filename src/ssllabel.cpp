/*
  Copyright (c) 2004 by İsmail Dönmez <ismail.donmez@boun.edu.tr>

  *************************************************************************
  *                                                                       *
  * This program is free software; you can redistribute it and/or modify  *
  * it under the terms of the GNU General Public License as published by  *
  * the Free Software Foundation; either version 2 of the License, or     *
  * (at your option) any later version.                                   *
  *                                                                       *
  *************************************************************************

 */

#include "ssllabel.h"


SSLLabel::SSLLabel(QWidget* parent)
: QLabel(parent)
{
}

void SSLLabel::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    emit clicked();
}

#include "ssllabel.moc"
