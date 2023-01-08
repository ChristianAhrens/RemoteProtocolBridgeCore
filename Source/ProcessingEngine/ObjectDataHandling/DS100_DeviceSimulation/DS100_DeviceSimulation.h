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
#include "../../TimerThreadBase.h"

#include <JuceHeader.h>

// Fwd. declarations
class ProcessingEngineNode;

/**
 * Class DS100_DeviceSimulation is a class for filtering received value data to only forward changed values.
 */
class DS100_DeviceSimulation : public ObjectDataHandling_Abstract, public TimerThreadBase
{
public:
	class DS100_DeviceSimulation_Listener :
		private MessageListener
	{
	public:
		virtual void simulationUpdated(const std::map<RemoteObjectAddressing, std::map<RemoteObjectIdentifier, std::vector<float>>>& simulationValues) = 0;

	public:
		struct SimulationUpdateMessage : public Message
		{
			SimulationUpdateMessage()
			{
			}
			SimulationUpdateMessage(const std::map<RemoteObjectAddressing, std::map<RemoteObjectIdentifier, std::vector<float>>>& currentSimValues)
				: _simulationUpdateContent(currentSimValues)
			{
			}

			std::map<RemoteObjectAddressing, std::map<RemoteObjectIdentifier, std::vector<float>>>	_simulationUpdateContent;		/**< The payload of the message. */
		};

		void addSimulationUpdate(const std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, RemoteObjectMessageData>>& currentSimValues)
		{
			auto updateMessage = std::make_unique<SimulationUpdateMessage>();

			for (auto const& csv : currentSimValues)
			{
				for (auto const& sv : csv.second)
				{
					auto& simVals = updateMessage->_simulationUpdateContent[sv.first][csv.first];
					switch (sv.second._valType)
					{
					case ROVT_FLOAT:
						if (sv.second._payloadSize == sv.second._valCount * sizeof(float))
							for (int i = 0; i < sv.second._valCount; i++)
								simVals.push_back(static_cast<float*>(sv.second._payload)[i]);
						break;
					case ROVT_INT:
						if (sv.second._payloadSize == sv.second._valCount * sizeof(int))
							for (int i = 0; i < sv.second._valCount; i++)
								simVals.push_back(static_cast<float>(static_cast<int*>(sv.second._payload)[i]));
						break;
					case ROVT_STRING:
					case ROVT_NONE:
					default:
						break;
					}
				}
			}

			postMessage(updateMessage.release());
		}

	private:
		void handleMessage(const Message& msg) override
		{
			if (auto* updateMessage = dynamic_cast<const SimulationUpdateMessage*> (&msg))
			{
				simulationUpdated(updateMessage->_simulationUpdateContent);
			}
		}
	};

public:
	DS100_DeviceSimulation(ProcessingEngineNode* parentNode);
	~DS100_DeviceSimulation();

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) override;

	//==============================================================================
	void timerThreadCallback() override;

	//==============================================================================
	void addListener(DS100_DeviceSimulation_Listener* listener);
	void removeListener(DS100_DeviceSimulation_Listener* listener);
	void notifyListeners();

private:
	bool IsStaticValueRemoteObject(const RemoteObjectIdentifier Id);
	bool IsDataRequestPollMessage(const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData);
	bool ReplyToDataRequest(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectAddressing adressing);

	void InitDataValues();
	void SetDataValue(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData);
	void UpdateDataValues();

	void PrintDataInfo(const String& actionName, const std::pair<RemoteObjectIdentifier, RemoteObjectMessageData>& idDataKV);
	
	CriticalSection						m_currentValLock;			/**< For securing access to current values map and other member variables. */
	std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, RemoteObjectMessageData>>	m_currentValues;	/**< Map of current value data to use to compare to incoming data regarding value changes. */
	std::vector<RemoteObjectIdentifier>	m_simulatedRemoteObjects;	/**< The remote objects that are simulated. */
	int									m_simulatedChCount;			/**< Count of channels that are currently simulated. */
	int									m_simulatedMappingsCount;	/**< Count of mapping areas (mappings) that are currently simulated. */
	int									m_refreshInterval;			/**< Refresh interval to update internal simulation. */

	float								m_simulationBaseValue;		/**< Rolling value that is used as base for simulated object values. */

	std::vector<DS100_DeviceSimulation_Listener*>	m_simulationListeners;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DS100_DeviceSimulation)
};
