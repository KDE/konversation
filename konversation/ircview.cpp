/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  ircview.cpp  -  the text widget used for all text based panels
  begin:     Sun Jan 20 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

// Comment this #define to try a different text widget
// #define TABLE_VERSION

#include <qstylesheet.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>
#include <qclipboard.h>
#include <qbrush.h>
#include <qpopupmenu.h>

// Check if we use special QT versions to keep text widget from displaying
// all lines after another without line breaks
#if QT_VERSION == 303
#define ADD_LINE_BREAKS
#endif

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kurl.h>
#include <kbookmark.h>
#include <kbookmarkmanager.h>
#include <kdeversion.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <kprocess.h>
#include <kshell.h>

#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(a,b,c) ( KDE_VERSION >= KDE_MAKE_VERSION(a,b,c) )
#endif

#include "konversationapplication.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"
#include "searchdialog.h"

IRCView::IRCView(QWidget* parent,Server* newServer) : KTextBrowser(parent)
{
  highlightColor=QString::null;
  copyUrlMenu=false;
  urlToCopy=QString::null;
  resetScrollbar=false;

  popup=new QPopupMenu(this,"ircview_context_menu");

  if(popup)
  {
    popup->insertItem(i18n("&Copy"),Copy);
    popup->insertItem(i18n("Select All"),SelectAll);
    popup->insertSeparator();
    popup->insertItem(i18n("Search Text..."),Search);
    popup->insertSeparator();
    popup->insertItem(i18n("Send File..."),SendFile);
  }
  else kdWarning() << "IRCView::IRCView(): Could not create popup!" << endl;

  findParagraph=0;
  findIndex=0;

  setVScrollBarMode(AlwaysOn);
  setHScrollBarMode(AlwaysOff);

  installEventFilter(this);

  // set basic style sheet for <p> to make paragraph spacing possible
  QStyleSheet* sheet=new QStyleSheet(this,"ircview_style_sheet");
  new QStyleSheetItem(sheet,"p");
  setStyleSheet(sheet);

  setServer(newServer);
  setFont(KonversationApplication::preferences.getTextFont());

  QString bgColor=KonversationApplication::preferences.getColor("TextViewBackground");
  setViewBackground(bgColor,QString::null);

#if QT_VERSION >= 0x030100
  setWrapPolicy(QTextEdit::AtWordOrDocumentBoundary);
#endif

  // Use default KDE web browser or custom command?
  setNotifyClick(!KonversationApplication::preferences.getWebBrowserUseKdeDefault());

#ifndef TABLE_VERSION
  setText("<qt>\n");
#endif
  connect(this,SIGNAL (highlighted(const QString&)),this,SLOT (highlightedSlot(const QString&)));
  connect(this,SIGNAL (urlClick (const QString&)),this,SLOT(urlClickSlot(const QString&)));
}

IRCView::~IRCView()
{
  if(popup) delete popup;
}

void IRCView::updateStyleSheet()
{
  // set style sheet for <p> to define paragraph spacing
  QStyleSheet* sheet=styleSheet();
  if(sheet==0)
  {
    return;
  }

  int paragraphSpacing;
  if(KonversationApplication::preferences.getUseParagraphSpacing())
    paragraphSpacing=KonversationApplication::preferences.getParagraphSpacing();
  else
    paragraphSpacing=0;

  QStyleSheetItem* style=sheet->item("p");
  if(sheet==0)
  {
    kdDebug() << "IRCView::updateStyleSheet(): style==0!" << endl;
    return;
  }

  style->setDisplayMode(QStyleSheetItem::DisplayBlock);
  style->setMargin(QStyleSheetItem::MarginVertical,paragraphSpacing);
  style->setSelfNesting(false);
}

void IRCView::setViewBackground(const QString& color,const QString& pixmapName)
{
  QColor backgroundColor("#"+color);
  backgroundPixmap.load(pixmapName);

  if(pixmapName.isEmpty() || backgroundPixmap.isNull()) setPaper(backgroundColor);
  else
  {
    backgroundBrush.setColor(backgroundColor);
    backgroundBrush.setPixmap(backgroundPixmap);

    setPaper(backgroundBrush);
//  setStaticBackground(true);
  }
}

