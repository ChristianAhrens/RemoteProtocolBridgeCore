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
		virtual void SetChangedProtocolState(ProtocolId id, ObjectHandlingState state)
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
	virtual bool OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData) = 0;

	//==============================================================================
	virtual std::unique_ptr<XmlElement> createStateXml() override;
	virtual bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	virtual void UpdateOnlineState(ProtocolId id);

	//==============================================================================
	void timerCallback() override;

protected:
	const ProcessingEngineNode*			GetParentNode();
	void								SetMode(ObjectHandlingMode mode);
	NodeId								GetParentNodeId();
	const std::vector<ProtocolId>&		GetProtocolAIds();
	const std::vector<ProtocolId>&		GetProtocolBIds();
	void								SetChangedProtocolState(ProtocolId id, ObjectHandlingState state);
	const std::map<ProtocolId, double>&	GetLastProtocolReactionTSMap();

private:
	ProcessingEngineNode*				m_parentNode;		/**< The parent node object. Needed for e.g. triggering receive notifications. */
	ObjectHandlingMode					m_mode;				/**< Mode identifier enabling resolving derived instance type. */
	NodeId								m_parentNodeId;		/**< The id of the objects' parent node. */
	std::vector<ProtocolId>				m_protocolAIds;		/**< Id list of protocols of type A that is active for the node and this handling module therefor. */
	std::vector<ProtocolId>				m_protocolBIds;		/**< Id list of protocols of type B that is active for the node and this handling module therefor. */

	std::vector<ProtocolId>				m_protocolsWithReactionMonitoring;	/**< List of protocols that shall be taken into account when processing reativeness (empty means all incoming are processed). */
	CriticalSection						m_protocolReactionTSLock;			/**< Threadsafety measure, since timestamps are written from node processing thread while others might access them from event loop. */
	std::map<ProtocolId, double>		m_protocolReactionTSMap;			/**< Map of protocols and their last-seen-active TimeStamps. */
	double								m_protocolReactionTimeout;			/**< Timeout in ms when a protocol is regarded as down. */

	std::vector<StateListener*>					m_stateListeners;			/**< The list of objects that are registered to be notified on internal status changes. */
	std::map<ProtocolId, ObjectHandlingState>	m_currentStateMap;

};
