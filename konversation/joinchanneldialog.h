/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson <psn@linux.se>
*/
#ifndef KONVERSATIONJOINCHANNELDIALOG_H
#define KONVERSATIONJOINCHANNELDIALOG_H

#include <kdialogbase.h>

namespace Konversation {

class JoinChannelUI;

class JoinChannelDialog : public KDialogBase
{
  Q_OBJECT
  public:
    JoinChannelDialog(const QString& network, QWidget *parent = 0, const char *name = 0);
    ~JoinChannelDialog();
    
    QString channel() const;
    QString password() const;

  private:
    JoinChannelUI* m_widget;
};

}

#endif
