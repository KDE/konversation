/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2005 by Peter Simonsson <psn@linux.se>
*/
#include "emoticon.h"

#include <qregexp.h>
#include <qdom.h>
#include <qfile.h>
#include <qfontmetrics.h>

#include <kstaticdeleter.h>
#include <kstandarddirs.h>

#include "konversationapplication.h"
#include "preferences.h"

namespace Konversation {

EmotIcon* EmotIcon::s_self = 0;
static KStaticDeleter<EmotIcon> staticEmotIconDeleter;

EmotIcon* EmotIcon::self()
{
  if(!s_self) {
    staticEmotIconDeleter.setObject(s_self, new EmotIcon());
  }

  return s_self;
}

EmotIcon::EmotIcon()
{
  s_self = this;

  if(KonversationApplication::preferences.emotIconsEnabled()) {
    changeTheme(KonversationApplication::preferences.emotIconsTheme());
  }
}

EmotIcon::~EmotIcon()
{
  if(s_self == this) {
    staticEmotIconDeleter.setObject(s_self, 0, false);
  }
}

void EmotIcon::changeTheme(const QString& themeName)
{
  if(themeName.isEmpty() || themeName == self()->self()->m_themeName) {
    return;
  }

  QString app = "konversation";
  QString filename = KGlobal::dirs()->findResource("data", app + "/pics/emoticons/" + themeName + "/emoticons.xml");

  if(filename.isEmpty()) {
    app = "kopete";
    filename = KGlobal::dirs()->findResource("data", app + "/pics/emoticons/" + themeName + "/emoticons.xml");
  }
  
  if(filename.isEmpty()) {
    return;
  }

  QFile file(filename);
  file.open(IO_ReadOnly);
  QDomDocument doc;
  doc.setContent(&file);

  QDomElement docElement = doc.documentElement();

  if(docElement.tagName() != "messaging-emoticon-map") {
    return;
  }

  self()->self()->m_themeName = app + "/" + themeName;
  self()->m_emotIconMap.clear();

  QDomNode node = docElement.firstChild();
  QDomElement element;
  QDomNode stringNode;
  QDomElement stringElement;
  QString fileAttrib;
  QString regExpStr;

  while(!node.isNull()) {
    element = node.toElement();

    if(!element.isNull() && element.tagName() == "emoticon") {
      fileAttrib = findIcon(element.attribute("file", QString()));
      regExpStr = "";
      stringNode = element.firstChild();

      while(!stringNode.isNull()) {
        stringElement = stringNode.toElement();

        if(!stringElement.isNull() && stringElement.tagName() == "string") {
          if(regExpStr.isEmpty()) {
            regExpStr = "(";
          } else {
            regExpStr += "|";
          }

          regExpStr += QRegExp::escape(stringElement.text());
        }

        stringNode = stringNode.nextSibling();
      }

      if(!regExpStr.isEmpty() && !fileAttrib.isEmpty()) {
        regExpStr += ")";
        self()->m_emotIconMap[fileAttrib] = regExpStr;
      }
    }

    node = node.nextSibling();
  }
}

QString EmotIcon::filter(const QString& txt, const QFontMetrics& fm)
{
  QString filteredTxt = txt;

  for(EmotIconMap::iterator it = self()->m_emotIconMap.begin(); it != self()->m_emotIconMap.end(); ++it) {
    QRegExp regExp(QString("(^|\\s)%1($|\\s)").arg(it.data()));
    filteredTxt.replace(regExp, " <img width=\"" + QString::number(fm.height()) + "\" height=\"" + QString::number(fm.height()) + "\" src=\"" + it.key() + "\"> ");
  }

  return filteredTxt;
}

QString EmotIcon::findIcon(const QString& filename)
{
  //
  // This code was copied from void KopeteEmoticons::addIfPossible( const QString& filename, QStringList emoticons )
  //  Copyright (c) 2002      by Stefan Gehn            <metz AT gehn.net>
  //  Copyright (c) 2002      by Olivier Goffart        <ogoffart@tiscalinet.be>
  //
  KStandardDirs *dirs = KGlobal::dirs();
  QString pic;
  QString app = self()->m_themeName.section('/', 0, 0);
  QString dir = self()->m_themeName.section('/', 1);
  QString file = app + "/pics/emoticons/" + dir + "/" + filename;

  //maybe an extension was given, so try to find the exact file
  pic = dirs->findResource("data", file);

  if(pic.isEmpty()) {
    pic = dirs->findResource("data", file + ".png");
  }
  if(pic.isEmpty()) {
    pic = dirs->findResource("data", file + ".mng");
  }
  if(pic.isEmpty()) {
    pic = dirs->findResource("data", file + ".gif");
  }

  return pic;
}

}
