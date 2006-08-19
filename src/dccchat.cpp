/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
    The class that controls dcc chats
    begin:     Sun Nov 16 2003
    copyright: (C) 2002 by Dario Abatianni
    email:     eisfuchs@tigress.com
*/

#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <qvbox.h>
#include <qhostaddress.h>
#include <qtextcodec.h>
#include <qsplitter.h>

#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kserversocket.h>
#include <ksocketaddress.h>
#include <kstreamsocket.h>

#include "konversationapplication.h"
#include "irccharsets.h"
#include "ircview.h"
#include "ircviewbox.h"
#include "ircinput.h"
#include "dccchat.h"
#include "topiclabel.h"

#define DCCCHAT_BUFFER_SIZE 1024

DccChat::DccChat(QWidget* parent,Server* newServer,const QString& myNickname,const QString& nickname,const QStringList& parameters,bool listen)
: ChatWindow(parent)
{
    kdDebug() << "DccChat::DccChat() [BEGIN]" << endl;
    m_dccSocket=0;
    m_listenSocket=0;
    m_port=0;
    m_initialShow = true;

    m_myNick=myNickname;
    m_partnerNick=nickname;

    setType(ChatWindow::DccChat);
    setChannelEncodingSupported(true);

    if ( !listen )
    {
        host=parameters[1];
        m_port=parameters[2].toInt();
    }

    m_headerSplitter = new QSplitter(Qt::Vertical, this);

    m_sourceLine = new Konversation::TopicLabel( m_headerSplitter );

    IRCViewBox* ircViewBox = new IRCViewBox( m_headerSplitter, 0 );
    setTextView( ircViewBox->ircView() );

    m_dccChatInput = new IRCInput( this );

    m_dccChatInput->setEnabled( false );

    setServer( newServer );
    ChatWindow::setName( '-' + m_partnerNick + '-' );
    ChatWindow::setLogfileName( '-' + m_partnerNick + '-' );

    // connect the signals and slots
    connect( m_dccChatInput, SIGNAL( submit() ), this, SLOT( dccChatTextEntered() ) );
    connect( m_dccChatInput, SIGNAL( textPasted( const QString& ) ), this, SLOT( textPasted( const QString& ) ) );

    connect( getTextView(), SIGNAL( textPasted(bool) ), m_dccChatInput, SLOT( paste(bool) ) );
    connect( getTextView(), SIGNAL( gotFocus() ), m_dccChatInput, SLOT( setFocus() ) );
    connect( getTextView(), SIGNAL( updateTabNotification(Konversation::TabNotifyType)),
        this, SLOT( activateTabNotification( Konversation::TabNotifyType ) ) );
    connect( getTextView(), SIGNAL( autoText(const QString&) ), this, SLOT( sendDccChatText( const QString& ) ) );

    if ( listen )
        listenForPartner();
    else
        connectToPartner();

    kdDebug() << "DccChat::DccChat() [END]" << endl;

    updateAppearance();
}

DccChat::~DccChat()
{
    kdDebug() << "DccChat::~DccChat()" << endl;
    if(m_dccSocket)
        m_dccSocket->close();
    if(m_listenSocket)
        m_listenSocket->close();
}

void DccChat::listenForPartner()
{
    kdDebug() << "DccChat::listenForPartner() [BEGIN]" << endl;

    // Set up server socket
    m_listenSocket = new KNetwork::KServerSocket( this );
    m_listenSocket->setFamily(KNetwork::KResolver::InetFamily);

                                                  // user specifies ports
    if(Preferences::dccSpecificChatPorts())
    {
        // set port
        bool found = false;                       // wether succeeded to set port
        unsigned long port = Preferences::dccChatPortsFirst();
        for( ; port <= Preferences::dccChatPortsLast() ; ++port )
        {
            kdDebug() << "DccChat::listenForPartner(): trying port " << port << endl;
            m_listenSocket->setAddress(QString::number(port));
            bool success = m_listenSocket->listen();
            if( found = ( success && m_listenSocket->error() == KNetwork::KSocketBase::NoError ) )
                break;
            m_listenSocket->close();
        }
        if(!found)
        {
            KMessageBox::sorry(this, i18n("There is no vacant port for DCC Chat."));
            return;
        }
    }
    else                                          // user doesn't specify ports
    {
        // Let the operating system choose a port
        m_listenSocket->setAddress("0");
        if(!m_listenSocket->listen())
        {
            kdDebug() << this << "DccChat::listenForPartner(): listen() failed!" << endl;
            return;
        }
    }

    connect( m_listenSocket, SIGNAL(readyAccept()), this, SLOT(heardPartner()) );

    // Get our own port number
    const KNetwork::KSocketAddress ipAddr = m_listenSocket->localAddress();
    const struct sockaddr_in* socketAddress = (sockaddr_in*)ipAddr.address();
    m_port = ntohs( socketAddress->sin_port );
    kdDebug() << "DccChat::listenForPartner(): using port " << m_port << endl;

    getTextView()->appendServerMessage( i18n("DCC"), i18n("Offering DCC Chat connection to %1 on port %2...").arg( m_partnerNick ).arg( m_port ) );
    m_sourceLine->setText(i18n( "DCC chat with %1 on port %2." ).arg( m_partnerNick ).arg( m_port ) );
    kdDebug() << "DccChat::listenForPartner() [END]" << endl;
}

