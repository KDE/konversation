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

  QString line;

  int x,y,pos,paragraph;
  for(paragraph=0;paragraph<paragraphs();paragraph++)
  {
    line=text(paragraph);
    pos=0;
    while((pos=line.find(" ",pos))!=-1)
    {
      space=mapToView(paragraph,pos);
      x=space.width()/2-1+space.x();
      y=space.height()/2-1+space.y();
      pa.drawRect(x,y,2,2);
      pos++;
    }
    space=mapToView(paragraph,line.length()-1);
    x=space.width()/2-1+space.x();
    y=space.height()/2-1+space.y();
    cr.putPoints(0,4, x,y, x,y+1, x+4, y+5, x+4, y-4);
    pa.drawPolygon(cr);
  }
}

void MultilineTextEdit::cursorChanged(int /* p */ ,int /* i */)
{
  drawWhitespaces();
}

// code below from kbabel. Thanks, Guys!

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

    int linewidth;

    // if the tag is not valid, easy
    if( (_tagStartPara == _tagEndPara) && (_tagStartIndex == _tagEndIndex) ) {
        linewidth = fm.width( paratext.mid( linestart, index-linestart ));
    } else {
        int tso = pos2Offset( _tagStartPara, _tagStartIndex );
        int teo = pos2Offset( _tagEndPara, _tagEndIndex );
        int off = pos2Offset( para, index );

        if( off < tso ) {
            // it is surely before the tag
            linewidth = fm.width( paratext.mid( linestart, index-linestart ));
        } else if( off >= teo ) {
            // it is surely after the tag

            // is it on the same line as the end of the tag?
            if( _tagEndPara < para || lineOfChar( _tagEndPara, _tagEndIndex ) < indexline ) {
                // no tag on the line, no bold
                linewidth = fm.width( paratext.mid( linestart, index-linestart ));
            } else {
                QFont f( font() );
                f.setBold( true );
                QFontMetrics bfm( f );
                // is tag single displayed line?
                if( _tagStartPara == _tagEndPara
                    && lineOfChar( _tagStartPara, _tagStartIndex ) == lineOfChar( _tagEndPara, _tagEndIndex ) )
                {
                    // yes, count the non-bold before the tag start
                    linewidth = fm.width( paratext.mid( linestart, _tagStartIndex-linestart ) )
                        + bfm.width( paratext.mid( _tagStartIndex, _tagEndIndex-_tagStartIndex ) );
                }
                else
                {
                    // count the part of the tag itself
                    linewidth = bfm.width( paratext.mid( linestart, _tagEndIndex-linestart ) );
                }

                // add the rest from tag to the index
                linewidth += fm.width( paratext.mid( _tagEndIndex, index-_tagEndIndex ) );
            }
        }
        else {
            // in tag
            QFont f( font() );
            f.setBold( true );
            QFontMetrics bfm( f );
            // is it the first line of the tag?
            if( para == _tagStartPara && indexline == lineOfChar( _tagStartPara, _tagStartIndex ) ) {
                // start of the line is normal
                linewidth = fm.width( paratext.mid( linestart, _tagStartIndex-linestart ) )
                    + bfm.width( paratext.mid( _tagStartIndex, index-_tagStartIndex ) );
            } else {
                // whole is bold
                linewidth = bfm.width( paratext.mid( linestart, index-linestart ) );
            }
        }
    }

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
            fm.charWidth( paratext, index ),
            fm.lineSpacing()
        ));
}

int MultilineTextEdit::pos2Offset(uint paragraph, uint index)
{
    paragraph = QMAX( QMIN( (int)paragraph, paragraphs() - 1), 0 ); // Sanity check
    index  = QMAX( QMIN( (int)index,  paragraphLength( paragraph )), 0 ); // Sanity check

    {
        uint lastI;
        lastI  = paragraphLength( paragraph );
        uint i = 0;
        uint tmp = 0;

        if( paragraph>=_lastParagraph )
        {
            tmp = _lastParagraphOffset;
            i  = _lastParagraph;
        }

        for( ;i < paragraph ; i++ )
        {
            tmp += paragraphLength( i ) + 1;
        }

        _lastParagraphOffset=tmp;
        _lastParagraph=paragraph;

        tmp += QMIN( lastI, index );

        return tmp;
    }
}

#include "multilinetextedit.moc"
