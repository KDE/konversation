/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 by Peter Simonsson <psn@linux.se>
*/
#include "joinchanneldialog.h"

#include <qlabel.h>

#include <klocale.h>
#include <kcombobox.h>
#include <klineedit.h>

#include "joinchannelui.h"

namespace Konversation {

JoinChannelDialog::JoinChannelDialog(const QString& network, QWidget *parent, const char *name)
  : KDialogBase(parent, name, true, i18n("Join Channel on %1").arg(network), Ok|Cancel, Ok)
{
  m_widget = new JoinChannelUI(this);
  setMainWidget(m_widget);

  m_widget->serverLbl->setText(network);
}

JoinChannelDialog::~JoinChannelDialog()
{
}

QString JoinChannelDialog::channel() const
{
  return m_widget->channelCombo->currentText();
}

QString JoinChannelDialog::password() const
{
  return m_widget->passwordEdit->text();
}

}

#include "joinchanneldialog.moc"
