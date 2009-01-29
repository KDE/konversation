/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2006 Dario Abatianni <eisfuchs@tigress.com>
*/

#include "multilinetextedit.h"

#include <qpainter.h>
#include <qregexp.h>
//Added by qt3to4:
#include <Q3PointArray>

#include <kdebug.h>


MultilineTextEdit::MultilineTextEdit(QWidget* parent,const char* name) : Q3TextEdit(parent,name)
{
  // make sure, our whitespace highlighting gets called whenever needed
  connect(this,SIGNAL(textChanged()),this,SLOT(drawWhitespaces()));
  connect(this,SIGNAL(cursorPositionChanged(int,int)),this,SLOT(cursorChanged(int,int)));
}

MultilineTextEdit::~MultilineTextEdit()
{
}

void MultilineTextEdit::drawContents(QPainter* p,int clipx,int clipy,int clipw,int cliph)
{
  // redraw text
  Q3TextEdit::drawContents(p,clipx,clipy,clipw,cliph);
  // overlay whitespace markup
  drawWhitespaces();
}

void MultilineTextEdit::drawWhitespaces()
{
  // prepare a rectangle to store the width of the whitespace found
  QRect space;
  // get the painter for the text area
  QPainter pa(viewport());

  // get a sane color
  QColor col=colorGroup().link();
  // and a brush of the same color
  QBrush fillBrush(col);
  // use it for line drawing
  pa.setPen(col);
  // and for filling
  pa.setBrush(fillBrush);

  // prepare the carriage return coordinates array
  Q3PointArray cr(4);
  // and the tabulator arrow coordinate array
  Q3PointArray tab(7);

  // whitespace expression
  QRegExp regex("\\s");

  // line buffer
  QString line;

  int x,y,pos,paragraph;
  // start looking in every paragraph
  for(paragraph=0;paragraph<paragraphs();paragraph++)
  {
    // get paragraph text
    line=text(paragraph);
    // start looking for whitespaces from the beginning
    pos=0;
    while((pos=line.find(regex,pos))!=-1)
    {
      // whitespace found is not the carriage return at the end of the line?
      if(pos<((int)line.length()-1))
      {
        // get whitespace rectangle
        space=mapToView(paragraph,pos);
        // extract x/y coordinates
        x=space.width()/2-1+space.x();
        y=space.height()/2-1+space.y();

        // if it was a regular blank ...
        if(regex.cap(0)==" ")
        {
          // dras a simple small square
          pa.drawRect(x-1,y,2,2);
        }
        // if it was a tabulator
        else if(regex.cap(0)=="\t")
        {
          // calculate arrow points and draw them filled
          tab.putPoints(0,7, x-5,y-1, x,y-1, x,y-3, x+3,y, x,y+3, x,y+1, x-5,y+1);
          pa.drawPolygon(tab);
        }
      }
      // go to next position and resume looking for more whitespaces
      pos++;
    } // while

    // end of line, get carriage return position
    space=mapToView(paragraph,line.length()-1);
    // extract x/y positions
    x=space.width()/2-1+space.x();
    y=space.height()/2-1+space.y();
    // calculate carriage return triangle coordinates and draw them filled
    cr.putPoints(0,4, x,y, x,y+1, x+4, y+5, x+4, y-4);
    pa.drawPolygon(cr);
  } // for
}

void MultilineTextEdit::cursorChanged(int /* p */ ,int /* i */)
{
  // update markup, since cursor destroys it
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

    int linewidth = fm.size(Qt::ExpandTabs, paratext.mid( linestart, index-linestart )).width();
    int linewidth2 = fm.size(Qt::ExpandTabs, paratext.mid( linestart, index-linestart+1 )).width();

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
