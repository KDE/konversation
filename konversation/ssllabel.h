#ifndef SSLLABEL_H
#define SSLLABEL_H

#include <qlabel.h>

class SSLLabel : public QLabel
{
  Q_OBJECT
 
 public:
  SSLLabel(QWidget* parent, const char* name);

 protected:
  void mouseReleaseEvent(QMouseEvent *e);

 signals:
  void clicked();
};

#endif
