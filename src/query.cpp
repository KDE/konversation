/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  begin:     Mon Jan 28 2002
  copyright: (C) 2002 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qhbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qlineedit.h>
#include <qtextcodec.h>
#include <qtooltip.h>
#include <qtextstream.h>
#include <qwhatsthis.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kstringhandler.h>

#include "channel.h"
#include "query.h"
#include "server.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "common.h"

const int POPUP_SEND=0xfd;
const int POPUP_WHOIS =0xfe;
const int POPUP_IGNORE=0xff;


Query::Query(QWidget* parent) : ChatWindow(parent)
{
    // don't setName here! It will break logfiles!
    //   setName("QueryWidget");
    setType(ChatWindow::Query);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;
    QHBox *box = new QHBox(this);
    addresseeimage = new QLabel(box, "query_image");
    addresseeimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseeimage->hide();
    addresseelogoimage = new QLabel(box, "query_logo_image");
    addresseelogoimage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    addresseelogoimage->hide();

    queryHostmask=new QLabel(box, "query_hostmask");
    queryHostmask->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QString whatsthis = i18n("<qt>Some details of the person you are talking to in this query is shown in this bar.  The full name and hostmask is shown, along with any image or logo this person has associated with them in the KDE Addressbook.<p>See the <i>Konversation Handbook</i> for information on associating a nick with a contact in the Addressbook, and for an explanation of what the hostmask is.</qt>");
    QWhatsThis::add(addresseeimage, whatsthis);
    QWhatsThis::add(addresseelogoimage, whatsthis);
    QWhatsThis::add(queryHostmask, whatsthis);

    IRCViewBox* ircBox = new IRCViewBox(this,0);
    setTextView(ircBox->ircView());               // Server will be set later in setServer();
    textView->setAcceptDrops(true);
    connect(textView,SIGNAL(filesDropped(const QStrList&)),this,SLOT(filesDropped(const QStrList&)));

    // link "Whois" and "Ignore" menu items into ircview popup
    QPopupMenu* popup=textView->getPopup();
    popup->insertItem(i18n("Whois"),POPUP_WHOIS); // TODO: let the ircview give the id back rather than specifying it ourselves?
    popup->insertItem(i18n("Ignore"),POPUP_IGNORE);
    if (kapp->authorize("allow_downloading"))
    {
        popup->insertItem(SmallIcon("2rightarrow"),i18n("Send &File..."),POPUP_SEND);
    }


    // This box holds the input line
    QHBox* inputBox=new QHBox(this, "input_log_box");
    inputBox->setSpacing(spacing());

    awayLabel=new QLabel(i18n("(away)"),inputBox);
    awayLabel->hide();
    queryInput=new IRCInput(inputBox);

    getTextView()->installEventFilter(queryInput);
    queryInput->installEventFilter(this);

    // connect the signals and slots
    connect(queryInput,SIGNAL (submit()),this,SLOT (queryTextEntered()) );
    connect(queryInput,SIGNAL (envelopeCommand()),this,SLOT (queryPassthroughCommand()) );
    connect(queryInput,SIGNAL (textPasted(const QString&)),this,SLOT (textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), queryInput, SLOT(paste(bool)));
    connect(getTextView(),SIGNAL (gotFocus()),queryInput,SLOT (setFocus()) );

    connect(textView, SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(activateTabNotification(Konversation::TabNotifyType)));
    connect(textView,SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(textView,SIGNAL (extendedPopup(int)),this,SLOT (popup(int)) );
    connect(textView,SIGNAL (autoText(const QString&)),this,SLOT (sendQueryText(const QString&)) );

    connect(KonversationApplication::instance(), SIGNAL (appearanceChanged()),this,SLOT (updateAppearance()) );

    updateAppearance();

    setLog(Preferences::log());
}

Query::~Query()
{
}

void Query::setName(const QString& newName)
{
    if(ChatWindow::getName() == newName) return;  // no change, so return

    ChatWindow::setName(newName);
    // don't change logfile name if query name changes
    // This will prevent Nick-Changers to create more than one log file,
    // unless we want this by turning the option Log Follows Nick off.

    if((logName.isEmpty()) || !(Preferences::logFollowsNick()))
    {
        QString logName =  (Preferences::lowerLog()) ? getName().lower() : getName() ;

        if(Preferences::addHostnameToLog())
        {
            if(m_nickInfo)
                logName += m_nickInfo->getHostmask();
        }

        setLogfileName(logName);
    }
}

void Query::queryTextEntered()
{
    QString line=queryInput->text();
    queryInput->clear();
    if(line.lower()=="/clear")
    {
        textView->clear();
    }
    else if(line.lower()=="/part")
    {
        m_server->closeQuery(getName());
    }
    else if(line.length())
    {
         sendQueryText(line);
    }
}

void Query::queryPassthroughCommand()
{
    QString commandChar = Preferences::commandChar();
    QString line = queryInput->text();

    queryInput->clear();

    if(!line.isEmpty())
    {
        // Prepend commandChar on Ctrl+Enter to bypass outputfilter command recognition
        if (line.startsWith(commandChar))
        {
            line = commandChar + line;
        }
        sendQueryText(line);
    }
}

void Query::sendQueryText(const QString& sendLine)
{
    // create a work copy
    QString output(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(output))
    {
        output = m_server->parseWildcards(output, m_server->getNickname(), getName(), QString::null, QString::null, QString::null);
    }

    // encoding stuff is done in Server()
    Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, getName());

    if(!result.output.isEmpty())
    {
        if(result.type == Konversation::Action) appendAction(m_server->getNickname(), result.output);
        else if(result.type == Konversation::Command) appendCommandMessage(result.typeString, result.output);
        else if(result.type == Konversation::Program) appendServerMessage(result.typeString, result.output);
        else if(!result.typeString.isEmpty()) appendQuery(result.typeString, result.output);
        else appendQuery(m_server->getNickname(), result.output);
    }

    m_server->queue(result.toServer);
}

