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
#include "serversettings.h"

namespace Konversation {

ServerSettings::ServerSettings()
{
  setPort(6667);
  setSSLEnabled(false);
}

ServerSettings::ServerSettings(const ServerSettings& settings)
{
  setServer(settings.server());
  setPort(settings.port());
  setPassword(settings.password());
  setSSLEnabled(settings.SSLEnabled());
}

ServerSettings::ServerSettings(const QString& server)
{
  setServer(server);
  setSSLEnabled(false);
}

ServerSettings::~ServerSettings()
{
}

}
