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

	bool IsCachedValuesQuery(const RemoteObjectIdentifier Id);
	bool SendValueCacheToProtocol(const ProtocolId PId);
	
private:
	std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, RemoteObjectMessageData>>	m_currentValues;	/**< Hash of current value data to use to compare to incoming data regarding value changes. */
	float m_precision;																								/**< Value precision to use for processing. */
};
