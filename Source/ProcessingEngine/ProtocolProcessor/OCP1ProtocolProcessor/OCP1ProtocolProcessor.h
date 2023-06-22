/* Copyright (c) 2023, Christian Ahrens
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

#include <JuceHeader.h>


/**
 * Fwd. decl.
 */
namespace NanoOcp1
{
	class NanoOcp1Base;
	class Ocp1Message;
	class Ocp1Notification;
	class Ocp1Response;
	struct Ocp1CommandDefinition;
};

/**
 * Class OCP1ProtocolProcessor is a derived class for OCA protocol interaction. 
 * This currently is only a dummy for potential future functionality.
 * Feel free to implement something yourself here.
 */
class OCP1ProtocolProcessor : public NetworkProtocolProcessorBase
{
public:
	OCP1ProtocolProcessor(const NodeId& parentNodeId);
	~OCP1ProtocolProcessor();

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	bool Start() override;
	bool Stop() override;

	//==============================================================================
	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	//==============================================================================
	static String GetRemoteObjectString(RemoteObjectIdentifier id);

private:
	//==============================================================================
	void timerThreadCallback() override;

	//==============================================================================
	void CreateKnownONosMap();

	//==============================================================================
	bool ocp1MessageReceived(const juce::MemoryBlock& data);
	bool CreateObjectSubscriptions();
	bool DeleteObjectSubscriptions();
	bool QueryObjectValues();

	//==============================================================================
	const std::vector<RemoteObject> GetOcp1SupportedActiveRemoteObjects();

	//==============================================================================
	void AddPendingSubscriptionHandle(const std::uint32_t handle);
	bool PopPendingSubscriptionHandle(const std::uint32_t handle);
	bool HasPendingSubscriptions();

	//==============================================================================
	void AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo);
	const std::uint32_t PopPendingGetValueHandle(const std::uint32_t handle);
	bool HasPendingGetValues();

	//==============================================================================
	void AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo);
	const std::uint32_t PopPendingSetValueHandle(const std::uint32_t handle);
	bool HasPendingSetValues();

	//==============================================================================
	void ClearPendingHandles();

	//==============================================================================
	bool UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj);
	bool UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj);
	bool UpdateObjectValue(const RemoteObjectIdentifier roi, NanoOcp1::Ocp1Message* msgObj, 
		const std::pair<std::pair<RecordId, ChannelId>, NanoOcp1::Ocp1CommandDefinition>& objectDetails);

	//==============================================================================
	std::unique_ptr<NanoOcp1::NanoOcp1Base>	m_nanoOcp;
	std::vector<std::uint32_t>				m_pendingSubscriptionHandles;
	std::map<std::uint32_t, std::uint32_t>	m_pendingGetValueHandlesWithONo;
	std::map<std::uint32_t, std::uint32_t>	m_pendingSetValueHandlesWithONo;

	//==============================================================================
	std::map<RemoteObjectIdentifier, std::map<std::pair<RecordId, ChannelId>, NanoOcp1::Ocp1CommandDefinition>>	m_ROIsToDefsMap;

};
