/*
 * Copyright (C) 2005 by Martin Sevior
 * Copyright (C) 2006-2008 by Marc Maurer <uwog@uwog.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef __XMPPACCOUNTHANDLER__
#define __XMPPACCOUNTHANDLER__

#include <string>

#include <loudmouth/loudmouth.h>
#include <libxml/tree.h>

#include <account/xp/AccountHandler.h>
#include "stdio.h"
#include "ut_string_class.h"
#include "ut_types.h"
#include "XMPPBuddy.h"

#define XMPP_RESOURCE "AbiCollab"

extern AccountHandlerConstructor XMPPAccountHandlerConstructor;

class XMPPAccountHandler : public AccountHandler
{
public:
	XMPPAccountHandler();
	virtual ~XMPPAccountHandler(void);

	// housekeeping
	static UT_UTF8String	getStaticStorageType();
	virtual UT_UTF8String getStorageType() override
		{ return getStaticStorageType(); }
	virtual UT_UTF8String getDescription() override;
	virtual UT_UTF8String getDisplayType() override;

	// connection management
	virtual ConnectResult connect() override;
	virtual bool disconnect(void) override;
	virtual bool isOnline() override
		{ return m_bLoggedIn; }

	// asynchronous connection helper functions
	bool					authenticate();
	bool					setup();
	bool					tearDown();

	// dialog management
	virtual void embedDialogWidgets(void* pEmbeddingParent) override = 0;
	virtual void removeDialogWidgets(void* pEmbeddingParent) override = 0;
	virtual void storeProperties() override = 0;

	// user management
	virtual BuddyPtr constructBuddy(const PropertyMap& vProps) override;
	virtual BuddyPtr constructBuddy(const std::string& descriptor, BuddyPtr pBuddy) override;
	virtual bool recognizeBuddyIdentifier(const std::string& identifier) override;
	virtual bool allowsManualBuddies() override
		{ return true; }
	virtual void forceDisconnectBuddy(BuddyPtr) override { /* TODO: implement me? */ }
	virtual bool hasPersistentAccessControl() override
		{ return true; }

	// session management
	virtual bool allowsSessionTakeover() override
		{ return false; } // no technical reason not to allow this; we just didn't implement session takeover for this backend yet

		// packet management
	virtual bool send(const Packet* pPacket) override;
	virtual bool send(const Packet* pPacket, BuddyPtr pBuddy) override;

	virtual void 			handleMessage(const gchar* packet_data, const std::string& from_address);

private:
	UT_UTF8String			_getNameFromFqa(const UT_UTF8String& fqa);
	bool					_send(const char* base64data, XMPPBuddyPtr pBuddy);
	XMPPBuddyPtr			_getBuddy(const std::string& from_address);

	// connection management
	LmConnection *			m_pConnection;
	LmMessageHandler *		m_pPresenceHandler;
	LmMessageHandler *		m_pStreamErrorHandler;
	LmMessageHandler *		m_pChatHandler;
	bool					m_bLoggedIn;
};

#endif /* __XMPPACCOUNTHANDLER__ */
