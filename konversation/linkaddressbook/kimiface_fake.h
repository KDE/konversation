/*
    kimiface.h - KDE Instant Messenger DCOP Interface
	
	Copyright (c) 2004    Will Stephenson	 <lists@stevello.free-online.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KIMIFACE_H
#define KIMIFACE_H

#include <qpixmap.h>
#include <dcopobject.h>
#include <qstringlist.h>
#include <kurl.h>

/**
 * Generic DCOP interface for KDE instant messenger applications
 * Note one omission of this interface is the lack of control over the range of values used for protocols' names.
 * @since 3.3
 * @author Will Stephenson <lists@stevello.free-online.co.uk>
 */
class KIMIface : virtual public DCOPObject
{
	K_DCOP

k_dcop:
// ACCESSORS
// contact list
	/**
	 * Obtain a list of IM-contactable entries in the KDE 
	 * address book.
	 * @return a list of KABC uids.
	 */
	virtual QStringList allContacts() = 0;
	/**
	 * Obtain a list of  KDE address book entries who are 
	 * currently reachable.
	 * @return a list of KABC uids who can receive a message, even if online.
	 */
	virtual QStringList reachableContacts() = 0;
	/**
	 * Obtain a list of  KDE address book entries who are 
	 * currently online.
	 * @return a list of KABC uids who are online with unspecified presence.
	 */
	virtual QStringList onlineContacts() = 0;
	/**
	 * Obtain a list of  KDE address book entries who may
	 * receive file transfers.
	 * @return a list of KABC uids capable of file transfer.
	 */
	virtual QStringList fileTransferContacts() = 0;
	
// individual
	/** 
	 * Confirm if a given KABC uid is known to KIMProxy
	 * @param uid the KABC uid you are interested in.
	 * @return whether one of the chat programs KIMProxy talks to knows of this KABC uid.
	 */
	virtual bool isPresent( const QString & uid ) = 0;
	/**
	 * Obtain the IM app's idea of the contact's display name
	 * Useful if KABC lookups may be too slow
	 * @param KABC uid.
	 * @return The corresponding display name.
	 */
	virtual QString displayName( const QString & uid ) = 0;
	/**
	 * Obtain the IM presence as a i18ned string for the specified addressee
	 * @param uid the KABC uid you want the presence for.
	 * @return the i18ned string describing presence.
	 */
	virtual QString presenceString( const QString & uid ) = 0;
	/**
	 * Obtain the IM presence as a number (see KIMIface) for the specified addressee
	 * @param uid the KABC uid you want the presence for.
	 * @return a numeric representation of presence - currently one of 0 (Unknown), 1 (Offline), 2 (Connecting), 3 (Away), 4 (Online)
	 */
	virtual int presenceStatus( const QString & uid ) = 0;
	/**
	 * Indicate if a given uid can receive files 
	 * @param uid the KABC uid you are interested in.
	 * @return Whether the specified addressee can receive files.
	 */
	virtual bool canReceiveFiles( const QString & uid ) = 0;
	/** 
	 * Some media are unidirectional (eg, sending SMS via a web interface).
	 * @param uid the KABC uid you are interested in.
	 * @return Whether the specified addressee can respond.
	 */
	virtual bool canRespond( const QString & uid ) = 0;
	/**
	 * Get the KABC uid corresponding to the supplied IM address
	 * Protocols should be 
	 * @param contactId the protocol specific identifier for the contact, eg UIN for ICQ, screenname for AIM, nick for IRC.
	 * @param protocol the protocol, eg one of "AIMProtocol", "MSNProtocol", "ICQProtocol", 
	 * @return a KABC uid or null if none found/
	 */
	virtual QString locate( const QString & contactId, const QString & protocol ) = 0;
// metadata
	/**
	 * Obtain the icon representing IM presence for the specified addressee
	 * @param uid the KABC uid you want the presence for.
	 * @return a pixmap representing the uid's presence.
	 */
	virtual QPixmap icon( const QString & uid ) = 0;
	/**
	 * Get the supplied addressee's current context (home, work, or any).  
	 * @param uid the KABC uid you want the context for.
	 * @return A QString describing the context, or null if not supported.
	 */
	virtual QString context( const QString & uid ) = 0;
// App capabilities
	/**
	 * Discover what protocols the application supports
	 * @return the set of protocols that the application supports
	 */
	virtual QStringList protocols() = 0;
	
// ACTORS
	/**
	 * Send a single message to the specified addressee
	 * Any response will be handled by the IM client as a normal 
	 * conversation.
	 * @param uid the KABC uid you want to chat with.
	 * @param message the message to send them.
	 */
	virtual void messageContact( const QString &uid, const QString& message ) = 0;

	/**
	 * Open a chat to a contact, and optionally set some initial text
	 */
	virtual void messageNewContact( const QString &contactId, const QString &protocol ) = 0;

	/**
	 * Start a chat session with the specified addressee
	 * @param uid the KABC uid you want to chat with.
	 */
	virtual void chatWithContact( const QString &uid ) = 0;

	/**
	 * Send the file to the contact
	 * @param uid the KABC uid you are sending to.
	 * @param sourceURL a @ref KURL to send.
	 * @param altFileName an alternate filename describing the file
	 * @param fileSize file size in bytes
	 */
	virtual void sendFile(const QString &uid, const KURL &sourceURL,
		const QString &altFileName = QString::null, uint fileSize = 0) = 0;

// MUTATORS
// Contact list
	/**
	 * Add a contact to the contact list
	 * @param contactId the protocol specific identifier for the contact, eg UIN for ICQ, screenname for AIM, nick for IRC.
	 * @param protocol the protocol, eg one of "AIMProtocol", "MSNProtocol", "ICQProtocol", ...
	 * @return whether the add succeeded.  False may signal already present, protocol not supported, or add operation not supported.
	 */
	virtual bool addContact( const QString &contactId, const QString &protocol ) = 0;
// SIGNALS
k_dcop_signals:
	/**
	 * Indicates that a contact's presence has changed
	 * @param uid the contact whose presence changed.
	 * @param appId the dcop application id of the program the signal originates from.
	 * @param presence the new numeric presence @ref presenceStatus
	 */
	void contactPresenceChanged( QString uid, QCString appId, int presence );
};

#endif



/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