void Query::updateAppearance()
{
    QColor fg;
    QColor bg;

    if(Preferences::inputFieldsBackgroundColor())
    {
        fg=Preferences::color(Preferences::ChannelMessage);
        bg=Preferences::color(Preferences::TextViewBackground);
    }
    else
    {
        fg=colorGroup().foreground();
        bg=colorGroup().base();
    }

    queryInput->unsetPalette();
    queryInput->setPaletteForegroundColor(fg);
    queryInput->setPaletteBackgroundColor(bg);
    queryInput->setFont(Preferences::textFont());

    //  queryHostmask->setPaletteForegroundColor(fg);
    //  queryHostmask->setPaletteBackgroundColor(bg);
    //  queryHostmask->setFont(Preferences::textFont());

    getTextView()->unsetPalette();
    getTextView()->setFont(Preferences::textFont());

    if(Preferences::showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            Preferences::backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            QString::null);
    }
}

void Query::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline=QStringList::split('\n',text);
        for(unsigned int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendQueryText(line);
        }
    }
}

void Query::indicateAway(bool show)
{
    // QT does not redraw the label properly when they are not on screen
    // while getting hidden, so we remember the "soon to be" state here.
    if(isHidden())
    {
        awayChanged=true;
        awayState=show;
    }
    else
    {
        if(show)
            awayLabel->show();
        else
            awayLabel->hide();
    }
}

// fix QTs broken behavior on hidden QListView pages
void Query::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }
}

void Query::popup(int id)
{
    if(id == POPUP_WHOIS)
        sendQueryText(Preferences::commandChar()+"WHOIS "+getName());
    else if(id == POPUP_IGNORE)
    {
        sendQueryText(Preferences::commandChar()+"IGNORE -ALL "+getName()+"!*");
        int rc=KMessageBox::questionYesNo(this,
            i18n("Do you want to close this query after ignoring this nickname?"),
            i18n("Close This Query"),
            i18n("Close"),
            i18n("Keep Open"),
            "CloseQueryAfterIgnore");

        if(rc==KMessageBox::Yes) closeYourself();
    }
    else if(id == POPUP_SEND)
    {
         sendQueryText(Preferences::commandChar()+"DCC SEND "+getName());
    }
    else
        kdDebug() << "Query::popup(): Popup id " << id << " does not belong to me!" << endl;
}

void Query::sendFileMenu()
{
    emit sendFile(getName());
}

void Query::childAdjustFocus()
{
    queryInput->setFocus();
}

