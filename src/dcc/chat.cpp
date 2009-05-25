/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  Copyright (C) 2002 Dario Abatianni <eisfuchs@tigress.com>
  Copyright (C) 2006 Eike Hein <hein@kde.org>
  Copyright (C) 2004,2007 Shintaro Matsuoka <shin@shoegazed.org>
*/

#include "chat.h"
#include "dcccommon.h"
#include "application.h" ////// header renamed
#include "mainwindow.h" ////// header renamed
#include "irccharsets.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "ircinput.h"
#include "topiclabel.h"
#include "server.h"
#include "channel.h"

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>


#include <qhostaddress.h>
#include <qtextcodec.h>
#include <qsplitter.h>
#include <QTextStream>
#include <QShowEvent>
#include <QTcpSocket>
#include <QTcpServer>

#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <KActionCollection>
#include <kaction.h>
#include <kmenu.h>
#include <kvbox.h>
#define DCCCHAT_BUFFER_SIZE 1024


DccChat::DccChat(QWidget* parent, bool listen, Server* server, const QString& ownNick, const QString& partnerNick, const QString& partnerHost, int partnerPort)
    : ChatWindow(parent)
{
    kDebug() << "[BEGIN]" ;

    m_dccSocket = 0;
    m_listenSocket = 0;

    m_ownNick = ownNick;

    m_partnerNick = partnerNick;
    m_partnerHost = partnerHost;
    m_partnerPort = partnerPort;

    setType(ChatWindow::DccChat);
    setChannelEncodingSupported(true);

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    m_sourceLine = 0;
    m_sourceLine = new Konversation::TopicLabel(m_headerSplitter);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(m_sourceLine), 0);

    IRCViewBox* ircViewBox = new IRCViewBox(m_headerSplitter, NULL);
    m_headerSplitter->setStretchFactor(m_headerSplitter->indexOf(ircViewBox), 1);
    setTextView(ircViewBox->ircView());

    m_dccChatInput = new IRCInput(this);
    getTextView()->installEventFilter(m_dccChatInput);
    m_dccChatInput->setEnabled( false );

    ChatWindow::setName( '-' + m_partnerNick + '-' );
    ChatWindow::setLogfileName( '-' + m_partnerNick + '-' );

    KMenu* popup = textView->getPopup();

    if (popup)
    {
        QAction* action = KonversationApplication::instance()->getMainWindow()->actionCollection()->action("open_logfile");

        if (action)
        {
            popup->addSeparator();
            action->setMenu(popup);
        }
    }

    // connect the signals and slots
    connect( m_dccChatInput, SIGNAL( submit() ), this, SLOT( dccChatTextEntered() ) );
    connect( m_dccChatInput, SIGNAL( textPasted( const QString& ) ), this, SLOT( textPasted( const QString& ) ) );

    connect( getTextView(), SIGNAL( textPasted(bool) ), m_dccChatInput, SLOT( paste(bool) ) );
    connect( getTextView(), SIGNAL( gotFocus() ), m_dccChatInput, SLOT( setFocus() ) );
    connect( getTextView(), SIGNAL( autoText(const QString&) ), this, SLOT( sendDccChatText( const QString& ) ) );

    if (listen)
    {
        listenForPartner();
        QString ownNumericalIp = DccCommon::textIpToNumericalIp( DccCommon::getOwnIp( server ) );
        server->requestDccChat( m_partnerNick, ownNumericalIp, m_ownPort );
    }
    else
    {
        connectToPartner();
    }

    kDebug() << "[END]" ;

    updateAppearance();
}

DccChat::~DccChat()
{
    kDebug();
    if(m_dccSocket)
        m_dccSocket->close();
    if(m_listenSocket)
        m_listenSocket->close();
}

void DccChat::listenForPartner()
{
    kDebug() << "[BEGIN]" ;

    // Set up server socket
    QString failedReason;
    if ( Preferences::self()->dccSpecificChatPorts() )
        m_listenSocket = DccCommon::createServerSocketAndListen( this, &failedReason, Preferences::self()->dccChatPortsFirst(), Preferences::self()->dccChatPortsLast() );
    else
        m_listenSocket = DccCommon::createServerSocketAndListen( this, &failedReason );
    if ( !m_listenSocket )
    {
        getTextView()->appendServerMessage(i18n("DCC"), i18n("Could not open a socket for listening: %1", failedReason));
        return;
    }

    connect( m_listenSocket, SIGNAL(newConnection()), this, SLOT(heardPartner()) );

    // Get our own port number
    m_ownPort = m_listenSocket->serverPort();
    kDebug() << "using port: " << m_ownPort ;

    getTextView()->appendServerMessage(i18n("DCC"), i18n("Offering DCC Chat connection to %1 on port %2...", m_partnerNick, QString::number(m_ownPort)));

    m_sourceLine->setText(i18n("DCC chat with %1 on port %2.", m_partnerNick, QString::number(m_ownPort)));
    kDebug() << "[END]"; 
}