void IRCView::setServer(Server* newServer)
{
  server=newServer;
}

void IRCView::clear()
{
  buffer=QString::null;
  KTextBrowser::clear();
}

void IRCView::highlightedSlot(const QString& link)
{
  if(link.isEmpty() && copyUrlMenu)
  {
    popup->removeItem(CopyUrl);
    popup->removeItem(Bookmark);
    copyUrlMenu=false;
  }
  else if(!link.isEmpty() && !copyUrlMenu)
  {
    popup->insertItem(i18n("Copy URL to Clipboard"),CopyUrl,1);
    popup->insertItem(i18n("Add to Bookmarks"),Bookmark,2);
    copyUrlMenu=true;
    urlToCopy=link;
  }
}

void IRCView::urlClickSlot(const QString &url)
{
  if (!url.isEmpty())
  {
    // Always use KDE default mailer.
    if (url.startsWith("mailto:", false))
    {
      new KRun(url);
    }
    else
    {
      QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
      cmd.replace("%u", url);
      KProcess *proc = new KProcess;
      QStringList cmdAndArgs = KShell::splitArgs(cmd);
      kdDebug() << "IRCView::urlClickSlot(): cmd = " << cmdAndArgs << endl;
      *proc << cmdAndArgs;
//      This code will also work, but starts an extra shell process.
//      kdDebug() << "IRCView::urlClickSlot(): cmd = " << cmd << endl;
//      *proc << cmd;
//      proc->setUseShell(true);
      proc->start(KProcess::DontCare);
      delete proc;
    }
  }
}

void IRCView::replaceDecoration(QString& line,char decoration,char replacement)
{
  int pos;
  bool decorated=false;

  while((pos=line.find(decoration))!=-1)
  {
    line.replace(pos,1,(decorated) ? QString("</%1>").arg(replacement) : QString("<%1>").arg(replacement));
    decorated=!decorated;
  }
}