void DccChat::connectToPartner()
{
    QHostAddress ip;

    ip.setAddress(host.toUInt());
    host=ip.toString();

    getTextView()->appendServerMessage( i18n( "DCC" ), i18n( "Establishing DCC Chat connection to %1 (%2:%3)..." ).arg( m_partnerNick ).arg( host ).arg( m_port ) );

    m_sourceLine->setText( i18n( "DCC chat with %1 on %2:%3." ).arg( m_partnerNick ).arg( host ).arg( m_port ) );

    m_dccSocket = new KNetwork::KStreamSocket( host, QString::number( m_port ), this );

    m_dccSocket->setBlocking(false);
    m_dccSocket->setFamily(KNetwork::KResolver::InetFamily);
    m_dccSocket->enableRead(false);
    m_dccSocket->enableWrite(false);
    m_dccSocket->setTimeout(10000);
    m_dccSocket->blockSignals(false);

    connect( m_dccSocket, SIGNAL( hostFound() ),                        this, SLOT( lookupFinished() )           );
    connect( m_dccSocket, SIGNAL( connected( const KResolverEntry& ) ), this, SLOT( dccChatConnectionSuccess() ) );
    connect( m_dccSocket, SIGNAL( gotError( int ) ),                    this, SLOT( dccChatBroken( int ) )       );
    connect( m_dccSocket, SIGNAL( readyRead() ),                        this, SLOT( readData() )                 );
    connect( m_dccSocket, SIGNAL( closed() ),                           this, SLOT( socketClosed() )             );

    m_dccSocket->connect();
    
#if 0
    //getTextView()->appendServerMessage(i18n("DCC"),i18n("Looking for host %1...").arg(host));
#endif

}

void DccChat::lookupFinished()
{
	
#if 0
	//getTextView()->appendServerMessage(i18n("DCC"),i18n("Host found, connecting..."));
#endif

}

void DccChat::dccChatConnectionSuccess()
{
    getTextView()->appendServerMessage( i18n( "DCC" ), i18n( "Established DCC Chat connection to %1." ).arg( m_partnerNick ) );
    m_dccSocket->enableRead(true);
    m_dccChatInput->setEnabled(true);
}

void DccChat::dccChatBroken(int error)
{
    getTextView()->appendServerMessage(i18n("Error"),i18n("Connection broken, error code %1.").arg(error));
    m_dccSocket->enableRead(false);
    m_dccSocket->blockSignals(true);
    m_dccSocket->close();
}

void DccChat::readData()
{
    kdDebug() << k_funcinfo << ( m_listenSocket == 0 ) << " BEGIN" << endl;
    int available=0;
    int actual=0;
    char* buffer=0;
    QString line;
    QTextCodec* codec = Konversation::IRCCharsets::self()->codecForName(m_encoding.isEmpty() ? Konversation::IRCCharsets::self()->encodingForLocale() : m_encoding);

    available = m_dccSocket->bytesAvailable();
    if( available > 0 )
    {
        buffer = new char[ available + 1 ];
        actual = m_dccSocket->readBlock( buffer, available );
        buffer[ actual ] = 0;
        line.append( codec->toUnicode( buffer ) );
        delete[] buffer;

        QStringList lines = QStringList::split( '\n', line );

        for( QStringList::iterator itLine = lines.begin() ; itLine != lines.end() ; itLine++ )
        {
            if( (*itLine).startsWith( "\x01" ) )
            {
                // cut out the CTCP command
                QString ctcp = (*itLine).mid( 1, (*itLine).find( 1, 1 ) - 1 );

                QString ctcpCommand = ctcp.section( " ", 0, 0 );
                QString ctcpArgument = ctcp.section( " ", 1 );

                if( ctcpCommand.lower() == "action" )
                    getTextView()->appendAction( m_partnerNick, ctcpArgument );
                else
                    getTextView()->appendServerMessage( i18n( "CTCP" ), i18n( "Received unknown CTCP-%1 request from %2" ).arg( ctcp ).arg( m_partnerNick ) );
            }
            else getTextView()->append( m_partnerNick, *itLine );
        }                                         // endfor
    } else {
        dccChatBroken(m_dccSocket->error());
    }

    kdDebug() << k_funcinfo << " END" << endl;
}

