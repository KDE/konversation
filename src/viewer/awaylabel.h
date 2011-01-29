/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2004, 2009 Peter Simonsson <peter.simonsson@gmail.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#ifndef KONVERSATIONAWAYLABEL_H
#define KONVERSATIONAWAYLABEL_H

#include <QLabel>


class AwayLabel : public QLabel
{
    Q_OBJECT

    public:
        explicit AwayLabel(QWidget *parent = 0);
        ~AwayLabel();

    signals:
        void awayMessageChanged(const QString&);
        void unaway();

    protected slots:
        void changeAwayMessage();
};

#endif
