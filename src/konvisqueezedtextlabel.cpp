/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Helper class to parse and disassemble IP+port combinations.
  WARNING: Does not attempt to validate IP addresses.
  begin:     Fri Jan 6 2006
  copyright: (C) 2006 by Eike Hein
  email:     sho@eikehein.com
*/

#include "konvisqueezedtextlabel.h"
#include "kdebug.h"

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
