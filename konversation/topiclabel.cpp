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

#include <qsimplerichtext.h>
#include <qtooltip.h>

#include <krun.h>
#include <kprocess.h>
#include <kshell.h>
#include <kstringhandler.h>
#include <kglobal.h>
#include <kdebug.h>

#include "konversationapplication.h"
#include "server.h"
#include "common.h"
#include "channel.h"

#include <kdeversion.h>

#if !KDE_IS_VERSION(3,3,0)
template<class T>
inline const T& kClamp( const T& x, const T& low, const T& high )
{
	    if ( x < low )       return low;
	        else if ( high < x ) return high;
		    else                 return x;
}
#endif

namespace Konversation {

TopicLabel::TopicLabel(QWidget *parent, const char *name)
 : KActiveLabel(parent, name)
{
  setWrapPolicy(QTextEdit::AtWordOrDocumentBoundary);
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

void TopicLabel::setServer(Server* server)
{
  m_server = server;
}

void TopicLabel::openLink(const QString& link)
{
  if (!link.isEmpty())
  {
    if(link.startsWith("#")) // channel link
      {
	QString channel(link);
	channel.replace("##","#");
	m_server->sendJoinCommand(channel);
      }
    // Always use KDE default mailer.
    else if (KonversationApplication::preferences.getWebBrowserUseKdeDefault() || link.lower().startsWith("mailto:"))
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
  QString text = m_fullText;
  text.replace("&", "&amp;").
      replace("<", "&lt;").
      replace(">", "&gt;");
  text = tagURLs(text, "");

  QToolTip::remove(this);

  if(height() < (fm.lineSpacing() * 2)) {
    text = rPixelSqueeze(text, visibleWidth() - 10);
    setWordWrap(NoWrap);
    QToolTip::add(this, "<qt>" + QStyleSheet::escape(m_fullText) + "</qt>");
  } else {
    setWordWrap(WidgetWidth);

    if(height() < contentsHeight()) {
      QToolTip::add(this, "<qt>" + QStyleSheet::escape(m_fullText) + "</qt>");
    }
  }

  KActiveLabel::setText("<qt>" + text + "</qt>");
}

void TopicLabel::resizeEvent(QResizeEvent* ev)
{
  KActiveLabel::resizeEvent(ev);
  updateSqueezedText();
}

QString TopicLabel::rPixelSqueeze(const QString& text, uint maxPixels)
{
  QFontMetrics fm(currentFont());
  uint tw = textWidth(text, fm);

  if(tw > maxPixels) {
    QString tmp = text;
    const uint em = fm.maxWidth();
    maxPixels -= fm.width("...");
    int len, delta;

    while((tw > maxPixels) && !tmp.isEmpty()) {
      len = tmp.length();
      delta = (tw - maxPixels) / em;
      delta = kClamp(delta, 1, len);

      tmp.remove(len - delta, delta);
      tw = textWidth(tmp, fm);
    }

    return tmp.append("...");
  }

  return text;
}

uint TopicLabel::textWidth(const QString& text, const QFontMetrics& fm)
{
  QSimpleRichText richText("<qt>" + text + "</qt>", currentFont());
  richText.setWidth(fm.width(text));

  return richText.widthUsed();
}

}

#include "topiclabel.moc"
