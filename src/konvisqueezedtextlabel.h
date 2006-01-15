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
