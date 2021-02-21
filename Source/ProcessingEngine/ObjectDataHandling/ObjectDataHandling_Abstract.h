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

#include <JuceHeader.h>

// Fwd. declarations
class ProcessingEngineNode;

/**
 * Class ObjectDataHandling_Abstract is an abstract interfacing base class for .
 */
class ObjectDataHandling_Abstract : public ProcessingEngineConfig::XmlConfigurableElement,
	public Timer
{
public:
	class StateListener : private MessageListener
	{
	public:
		/**
		 * Implementation of a protocol state message to use with JUCE's message queue.
		 */
		struct StateCallbackMessage : public Message
		{
			/**
			 * Constructor with default initialization.
			 * @param id		The protocol id this state message refers to
			 * @param status	The state enum value that this message transports
			 */
			StateCallbackMessage(ProtocolId id, ObjectHandlingState state) :
				_protocolId(id), _state(state) {};

			/**
			 * Destructor.
			 */
			virtual ~StateCallbackMessage() override {};

			ProtocolId			_protocolId;
			ObjectHandlingState	_state;
		};

	public:
		/**
		 * Convenience method to be used by ObjectHandling classes to publish
		 * state updates to listeners.
		 * @param	id		The protocol id of the protocol that had changes
		 * @param	state	The new state for the protocol that changed
		 * @return	True if change was published successfully, false if not
		 */
		void SetChangedProtocolState(ProtocolId id, ObjectHandlingState state)
		{
			postMessage(std::make_unique<StateCallbackMessage>(id, state).release());
		}

		/**
		 * Reimplmented from MessageListener.
		 * @param msg	The message data to handle.
		 */
		void handleMessage(const Message& msg) override
		{
			if (auto* stateMessage = dynamic_cast<const StateCallbackMessage*> (&msg))
				protocolStateChanged(stateMessage->_protocolId, stateMessage->_state);
		}

		//==============================================================================
		virtual void protocolStateChanged(ProtocolId id, ObjectHandlingState state) = 0;

	};

public:
	ObjectDataHandling_Abstract(ProcessingEngineNode* parentNode);
	virtual ~ObjectDataHandling_Abstract();

	ObjectHandlingMode GetMode();

	virtual void AddProtocolAId(ProtocolId PAId);
	virtual void AddProtocolBId(ProtocolId PBId);
	void ClearProtocolIds();

	void SetProtocolReactionTimeout(double timeout);
	double GetProtocolReactionTimeout();

	//==============================================================================
	void AddStateListener(StateListener* listener);
	void RemoveStateListener(StateListener* listener);

	ObjectHandlingState GetProtocolState(ProtocolId id);

	//==============================================================================
	virtual bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) = 0;

	//==============================================================================
	virtual std::unique_ptr<XmlElement> createStateXml() override;
	virtual bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	void timerCallback() override;

protected:
	const ProcessingEngineNode*			GetParentNode();
	void								SetMode(ObjectHandlingMode mode);
	NodeId								GetParentNodeId();
	const std::vector<ProtocolId>&		GetProtocolAIds();
	const std::vector<ProtocolId>&		GetProtocolBIds();
	void								SetChangedProtocolState(ProtocolId id, ObjectHandlingState state);
	void								UpdateOnlineState(ProtocolId id);
	const std::map<ProtocolId, double>&	GetLastProtocolReactionTSMap();

private:
	ProcessingEngineNode*				m_parentNode;		/**< The parent node object. Needed for e.g. triggering receive notifications. */
	ObjectHandlingMode					m_mode;				/**< Mode identifier enabling resolving derived instance type. */
	NodeId								m_parentNodeId;		/**< The id of the objects' parent node. */
	std::vector<ProtocolId>				m_protocolAIds;		/**< Id list of protocols of type A that is active for the node and this handling module therefor. */
	std::vector<ProtocolId>				m_protocolBIds;		/**< Id list of protocols of type B that is active for the node and this handling module therefor. */

	CriticalSection						m_protocolReactionTSLock;	/**< Threadsafety measure, since timestamps are written from node processing thread while others might access them from event loop. */
	std::map<ProtocolId, double>		m_protocolReactionTSMap;	/**< Map of protocols and their last-seen-active TimeStamps. */
	double								m_protocolReactionTimeout;	/**< Timeout in ms when a protocol is regarded as down. */

	std::vector<StateListener*>					m_stateListeners;			/**< The list of objects that are registered to be notified on internal status changes. */
	std::map<ProtocolId, ObjectHandlingState>	m_currentStateMap;

};
