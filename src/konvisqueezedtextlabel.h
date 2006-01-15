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

#ifndef KONVISQUEEZEDTEXTLABEL_H
#define KONVISQUEEZEDTEXTLABEL_H

#include <ksqueezedtextlabel.h>

/*
  @author Eike Hein
*/

class KonviSqueezedTextLabel : public KSqueezedTextLabel
{
    Q_OBJECT

        public:
        KonviSqueezedTextLabel(const QString &text, QWidget *parent, const char *name = 0);
        ~KonviSqueezedTextLabel();

        public slots:
        void setText(const QString& text);
        void setTempText(const QString& text);
        void clearTempText();

        private:
        QString m_oldText;
        QString m_tempText;
};

#endif
