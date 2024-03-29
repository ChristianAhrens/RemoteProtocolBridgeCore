/* Copyright (c) 2020-2023, Christian Ahrens
 *
 * This file is part of RemoteProtocolBridgeCore <https://github.com/ChristianAhrens/RemoteProtocolBridgeCore>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "BypassHandling.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class BypassHandling
// **************************************************************************************
/**
 * Constructor of class BypassHandling.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
BypassHandling::BypassHandling(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Bypass);
}

/**
 * Destructor
 */
BypassHandling::~BypassHandling()
{
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param roi		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @param msgMeta	The meta information on the message data that was received
 * @return	True if successful sent/forwarded, false if not
 */
bool BypassHandling::OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta)
{
	ignoreUnused(msgMeta);

	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	bool sendSuccess = false;

	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end())
	{
		// Send to all typeB protocols
		sendSuccess = true;
		for (auto const& protocolB : GetProtocolBIds())
			sendSuccess = parentNode->SendMessageTo(protocolB, roi, msgData, ASYNC_EXTID) && sendSuccess;
	}
	else if (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end())
	{
		// Send to all typeA protocols
		sendSuccess = true;
		for (auto const& protocolA : GetProtocolAIds())
			sendSuccess = parentNode->SendMessageTo(protocolA, roi, msgData, ASYNC_EXTID) && sendSuccess;
	}

	return sendSuccess;
}
