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

	bool OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData) override;

protected:
	int GetProtoChCntA();
	int GetProtoChCntB();

private:
	std::pair<std::vector<ProtocolId>, ChannelId> GetTargetProtocolsAndSource(ProtocolId PId, const RemoteObjectMessageData &msgData);
	RemoteObjectAddressing GetMappedOriginAddressing(ProtocolId PId, const RemoteObjectMessageData& msgData);

	int m_protoChCntA; /**< Channel count configuration value that is to be expected per protocol type A. */
	int m_protoChCntB; /**< Channel count configuration value that is to be expected per protocol type B. */

};
