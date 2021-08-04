/* Copyright (c) 2020-2021, Christian Ahrens
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

#include "Reverse_B_to_A_only.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Reverse_B_to_A_only
// **************************************************************************************
/**
 * Constructor of class BypassHandling.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Reverse_B_to_A_only::Reverse_B_to_A_only(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Reverse_B_to_A_only);
}

/**
 * Destructor
 */
Reverse_B_to_A_only::~Reverse_B_to_A_only()
{
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @return	True if successful sent/forwarded, false if not
 */
bool Reverse_B_to_A_only::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	auto sendSuccess = false;

	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end())
	{
		sendSuccess = true;
		// the message was received by a typeA protocol, which we do not want to forward in this OHM
	}
	else if (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end())
	{
		// Send to all typeA protocols
		sendSuccess = true;
		for (auto const& protocolA : GetProtocolAIds())
			sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, Id, msgData);
	}

	return sendSuccess;
}