void DccChat::dccChatTextEntered()
{
    QString line = m_dccChatInput->text();
    m_dccChatInput->clear();
    if ( line.lower() == "/clear" )
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
    kdDebug() << k_funcinfo << " BEGIN" << endl;
    // create a work copy
    QString output(sendLine);
    QString cc=Preferences::commandChar();

    if(!output.isEmpty())
    {
        QStringList lines = QStringList::split('\n',output);
        // wrap socket into a stream
        QTextStream stream(m_dccSocket);
        // init stream props
        stream.setCodec(Konversation::IRCCharsets::self()->codecForName(m_encoding.isEmpty() ? Konversation::IRCCharsets::self()->encodingForLocale() : m_encoding));

        for( QStringList::iterator itLine = lines.begin() ; itLine != lines.end() ; itLine++ )
        {
            QString line( *itLine );

            // replace aliases and wildcards
            //  if(filter.replaceAliases(line)) line=server->parseWildcards(line,nick,getName(),QString::null,QString::null,QString::null);

            //  line=filter.parse(nick,line,getName());

            // convert /me actions
            if(line.lower().startsWith(cc+"me "))
            {
                getTextView()->appendAction( m_myNick, line.section( " ", 1 ) );
                line=QString("\x01%1 %2\x01").arg("ACTION").arg(line.section(" ",1));
            }
            else getTextView()->append( m_myNick, line );

            stream << line << endl;
        }                                         // endfor

        // detach stream
        stream.unsetDevice();
    }
    kdDebug() << k_funcinfo << " END" << endl;
}

void DccChat::heardPartner()
{
    m_dccSocket = static_cast<KNetwork::KStreamSocket*>( m_listenSocket->accept() );

    if( !m_dccSocket )
    {
        getTextView()->appendServerMessage( i18n( "Error" ),i18n( "Could not accept the client." ) );
        return;
    }

    connect( m_dccSocket, SIGNAL( readyRead() ),     this, SLOT( readData() )           );
    connect( m_dccSocket, SIGNAL( closed() ),        this, SLOT( socketClosed() )       );
    connect( m_dccSocket, SIGNAL( gotError( int ) ), this, SLOT( dccChatBroken( int ) ) );

    // the listen socket isn't needed anymore
    disconnect( m_listenSocket, 0, 0, 0 );
    m_listenSocket->close();
    m_listenSocket = 0;

    m_dccSocket->enableRead(true);
    m_dccChatInput->setEnabled(true);

    getTextView()->appendServerMessage( i18n( "DCC" ), i18n( "Established DCC Chat connection to %1." ).arg( m_partnerNick ) );
}

void DccChat::socketClosed()
{
    getTextView()->appendServerMessage(i18n("DCC"),"Connection closed.");
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

int DccChat::getPort()
{
    return m_port;
}

QString DccChat::getTextInLine() 
{
    return m_dccChatInput->text();
}

void DccChat::appendInputText( const QString& s )
{
    m_dccChatInput->setText( m_dccChatInput->text() + s );
}

bool DccChat::closeYourself()
{
    delete this;
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
    return i18n("Default ( %1 )").arg(Konversation::IRCCharsets::self()->encodingForLocale());
}

void DccChat::showEvent(QShowEvent* /* event */)
{
    if(m_initialShow) {
        m_initialShow = false;
        QValueList<int> sizes;
        sizes << m_sourceLine->sizeHint().height() << (height() - m_sourceLine->sizeHint().height());
        m_headerSplitter->setSizes(sizes);
    }
}

void DccChat::updateAppearance()
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

    m_dccChatInput->unsetPalette();
    m_dccChatInput->setPaletteForegroundColor(fg);
    m_dccChatInput->setPaletteBackgroundColor(bg);
    m_dccChatInput->setFont(Preferences::textFont());

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

    ChatWindow::updateAppearance();
}

#include "dccchat.moc"