QString IRCView::filter(const QString& line,const QString& whoSent,bool doHilight)
{
  QString filteredLine(line);

  // TODO: Use QStyleSheet::escape() here

  // Replace all & with &amp;
  filteredLine.replace('&',"&amp;");
  // Replace all < with &lt;
  filteredLine.replace('<',"&lt;");
  // Replace all > with &gt;
  filteredLine.replace('>',"&gt;");
  // Replace all 0x0f (reset to defaults) with \0x031,0 (should be extended to reset all text decorations as well)
  filteredLine.replace('\017',"\0031,0");
  // Replace all 0x03 without color number (reset color) with \0x031,0
  filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0031,0\\1");

  if(filteredLine.find("\x07")!=-1)
  {
    if(KonversationApplication::preferences.getBeep()) kapp->beep();
  }

  // replace \003 codes with rich text color codes
  QRegExp colorRegExp("\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)");

  // TODO: Make Background colors work somehow. The code is in comments until we
  //       find some way to use it
//  bool bgColor=false;
  int pos;
  bool firstColor=true;
  QString colorString;
  QStringList colorCodes = KonversationApplication::preferences.getIRCColorList();

  while((pos=colorRegExp.search(filteredLine))!=-1)
  {
    colorString=(firstColor) ? QString::null : QString("</font>");

    int foregroundColor=colorRegExp.cap(1).toInt();
/*
    int backgroundColor=colorRegExp.cap(3).toInt();

    if(bgColor) colorString+="</td></tr></table>";

    if(colorRegExp.cap(3).length())
    {
      colorString+="<table cellpadding=0 cellspacing=0 bgcolor=\"#"+QString(colorCodes[backgroundColor])+"\"><tr><td>";
      bgColor=true;
    }
    else
      bgColor=false;
*/
    colorString+="<font color=\""+colorCodes[foregroundColor]+"\">";

    filteredLine.replace(pos,colorRegExp.cap(0).length(),colorString);
    firstColor=false;
  }

  if(!firstColor) filteredLine+="</font>";
//  if(bgColor) colorString+="</td></tr></table>";

  // Replace all text decorations
  replaceDecoration(filteredLine,'\x02','b');
  replaceDecoration(filteredLine,'\x09','i');
  replaceDecoration(filteredLine,'\x13','s');
  replaceDecoration(filteredLine,'\x15','u');
  replaceDecoration(filteredLine,'\x16','b'); // should be reverse
  replaceDecoration(filteredLine,'\x1f','u');

  // URL Catcher
  QString linkColor=KonversationApplication::preferences.getColor("LinkMessage");

// Maybe switch to this function some day when it does hilight better than my own stuff ;)
//  filteredLine=KStringHandler::tagURLs(filteredLine);

  QRegExp pattern("(((http://|https://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
                  // IP Address
                  "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|"
                  // Decimal IP address
                  "[0-9]{1,12}|"
                  // Standard host name
                  "[a-z0-9][\\.@%a-z0-9_-]+\\.[a-z]{2,}"
                  // Port number, path to document
                  ")(:[0-9]{1,5})?(/[^)>\"'\\s]*)?|"
                  // eDonkey2000 links need special treatment
                  "ed2k://\\|([^|]+\\|){4})|"
                  "(mailto:|)((([a-z]|\\d)+[\\w\\x2E\\x2D]+)\\x40([\\w\\x2E\\x2D]{2,})\\x2E(\\w{2,})))");

  pattern.setCaseSensitive(false);

  pos=0;
  while(pattern.search(filteredLine,pos)!=-1) {
      // Remember where we found the url
      pos=pattern.pos();

      // Extract url
      QString url=pattern.capturedTexts()[0];
      QString href(url);

      // clean up href for browser
      if(href.startsWith("www.")) href="http://"+href;
      else if(href.startsWith("ftp.")) href="ftp://"+href;
      else if(href.find(QRegExp("(([a-z]+[\\w\\x2E\\x2D]+)\\x40)")) == 0) href = "mailto:" + href;

      // Fix &amp; back to & in href ... kludgy but I don't know a better way.
      href.replace(QRegExp("&amp;"),"&");
      // Replace all spaces with %20 in href
      href.replace(' ',"%20");
      // Build rich text link
      QString link("<font color=\"#"+linkColor+"\"><a href=\""+href+"\">"+url+"</a></font>");

      // replace found url with built link
      filteredLine.replace(pos,url.length(),link);
      // next search begins right after the link
      pos+=link.length();
      // tell the program that we have found a new url

      KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
      konvApp->storeUrl(whoSent,href);
  }
  
  // Hilight

  if(doHilight)
  {
    highlightColor=QString::null;

    // FIXME: We got to get rid of server dependance here
    if(server && whoSent==server->getNickname() && KonversationApplication::preferences.getHilightOwnLines())
    {
      // hilight own lines
      filteredLine=QString("<font color=\""+KonversationApplication::preferences.getHilightOwnLinesColor().name()+"\">")+filteredLine+QString("</font>");
    }
    else
    {
      QString who(QString::null);
      // only copy sender name if it was not our own line
      if(server && whoSent!=server->getNickname()) who=whoSent;

      // FIXME: We got to get rid of server dependance here
      if(server && KonversationApplication::preferences.getHilightNick() &&
         filteredLine.lower().find(QRegExp("(^|[^\\d\\w])"+
#if QT_VERSION >= 0x030100
         QRegExp::escape(server->getNickname().lower())+
#else
         QString(server->getNickname().lower())+
#endif
         "([^\\d\\w]|$)"))!=-1)
      {
        // hilight current nickname
        highlightColor=KonversationApplication::preferences.getHilightNickColor().name();
        if (KonversationApplication::preferences.getOSDShowOwnNick() && !KonversationApplication::preferences.getOSDShowChannel())
        {
          KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
          konvApp->osd->showOSD("(HIGHLIGHT) <" + whoSent + "> " + filteredLine);
        }
      }
      else
      {
        QPtrList<Highlight> hilightList=KonversationApplication::preferences.getHilightList();
        unsigned int index;

        for(index=0;index<hilightList.count();index++)
        {
          QString needle=hilightList.at(index)->getText().lower();
          if(filteredLine.lower().find(needle)!=-1 ||   // hilight patterns in text
             who.lower().find(needle)!=-1 )             // hilight patterns in nickname
          {
            highlightColor=hilightList.at(index)->getColor().name();
            break;
          }
        } // endfor
      }
    }
    // apply found highlight color to line
    if(!highlightColor.isEmpty())
      filteredLine=QString("<font color=\""+highlightColor+"\">")+filteredLine+QString("</font>");
  }

  // Replace multiple Spaces with "<space>&nbsp;"
  while((pos = filteredLine.find("  ")) != -1)
  {
    filteredLine.replace(pos+1,1,"&nbsp;");
  }

  return filteredLine;
}

