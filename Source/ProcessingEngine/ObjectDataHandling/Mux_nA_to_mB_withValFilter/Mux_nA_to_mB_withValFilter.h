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

#pragma once

#include "../Forward_only_valueChanges/Forward_only_valueChanges.h"
#include "../../../RemoteProtocolBridgeCommon.h"
#include "../../ProcessingEngineConfig.h"

#include <JuceHeader.h>

// Fwd. declarations
class ProcessingEngineNode;

/**
 * Class Mux_nA_to_mB_withValFilter is a class for multiplexing n channels of protocols typeA
 * to m channels of protocols typeB combined with filtering to only forward changed object values.
 */
class Mux_nA_to_mB_withValFilter : public Forward_only_valueChanges
{
public:
	Mux_nA_to_mB_withValFilter(ProcessingEngineNode* parentNode);
	~Mux_nA_to_mB_withValFilter();

	bool setStateXml(XmlElement* stateXml) override;

	bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) override;

protected:
	int GetProtoChCntA();
	int GetProtoChCntB();

private:
	std::pair<std::vector<ProtocolId>, SourceId> GetTargetProtocolsAndSource(ProtocolId PId, const RemoteObjectMessageData &msgData);
	RemoteObjectAddressing GetMappedOriginAddressing(ProtocolId PId, const RemoteObjectMessageData& msgData);

	int m_protoChCntA; /**< Channel count configuration value that is to be expected per protocol type A. */
	int m_protoChCntB; /**< Channel count configuration value that is to be expected per protocol type B. */

};
