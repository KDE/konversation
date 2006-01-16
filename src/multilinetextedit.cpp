//
// C++ Implementation: multilinetextedit
//
// Description:
//
//
// Author: Dario Abatianni <eisfuchs@tigress.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <qpainter.h>
#include <qregexp.h>

#include <kdebug.h>

#include "multilinetextedit.h"

MultilineTextEdit::MultilineTextEdit(QWidget* parent,const char* name) : QTextEdit(parent,name)
{
  connect(this,SIGNAL(textChanged()),this,SLOT(drawWhitespaces()));
  connect(this,SIGNAL(cursorPositionChanged(int,int)),this,SLOT(cursorChanged(int,int)));
}

MultilineTextEdit::~MultilineTextEdit()
{
}

void MultilineTextEdit::drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph)
{
  QTextEdit::drawContents(p,clipx,clipy,clipw,cliph);
  drawWhitespaces();
}

void MultilineTextEdit::drawWhitespaces()
{
  QColor red("#ff0000");
  QBrush redBrush(red);
  QRect space;
  QPainter pa(viewport());
  pa.setPen(red);
  pa.setBrush(redBrush);

  QPointArray cr(4);
  QPointArray tab(7);
  QRegExp regex("\\s");
  QString line;

  int x,y,pos,paragraph;
  for(paragraph=0;paragraph<paragraphs();paragraph++)
  {
    line=text(paragraph);
    pos=0;
    while((pos=line.find(regex,pos))!=-1)
    {
      space=mapToView(paragraph,pos);
      x=space.width()/2-1+space.x();
      y=space.height()/2-1+space.y();
      if(pos<((int)line.length()-1))
      {
        if(regex.cap(0)==" ")
        {
          pa.drawRect(x-1,y,2,2);
        }
        else if(regex.cap(0)=="\t")
        {
          tab.putPoints(0,7, x-5,y-1, x,y-1, x,y-3, x+3,y, x,y+3, x,y+1, x-5,y+1);
          pa.drawPolygon(tab);
        }
      }
      pos++;
    } // while
    space=mapToView(paragraph,line.length()-1);
    x=space.width()/2-1+space.x();
    y=space.height()/2-1+space.y();
    cr.putPoints(0,4, x,y, x,y+1, x+4, y+5, x+4, y-4);
    pa.drawPolygon(cr);
  } // for
}

void MultilineTextEdit::cursorChanged(int /* p */ ,int /* i */)
{
  drawWhitespaces();
}

// code below from kbabel and adapted by me (Eisfuchs). Thanks, Guys!

QRect MultilineTextEdit::mapToView(int para,int index)
{
    if( para < 0 || para > paragraphs() ||
        index < 0 || index > paragraphLength(para) )
            return QRect(); //invalid rectangle

    const QFontMetrics& fm = fontMetrics();
    const QString& paratext = text(para);

    // Find index of the first character on the same line as parameter
    // 'index' using binary search. Very fast, even for long texts.
    int linestart = 0;
    int indexline = lineOfChar( para, index );
    if ( indexline > 0 )
    {
        int min = 0, max = index;
        int i = (min + max)/2;
        int iline = lineOfChar( para, i );
        while ( iline != indexline-1 ||
                lineOfChar( para, i+1 ) != indexline )
        {
            Q_ASSERT( min != max && min != i && max != i );
            if ( iline < indexline )
                min = i;
            else
                max = i;
            i = (min + max)/2;
            iline = lineOfChar( para, i );
        }
        linestart = i+1;
    }
    Q_ASSERT( linestart >= 0 );

    int linewidth = fm.size(ExpandTabs, paratext.mid( linestart, index-linestart )).width();
    int linewidth2 = fm.size(ExpandTabs, paratext.mid( linestart, index-linestart+1 )).width();

    // FIXME as soon as it's possible to ask real margins from QTextEdit:
    const int left_margin = 4;
    // const int top_margin = 4;

    QPainter painter( viewport());
    const QRect& linerect = paragraphRect(para);

    return QRect(
        contentsToViewport( QPoint(
            left_margin + linerect.left() + linewidth ,
            /*top_margin + */linerect.top() + indexline * fm.lineSpacing() + fm.leading())),
        QSize(
            linewidth2-linewidth,
            fm.lineSpacing()
        ));
}

#include "multilinetextedit.moc"
