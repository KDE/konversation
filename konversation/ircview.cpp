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

#include <private/qrichtext_p.h>

#include <qstylesheet.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtextbrowser.h>
#include <qclipboard.h>
#include <qbrush.h>
#include <qpopupmenu.h>

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
#include <kiconloader.h>

#ifndef KDE_MAKE_VERSION
#define KDE_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))
#endif

#ifndef KDE_IS_VERSION
#define KDE_IS_VERSION(a,b,c) ( KDE_VERSION >= KDE_MAKE_VERSION(a,b,c) )
#endif

#if KDE_IS_VERSION(3,1,94)
#include <kshell.h>
#endif

#include "konversationapplication.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"
#include "searchdialog.h"
#include "konversationsound.h"
#include "chatwindow.h"

IRCView::IRCView(QWidget* parent,Server* newServer) : KTextBrowser(parent)
{
  highlightColor=QString::null;
  copyUrlMenu=false;
  urlToCopy=QString::null;
  resetScrollbar=false;
  autoTextToSend=QString::null;

  setAutoFormatting(0);
  setUndoRedoEnabled(0);

  popup=new QPopupMenu(this,"ircview_context_menu");

  if(popup)
  {
    popup->insertItem(SmallIcon("editcopy"),i18n("&Copy"),Copy);
    popup->insertItem(i18n("Select All"),SelectAll);
    popup->insertSeparator();
    popup->insertItem(SmallIcon("find"),i18n("Find Text..."),Search);
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

  setNotifyClick(true);

  connect(this,SIGNAL (highlighted(const QString&)),this,SLOT (highlightedSlot(const QString&)));
  connect(this,SIGNAL (linkClicked(const QString&)),this,SLOT(urlClickSlot(const QString&)));
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
    if (KonversationApplication::preferences.getWebBrowserUseKdeDefault() || url.lower().startsWith("mailto:"))
    {
      new KRun(KURL(url));
    }
    else
    {
      QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
      cmd.replace(QRegExp("%u"),url);
      KProcess *proc = new KProcess;
#if KDE_IS_VERSION(3,1,94)
      QStringList cmdAndArgs = KShell::splitArgs(cmd);
#else
      QStringList cmdAndArgs = QStringList::split(' ',cmd);
#endif
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

QString IRCView::filter(const QString& line,const QString& defaultColor,const QString& whoSent,bool doHilight, bool parseURL)
{
  QString filteredLine(line);

  // TODO: Use QStyleSheet::escape() here

  // Replace all & with &amp;   We use QRegExp here because of pre 3.1 compatibility!
  filteredLine.replace(QRegExp("&"),"&amp;");
  // Replace all < with &lt;
  filteredLine.replace(QRegExp("<"),"&lt;");
  // Replace all > with &gt;
  filteredLine.replace(QRegExp(">"),"&gt;");
  // Replace all 0x03 without color number (reset color) with \0x031,0
  filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0031,0\\1");
  // Hack to allow for whois info hostmask info to not be parsed as email
  filteredLine.replace(QRegExp("&amp;#64;"),"&#64;");

  if(filteredLine.find("\x07")!=-1)
  {
    if(KonversationApplication::preferences.getBeep()) kapp->beep();
  }

  // replace \003 and \017 codes with rich text color codes

  // captures          1    2                   23 4                   4 3     1
  QRegExp colorRegExp("(\003([0-9]|0[0-9]|1[0-5])(,([0-9]|0[0-9]|1[0-5])|)|\017)");

  // TODO: Make Background colors work somehow. The code is in comments until we
  //       find some way to use it
//  bool bgColor=false;
  int pos;
  bool filterColors=KonversationApplication::preferences.getFilterColors();
  bool firstColor=true;
  QString colorString;
  QStringList colorCodes = KonversationApplication::preferences.getIRCColorList();

  while((pos=colorRegExp.search(filteredLine))!=-1)
  {
    colorString=(firstColor) ? QString::null : QString("</font>");

    // reset colors on \017 to default value
    if(colorRegExp.cap(1)=="\017")
      colorString+="<font color=\"#"+defaultColor+"\">";
    else
    {
      int foregroundColor=colorRegExp.cap(2).toInt();
/*
    int backgroundColor=colorRegExp.cap(4).toInt();

    if(bgColor) colorString+="</td></tr></table>";

    if(colorRegExp.cap(4).length())
    {
      colorString+="<table cellpadding=0 cellspacing=0 bgcolor=\"#"+QString(colorCodes[backgroundColor])+"\"><tr><td>";
      bgColor=true;
    }
    else
      bgColor=false;
*/
      colorString+="<font color=\""+colorCodes[foregroundColor]+"\">";
    }

    if (filterColors == true) colorString = QString::null;
    filteredLine.replace(pos,colorRegExp.cap(0).length(),colorString);
    firstColor=false;
  }

  if(!firstColor) filteredLine+="</font>";
//  if(bgColor) colorString+="</td></tr></table>";

  // Replace all text decorations
  // TODO: \017 should reset all textt decorations to plain text
  replaceDecoration(filteredLine,'\x02','b');
  replaceDecoration(filteredLine,'\x09','i');
  replaceDecoration(filteredLine,'\x13','s');
  replaceDecoration(filteredLine,'\x15','u');
  replaceDecoration(filteredLine,'\x16','b'); // should be inverse
  replaceDecoration(filteredLine,'\x1f','u');

  // URL Catcher
  QString linkColor=KonversationApplication::preferences.getColor("LinkMessage");

// Maybe switch to this function some day when it does hilight better than my own stuff ;)
//  filteredLine=KStringHandler::tagURLs(filteredLine);

  QRegExp pattern("(((http://|https://|ftp://|nntp://|news://|gopher://|www\\.|ftp\\.)"
                  "(([-_.%\\d\\w]*(:[-_.%\\d\\w]*)?@)|)"
                  // IP Address
                  "([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}|"
                  // Decimal IP address
                  "[0-9]{1,12}|"
                  // Standard host name
                  "[a-z0-9][\\.%a-z0-9_-]+\\.[a-z]{2,}"
                  // Port number, path to document
                  ")(:[0-9]{1,5})?((:|)(/[^>\"\\s]*))?|"
                  // eDonkey2000 links need special treatment
                  "ed2k://\\|([^|]+\\|){4})|"
                  "(mailto:|)((([a-z]|\\d)+[\\w\\x2E\\x2D]+)\\x40([\\w\\x2E\\x2D]{2,})\\x2E(\\w{2,})))");

  pattern.setCaseSensitive(false);

  pos=0;
  while(pattern.search(filteredLine,pos)!=-1 && parseURL) {
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
      href.replace(QRegExp(" "),"%20");
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

  if(doHilight && whoSent!=server->getNickname())
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
        ChatWindow* chatWin = static_cast<ChatWindow*>(parent());
        if (KonversationApplication::preferences.getOSDShowOwnNick() && !KonversationApplication::preferences.getOSDShowChannel() && chatWin->notificationsEnabled())
        {
          KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
          konvApp->osd->showOSD( i18n("(HIGHLIGHT)") + " <" + whoSent + "> " + filteredLine);
        }
      }
      else
      {
        QPtrList<Highlight> hilightList=KonversationApplication::preferences.getHilightList();
        unsigned int index;

        for(index=0;index<hilightList.count();index++)
        {
          bool patternFound=0;
          Highlight* hilight=hilightList.at(index);

          if(hilight->getRegExp())
          {
            QRegExp needle(hilight->getPattern().lower());
            patternFound=(filteredLine.lower().find(needle)!=-1 ||   // hilight regexp in text
                          who.lower().find(needle)!=-1 );            // hilight regexp in nickname
          }
          else
          {
            QString needle(hilight->getPattern().lower());
            patternFound=(filteredLine.lower().find(needle)!=-1 ||   // hilight patterns in text
                          who.lower().find(needle)!=-1 );            // hilight patterns in nickname
          }

          if(patternFound)
          {
            highlightColor=hilightList.at(index)->getColor().name();

            if(KonversationApplication::preferences.getHilightSoundEnabled()) {
              KonversationApplication *konvApp=static_cast<KonversationApplication *>(KApplication::kApplication());
              konvApp->sound()->play(hilightList.at(index)->getSoundURL());
            }
            // only set auto text if it was someone else but ourselves
            if(whoSent!=server->getNickname()) autoTextToSend=hilight->getAutoText();
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
  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + channelColor + "\"><b>&lt;%2&gt;</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + channelColor + "\">%1 <b>&lt;%2&gt;</b> %3</font></p>\n";
  }

  line = line.arg(timeStamp(), filter(nick,channelColor,NULL,false), filter(message,channelColor,nick,true));

  emit textToLog(QString("<%1>\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendRaw(const QString& message, bool suppressTimestamps)
{
  QString channelColor=KonversationApplication::preferences.getColor("ChannelMessage");
  QString line;

  if(suppressTimestamps) {
    line = QString("<p><font color=\"#" + channelColor + "\">" + message + "</font></p>\n");
  } else {
    line = QString("<p>" + timeStamp() + " <font color=\"#" + channelColor + "\">" + message + "</font></p>\n");
  }

  doAppend(line);
}

void IRCView::appendQuery(const QString& nick,const QString& message)
{
  QString queryColor=KonversationApplication::preferences.getColor("QueryMessage");
  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + queryColor + "\"><b>*%2*</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + queryColor + "\">%1 <b>*%2*</b> %3</font></p>\n";
  }

  line = line.arg(timeStamp(), filter(nick,queryColor,NULL,false), filter(message,queryColor,nick,true));

  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const QString& nick,const QString& message)
{
  QString actionColor=KonversationApplication::preferences.getColor("ActionMessage");
  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#"+actionColor+"\">%2 * %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#"+actionColor+"\">%1 * %2 %3</font></p>\n";
  }

  line = line.arg(timeStamp(), filter(nick,actionColor,NULL,false), filter(message,actionColor,nick,true));

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

  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + serverColor + "\"" + fixed + "><b>[%2]</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + serverColor + "\"" + fixed + ">%1 <b>[%2]</b> %3</font></p>\n";
  }

  line = line.arg(timeStamp(), type, filter(message,serverColor));

  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line);
}

void IRCView::appendCommandMessage(const QString& type,const QString& message, bool important, bool parseURL, bool self)
{
  QString commandColor=KonversationApplication::preferences.getColor("CommandMessage");
  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + commandColor + "\">*** %1" + PDF + " %2</font></p>\n";
  } else {
    line = "<p><font color=\"#" + commandColor + "\">%1 *** %2</font></p>\n";
  }

  line = line.arg(timeStamp(), filter(message,commandColor,0,true,parseURL));

  emit textToLog(QString("%1\t%2").arg(type).arg(message));

  doAppend(line, important, self);
}

void IRCView::appendBacklogMessage(const QString& firstColumn,const QString& rawMessage)
{
  QString time;
  QString message(rawMessage);
  QString first(firstColumn);
  QString backlogColor=KonversationApplication::preferences.getColor("BacklogMessage");

  // Nicks are in "<nick>" format so replace the "<>"
  first.replace(QRegExp("<"),"&lt;");
  first.replace(QRegExp(">"),"&gt;");

  // extract timestamp from message string
  if(message.startsWith("["))
  {
    time=message.section(' ',0,0);
    message=message.section(' ',1);
  }

  QString line;

  if(basicDirection(message) == QChar::DirR) {
    line = "<p><font color=\"#" + backlogColor + "\">%2 %1 %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + backlogColor + "\">%1 %2 %3</font></p>\n";
  }

  line = line.arg(time, first, filter(message, backlogColor, NULL, false));

  doAppend(line);
}

//without any display update stuff that freaks out the scrollview
void IRCView::removeSelectedText( int selNum )
{
    QTextDocument* doc=document();
    for ( int i = 0; i < (int)doc->numSelections(); ++i ) {
      if ( i == selNum )
        continue;
      doc->removeSelection( i );
    }
    // ...snip...

    doc->removeSelectedText( selNum, QTextEdit::textCursor() );

    // ...snip...
}

void IRCView::scrollToBottom()
{
    //sync(); //sync freaks out the scrollview too
    setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
}

void IRCView::doAppend(QString newLine, bool important, bool self)
{
  // Add line to buffer
  QString line(newLine);

  if(important || !KonversationApplication::preferences.getHideUnimportantEvents())
  {
    if(!self) {
      emit newText(highlightColor,important);
    }

    // scroll view only if the scroll bar is already at the bottom
    bool doScroll=KTextBrowser::verticalScrollBar()->value()==KTextBrowser::verticalScrollBar()->maxValue();

    line.remove('\n');// TODO why have newlines? we get <p>, so the \n are unnecessary...

    bool up=KTextBrowser::viewport()->isUpdatesEnabled();

    KTextBrowser::viewport()->setUpdatesEnabled(FALSE);
    KTextBrowser::append(line);

    document()->lastParagraph()->format();
    resizeContents(contentsWidth(), document()->height());

    //Explanation: the scrolling mechanism cannot handle the buffer changing when the scrollbar is not at an end,
    //             so the scrollbar wets its pants and forgets who it is for ten minutes

    if (doScroll) // TODO: make this eat multiple lines at once when the preference is changed so it doesn't take so long
    {
      int sbm=KonversationApplication::preferences.getScrollbackMax();
      if (sbm) {
        for(sbm=paragraphs()-sbm;sbm>0;sbm--) //loop for two reasons: 1) preference changed 2) lines added while scrolled up
          removeParagraph(0);
        resizeContents(contentsWidth(), document()->height());
      }
    }

    KTextBrowser::viewport()->setUpdatesEnabled(up);

    if (doScroll)
    {
      setContentsPos( contentsX(), contentsHeight() - visibleHeight() );
      repaintContents(FALSE);
    }
  }
  if(!autoTextToSend.isEmpty())
  {
    // replace placeholders in autoText
    QString sendText=server->parseWildcards(autoTextToSend,server->getNickname(),QString::null,QString::null,QString::null,QString::null);
    // avoid recursion due to signalling
    autoTextToSend=QString::null;
    // send signal only now
    emit autoText(sendText);
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
      if(copyUrlMenu) {
        urlClickSlot(urlToCopy);
      }
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
  caseSensitive = false;
  wholeWords = false;
  forward = false;
  fromCursor = false;

  pattern = SearchDialog::search(this, &caseSensitive, &wholeWords, &forward, &fromCursor);

  if(!fromCursor)
  {
    if(forward)
    {
      findParagraph = 1;
      findIndex = 1;
    }
    else
    {
      findParagraph = paragraphs();
      findIndex = paragraphLength(paragraphs());
    }
  }

  searchAgain();
}

void IRCView::searchAgain()
{
  if(!pattern.isEmpty())
  {
    // next search must begin one index before / after the last search
    // depending on the search direction.
    if(forward)
    {
      findIndex++;
      if(findIndex == paragraphLength(findParagraph))
      {
        findIndex = 0;
        findParagraph++;
      }
    } else {
      if(findIndex) {
        findIndex--;
      } else {
        findParagraph--;
        findIndex = paragraphLength(findParagraph);
      }
    }

    if(!find(pattern, caseSensitive, wholeWords, forward, &findParagraph, &findIndex)) {
      KMessageBox::information(this,i18n("No matches found for \"%1\".").arg(pattern),i18n("Information"));
    }
  }
}

// other windows can link own menu entries here
QPopupMenu* IRCView::getPopup()
{
  return popup;
}

QChar IRCView::LRM = (ushort)0x200e;
QChar IRCView::RLM = (ushort)0x200f;
QChar IRCView::LRE = (ushort)0x202a;
QChar IRCView::RLE = (ushort)0x202b;
QChar IRCView::RLO = (ushort)0x202e;
QChar IRCView::LRO = (ushort)0x202d;
QChar IRCView::PDF = (ushort)0x202c;

QChar::Direction IRCView::basicDirection(const QString &string)
{
  // find base direction
  unsigned int pos = 0;
  while ((pos < string.length()) &&
    (string.at(pos) != RLE) &&
    (string.at(pos) != LRE) &&
    (string.at(pos) != RLO) &&
    (string.at(pos) != LRO) &&
    (string.at(pos).direction() > 1) &&
    (string.at(pos).direction() != QChar::DirAL)) // not R and not L
  {
    pos++;
  }

  if ((string.at(pos).direction() == QChar::DirR) ||
    (string.at(pos).direction() == QChar::DirAL) ||
    (string.at(pos) == RLE) ||
    (string.at(pos) == RLO))
  {
    return QChar::DirR;
  }

  return QChar::DirL;
}

QString IRCView::timeStamp()
{
  if(KonversationApplication::preferences.getTimestamping())
  {
    QTime time = QTime::currentTime();
    QString timeColor = KonversationApplication::preferences.getColor("Time");
    QString timeFormat = KonversationApplication::preferences.getTimestampFormat();
    QString timeString;

    if(!KonversationApplication::preferences.getShowDate())
    {
      timeString = QString("<font color=\"#" + timeColor + "\">[%1]</font> ").arg(time.toString(timeFormat));
    }
    else
    {
      QDate date = QDate::currentDate();
      timeString = QString("<font color=\"#" + timeColor + "\">[%1 %2]</font> ").arg(
        date.toString(Qt::ISODate), time.toString(timeFormat));
    }

    return timeString;
  }

  return QString::null;
}

#include "ircview.moc"
