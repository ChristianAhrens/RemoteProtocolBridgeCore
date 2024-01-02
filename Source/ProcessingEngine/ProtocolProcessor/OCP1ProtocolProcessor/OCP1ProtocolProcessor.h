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
	bool PreparePositionMessageData(const RemoteObject& targetObj, RemoteObjectMessageData& msgDataToSet);
	bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId = -1) override;

	//==============================================================================
	static juce::String GetRemoteObjectString(const RemoteObjectIdentifier roi);

private:
	//==============================================================================
	void timerThreadCallback() override;

	//==============================================================================
	void CreateKnownONosMap();

	//==============================================================================
	bool ocp1MessageReceived(const juce::MemoryBlock& data);
	bool GetObjectDefinition(const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, NanoOcp1::Ocp1CommandDefinition** objDef);
	bool CreateObjectSubscriptions();
	bool DeleteObjectSubscriptions();
	bool QueryObjectValues();
	bool QueryObjectValue(const RemoteObjectIdentifier roi, const ChannelId& channel, const RecordId& record);

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
	void AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo, const int externalId);
	const std::uint32_t PopPendingSetValueHandle(const std::uint32_t handle, int& externalId);
	bool HasPendingSetValues();
	const std::optional<std::pair<std::uint32_t, int>> HasPendingSetValue(const std::uint32_t ONo);

	//==============================================================================
	void ClearPendingHandles();

	//==============================================================================
	bool UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj);
	bool UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj);
	bool UpdateObjectValue(const RemoteObjectIdentifier roi, NanoOcp1::Ocp1Message* msgObj, 
		const std::pair<std::pair<RecordId, ChannelId>, NanoOcp1::Ocp1CommandDefinition>& objectDetails);

	//==============================================================================
	std::unique_ptr<NanoOcp1::NanoOcp1Base>					m_nanoOcp;
	std::vector<std::uint32_t>								m_pendingSubscriptionHandles;
	std::map<std::uint32_t, std::uint32_t>					m_pendingGetValueHandlesWithONo;
	std::map<std::uint32_t, std::pair<std::uint32_t, int>>	m_pendingSetValueHandlesWithONo;

	//==============================================================================
	std::map<RemoteObjectIdentifier, std::map<std::pair<RecordId, ChannelId>, NanoOcp1::Ocp1CommandDefinition>>	m_ROIsToDefsMap;

	//==============================================================================
	// Helpers
	
	/**
	 *  @brief Helper to check the msgData against parameter count and payload size
	 *  @tparam		T			Template class (int | float | char)
	 *  @param[in]	paramCount	Amount of expected parameters within msgData
	 *  @param[in]	msgData		The message data to check
	 *  @returns				True if the message data has expected parameter count and size
	 */
	template <class T>
	bool CheckMessagePayload(const uint8_t paramCount, const RemoteObjectMessageData& msgData)
	{
		return (msgData._valCount != paramCount && msgData._payloadSize != paramCount * sizeof(T));
	}

	/**
	 *  @brief Helper to check the msgData for 1 parameter with payload matching type and convert the data
	 *  @note Use this only for simple types
	 *  @tparam		T		Template class (int | float | char)
	 *  @param[in]	msgData	The message data to check
	 *  @param[out]	value	The parsed msgData as juce::var
	 *  @returns			True if the message data exactly 1 parameter matching the template class size
	 */
	template <class T>
	bool CheckAndParseMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value)
	{
		if (CheckMessagePayload<T>(1, msgData))
			return false;
		value = juce::var(*static_cast<T*>(msgData._payload));
		return true;
	}

	bool CheckAndParseStringMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value);
	bool CheckAndParseMuteMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value);
	bool ParsePositionMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value, NanoOcp1::Ocp1CommandDefinition* objDef);
	bool ParsePositionAndRotationMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value, NanoOcp1::Ocp1CommandDefinition* objDef);
};