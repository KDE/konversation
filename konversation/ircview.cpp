/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  the text widget used for all text based panels
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
#include <qwhatsthis.h>
#include <qmap.h>
#include <qcolor.h>

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
#include <kshell.h>
#include <kpopupmenu.h>

#include "konvidebug.h"
#include "konversationapplication.h"
#include "ircview.h"
#include "highlight.h"
#include "server.h"
#include "searchdialog.h"
#include "konversationsound.h"
#include "chatwindow.h"
#include "common.h"
#include "images.h"

IRCView::IRCView(QWidget* parent,Server* newServer) : KTextBrowser(parent)
{
  QWhatsThis::add(this, i18n("<qt>The text for the channel, server or query is shown here.  You can view the history by choosing <em>Open logfile</em> from the Window menu.</qt>"));
  copyUrlMenu=false;
  resetScrollbar=TRUE;
  offset=0;
  m_currentNick=QString::null;
  m_isOnNick=false;

  setAutoFormatting(QTextEdit::AutoNone);
  setUndoRedoEnabled(0);
  setLinkUnderline(false);

  popup=new QPopupMenu(this,"ircview_context_menu");

  if(popup)
  {
    popup->insertItem(SmallIconSet("editcopy"),i18n("&Copy"),Copy);
    popup->insertItem(i18n("Select All"),SelectAll);
    popup->insertSeparator();
    popup->insertItem(SmallIcon("find"),i18n("Find Text..."),Search);
    popup->insertSeparator();
    popup->insertItem(i18n("Send File..."),SendFile);
  }
  else kdWarning() << "IRCView::IRCView(): Could not create popup!" << endl;

  setupNickPopupMenu();

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

  setWrapPolicy(QTextEdit::AtWordOrDocumentBoundary);

  setNotifyClick(true);

  connect(this,SIGNAL (highlighted(const QString&)),this,SLOT (highlightedSlot(const QString&)));
  connect(this,SIGNAL (linkClicked(const QString&)),this,SLOT(urlClickSlot(const QString&)));
}

IRCView::~IRCView()
{
    delete popup;
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
  QPixmap backgroundPixmap;
  backgroundPixmap.load(pixmapName);

  if(backgroundPixmap.isNull()) {
    setPaper(backgroundColor);
  } else {
    QBrush backgroundBrush;
    backgroundBrush.setColor(backgroundColor);
    backgroundBrush.setPixmap(backgroundPixmap);

    setPaper(backgroundBrush);
  }
}

void IRCView::setServer(Server* newServer)
{
  m_server=newServer;
}

const QString& IRCView::getContextNick() const
{
  return m_currentNick;
}

void IRCView::clear()
{
  buffer=QString::null;
  KTextBrowser::clear();
}

