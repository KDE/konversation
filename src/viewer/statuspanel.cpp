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
#include "application.h"
#include "ircinput.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "awaylabel.h"
#include "server.h"

#include <KComboBox>
#include <KLineEdit>
#include <KMessageBox>

using namespace Konversation;

StatusPanel::StatusPanel(QWidget* parent) : ChatWindow(parent)
{
    setType(ChatWindow::Status);

    setChannelEncodingSupported(true);

    awayChanged=false;
    awayState=false;

    // set up text view, will automatically take care of logging
    IRCViewBox* ircBox = new IRCViewBox(this); // Server will be set later in setServer()
    setTextView(ircBox->ircView());

    KHBox* commandLineBox=new KHBox(this);
    commandLineBox->setSpacing(spacing());
    commandLineBox->setMargin(0);

    nicknameCombobox = new KComboBox(commandLineBox);
    nicknameCombobox->setEditable(true);
    nicknameCombobox->setSizeAdjustPolicy(KComboBox::AdjustToContents);
    KLineEdit* nicknameComboboxLineEdit = qobject_cast<KLineEdit*>(nicknameCombobox->lineEdit());
    if (nicknameComboboxLineEdit) nicknameComboboxLineEdit->setClearButtonShown(false);
    nicknameCombobox->setWhatsThis(i18n("<qt><p>This shows your current nick, and any alternatives you have set up.  If you select or type in a different nickname, then a request will be sent to the IRC server to change your nick.  If the server allows it, the new nickname will be selected.  If you type in a new nickname, you need to press 'Enter' at the end.</p><p>You can edit the alternative nicknames from the <em>Identities</em> option in the <em>Settings</em> menu.</p></qt>"));

    awayLabel=new AwayLabel(commandLineBox);
    awayLabel->hide();

    m_inputBar=new IRCInput(commandLineBox);

    getTextView()->installEventFilter(m_inputBar);
    m_inputBar->installEventFilter(this);

    connect(getTextView(),SIGNAL (gotFocus()),m_inputBar,SLOT (setFocus()) );

    connect(getTextView(),SIGNAL (sendFile()),this,SLOT (sendFileMenu()) );
    connect(getTextView(),SIGNAL (autoText(QString)),this,SLOT (sendText(QString)) );

    connect(m_inputBar,SIGNAL (submit()),this,SLOT(statusTextEntered()) );
    connect(m_inputBar,SIGNAL (textPasted(QString)),this,SLOT(textPasted(QString)) );
    connect(getTextView(), SIGNAL(textPasted(bool)), m_inputBar, SLOT(paste(bool)));

    connect(nicknameCombobox,SIGNAL (activated(int)),this,SLOT(nicknameComboboxChanged()));
    Q_ASSERT(nicknameCombobox->lineEdit());       //it should be editable.  if we design it so it isn't, remove these lines.
    if(nicknameCombobox->lineEdit())
        connect(nicknameCombobox->lineEdit(), SIGNAL (editingFinished()),this,SLOT(nicknameComboboxChanged()));

    updateAppearance();
}

StatusPanel::~StatusPanel()
{
}

void StatusPanel::cycle()
{
    if (m_server) m_server->cycle();
}

void StatusPanel::setNickname(const QString& newNickname)
{
    nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(newNickname));
}

void StatusPanel::childAdjustFocus()
{
    m_inputBar->setFocus();
}

void StatusPanel::sendText(const QString& sendLine)
{
    // create a work copy
    QString outputAll(sendLine);

    // replace aliases and wildcards
    m_server->getOutputFilter()->replaceAliases(outputAll, this);

    // Send all strings, one after another
    QStringList outList=outputAll.split('\n');
    for(int index=0;index<outList.count();index++)
    {
        QString output(outList[index]);

        // encoding stuff is done in Server()
        Konversation::OutputFilterResult result = m_server->getOutputFilter()->parse(m_server->getNickname(), output, QString(), this);

        if(!result.output.isEmpty())
        {
            if(result.type == Konversation::PrivateMessage) msgHelper(result.typeString, result.output);
            else appendServerMessage(result.typeString, result.output);
        }
        m_server->queue(result.toServer);
    } // for
}