void IRCView::append(const QString& nick,const QString& message)
{
  QString channelColor=KonversationApplication::preferences.getColor("ChannelMessage");

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+channelColor()+"\">%1:</font></td><td><font color=\"#"+channelColor()+"\">%2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+channelColor+"\"><b>&lt;%1&gt;</b> %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick));
#else
  QString line=QString("<p><font color=\"#"+channelColor+"\"><b>&lt;%1&gt;</b> %2</font></p>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendRaw(const QString& message)
{
  QString channelColor=KonversationApplication::preferences.getColor("ChannelMessage");

#ifdef TABLE_VERSION
  QString line=QString("<tr><td colspan=2><font color=\"#"+channelColor()+"\">"+message+"</font></td></tr>\n");
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+channelColor+"\">"+message+"</font><br>\n");
#else
  QString line=QString("<p><font color=\"#"+channelColor+"\">"+message+"</font></p>\n");
#endif
#endif

  doAppend(line);
}

void IRCView::appendQuery(const QString& nick,const QString& message)
{
  QString queryColor=KonversationApplication::preferences.getColor("QueryMessage");

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+queryColor+"\">*%1*</font></td><td><font color=\"#"+queryColor+"\">%2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+queryColor+"\"><b>*%1*</b> %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
  QString line=QString("<p><font color=\"#"+queryColor+"\"><b>*%1*</b> %2</font></p>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const QString& nick,const QString& message)
{
  QString actionColor=KonversationApplication::preferences.getColor("ActionMessage");

#ifdef TABLE_VERSION
  QString line=QString("<tr><td>&nbsp;</td><td><font color=\"#"+actionColor+"\">* %1 %2</font></td></tr>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+actionColor+"\">* %1 %2</font><br>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#else
  QString line=QString("<p><font color=\"#"+actionColor+"\">* %1 %2</font></p>\n").arg(filter(nick,NULL,false)).arg(filter(message,nick,true));
#endif
#endif

  emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendServerMessage(const QString& type,const QString& message)
{
  QString serverColor=KonversationApplication::preferences.getColor("ServerMessage");

  // Fixed width font option for MOTD
  QString fixed;
  if(KonversationApplication::preferences.getFixedMOTD())
  {
    if(QString("MOTD")==type) fixed=" face=\"courier\"";
  }

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+serverColor+"\">%1</font></td><td><font color=\"#"+serverColor+"\""+fixed+">%2</font></td></tr>\n").arg(type).arg(filter(message));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+serverColor+"\""+fixed+"><b>[%1]</b> %2</font><br>\n").arg(type).arg(filter(message));
#else
  QString line=QString("<p><font color=\"#"+serverColor+"\""+fixed+"><b>[%1]</b> %2</font></p>\n").arg(type).arg(filter(message));
#endif
#endif
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool important)
{
  QString commandColor=KonversationApplication::preferences.getColor("CommandMessage");

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+commandColor+"\">%1</font></td><td><font color=\"#"+commandColor+"\">%2</font></td></tr>\n").arg(type).arg(filter(message));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+commandColor+"\">*** %2</font><br>\n").arg(filter(message));
#else
  QString line=QString("<p><font color=\"#"+commandColor+"\">*** %2</font></p>\n").arg(filter(message));
#endif
#endif
  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line,false,important);
}

