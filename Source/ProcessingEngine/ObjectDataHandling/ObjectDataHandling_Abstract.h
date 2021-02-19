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
class ObjectDataHandling_Abstract : public ProcessingEngineConfig::XmlConfigurableElement
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
		 * Convenience method to be used by derived classes to publish
		 * status updates through std::function callback, if it is valid
		 * @param	id		The protocol id of the protocol that had changes
		 * @param	status	The new status for the protocol that changed
		 * @return	True if change was published successfully, false if not
		 */
		void SetChangedProtocolStatus(ProtocolId id, ObjectHandlingStatus status)
		{
			postMessage(std::make_unique<StatusCallbackMessage>(id, status).release());
		}

		/**
		 * Reimplmented from MessageListener.
		 * @param msg	The message data to handle.
		 */
		void handleMessage(const Message& msg)
		{
			if (auto* statusMessage = dynamic_cast<const StatusCallbackMessage*> (&msg))
				protocolStatusChanged(statusMessage->_protocolId, statusMessage->_status);
		}

		//==============================================================================
		virtual void protocolStatusChanged(ProtocolId id, ObjectHandlingStatus status) = 0;
	};

public:
	ObjectDataHandling_Abstract(ProcessingEngineNode* parentNode);
	virtual ~ObjectDataHandling_Abstract();

	ObjectHandlingMode GetMode();

	virtual void AddProtocolAId(ProtocolId PAId);
	virtual void AddProtocolBId(ProtocolId PBId);
	void ClearProtocolIds();

	//==============================================================================
	void AddStatusListener(StatusListener* listener);
	void RemoveStatusListener(StatusListener* listener);

	//==============================================================================
	virtual bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) = 0;

	//==============================================================================
	virtual std::unique_ptr<XmlElement> createStateXml() override;
	virtual bool setStateXml(XmlElement* stateXml) override;

protected:
	const ProcessingEngineNode*		GetParentNode();
	void							SetMode(ObjectHandlingMode mode);
	NodeId							GetParentNodeId();
	const std::vector<ProtocolId>&	GetProtocolAIds();
	const std::vector<ProtocolId>&	GetProtocolBIds();
	void							SetChangedProtocolStatus(ProtocolId id, ObjectHandlingStatus status);
	void							UpdateOnlineState(ProtocolId id);
	const std::map<ProtocolId, double>&	GetLastProtocolReactionTSMap();

private:
	ProcessingEngineNode*			m_parentNode;		/**< The parent node object. Needed for e.g. triggering receive notifications. */
	ObjectHandlingMode				m_mode;				/**< Mode identifier enabling resolving derived instance type. */
	NodeId							m_parentNodeId;		/**< The id of the objects' parent node. */
	std::vector<ProtocolId>			m_protocolAIds;		/**< Id list of protocols of type A that is active for the node and this handling module therefor. */
	std::vector<ProtocolId>			m_protocolBIds;		/**< Id list of protocols of type B that is active for the node and this handling module therefor. */

	std::map<ProtocolId, double>	m_lastProtocolReactionTSMap;	/**< Map of protocols and their last-seen-active TimeStamps. */
	std::vector<StatusListener*>	m_statusListeners;				/**< The list of objects that are registered to be notified on internal status changes. */

};
