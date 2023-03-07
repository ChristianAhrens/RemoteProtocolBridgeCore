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

#include "../../../RemoteProtocolBridgeCommon.h"
#include "../NetworkProtocolProcessorBase.h"

#include "RTTrPMReceiver/RTTrPMReceiver.h"

#include <JuceHeader.h>

/**
 * Class RTTrPMProtocolProcessor is a derived class for OSC protocol interaction.
 */
class RTTrPMProtocolProcessor : public RTTrPMReceiver::RealtimeDataListener,
	public NetworkProtocolProcessorBase
{
public:
	RTTrPMProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	~RTTrPMProtocolProcessor();

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	bool Start() override;
	bool Stop() override;

	//==============================================================================
	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	//==============================================================================
	void RTTrPMModuleReceived(const RTTrPMReceiver::RTTrPMMessage& rttrpmMessage, const String& senderIPAddress, const int& senderPort) override;

	//==============================================================================
	static const juce::String GetRTTrPMModuleString(PacketModule::PacketModuleType moduleType);

protected:
	//==============================================================================
	void SetHostPort(std::int32_t hostPort) override;

private:
	std::unique_ptr<RTTrPMReceiver>	m_rttrpmReceiver;	/**< A receiver object for BlackTrax RTTrPM protocol that binds to a network port to receive data
														 *   via UDP, parse it, and forwards the included RTTrPM packet modules to its listeners.
														 */
	MappingAreaId	m_mappingAreaId{ MAI_Invalid };	/**< The DS100 mapping area to be used when converting incoming coords into relative messages. 
													 *   If this is MAI_Invalid, absolute messages will be generated.
													 */
	PacketModule::PacketModuleType	m_packetModuleTypeForPositioning{ PacketModule::CentroidPosition };


	float m_floatValueBuffer[3] = { 0.0f, 0.0f, 0.0f };
	int m_intValueBuffer[2] = { 0, 0 };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RTTrPMProtocolProcessor)
};