void IRCView::appendBacklogMessage(const QString& firstColumn,const QString& rawMessage)
{
  QString time;
  QString message(rawMessage);
  QString first(firstColumn);
  QString backlogColor=KonversationApplication::preferences.getColor("BacklogMessage");

  // Nicks are in "<nick>" format so replace the "<>"
  first.replace(QRegExp("\\<"),"&lt;");
  first.replace(QRegExp("\\>"),"&gt;");

  // extract timestamp from message string
  if(message.startsWith("["))
  {
    time=message.section(' ',0,0);
    message=message.section(' ',1);
  }

#ifdef TABLE_VERSION
  QString line=QString("<tr><td><font color=\"#"+backlogColor+"\">%1</font></td><td><font color=\"#"+backlogColor+"\">%2 %3</font></td></tr>\n").arg(time).arg(first).arg(filter(message,NULL,false));
#else
#ifdef ADD_LINE_BREAKS
  QString line=QString("<font color=\"#"+backlogColor+"\">%1 %2 %3</font><br>\n").arg(time).arg(first).arg(filter(message,NULL,false));
#else
  QString line=QString("<p><font color=\"#"+backlogColor+"\">%1 %2 %3</font></p>\n").arg(time).arg(first).arg(filter(message,NULL,false));
#endif
#endif

  // no additional time stamps on backlog messages
  doAppend(line,true);
}

void IRCView::doAppend(QString newLine,bool suppressTimestamps,bool important)
{
  // Add line to buffer
  QString line(newLine);

  if(!suppressTimestamps && KonversationApplication::preferences.getTimestamping())
  {
    QTime time=QTime::currentTime();
    QString timeColor=KonversationApplication::preferences.getColor("Time");
    QString timeFormat=KonversationApplication::preferences.getTimestampFormat();
    QString timeString(QString("<font color=\"#"+timeColor+"\">[%1]</font> ").arg(time.toString(timeFormat)));

#ifdef ADD_LINE_BREAKS
      line.prepend(timeString);
#else
      line.insert(3,timeString);
#endif
  }

  if(important || !KonversationApplication::preferences.getHideUnimportantEvents())
  {
    buffer+=line;
    emit newText(highlightColor,important);

    // scroll view only if the scroll bar is already at the bottom
#if QT_VERSION == 303
    // Does not seem to work very well with QT 3.0.3
    bool doScroll=true;
#else
    bool doScroll=((contentsHeight()-visibleHeight())==contentsY());
#endif

#ifdef TABLE_VERSION
    setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#else
    KTextBrowser::append(line);
#endif

    if(doScroll)
    {
      moveCursor(MoveEnd,false);
      ensureVisible(0,contentsHeight());
    }
  }
}

// remember if scrollbar was positioned at the end of the text or not
void IRCView::hideEvent(QHideEvent* /* event */)
{
#if QT_VERSION == 303
  // Does not seem to work very well with QT 3.0.3
  bool doScroll=true;
#else
  resetScrollbar=((contentsHeight()-visibleHeight())==contentsY());
#endif
}

// Workaround to scroll to the end of the TextView when it's shown
void IRCView::showEvent(QShowEvent* /* event */)
{
#ifdef TABLE_VERSION
  setText("<qt><table cellpadding=\"0\" cellspacing=\"0\">"+buffer+"</table></qt>");
#endif

  // did the user scroll the view to the end of the text before hiding?
  if(resetScrollbar)
  {
    moveCursor(MoveEnd,false);
    ensureVisible(0,contentsHeight());
  }
  // Set focus to input line (must be connected)
  emit gotFocus();
}

