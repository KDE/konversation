#include "ssllabel.h"

SSLLabel::SSLLabel(QWidget* parent,const char* name)
  : QLabel(parent,name)
{
}

void SSLLabel::mouseReleaseEvent(QMouseEvent *e)
{
  emit clicked();
}

#include "ssllabel.moc"
