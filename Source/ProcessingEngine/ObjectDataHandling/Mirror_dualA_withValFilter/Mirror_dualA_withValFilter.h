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
 * Class Mirror_dualA_withValFilter is a class for multiplexing n channels of protocols typeA
 * to m channels of protocols typeB combined with filtering to only forward changed object values.
 */
class Mirror_dualA_withValFilter : public Forward_only_valueChanges
{
public:
	Mirror_dualA_withValFilter(ProcessingEngineNode* parentNode);
	~Mirror_dualA_withValFilter();

	//==============================================================================
	void AddProtocolAId(ProtocolId PAId) override;

	//==============================================================================
	bool OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData) override;

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	void UpdateOnlineState(ProtocolId id) override;

private:
	void SetProtoFailoverTime(double timeout);
	double GetProtoFailoverTime();
	
	bool MirrorDataIfRequired(ProtocolId PId, RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData);

	double		m_protoFailoverTime;	/**< . */
	ProtocolId	m_currentMaster;		/**< Protocol Id of the protocol currently handled as master. */
	ProtocolId	m_currentSlave;			/**< Protocol Id of the protocol currently handled as slave. */

};