void IRCView::highlightedSlot(const QString& link)
{
  if(!link.startsWith("#")){
    m_isOnNick=false;
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
  else if(link.startsWith("#") && !link.startsWith("##"))
    {
      m_currentNick=link;
      m_currentNick.remove("#");
      nickPopup->changeTitle(popupId,m_currentNick);
      m_isOnNick=true;
    }
}

void IRCView::urlClickSlot(const QString &url)
{
  if (!url.isEmpty() && !url.startsWith("#"))
  {
    // Always use KDE default mailer.
    if (KonversationApplication::preferences.getWebBrowserUseKdeDefault() || url.lower().startsWith("mailto:"))
    {
      new KRun(KURL(url));
    }
    else
    {
      QString cmd = KonversationApplication::preferences.getWebBrowserCmd();
      cmd.replace("%u",url);
      KProcess *proc = new KProcess;
      QStringList cmdAndArgs = KShell::splitArgs(cmd);
      *proc << cmdAndArgs;
//      This code will also work, but starts an extra shell process.
//      kdDebug() << "IRCView::urlClickSlot(): cmd = " << cmd << endl;
//      *proc << cmd;
//      proc->setUseShell(true);
      proc->start(KProcess::DontCare);
      delete proc;
    }
  }
  else if(url.startsWith("##")) // Channel
    {
      QString channel(url);
      channel.replace("##","#");
      m_server->sendJoinCommand(channel);
    }
  else if(url.startsWith("#")) // Nick
    {
      QString recepient(url);
      recepient.remove("#");
      NickInfoPtr nickInfo = m_server->obtainNickInfo(recepient);
      m_server->addQuery(nickInfo, true /*we initiated*/);
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

QString IRCView::filter(const QString& line,const QString& defaultColor,const QString& whoSent,bool doHighlight, bool parseURL)
{
  QString filteredLine(line);
  KonversationApplication* konvApp = static_cast<KonversationApplication*>(kapp);

  // TODO: Use QStyleSheet::escape() here

  filteredLine.replace("&","&amp;");
  // Replace all < with &lt;
  filteredLine.replace("<","&lt;");
  // Replace all > with &gt;
  filteredLine.replace(">","&gt;");
  // Replace all 0x03 without color number (reset color) with \0x031,0 or \0x030,1, depending on which one fits
  // with the users chosen colours, based on the relative brightness. TODO defaultColor needs explanation
  bool inverted = false; // TODO this flag should be stored somewhere
  {
    QColor fg("#"+KonversationApplication::preferences.getColor("ChannelMessage")),
        bg("#"+KonversationApplication::preferences.getColor("TextViewBackground"));
    int h = 0, s = 0,fv = 0,bv = 0;
    fg.getHsv(&h,&s,&fv);
    bg.getHsv(&h,&s,&bv);

    if (bv <= fv) {
      inverted = false;
    }
  }

  if(inverted) {
    filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0030,1\\1");
  } else {
    filteredLine.replace(QRegExp("\003([^0-9]|$)"),"\0031,0\\1");
  }

  // Hack to allow for whois info hostmask info to not be parsed as email
  filteredLine.replace("&amp;#64;","&#64;");

  if(filteredLine.find("\x07") != -1)
  {
    if(KonversationApplication::preferences.getBeep()) {
      kapp->beep();
    }
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
    if(filterColors) {
      colorString = QString::null;
    } else {
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
        colorString += "<font color=\"" + colorCodes[foregroundColor] + "\">";
      }

      firstColor = false;
    }

    filteredLine.replace(pos,colorRegExp.cap(0).length(),colorString);
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

  if(parseURL) {
    filteredLine = Konversation::tagURLs(filteredLine, whoSent);
  }

  // Highlight
  QString ownNick;

  if(m_server) {
    ownNick = m_server->getNickname();
  }

  if(doHighlight && m_server && (whoSent != ownNick))
  {
    m_highlightColor = QString::null;

    // FIXME: We got to get rid of m_server dependance here
    if(KonversationApplication::preferences.getHighlightNick() &&
        filteredLine.lower().find(QRegExp("(^|[^\\d\\w])" +
        QRegExp::escape(ownNick.lower()) +
        "([^\\d\\w]|$)")) != -1)
    {
      // highlight current nickname
      m_highlightColor = KonversationApplication::preferences.getHighlightNickColor().name();
    } else {
      QPtrList<Highlight> highlightList = KonversationApplication::preferences.getHighlightList();
      QPtrListIterator<Highlight> it(highlightList);
      Highlight* highlight = it.current();
      bool patternFound = false;

      while(highlight != 0)
      {
        if(highlight->getRegExp())
        {
          QRegExp needle(highlight->getPattern().lower());
          patternFound = ((filteredLine.lower().find(needle) != -1) ||   // highlight regexp in text
                          (whoSent.lower().find(needle) != -1));            // highlight regexp in nickname
        }
        else
        {
          QString needle(highlight->getPattern().lower());
          patternFound = ((filteredLine.lower().find(needle) != -1) ||   // highlight patterns in text
                          (whoSent.lower().find(needle) != -1));            // highlight patterns in nickname
        }

        if(!patternFound) {
          ++it;
          highlight = it.current();
        } else {
          break;
        }
      }

      if(patternFound)
      {
        m_highlightColor = highlight->getColor().name();

        if(KonversationApplication::preferences.getHighlightSoundEnabled()) {
          konvApp->sound()->play(highlight->getSoundURL());
        }

        autoTextToSend = highlight->getAutoText();
      }
    }

    // apply found highlight color to line
    if(!m_highlightColor.isEmpty()) {
      filteredLine = "<font color=\"" + m_highlightColor + "\">" + filteredLine + "</font>";
    }
  } else if(doHighlight && (whoSent == ownNick) &&
    KonversationApplication::preferences.getHighlightOwnLines())
  {
    // highlight own lines
    filteredLine = "<font color=\"" + KonversationApplication::preferences.getHighlightOwnLinesColor().name() +
      "\">" + filteredLine + "</font>";
  }

  // Replace multiple Spaces with "<space>&nbsp;"
  while((pos = filteredLine.find("  ")) != -1)
  {
    filteredLine.replace(pos + (pos == 0 ? 0 : 1), 1, "&nbsp;");
  }

  return filteredLine;
}

void IRCView::append(const QString& nick,const QString& message)
{
  QString channelColor = KonversationApplication::preferences.getColor("ChannelMessage");
  QString line;
  QString nickLine = "%2";
  
  if(KonversationApplication::preferences.getUseColoredNicks() && nick != m_server->getNickname()) {
    NickInfoPtr nickinfo = m_server->obtainNickInfo(nick);
    nickLine = "<a href=\"#" + nick + "\"><font color=\"" + nickinfo->getNickColor() + "\">%2</font></a>";
  }
  
  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + channelColor + "\"><b>&lt;</b>" + nickLine + "<b>&gt;</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + channelColor + "\">%1 <b>&lt;</b>" + nickLine + "<b>&gt;</b> %3</font></p>\n";
  }
  
  line = line.arg(timeStamp(), nick, filter(message,channelColor,nick,true));

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
  QString nickLine = "%2";

  if(KonversationApplication::preferences.getUseColoredNicks() && nick != m_server->getNickname())
  {
    NickInfoPtr nickinfo = m_server->obtainNickInfo(nick);
    nickLine = "<font color=\"" + nickinfo->getNickColor() + "\">%2</font>";
  }

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + queryColor + "\"><b>*</b>" + nickLine + "<b>*</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + queryColor + "\">%1 <b>*</b>" + nickLine + "<b>*</b> %3</font></p>\n";
  }

  line = line.arg(timeStamp(), nick, filter(message,queryColor,nick,true));

  emit textToLog(QString("*%1*\t%2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendAction(const QString& nick,const QString& message)
{
  QString actionColor=KonversationApplication::preferences.getColor("ActionMessage");
  QString line;
  QString nickLine = "%2";

  if(KonversationApplication::preferences.getUseColoredNicks() && nick != m_server->getNickname())
  {
    NickInfoPtr nickinfo = m_server->obtainNickInfo(nick);
    nickLine = "<a href=\"#" + nick + "\"><font color=\"" + nickinfo->getNickColor() + "\">%2</font></a>";
  }

  if(basicDirection(message) == QChar::DirR) {
    line = RLO;
    line += LRE;
    line += "<p><font color=\"#" + actionColor + "\">" + nickLine + " * %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + actionColor + "\">%1 * " + nickLine + " %3</font></p>\n";
  }

  line = line.arg(timeStamp(), nick, filter(message,actionColor,nick,true));

  emit textToLog(QString("\t * %1 %2").arg(nick).arg(message));

  doAppend(line);
}

void IRCView::appendServerMessage(const QString& type,const QString& message)
{
  QString m_serverColor=KonversationApplication::preferences.getColor("ServerMessage");

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
    line += "<p><font color=\"#" + m_serverColor + "\"" + fixed + "><b>[</b>%2<b>]</b> %1" + PDF + " %3</font></p>\n";
  } else {
    line = "<p><font color=\"#" + m_serverColor + "\"" + fixed + ">%1 <b>[</b>%2<b>]</b> %3</font></p>\n";
  }

  line = line.arg(timeStamp(), type, filter(message,m_serverColor));

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

  if(!first.isEmpty() && !first.startsWith("<") && !first.startsWith("*")) {
    first = "|" + first + "|";
  }

  // Nicks are in "<nick>" format so replace the "<>"
  first.replace("<","&lt;");
  first.replace(">","&gt;");

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

void IRCView::doAppend(QString newLine, bool important, bool self)
{
  // Add line to buffer
  QString line(newLine);

  if(important || !KonversationApplication::preferences.getHideUnimportantEvents())
  {
    if(!self) {
      emit newText(m_highlightColor,important);
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
    QString sendText=m_server->parseWildcards(autoTextToSend,m_server->getNickname(),QString::null,QString::null,QString::null,QString::null);
    // avoid recursion due to signalling
    autoTextToSend=QString::null;
    // send signal only now
    emit autoText(sendText);
  }
}

// remember if scrollbar was positioned at the end of the text or not
void IRCView::hideEvent(QHideEvent* /* event */)
{
  resetScrollbar = ((contentsHeight()-visibleHeight()) == contentsY());
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

void IRCView::focusInEvent(QFocusEvent*)
{
  // Set focus to input line (must be connected)
  emit gotFocus();
}

bool IRCView::eventFilter(QObject* object,QEvent* event)
{
  if(event->type()==QEvent::MouseButtonRelease) {
    QMouseEvent* me=(QMouseEvent*) event;

    if(me->button()==QMouseEvent::MidButton)
    {
      if(copyUrlMenu) {
        urlClickSlot(urlToCopy);
      } else {
        emit textPasted();
      }
    }
  } else if(event->type()==QEvent::ContextMenu) {
    return contextMenu((QContextMenuEvent*) event);
  }

  return KTextBrowser::eventFilter(object,event);
}

bool IRCView::contextMenu(QContextMenuEvent* ce)
{
  if(m_isOnNick)
    {
      nickPopup->exec(ce->pos());
    }
  else {
      
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
	  cb->setText(urlToCopy,QClipboard::Selection);
	  cb->setText(urlToCopy,QClipboard::Clipboard);
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
	  KBookmarkManager* bm = KBookmarkManager::userBookmarksManager();
	  KBookmarkGroup bg = bm->addBookmarkDialog(urlToCopy, QString::null);
	  bm->save();
	  bm->emitChanged(bg);
	  break;
	}
      default:
	emit extendedPopup(r);
      }
  }
  return true;
}

void IRCView::setupNickPopupMenu()
{
  nickPopup=new KPopupMenu(this,"nicklist_context_menu");
  modes=new KPopupMenu(this,"nicklist_modes_context_submenu");
  kickban=new KPopupMenu(this,"nicklist_kick_ban_context_submenu");
  popupId= nickPopup->insertTitle(m_currentNick);
  modes->insertItem(i18n("Give Op"),GiveOp);
  modes->insertItem(i18n("Take Op"),TakeOp);
  modes->insertItem(i18n("Give Voice"),GiveVoice);
  modes->insertItem(i18n("Take Voice"),TakeVoice);
  nickPopup->insertItem(i18n("Modes"),modes,ModesSub);
  nickPopup->insertSeparator();
  nickPopup->insertItem(i18n("&Whois"),Whois);
  nickPopup->insertItem(i18n("&Version"),Version);
  nickPopup->insertItem(i18n("&Ping"),Ping);
  nickPopup->insertSeparator();
  nickPopup->insertItem(i18n("Open Query"),Query);
  nickPopup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),DccSend);
  nickPopup->insertSeparator();
  kickban->insertItem(i18n("Kick"),Kick);
  kickban->insertItem(i18n("Kickban"),KickBan);
  kickban->insertItem(i18n("Ban Nickname"),BanNick);
  kickban->insertSeparator();
  kickban->insertItem(i18n("Ban *!*@*.host"),BanHost);
  kickban->insertItem(i18n("Ban *!*@domain"),BanDomain);
  kickban->insertItem(i18n("Ban *!user@*.host"),BanUserHost);
  kickban->insertItem(i18n("Ban *!user@domain"),BanUserDomain);
  kickban->insertSeparator();
  kickban->insertItem(i18n("Kickban *!*@*.host"),KickBanHost);
  kickban->insertItem(i18n("Kickban *!*@domain"),KickBanDomain);
  kickban->insertItem(i18n("Kickban *!user@*.host"),KickBanUserHost);
  kickban->insertItem(i18n("Kickban *!user@domain"),KickBanUserDomain);
  nickPopup->insertItem(i18n("Kick / Ban"),kickban,KickBanSub);
  nickPopup->insertItem(i18n("Ignore"),Ignore);
  
  connect (nickPopup, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
  connect (modes, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
  connect (kickban, SIGNAL(activated(int)), this, SIGNAL(popupCommand(int)));
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
