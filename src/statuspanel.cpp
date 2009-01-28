/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2003 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006-2008 Eike Hein <hein@kde.org>
*/

#include "statuspanel.h"
#include "channel.h"
#include "konversationapplication.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "server.h"

#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <q3hbox.h>
#include <qtextcodec.h>
#include <qlineedit.h>
//Added by qt3to4:
#include <QShowEvent>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>


StatusPanel::StatusPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::Status);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;

    // set up text view, will automatically take care of logging
    IRCViewBox* ircBox = new IRCViewBox(this, 0); // Server will be set later in setServer()
    setTextView(ircBox->ircView());

    Q3HBox* commandLineBox=new Q3HBox(this);
    commandLineBox->setSpacing(spacing());
    commandLineBox->setMargin(0);

    nicknameCombobox=new QComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->insertStringList(Preferences::nicknameList());
    oldNick=nicknameCombobox->currentText();

    awayLabel=new QLabel(i18n("(away)"),commandLineBox);
    awayLabel->hide();
    statusInput=new IRCInput(commandLineBox);

    getTextView()->installEventFilter(statusInput);
    statusInput->installEventFilter(this);

    setLog(Preferences::log());

    connect(getTextView(),SIGNAL (gotFocus()),statusInput,SLOT (setFocus()) );

    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(const QString&)),this,SLOT (sendStatusText(const QString&)) );

    connect(statusInput,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
    connect(statusInput,SIGNAL (textPasted(const QString&)),this,SLOT(textPasted(const QString&)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), statusInput, SLOT(paste(bool)));
    connect(getTextView(), SIGNAL(popupCommand(int)), this, SLOT(popupCommand(int)));

    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editedable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (lostFocus()),this,SLOT(nicknameComboboxChanged()));

    updateAppearance();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::serverSaysClose()
{
    closeYourself(false);
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
    QString outputAll(sendLine);
    // replace aliases and wildcards
    if(m_server->getOutputFilter()->replaceAliases(outputAll))
    {
        outputAll = m_server->parseWildcards(outputAll, m_server->getNickname(), QString(), QString(), QString(), QString());
    }

    // Send all strings, one after another
    QStringList outList=QStringList::split('\n',outputAll);
    for(unsigned int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, QString());

        if(!result.output.isEmpty())
        {
            appendServerMessage(result.typeString, result.output);
        }
        m_server->queue(result.toServer);
    } // for
}

void StatusPanel::statusTextEntered()
{
    QString line=statusInput->text();
    statusInput->setText("");

    if(line.lower()==Preferences::commandChar()+"clear") textView->clear();
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

void StatusPanel::updateAppearance()
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

    getTextView()->unsetPalette();

    if(Preferences::showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            Preferences::backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::color(Preferences::TextViewBackground),
            QString());
    }

    if (Preferences::customTextFont())
    {
        getTextView()->setFont(Preferences::textFont());
        statusInput->setFont(Preferences::textFont());
        nicknameCombobox->setFont(Preferences::textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        statusInput->setFont(KGlobalSettings::generalFont());
        nicknameCombobox->setFont(KGlobalSettings::generalFont());
    }

    showNicknameBox(Preferences::showNicknameBox());

    ChatWindow::updateAppearance();
}

void StatusPanel::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.lower());
}

void StatusPanel::updateName()
{
    QString newName = getServer()->getDisplayName();
    setName(newName);
    setLogfileName(newName.lower());
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

void StatusPanel::setNotificationsEnabled(bool enable)
{
    if (m_server->getServerGroup()) m_server->getServerGroup()->setNotificationsEnabled(enable);

    m_notificationsEnabled = enable;
}

bool StatusPanel::closeYourself(bool confirm)
{
    int result;

    //FIXME: Show "Do you really want to close ..." warnings in
    // disconnected state instead of closing directly. Can't do
    // that due to string freeze at the moment.
    if (confirm && !m_server->isConnected())
    {
        result = KMessageBox::warningContinueCancel(this, i18n("Do you really want to close '%1'?\n\n All associated tabs will be closed as well.").arg(getName()),
            i18n("Close Tab"), i18n("Close"), "QuitServerTab");
    }
    else
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("Do you want to disconnect from '%1'?\n\n All associated tabs will be closed as well.").arg(m_server->getServerName()),
            i18n("Disconnect From Server"),
            i18n("Disconnect"),
            "QuitServerTab");
    }

    if (result==KMessageBox::Continue)
    {
        if (m_server->getServerGroup()) m_server->getServerGroup()->setNotificationsEnabled(notificationsEnabled());
        m_server->quitServer();
        // This will delete the status view as well.
        m_server->deleteLater();
        m_server = 0;
        return true;
    }
    return false;
}

void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick=m_server->getNickname();
    if(oldNick!=newNick)
    {
      nicknameCombobox->setCurrentText(oldNick);
      m_server->queue("NICK "+newNick);
    }
    // return focus to input line
    statusInput->setFocus();
}

void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue("NICK "+newNickname);
}

void StatusPanel::emitUpdateInfo()
{
    emit updateInfo(getServer()->getDisplayName());
}

void StatusPanel::appendInputText(const QString& text, bool fromCursor)
{
    if(!fromCursor)
    {
        statusInput->append(text);
    }
    else
    {
        int para = 0, index = 0;
        statusInput->getCursorPosition(&para, &index);
        statusInput->insertAt(text, para, index);
        statusInput->setCursorPosition(para, index + text.length());
    }
}

                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
    Preferences::setChannelEncoding(m_server->getDisplayName(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()         // virtual
{
    return Preferences::channelEncoding(m_server->getDisplayName(), ":server");
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc()
{
    return i18n("Identity Default ( %1 )").arg(getServer()->getIdentity()->getCodecName());
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    //statusInput->setEnabled(online);
    getTextView()->setNickAndChannelContextMenusEnabled(online);
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::showNicknameBox(bool show)
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

void StatusPanel::setIdentity(const IdentityPtr identity)
{
    if (identity)
    {
        nicknameCombobox->clear();
        nicknameCombobox->insertStringList(identity->getNicknameList());
    }
}

void StatusPanel::popupCommand(int command)
{
    switch(command)
    {
        case Konversation::Join:
            m_server->queue("JOIN " + getTextView()->currentChannel());
            break;
        case Konversation::Topic:
            m_server->requestTopic(getTextView()->currentChannel());
            break;
        case Konversation::Names:
            m_server->queue("NAMES " + getTextView()->currentChannel(), Server::LowPriority);
            break;
    }
}

#include "statuspanel.moc"
