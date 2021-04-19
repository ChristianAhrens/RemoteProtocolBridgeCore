/*
===============================================================================

Copyright (C) 2019 d&b audiotechnik GmbH & Co. KG. All Rights Reserved.

This file is part of RemoteProtocolBridge.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY d&b audiotechnik GmbH & Co. KG "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===============================================================================
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
			sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolB, ObjIdToSend, msgData);

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
				sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, ROI_CoordinateMapping_SourcePosition_X, msgData);

				msgData._payload = &newYVal;
				sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, ROI_CoordinateMapping_SourcePosition_Y, msgData);
			}

			return sendSuccess;
		}
		else
		{
			// Send to all typeA protocols
			auto sendSuccess = true;
			for (auto const& protocolA : GetProtocolAIds())
				sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, Id, msgData);

			return sendSuccess;
		}
	}

	return false;
}