void StatusPanel::statusTextEntered()
{
    QString line = sterilizeUnicode(m_inputBar->toPlainText());

    m_inputBar->clear();

    if (!line.isEmpty()) sendText(line);
}

void StatusPanel::textPasted(const QString& text)
{
    if(m_server)
    {
        QStringList multiline=text.split('\n');
        for(int index=0;index<multiline.count();index++)
        {
            QString line=multiline[index];
            QString cChar(Preferences::self()->commandChar());
            // make sure that lines starting with command char get escaped
            if(line.startsWith(cChar)) line=cChar+line;
            sendText(line);
        }
    }
}

void StatusPanel::updateAppearance()
{
    if (Preferences::self()->showNicknameBox())
    {
        if (Preferences::self()->customTextFont())
            nicknameCombobox->setFont(Preferences::self()->textFont());
        else
            nicknameCombobox->setFont(KGlobalSettings::generalFont());

        nicknameCombobox->show();
    }
    else
        nicknameCombobox->hide();

    ChatWindow::updateAppearance();
}

void StatusPanel::setName(const QString& newName)
{
    ChatWindow::setName(newName);
    setLogfileName(newName.toLower());
}

void StatusPanel::updateName()
{
    QString newName = getServer()->getDisplayName();
    setName(newName);
    setLogfileName(newName.toLower());
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

    if (confirm && !m_server->isConnected())
    {
        result = KMessageBox::warningContinueCancel(
                this,
                i18n("Do you really want to close '%1'?\n\nAll associated tabs will be closed as well.",getName()),
                i18n("Close Tab"),
                KStandardGuiItem::close(),
                KStandardGuiItem::cancel(),
                "QuitServerTab");
    }
    else
    {
        result = KMessageBox::warningContinueCancel(
            this,
            i18n("Do you want to disconnect from '%1'?\n\nAll associated tabs will be closed as well.",m_server->getServerName()),
            i18n("Disconnect From Server"),
            KGuiItem(i18n("Disconnect")),
            KStandardGuiItem::cancel(),
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
    else
    {
        m_recreationScheduled = false;

        m_server->abortScheduledRecreation();
    }

    return false;
}

void StatusPanel::nicknameComboboxChanged()
{
    QString newNick=nicknameCombobox->currentText();
    oldNick=m_server->getNickname();
    if (oldNick != newNick)
    {
        nicknameCombobox->setCurrentIndex(nicknameCombobox->findText(oldNick));
        m_server->queue("NICK "+newNick);
    }
    // return focus to input line
    m_inputBar->setFocus();
}

void StatusPanel::changeNickname(const QString& newNickname)
{
    m_server->queue("NICK "+newNickname);
}

void StatusPanel::emitUpdateInfo()
{
    emit updateInfo(getServer()->getDisplayName());
}
                                                  // virtual
void StatusPanel::setChannelEncoding(const QString& encoding)
{
    if(m_server->getServerGroup())
        Preferences::setChannelEncoding(m_server->getServerGroup()->id(), ":server", encoding);
    else
        Preferences::setChannelEncoding(m_server->getDisplayName(), ":server", encoding);
}

QString StatusPanel::getChannelEncoding()         // virtual
{
    if(m_server->getServerGroup())
        return Preferences::channelEncoding(m_server->getServerGroup()->id(), ":server");
    return Preferences::channelEncoding(m_server->getDisplayName(), ":server");
}

                                                  // virtual
QString StatusPanel::getChannelEncodingDefaultDesc()
{
    return i18n("Identity Default ( %1 )", getServer()->getIdentity()->getCodecName());
}

//Used to disable functions when not connected
void StatusPanel::serverOnline(bool online)
{
    //m_inputBar->setEnabled(online);
    nicknameCombobox->setEnabled(online);
}

void StatusPanel::setServer(Server* server)
{
    ChatWindow::setServer(server);
    nicknameCombobox->setModel(m_server->nickListModel());
    connect(awayLabel, SIGNAL(unaway()), m_server, SLOT(requestUnaway()));
    connect(awayLabel, SIGNAL(awayMessageChanged(QString)), m_server, SLOT(requestAway(QString)));
}

#include "statuspanel.moc"
