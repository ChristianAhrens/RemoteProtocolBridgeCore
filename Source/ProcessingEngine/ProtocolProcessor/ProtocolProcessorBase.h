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

#include "../../RemoteProtocolBridgeCommon.h"
#include "../ProcessingEngineConfig.h"
#include "../RemoteObjectValueCache.h"
#include "../TimerThreadBase.h"

#include <JuceHeader.h>


/**
 * Class ProtocolProcessorBase is an abstract interfacing base class for protocol interaction.
 * It provides a gerenic interface to start, stop, initialize and interact with the protocol it
 * implements in a derived object. Its parent node object provides a handler method to processed
 * received protocol message data.
 */
class ProtocolProcessorBase :	public ProcessingEngineConfig::XmlConfigurableElement,
								public TimerThreadBase
{
public:
	/**
	 * Abstract embedded interface class for message data handling
	 */
	class Listener
	{
	public:
		Listener() {};
		virtual ~Listener() {};

		/**
		 * Method to be overloaded by derived implementations to act
		 * as an interface for handling of received message data
		 */
		virtual void OnProtocolMessageReceived(ProtocolProcessorBase* receiver, 
			const RemoteObjectIdentifier roi, 
			const RemoteObjectMessageData& msgData, 
			const RemoteObjectMessageMetaInfo& msgMeta = RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_None, -1)) = 0;
	};

public:
	ProtocolProcessorBase(const NodeId& parentNodeId);
	virtual ~ProtocolProcessorBase();

	RemoteObjectValueCache& GetValueCache();

	void AddListener(Listener *messageReceiver);
	ProtocolType GetType();
	ProtocolId GetId();
	ProtocolRole GetRole();

	//==============================================================================
	virtual bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId = -1) = 0;

	virtual bool Start() = 0;
	virtual bool Stop() = 0;

	//==============================================================================
	void SetActiveRemoteObjectsInterval(int interval);
	int GetActiveRemoteObjectsInterval();
	
	//==============================================================================
	void SetRemoteObjectsActive(XmlElement* activeObjsXmlElement);	/**< Objects that are to be handled actively (OSC polling, OCA subscribing). */

	//==============================================================================
	void SetRemoteObjectsMuted(XmlElement* mutedObjChsXmlElement);
	bool IsRemoteObjectMuted(RemoteObject object);

	//==============================================================================
	static float NormalizeValueByRange(float value, const juce::Range<float>& normalizationRange);
	static float MapNormalizedValueToRange(float normalizedValue, const juce::Range<float>& range, bool invert = false);
	static bool MapMessageDataToTargetRangeAndType(const RemoteObjectMessageData& sourceData, const juce::Range<float>& sourceRange, const juce::Range<float>& targetRange, const RemoteObjectValueType targetType, RemoteObjectMessageData& targetData);

	//==============================================================================
	std::unique_ptr<XmlElement> createStateXml() override { return nullptr; };
	virtual bool setStateXml(XmlElement* stateXml) override;								

protected:
	//==============================================================================
	const std::vector<RemoteObject>& GetActiveRemoteObjects();

	//==============================================================================
	Listener				*m_messageListener;				/**< The parent node object. Needed for e.g. triggering receive notifications. */
	ProtocolType			m_type;							/**< Processor type regarding the protocol being handled */
	NodeId					m_parentNodeId;					/**< The id of the objects' parent node. */
	ProtocolId				m_protocolProcessorId;			/**< The id of the processor object itself. */
	ProtocolRole			m_protocolProcessorRole;		/**< The role (a/b) configured for this processor object. */
	bool					m_IsRunning;					/**< Bool indication if the processor is successfully running. */

private:
	virtual void timerThreadCallback() override;

	std::vector<RemoteObject>	m_mutedRemoteObjects;			/**< List of remote objects to be muted. */
	std::vector<RemoteObject>	m_activeRemoteObjects;			/**< List of remote objects to be activly handled. */
	int							m_activeRemoteObjectsInterval;	/**< Interval at which data is polled/requested from protocol peer. */
	CriticalSection				m_activeRemoteObjectsLock;

	RemoteObjectValueCache		m_valueCache;

};
