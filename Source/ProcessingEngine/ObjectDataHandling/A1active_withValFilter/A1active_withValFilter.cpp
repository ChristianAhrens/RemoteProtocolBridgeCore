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

#include "A1active_withValFilter.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class A1active_withValFilter_withValFilter
// **************************************************************************************
/**
 * Constructor of class A1active_withValFilter_withValFilter.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
A1active_withValFilter::A1active_withValFilter(ProcessingEngineNode* parentNode)
	: Forward_only_valueChanges(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_A1active_withValFilter);
}

/**
 * Destructor
 */
A1active_withValFilter::~A1active_withValFilter()
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
bool A1active_withValFilter::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	auto isProtocolBId = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) == GetProtocolAIds().end();
	auto isFirstProtocolAId = GetProtocolAIds().size() >= 1 && PId == GetProtocolAIds().at(0);

	if (isProtocolBId || isFirstProtocolAId)
		return Forward_only_valueChanges::OnReceivedMessageFromProtocol(PId, Id, msgData);
	else
		return false;
}
