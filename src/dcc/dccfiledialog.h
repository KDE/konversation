/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2011 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef DCCFILEDIALOG_H
#define DCCFILEDIALOG_H

#include <KFileDialog>

class QCheckBox;

class DccFileDialog : public KFileDialog
{

public:
    DccFileDialog(const KUrl& startDir, const QString& filter, QWidget* parent, QWidget* widget = 0);
    
    KUrl::List getOpenUrls(const KUrl& startDir = KUrl(), const QString& filter = QString(), const QString& caption = QString());

    bool passiveSend();
private:
    QCheckBox* m_checkBox;
};

#endif // DCCFILEDIALOG_H
