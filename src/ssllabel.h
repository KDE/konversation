#ifndef SSLLABEL_H
#define SSLLABEL_H

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

#include <QLabel>

class SSLLabel : public QLabel
{
    Q_OBJECT

    public:
        explicit SSLLabel(QWidget* parent);

    protected:
        void mouseReleaseEvent(QMouseEvent *e);

        Q_SIGNALS:
        void clicked();
};
#endif