void DccChat::connectToPartner()
{
    QHostAddress ip;

    ip.setAddress(m_partnerHost.toUInt());
    m_partnerHost=ip.toString();

    getTextView()->appendServerMessage( i18n( "DCC" ), i18nc("%1 = nickname, %2 = IP, %3 = port",
        "Establishing DCC Chat connection to %1 (%2:%3)...", m_partnerNick, m_partnerHost, QString::number(m_partnerPort)));

    m_sourceLine->setText(i18nc("%1 = nickname, %2 = IP, %3 = port", "DCC chat with %1 on %2:%3.", m_partnerNick, host, QString::number(m_partnerPort)));

    m_dccSocket = new QTcpSocket( this );

    //m_dccSocket->setTimeout(10000);

    connect( m_dccSocket, SIGNAL( hostFound() ), this, SLOT( lookupFinished() ) );
    connect( m_dccSocket, SIGNAL( connected() ), this, SLOT( dccChatConnectionSuccess() ) );
    connect( m_dccSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( dccChatBroken( QAbstractSocket::SocketError ) ) );
    connect( m_dccSocket, SIGNAL( readyRead() ), this, SLOT( readData() ) );
    connect( m_dccSocket, SIGNAL( disconnected() ), this, SLOT( socketClosed() ) );

    m_dccSocket->connectToHost( m_partnerHost, m_partnerPort );


    getTextView()->appendServerMessage(i18n("DCC"), i18n("Looking for host %1...", host));
}

void DccChat::lookupFinished()
{
    getTextView()->appendServerMessage(i18n("DCC"),i18n("Host found, connecting..."));
}

void DccChat::dccChatConnectionSuccess()
{
    getTextView()->appendServerMessage(i18n("DCC"), i18n("Established DCC Chat connection to %1.", m_partnerNick));
    m_dccChatInput->setEnabled(true);
}

void DccChat::dccChatBroken(QAbstractSocket::SocketError error)
{
    getTextView()->appendServerMessage(i18n("Error"), i18n("Connection broken, error (%1) %2.", error, m_dccSocket->errorString()));
    m_dccSocket->blockSignals(true);
    m_dccSocket->close();
}

void DccChat::readData()
{
    kDebug() << ( m_listenSocket == 0 ) << " BEGIN"; 
    int available=0;
    int actual=0;
    char* buffer=0;
    QString line;
    QTextCodec* codec = Konversation::IRCCharsets::self()->codecForName(m_encoding.isEmpty() ? Konversation::IRCCharsets::self()->encodingForLocale() : m_encoding);

    available = m_dccSocket->bytesAvailable();
    if( available > 0 )
    {
        buffer = new char[ available + 1 ];
        actual = m_dccSocket->read( buffer, available );
        buffer[ actual ] = 0;
        line.append( codec->toUnicode( buffer ) );
        delete[] buffer;

        QStringList lines = line.split('\n', QString::SkipEmptyParts);

        for( QStringList::iterator itLine = lines.begin() ; itLine != lines.end() ; ++itLine )
        {
            if( (*itLine).startsWith( QLatin1String("\x01") ) )
            {
                // cut out the CTCP command
                QString ctcp = (*itLine).mid( 1, (*itLine).indexOf( 1, 1 ) - 1 );

                QString ctcpCommand = ctcp.section( ' ', 0, 0 );
                QString ctcpArgument = ctcp.section( ' ', 1 );

                if( ctcpCommand.toLower() == "action" )
                    appendAction( m_partnerNick, ctcpArgument );
                else
                    getTextView()->appendServerMessage(i18n("CTCP"), i18n("Received unknown CTCP-%1 request from %2", ctcp, m_partnerNick));
            }
            else getTextView()->append( m_partnerNick, *itLine );
        }                                         // endfor
    } else {
        dccChatBroken(m_dccSocket->error());
    }

    kDebug() << "END" ;
}

void DccChat::dccChatTextEntered()
{
    QString line = m_dccChatInput->toPlainText();
    m_dccChatInput->setText("");
    if ( line.toLower() == Preferences::self()->commandChar()+"clear" )
    {
        textView->clear();
    }
    else if ( !line.isEmpty() )
    {
        sendDccChatText(line);
    }
}

void DccChat::sendDccChatText(const QString& sendLine)
{
    kDebug() << "BEGIN" ;
    // create a work copy
    QString output(sendLine);
    QString cc=Preferences::self()->commandChar();

    if(!output.isEmpty())
    {
        QStringList lines = output.split('\n', QString::SkipEmptyParts);
        // wrap socket into a stream
        QTextStream stream(m_dccSocket);
        // init stream props
        stream.setCodec(Konversation::IRCCharsets::self()->codecForName(m_encoding.isEmpty() ? Konversation::IRCCharsets::self()->encodingForLocale() : m_encoding));

        for( QStringList::iterator itLine = lines.begin() ; itLine != lines.end() ; ++itLine )
        {
            QString line( *itLine );

            // replace aliases and wildcards
            //  if(filter.replaceAliases(line)) line=server->parseWildcards(line,nick,getName(),QString::null,QString::null,QString::null);

            //  line=filter.parse(nick,line,getName());

            // convert /me actions
            QString cmd=line.section(' ', 0,0).toLower();
            if (cmd == cc+"me")
            {
                appendAction( m_ownNick, line.section( ' ', 1 ) );
                line=QString("\x01%1 %2\x01").arg("ACTION").arg(line.section(' ',1));
            }
            else if (cmd == cc+"close")
            {
                closeYourself(false);
                return;
            }
            else
                getTextView()->append( m_ownNick, line );

            stream << line << endl;
        }                                         // endfor

        // detach stream
        stream.setDevice(0);
    }
    kDebug() << "END" ;
}