void IRCView::focusInEvent(QFocusEvent* event)
{
  // Suppress Compiler Warning
  event->type();
  // Set focus to input line (must be connected)
  emit gotFocus();
}

// Copy selected text into clipboard immediately
bool IRCView::eventFilter(QObject* object,QEvent* event)
{
  if(event->type()==QEvent::MouseButtonRelease)
  {
    QMouseEvent* me=(QMouseEvent*) event;

    if(me->button()==QMouseEvent::LeftButton)
    {
      if(hasSelectedText()) copy();
    }
    else if(me->button()==QMouseEvent::MidButton)
    {
      // TODO: maybe implement pasting here?
    }
  }
  else if(event->type()==QEvent::ContextMenu)
  {
    return contextMenu((QContextMenuEvent*) event);
  }

  return KTextBrowser::eventFilter(object,event);
}

bool IRCView::contextMenu(QContextMenuEvent* ce)
{
  popup->setItemEnabled(Copy,(hasSelectedText()));

  int r=popup->exec(ce->globalPos());

  switch(r)
  {
    case -1:
      // dummy. -1 means, no entry selected. we don't want -1to go in default, so
      // we catch it here
      break;
    case Copy:
      copy();
      break;
    case CopyUrl:
    {
      QClipboard *cb=KApplication::kApplication()->clipboard();
#if QT_VERSION >= 0x030100
      cb->setText(urlToCopy,QClipboard::Selection);
      cb->setText(urlToCopy,QClipboard::Clipboard);
#else
      cb->setSelectionMode(true);
      cb->setText(urlToCopy);
      cb->setSelectionMode(false);
      cb->setText(urlToCopy);
#endif
      break;
    }
    case SelectAll:
      selectAll();
      break;
    case Search:
      search();
      break;
    case SendFile:
      emit sendFile();
      break;
    case Bookmark:
    {
#if KDE_IS_VERSION(3,1,90)
      KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
      KBookmarkGroup bg = bm->addBookmarkDialog(urlToCopy, QString::null);
      bm->save();
      bm->emitChanged(bg);
#else
      KBookmarkManager* bm = KBookmarkManager::managerForFile(locateLocal("data",
        "/konqueror/bookmarks.xml"));
      KBookmarkGroup bg = bm->root();
      bg.addBookmark(bm, urlToCopy, KURL(urlToCopy));
      bm->save();
      bm->emitChanged(bg);
#endif
      break;
    }
    default:
      emit extendedPopup(r);
  }

  return true;
}

void IRCView::search()
{
  bool caseSensitive=false;
  bool wholeWords=false;
  bool forward=false;
  bool fromCursor=false;

  QString pattern=SearchDialog::search(this,&caseSensitive,&wholeWords,&forward,&fromCursor);
  if(!pattern.isEmpty())
  {
    // don't change search variables if fromCursor
    if(fromCursor)
    {
      // next search must begin one index before / after the last search
      // depending on the search direction.
      if(forward)
      {
        findIndex++;
        if(findIndex==paragraphLength(findParagraph))
        {
          findIndex=0;
          findParagraph++;
        }
      }
      else
      {
        if(findIndex) findIndex--;
        else
        {
          findParagraph--;
          findIndex=paragraphLength(findParagraph);
        }
      }
    }
    // start over from beginning / end
    else
    {
      if(forward)
      {
        findParagraph=1;
        findIndex=1;
      }
      else
      {
        findParagraph=paragraphs();
        findIndex=paragraphLength(paragraphs());
      }
    }

    if(!find(pattern,caseSensitive,wholeWords,forward,&findParagraph,&findIndex))
      KMessageBox::information(this,i18n("No matches found for \"%1\".").arg(pattern),i18n("Information"));
  }
}

void IRCView::pageUp()
{
  moveCursor(MovePgUp,false);
}

void IRCView::pageDown()
{
  moveCursor(MovePgDown,false);
}

// other windows can link own menu entries here
QPopupMenu* IRCView::getPopup()
{
  return popup;
}

#include "ircview.moc"
