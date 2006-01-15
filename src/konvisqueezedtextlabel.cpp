/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Thin wrapper around KSqueezedTextLabel capable of showing a temporary
  label text all the while storing any text set via the regular setText()
  and reverting to it when the clearTempText() slot is called. Use case:
  menu item and link hover updating the status bar across the app.
  begin:     Fri Jan 15 2006
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#include "konvisqueezedtextlabel.h"

/*
  @author Eike Hein
*/


KonviSqueezedTextLabel::KonviSqueezedTextLabel(const QString &text, QWidget *parent, const char *name)
    : KSqueezedTextLabel(text, parent, name)
{
}

KonviSqueezedTextLabel::~KonviSqueezedTextLabel()
{
}

void KonviSqueezedTextLabel::setText(const QString& text)
{
    m_oldText = text;

    // Don't overwrite the temp text if there is any
    if (m_tempText.isEmpty())
    {
        KSqueezedTextLabel::setText(text);
    }
}


void KonviSqueezedTextLabel::setTempText(const QString& text)
{
   if (!text.isEmpty())
    {
        m_tempText = text;
        KSqueezedTextLabel::setText(text);
    }
}

void KonviSqueezedTextLabel::clearTempText()
{
    // Unset the temp text so the net setText won't fail
    m_tempText = QString::null;

    KSqueezedTextLabel::setText(m_oldText);
}

#include "konvisqueezedtextlabel.moc"
