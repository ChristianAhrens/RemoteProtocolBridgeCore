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

#include "../ObjectDataHandling_Abstract.h"
#include "../../../RemoteProtocolBridgeCommon.h"
#include "../../ProcessingEngineConfig.h"

#include <JuceHeader.h>

// Fwd. declarations
class ProcessingEngineNode;

/**
 * Class Forward_only_valueChanges is a class for filtering received value data to only forward changed values.
 */
class Forward_only_valueChanges : public ObjectDataHandling_Abstract
{
public:
	Forward_only_valueChanges(ProcessingEngineNode* parentNode);
	~Forward_only_valueChanges();

	bool setStateXml(XmlElement* stateXml) override;

	bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) override;

protected:
	bool IsChangedDataValue(const RemoteObjectIdentifier Id, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData, bool setAsNewCurrentData = true);
	void SetCurrentDataValue(const RemoteObjectIdentifier Id, const RemoteObjectAddressing& roAddr, const RemoteObjectMessageData& msgData);

	float GetPrecision();
	void SetPrecision(float precision);
	
private:
	std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, RemoteObjectMessageData>>	m_currentValues;	/**< Hash of current value data to use to compare to incoming data regarding value changes. */
	float m_precision;																								/**< Value precision to use for processing. */
};
