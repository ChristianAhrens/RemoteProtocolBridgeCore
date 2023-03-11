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

#include "Remap_A_X_Y_to_B_XY_Handling.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class Remap_A_X_Y_to_B_XY_Handling
// **************************************************************************************
/**
 * Constructor of class Remap_A_X_Y_to_B_XY_Handling.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
Remap_A_X_Y_to_B_XY_Handling::Remap_A_X_Y_to_B_XY_Handling(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_Remap_A_X_Y_to_B_XY);
}

/**
 * Destructor
 */
Remap_A_X_Y_to_B_XY_Handling::~Remap_A_X_Y_to_B_XY_Handling()
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
bool Remap_A_X_Y_to_B_XY_Handling::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	UpdateOnlineState(PId);

	if (std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId) != GetProtocolAIds().end())
	{
		// the message was received by a typeA protocol

		RemoteObjectIdentifier ObjIdToSend = Id;

		if (Id == ROI_CoordinateMapping_SourcePosition_X)
		{
			// special handling of merging separate x message to a combined xy one
			jassert(msgData._valType == ROVT_FLOAT);
			jassert(msgData._valCount == 1);
			jassert(msgData._payloadSize == sizeof(float));

			uint32 addrId = msgData._addrVal._first + (msgData._addrVal._second << 16);

			xyzVals newVals = m_currentPosValue[addrId];
			newVals.x = ((float*)msgData._payload)[0];
			m_currentPosValue.set(addrId, newVals);

			float newXYVal[2];
			newXYVal[0] = m_currentPosValue[addrId].x;
			newXYVal[1] = m_currentPosValue[addrId].y;

			msgData._valCount = 2;
			msgData._payload = &newXYVal;
			msgData._payloadSize = 2 * sizeof(float);

			ObjIdToSend = ROI_CoordinateMapping_SourcePosition_XY;
		}
		else if (Id == ROI_CoordinateMapping_SourcePosition_Y)
		{
			// special handling of merging separate y message to a combined xy one
			jassert(msgData._valType == ROVT_FLOAT);
			jassert(msgData._valCount == 1);
			jassert(msgData._payloadSize == sizeof(float));

			int32 addrId = msgData._addrVal._first + (msgData._addrVal._second << 16);

			xyzVals newVals = m_currentPosValue[addrId];
			newVals.y = ((float*)msgData._payload)[0];
			m_currentPosValue.set(addrId, newVals);

			float newXYVal[2];
			newXYVal[0] = m_currentPosValue[addrId].x;
			newXYVal[1] = m_currentPosValue[addrId].y;

			msgData._valCount = 2;
			msgData._payload = &newXYVal;
			msgData._payloadSize = 2 * sizeof(float);

			ObjIdToSend = ROI_CoordinateMapping_SourcePosition_XY;
		}

		// Send to all typeB protocols
		auto sendSuccess = true;
		for (auto const& protocolB : GetProtocolBIds())
			sendSuccess = parentNode->SendMessageTo(protocolB, ObjIdToSend, msgData) && sendSuccess;

		return sendSuccess;
			
	}
	if (std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId) != GetProtocolBIds().end())
	{
		if (Id == ROI_CoordinateMapping_SourcePosition_XY)
		{
			// special handling of splitting a combined xy message to  separate x, y ones
			jassert(msgData._valType == ROVT_FLOAT);
			jassert(msgData._valCount == 2);
			jassert(msgData._payloadSize == 2 * sizeof(float));

			int32 addrId = msgData._addrVal._first + (msgData._addrVal._second << 16);

			xyzVals newVals = m_currentPosValue[addrId];
			newVals.x = ((float*)msgData._payload)[0];
			newVals.y = ((float*)msgData._payload)[1];
			m_currentPosValue.set(addrId, newVals);

			float newXVal = m_currentPosValue[addrId].x;
			float newYVal = m_currentPosValue[addrId].y;

			msgData._valCount = 1;
			msgData._payloadSize = sizeof(float);

			// Send to all typeA protocols
			auto sendSuccess = true;
			for (auto const& protocolA : GetProtocolAIds())
			{
                msgData._payload = &newXVal;
				sendSuccess = parentNode->SendMessageTo(protocolA, ROI_CoordinateMapping_SourcePosition_X, msgData) && sendSuccess;

                msgData._payload = &newYVal;
				sendSuccess = parentNode->SendMessageTo(protocolA, ROI_CoordinateMapping_SourcePosition_Y, msgData) && sendSuccess;
			}

			return sendSuccess;
		}
		else
		{
			// Send to all typeA protocols
			auto sendSuccess = true;
			for (auto const& protocolA : GetProtocolAIds())
				sendSuccess = parentNode->SendMessageTo(protocolA, Id, msgData) && sendSuccess;

			return sendSuccess;
		}
	}

	return false;
}
