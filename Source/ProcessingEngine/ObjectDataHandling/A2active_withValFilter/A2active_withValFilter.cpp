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

#include "A2active_withValFilter.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class A2active_withValFilter_withValFilter
// **************************************************************************************
/**
 * Constructor of class A2active_withValFilter_withValFilter.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
A2active_withValFilter::A2active_withValFilter(ProcessingEngineNode* parentNode)
	: Forward_only_valueChanges(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_A2active_withValFilter);
}

/**
 * Destructor
 */
A2active_withValFilter::~A2active_withValFilter()
{
}

/**
 * Overridden from ObjectDataHandling_Abstract to use incoming ids
 * to initialize internal master and slave protocol ids.
 *
 * @param PAId	Protocol Id of typeA protocol to add.
 */
void A2active_withValFilter::AddProtocolAId(ProtocolId PAId)
{
	ObjectDataHandling_Abstract::AddProtocolAId(PAId);

	if (GetProtocolAIds().size() == 1)
		SetChangedProtocolState(PAId, OHS_Protocol_Slave);
	else if (GetProtocolAIds().size() == 2)
		SetChangedProtocolState(PAId, OHS_Protocol_Master);
	else
		jassertfalse; // only two typeA protocols are supported by this OHM!
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @param msgMeta	The meta information on the message data that was received
 * @return	True if successful sent/forwarded, false if not
 */
bool A2active_withValFilter::OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	auto isProtocolBId = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) == GetProtocolAIds().end();
	auto isSecondProtocolAId = GetProtocolAIds().size() >= 2 && PId == GetProtocolAIds().at(1);

	if (isProtocolBId || isSecondProtocolAId)
		return Forward_only_valueChanges::OnReceivedMessageFromProtocol(PId, Id, msgData, msgMeta);
	else
		return false;
}
