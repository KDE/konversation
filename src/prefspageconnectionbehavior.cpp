/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2005 by Peter Simonsson <psn@linux.se>
*/
#include "prefspageconnectionbehavior.h"

#include <qcheckbox.h>
#include <qgroupbox.h>

#include <knuminput.h>

#include "preferences.h"

PrefsPageConnectionBehavior::PrefsPageConnectionBehavior(QWidget* newParent, Preferences* newPreferences)
  : ConnectionBehavior_Config(newParent)
{
  preferences = newPreferences;

  rawLogCheck->setChecked(preferences->getRawLog());

  reconnectGBox->setChecked(preferences->getAutoReconnect());
  reconnectTimeoutSpin->setValue(preferences->getMaximumLagTime());
  autoRejoinCheck->setChecked(preferences->getAutoRejoin());
}

PrefsPageConnectionBehavior::~PrefsPageConnectionBehavior()
{
}

void PrefsPageConnectionBehavior::applyPreferences()
{
  preferences->setRawLog(rawLogCheck->isChecked());

  preferences->setAutoReconnect(reconnectGBox->isChecked());
  preferences->setAutoRejoin(autoRejoinCheck->isChecked());
  preferences->setMaximumLagTime(reconnectTimeoutSpin->value());
}

#include "prefspageconnectionbehavior.moc"
