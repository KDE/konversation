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

#include <QDialog>
#include <QUrl>

class QCheckBox;
class KFileWidget;

class DccFileDialog : public QDialog
{

public:
    DccFileDialog(QWidget* parent);

    QList<QUrl> getOpenUrls(const QUrl &startDir = QUrl(), const QString& filter = QString(), const QString& caption = QString());

    bool passiveSend();

    QSize sizeHint() const;

private:
    KFileWidget* m_fileWidget;
    QCheckBox* m_checkBox;
};

#endif // DCCFILEDIALOG_H