void Query::setNickInfo(const NickInfoPtr & nickInfo)
{
    if(m_nickInfo)
        disconnect(m_nickInfo, SIGNAL(nickInfoChanged()), this, SLOT(nickInfoChanged()));

    m_nickInfo = nickInfo;
    Q_ASSERT(m_nickInfo); if(!m_nickInfo) return;
    setName(m_nickInfo->getNickname());
    connect(m_nickInfo, SIGNAL(nickInfoChanged()), this, SLOT(nickInfoChanged()));
    nickInfoChanged();
}

void Query::nickInfoChanged()
{
    if(m_nickInfo)
    {
        setName(m_nickInfo->getNickname());
        QString text = m_nickInfo->getBestAddresseeName();
        if(!m_nickInfo->getHostmask().isEmpty() && !text.isEmpty())
            text += " - ";
        text += m_nickInfo->getHostmask();
        if(m_nickInfo->isAway() )
            text += " (" + KStringHandler::rsqueeze(m_nickInfo->getAwayMessage(),100) + ") ";
        queryHostmask->setText(Konversation::removeIrcMarkup(text));

        KABC::Picture pic = m_nickInfo->getAddressee().photo();
        if(pic.isIntern())
        {
            QPixmap qpixmap(pic.data().scaleHeight(queryHostmask->height()));
            if(!qpixmap.isNull())
            {
                addresseeimage->setPixmap(qpixmap);
                addresseeimage->show();
            }
            else
            {
                addresseeimage->hide();
            }
        }
        else
        {
            addresseeimage->hide();
        }
        KABC::Picture logo = m_nickInfo->getAddressee().logo();
        if(logo.isIntern())
        {
            QPixmap qpixmap(logo.data().scaleHeight(queryHostmask->height()));
            if(!qpixmap.isNull())
            {
                addresseelogoimage->setPixmap(qpixmap);
                addresseelogoimage->show();
            }
            else
            {
                addresseelogoimage->hide();
            }
        }
        else
        {
            addresseelogoimage->hide();
        }

        QString strTooltip;
        QTextStream tooltip( &strTooltip, IO_WriteOnly );

        tooltip << "<qt>";

        tooltip << "<table cellspacing=\"0\" cellpadding=\"0\">";

        m_nickInfo->tooltipTableData(tooltip);

        tooltip << "</table></qt>";
        QToolTip::add(queryHostmask, strTooltip);
        QToolTip::add(addresseeimage, strTooltip);
        QToolTip::add(addresseelogoimage, strTooltip);

    }
    else
    {
        addresseeimage->hide();
        addresseelogoimage->hide();
    }
    emitUpdateInfo();
}

NickInfoPtr Query::getNickInfo()
{
    return m_nickInfo;
}

QString Query::getTextInLine() { return queryInput->text(); }

bool Query::canBeFrontView()        { return true; }
bool Query::searchView()       { return true; }

void Query::appendInputText(const QString& s)
{
    queryInput->setText(queryInput->text() + s);
}

                                                  // virtual
void Query::setChannelEncoding(const QString& encoding)
{
    Preferences::setChannelEncoding(m_server->getServerGroup(), getName(), encoding);
}

QString Query::getChannelEncoding()               // virtual
{
    return Preferences::channelEncoding(m_server->getServerGroup(), getName());
}

QString Query::getChannelEncodingDefaultDesc()    // virtual
{
    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
}

bool Query::closeYourself()
{
    int result=KMessageBox::warningContinueCancel(
        this,
        i18n("Do you want close your query with %1?").arg(getName()),
        i18n("Close Query"),
        i18n("Close"),
        "QuitQueryTab");

    if(result==KMessageBox::Continue)
    {
        m_server->removeQuery(this);
        return true;
    }

    return false;
}

void Query::filesDropped(const QStrList& files)
{
    m_server->sendURIs(files,getName());
}

void Query::emitUpdateInfo()
{
    QString info;
    if(m_nickInfo->loweredNickname() == m_server->loweredNickname())
        info = i18n("Talking to yourself");
    else if(m_nickInfo)
        info = m_nickInfo->getBestAddresseeName();
    else
        info = getName();

    emit updateInfo(info);
}

#include "query.moc"
