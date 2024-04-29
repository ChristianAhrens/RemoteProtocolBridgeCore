/* Copyright (c) 2024, Christian Ahrens
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

#include "NoProtocolProtocolProcessor.h"

/**
 * Class AURAProtocolProtocolProcessor is a derived class for dummy protocol interaction.
 */
class AURAProtocolProtocolProcessor : public NoProtocolProtocolProcessor, juce::Timer
{
public:
	enum AuraPacketType
	{
		APT_None = 0,
		APT_ListenerPosition,
		APT_ObjectPosition
	};

public:
	AURAProtocolProtocolProcessor(const NodeId& parentNodeId);
	virtual ~AURAProtocolProtocolProcessor() override;

	void timerCallback() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool Start() override;
	bool Stop() override;

	void SetIpAddress(const juce::IPAddress& ipAddress);
	void SetPort(int port);

	void SetListenerPosition(const juce::Vector3D<float>& pos);
	void SetArea(const juce::Rectangle<float>& area);

protected:
	//==============================================================================
	class AURAConnection : public juce::InterprocessConnection
	{
	public:
		AURAConnection() : juce::InterprocessConnection(false) {};
		virtual ~AURAConnection() override { disconnect(); };

		bool connect(const juce::IPAddress& address, int port = s_auraPort, int timeoutMs = 1000) {
			if (isConnected())
				disconnect(); 
			return connectToSocket(address.toString(), port, timeoutMs);
		};

		//==============================================================================
		std::function<bool(const juce::MemoryBlock&)> onDataReceived;
		std::function<void()> onConnectionEstablished;
		std::function<void()> onConnectionLost;

		//==============================================================================
		static constexpr int s_auraPort = 60123;

	protected:
		void connectionMade() override {
			if (onConnectionEstablished)
				onConnectionEstablished();
		};
		void connectionLost() override {
			if (onConnectionLost)
				onConnectionLost();
		};
		void messageReceived(const juce::MemoryBlock& data) override {
			if (onDataReceived)
				onDataReceived(data);
		};
	};
	
	//==============================================================================
	void InitializeObjectValueCache() override;
	void SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData) override;

private:
	//==============================================================================
	bool SendListenerPositionToAURA();
	bool SendKnownSourcePositionsToAURA();
	bool SendSourcePositionToAURA(std::int32_t sourceId, const juce::Vector3D<float>& sourcePosition);

	juce::Vector3D<float> RelativeToAbsolutePosition(const juce::Vector3D<float>& relativePosition);
	
	//==============================================================================
	std::unique_ptr<AURAConnection> m_networkConnection;

	juce::IPAddress	m_ipAddress{ "127.0.0.1" };
	int m_port{ AURAConnection::s_auraPort };

	juce::Vector3D<float>	m_listenerPosition{ 5.0f, 5.0f, 0.0f };
	juce::Rectangle<float>	m_area{ 10.0f, 10.0f };
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AURAProtocolProtocolProcessor)
};
