/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  copyright: (C) 2004 Peter Simonsson <psn@linux.se>
*/
#ifndef KONVERSATIONTOPICLABEL_H
#define KONVERSATIONTOPICLABEL_H

#include <kactivelabel.h>

class QFontMetrics;
class Server;

namespace Konversation {

class TopicLabel : public KActiveLabel
{
  Q_OBJECT
  public:
    TopicLabel(QWidget *parent = 0, const char *name = 0);
    ~TopicLabel();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setServer(Server* server);

  public slots:
    virtual void openLink(const QString& link);
    void setText(const QString& text);

  protected:
    void updateSqueezedText();
    QString rPixelSqueeze(const QString& text, uint maxPixels);
    uint textWidth(const QString& text, const QFontMetrics& fm);

    void resizeEvent(QResizeEvent*);

  private:
    QString m_fullText;
    Server* m_server;
};

};

#endif
