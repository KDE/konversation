/*
    SPDX-License-Identifier: GPL-2.0-or-later

    SPDX-FileCopyrightText: 2011 Bernd Buschinski <b.buschinski@web.de>
*/

#ifndef DCCFILEDIALOG_H
#define DCCFILEDIALOG_H

#include <QDialog>
#include <QUrl>

class QCheckBox;
class KFileWidget;

class DccFileDialog : public QDialog
{
    Q_OBJECT

public:
    DccFileDialog(QWidget* parent);

    QList<QUrl> getOpenUrls(const QUrl &startDir = QUrl(), const QString& caption = QString());

    bool passiveSend() const;

    QSize sizeHint() const override;

private:
    KFileWidget* m_fileWidget;
    QCheckBox* m_checkBox;
};

#endif // DCCFILEDIALOG_H
