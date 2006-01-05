/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  The panel where the server status messages go
  begin:     Sam Jan 18 2003
  copyright: (C) 2003 by Dario Abatianni
  email:     eisfuchs@tigress.com
*/

#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qtextcodec.h>
#include <qlineedit.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "channel.h"
#include "statuspanel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "server.h"

StatusPanel::StatusPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::Status);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;

    // set up text view, will automatically take care of logging
    IRCViewBox* ircBox = new IRCViewBox(this, 0); // Server will be set later in setServer()
    setTextView(ircBox->ircView());

    QHBox* commandLineBox=new QHBox(this);
    commandLineBox->setSpacing(spacing());
    commandLineBox->setMargin(0);

    nicknameCombobox=new QComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->insertStringList(Preferences::nicknameList());
    oldNick=nicknameCombobox->currentText();

    setShowNicknameBox(Preferences::showNicknameBox());

    awayLabel=new QLabel(i18n("(away)"),commandLineBox);
    awayLabel->hide();
    statusInput=new IRCInput(commandLineBox);

    getTextView()->installEventFilter(statusInput);
    statusInput->installEventFilter(this);

    setLog(Preferences::log());
    setLogfileName("konversation");

    connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

    connect(getTextView(), SIGNAL(updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT(activateTabNotification(Konversation::TabNotifyType)));
    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );

    connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
    connect(statusInput,SIGNAL (textPasted(const QString&)),this,SLOT(textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), statusInput, SLOT(paste(bool)));

    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editedable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

    updateFonts();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentText(newNickname);
}

void StatusPanel::childAdjustFocus()
{
    statusInput->setFocus();
}

void StatusPanel::sendStatusText(const QString& sendLine)
{
    // create a work copy
    QString output(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(output))
    {
        output = m_server->parseWildcards(output, m_server->getNickname(), QString::null, QString::null, QString::null, QString::null);
    }

    // encoding stuff is done in Server()
    Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, QString::null);

    if(!result.output.isEmpty())
    {
        appendServerMessage(result.typeString, result.output);
    }

    m_server->queue(result.toServer);
}

void StatusPanel::statusTextEntered()
{
    QString line=statusInput->text();
    statusInput->clear();

    if(line.lower()=="/clear") textView->clear();
    else
    {
        if(line.length()) sendStatusText(line);
    }
}

void StatusPanel::textPasted(const QString& text)
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
            sendStatusText(line);
        }
    }
}

void StatusPanel::updateFonts()
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

    statusInput->unsetPalette();
    statusInput->setPaletteForegroundColor(fg);
    statusInput->setPaletteBackgroundColor(bg);
    statusInput->setFont(Preferences::textFont());

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

    nicknameCombobox->setFont(Preferences::textFont());
}

void StatusPanel::sendFileMenu()
{
    emit sendFile();
}

void StatusPanel::indicateAway(bool show)
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

// fix Qt's broken behavior on hidden QListView pages
void StatusPanel::showEvent(QShowEvent*)
{
    if(awayChanged)
    {
        awayChanged=false;
        indicateAway(awayState);
    }
}

QString StatusPanel::getTextInLine() { return statusInput->text(); }

bool StatusPanel::canBeFrontView()        { return true; }
bool StatusPanel::searchView()       { return true; }

bool StatusPanel::closeYourself()
{
    int result=KMessageBox::warningContinueCancel(
        this,
        i18n("Do you want to disconnect from '%1'?").arg(m_server->getServerName()),
        i18n("Disconnect From Server"),
        i18n("Disconnect"),
        "QuitServerTab");

    if(result==KMessageBox::Continue)
    {
        m_server->serverGroupSettings()->setNotificationsEnabled(notificationsEnabled());
        m_server->quitServer();
        //Why are these seperate?  why would deleting the server not quit it? FIXME
        delete m_server;
        m_server=0;
        deleteLater();                            //NO NO!  Deleting the server should delete this! FIXME
        return true;
    }
    return false;
}

void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick=m_server->getNickname();
    if(oldNick == newNick) return;
    nicknameCombobox->setCurrentText(oldNick);
    m_server->queue("NICK "+newNick);
}

void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue("NICK "+newNickname);
}

void StatusPanel::emitUpdateInfo()
{
    QString info = m_server->getServerGroup();

    emit updateInfo(info);
}

void StatusPanel::appendInputText(const QString& text)
{
    statusInput->setText(statusInput->text() + text);
}

                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
    Preferences::setChannelEncoding(m_server->getServerGroup(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()         // virtual
{
    return Preferences::channelEncoding(m_server->getServerGroup(), ":server");
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc()
{
    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::setShowNicknameBox(bool show)
{
    if(show)
    {
        nicknameCombobox->show();
    }
    else
    {
        nicknameCombobox->hide();
    }
}

void StatusPanel::setIdentity(const Identity *newIdentity)
{
    if(!newIdentity)
    {
        return;
    }

    ChatWindow::setIdentity(newIdentity);
    nicknameCombobox->clear();
    nicknameCombobox->insertStringList(newIdentity->getNicknameList());
}

#include "statuspanel.moc"
