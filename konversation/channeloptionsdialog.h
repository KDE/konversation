/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson
  email:     psn@linux.se
*/
#ifndef KONVERSATIONCHANNELOPTIONSDIALOG_H
#define KONVERSATIONCHANNELOPTIONSDIALOG_H

#include <kdialogbase.h>

#include <qstringlist.h>

namespace Konversation {

class ChannelOptionsUI;

class ChannelOptionsDialog : public KDialogBase
{
  Q_OBJECT
  public:
    ChannelOptionsDialog(const QString& channel, QWidget *parent = 0, const char *name = 0);
    ~ChannelOptionsDialog();

    QString topic();
    QStringList modes();

  public slots:
    void setTopicHistory(const QStringList& history);
    void setAllowedChannelModes(const QString& modes);
    void setModes(const QStringList& modes);

  protected slots:
    void topicHistoryItemClicked(QListViewItem* item);

  private:
    ChannelOptionsUI* m_widget;
};

};

#endif
