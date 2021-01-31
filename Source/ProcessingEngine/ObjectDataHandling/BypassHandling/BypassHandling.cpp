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
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @return	True if successful sent/forwarded, false if not
 */
bool BypassHandling::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	bool sendSuccess = false;

	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (parentNode)
	{
		if (GetProtocolAIds().contains(PId))
		{
			sendSuccess = true;
			// the message was received by a typeA protocol -> forward it to all typeB protocols
			int typeBProtocolCount = GetProtocolBIds().size();
			for (int i = 0; i < typeBProtocolCount; ++i)
				sendSuccess = sendSuccess && parentNode->SendMessageTo(GetProtocolBIds()[i], Id, msgData);
		}
		else if (GetProtocolBIds().contains(PId))
		{
			sendSuccess = true;
			// the message was received by a typeB protocol
			int typeAProtocolCount = GetProtocolAIds().size();
			for (int i = 0; i < typeAProtocolCount; ++i)
				sendSuccess = sendSuccess && parentNode->SendMessageTo(GetProtocolAIds()[i], Id, msgData);
		}
	}

	return sendSuccess;
}
