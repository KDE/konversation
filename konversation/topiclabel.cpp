/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 Peter Simonsson <psn@linux.se>
*/
#include "topiclabel.h"

#include <krun.h>
#include <kprocess.h>
#include <kshell.h>
#include <kstringhandler.h>

#include "konversationapplication.h"
#include "common.h"

namespace Konversation {

TopicLabel::TopicLabel(QWidget *parent, const char *name)
 : KActiveLabel(parent, name)
{
}

TopicLabel::~TopicLabel()
{
}

QSize TopicLabel::minimumSizeHint() const
{
  return QSize(0, 0);
}

QSize TopicLabel::sizeHint() const
{
  return QSize(0, 0);
}

void TopicLabel::openLink(const QString& link)
{
  if (!link.isEmpty())
  {
    // Always use KDE default mailer.
    if (KonversationApplication::preferences.getWebBrowserUseKdeDefault() || link.lower().startsWith("mailto:"))
    {
      new KRun(KURL(link));
    }
    else
    {
      QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
      cmd.replace("%u",link);
      KProcess *proc = new KProcess;
      QStringList cmdAndArgs = KShell::splitArgs(cmd);
      *proc << cmdAndArgs;
//      This code will also work, but starts an extra shell process.
//      kdDebug() << "IRCView::linkClickSlot(): cmd = " << cmd << endl;
//      *proc << cmd;
//      proc->setUseShell(true);
      proc->start(KProcess::DontCare);
      delete proc;
    }
  }
}

void TopicLabel::setText(const QString& text)
{
  m_fullText = text;
  updateSqueezedText();
}

void TopicLabel::updateSqueezedText()
{
  QFontMetrics fm(currentFont());
  QString text;

  if(height() < (fm.height() * 2)) {
    text = KStringHandler::rPixelSqueeze(m_fullText, fm, contentsWidth() - 10);
    setWordWrap(NoWrap);
  } else {
    text = m_fullText;
    setWordWrap(WidgetWidth);
  }
  
  text.replace("&", "&amp;").
      replace("<", "&lt;").
      replace(">", "&gt;");
  KActiveLabel::setText("<qt>" + tagURLs(text, "") + "</qt>");
}

void TopicLabel::resizeEvent(QResizeEvent* ev)
{
  KActiveLabel::resizeEvent(ev);
  updateSqueezedText();
}

}

#include "topiclabel.moc"