void DccChat::heardPartner()
{
    m_dccSocket = m_listenSocket->nextPendingConnection();

    if( !m_dccSocket )
    {
        getTextView()->appendServerMessage( i18n( "Error" ),i18n( "Could not accept the client." ) );
        return;
    }

    connect( m_dccSocket, SIGNAL( readyRead() ),     this, SLOT( readData() )           );
    connect( m_dccSocket, SIGNAL( disconnected() ),        this, SLOT( socketClosed() )       );
    connect( m_dccSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( dccChatBroken( QAbstractSocket::SocketError ) ) );

    // the listen socket isn't needed anymore
    disconnect( m_listenSocket, 0, 0, 0 );
    m_listenSocket->close();
    m_listenSocket = 0;

    m_dccChatInput->setEnabled(true);

    getTextView()->appendServerMessage(i18n("DCC"), i18n("Established DCC Chat connection to %1.", m_partnerNick));
}

void DccChat::socketClosed()
{
    getTextView()->appendServerMessage(i18n("DCC"), i18n("Connection closed."));
    m_dccChatInput->setEnabled(false);
    m_dccSocket = 0;
}

void DccChat::textPasted(const QString& text)
{
    sendDccChatText(text);
}

void DccChat::childAdjustFocus()
{
    m_dccChatInput->setFocus();
}

bool DccChat::canBeFrontView()
{
    return true;
}

bool DccChat::searchView()
{
    return true;
}

int DccChat::getOwnPort()
{
    return m_ownPort;
}

QString DccChat::getTextInLine()
{
    return m_dccChatInput->toPlainText();
}

void DccChat::appendInputText( const QString& s, bool fromCursor )
{
    if(!fromCursor)
    {
        m_dccChatInput->append(s);
    }
    else
    {
        const int position = m_dccChatInput->textCursor().position();
        m_dccChatInput->textCursor().insertText(s);
        QTextCursor cursor = m_dccChatInput->textCursor();
        cursor.setPosition(position + s.length());
        m_dccChatInput->setTextCursor(cursor);
    }
}

//FIXME uh... where is the confimation for this?
bool DccChat::closeYourself(bool)
{
    deleteLater();
    return true;
}

void DccChat::setChannelEncoding(const QString& encoding) // virtual
{
    m_encoding = encoding;
}

QString DccChat::getChannelEncoding() // virtual
{
    return m_encoding;
}

QString DccChat::getChannelEncodingDefaultDesc()  // virtual
{
    return i18n("Default ( %1 )", Konversation::IRCCharsets::self()->encodingForLocale());
}

void DccChat::showEvent(QShowEvent* /* event */)
{
    if(m_initialShow) {
        m_initialShow = false;
        QList<int> sizes;
        sizes << m_sourceLine->sizeHint().height() << (height() - m_sourceLine->sizeHint().height());
        m_headerSplitter->setSizes(sizes);
    }
}

void DccChat::updateAppearance()
{
    QColor fg;
    QColor bg;

    if(Preferences::self()->inputFieldsBackgroundColor())
    {
        fg=Preferences::self()->color(Preferences::ChannelMessage);
        bg=Preferences::self()->color(Preferences::TextViewBackground);
    }
    else
    {
        fg = palette().windowText().color();
        bg = palette().base().color();
    }
    QPalette dccChatInputPalette(m_dccChatInput->palette());
    dccChatInputPalette.setColor(QPalette::WindowText, fg);
    dccChatInputPalette.setColor(QPalette::Text, fg);
    dccChatInputPalette.setColor(QPalette::Base, bg);
    m_dccChatInput->setPalette(dccChatInputPalette);

    getTextView()->setPalette(QPalette()); 

    if(Preferences::self()->showBackgroundImage())
    {
        getTextView()->setViewBackground(Preferences::self()->color(Preferences::TextViewBackground),
        Preferences::self()->backgroundImage());
    }
    else
    {
        getTextView()->setViewBackground(Preferences::self()->color(Preferences::TextViewBackground),
        QString());
    }

    if (Preferences::self()->customTextFont())
    {
        getTextView()->setFont(Preferences::self()->textFont());
        m_dccChatInput->setFont(Preferences::self()->textFont());
    }
    else
    {
        getTextView()->setFont(KGlobalSettings::generalFont());
        m_dccChatInput->setFont(KGlobalSettings::generalFont());
    }

    ChatWindow::updateAppearance();
}

#include "chat.moc"
