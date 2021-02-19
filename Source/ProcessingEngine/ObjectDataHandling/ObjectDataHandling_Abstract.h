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
	/**
	 * Enum that describes the different
	 * status a ObjectHandling instance can
	 * notify registered listners of.
	 */
	enum ObjectHandlingStatus
	{
		OHS_Invalid,
		OHS_Protocol_Up,
		OHS_Protocol_Down,
		OHS_Protocol_Fail,
		OHS_Protocol_PromotedPrimary,
		OHS_Protocol_DegradedSecondary,
	};

	class StatusListener : private MessageListener
	{
	public:
		/**
		 * Implementation of a protocol status message to use with JUCE's message queue.
		 */
		struct StatusCallbackMessage : public Message
		{
			/**
			 * Constructor with default initialization.
			 * @param id		The protocol id this status message refers to
			 * @param status	The status enum value that this message transports
			 */
			StatusCallbackMessage(ProtocolId id, ObjectHandlingStatus status) :
				_protocolId(id), _status(status) {};

			/**
			 * Destructor.
			 */
			virtual ~StatusCallbackMessage() override {};

			ProtocolId				_protocolId;
			ObjectHandlingStatus	_status;
		};

	public:
		/**
		 * Convenience method to be used by ObjectHandling classes to publish
		 * status updates to listeners. This method internally stores the changed status
		 * in a map and uses message queue to asynchronously notify the derived object
		 * of changes.
		 * @param	id		The protocol id of the protocol that had changes
		 * @param	status	The new status for the protocol that changed
		 * @return	True if change was published successfully, false if not
		 */
		void SetChangedProtocolStatus(ProtocolId id, ObjectHandlingStatus status)
		{
			if (m_currentStatusMap.count(id) <= 0 || m_currentStatusMap.at(id) != status)
			{
				postMessage(std::make_unique<StatusCallbackMessage>(id, status).release());
				m_currentStatusMap[id] = status;
			}
		}
		/**
		 * Getter for the internal status for a given protocol from the private member map.
		 * @param	id	The protocol id to get the status for.
		 * @return	The status for the given protocol id or Invalid of unknown.
		 */
		ObjectHandlingStatus GetProtocolStatus(ProtocolId id)
		{
			if (m_currentStatusMap.count(id) <= 0)
				return OHS_Invalid;
			else
				return m_currentStatusMap.at(id);
		}
		/**
		 * Method to clear all hashed status entries.
		 */
		void ClearProtocolStatus()
		{
			m_currentStatusMap.clear();
		}

		/**
		 * Reimplmented from MessageListener.
		 * @param msg	The message data to handle.
		 */
		void handleMessage(const Message& msg) override
		{
			if (auto* statusMessage = dynamic_cast<const StatusCallbackMessage*> (&msg))
				protocolStatusChanged(statusMessage->_protocolId, statusMessage->_status);
		}

		//==============================================================================
		virtual void protocolStatusChanged(ProtocolId id, ObjectHandlingStatus status) = 0;

	private:
		std::map<ProtocolId, ObjectHandlingStatus> m_currentStatusMap;
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
	void AddStatusListener(StatusListener* listener);
	void RemoveStatusListener(StatusListener* listener);

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
	void								SetChangedProtocolStatus(ProtocolId id, ObjectHandlingStatus status);
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

	std::vector<StatusListener*>		m_statusListeners;			/**< The list of objects that are registered to be notified on internal status changes. */

};
