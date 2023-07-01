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

#include "../RemoteProtocolBridgeCommon.h"

#include "ProcessingEngineConfig.h"
#include "ProtocolProcessor/ProtocolProcessorBase.h"

// Fwd. declarations
class ObjectDataHandling_Abstract;
class ProcessingEngine;

/**
 * Class ProcessingEngineNode is a class to hold a processing element handled by engine class.
 */
class ProcessingEngineNode :	public ProtocolProcessorBase::Listener,
								public ProcessingEngineConfig::XmlConfigurableElement,
								private Thread,
								private MessageListener
{
public:
	/**
	 * Implementation of a message with contents to transport all relevant info from one
	 * and inbetween several protocols.
	 */
	struct InterProtocolMessage
	{
		/**
		 * Default Constructor.
		 */
		InterProtocolMessage() {};

		/**
		 * Copy Constructor.
		 */
		InterProtocolMessage(const InterProtocolMessage& rhs)
		{
			*this = rhs;
		}

		/**
		 * Constructor with default initialization.
		 *
		 * @param nodeId				The node id to initialize the member with.
		 * @param senderProtocolId		The sender protocol id to initialize the member with.
		 * @param senderProtocolType	The sender protocol type to initialize the member with.
		 * @param Id					The remote object id to initialize the member with.
		 * @param msgData				The message data to initialize the member with.
		 * @param msgMeta				The message metadata to initialize the member with.
		 */
		InterProtocolMessage(NodeId nodeId, ProtocolId senderProtocolId, ProtocolType senderProtocolType, RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta) :
			_nodeId(nodeId),
			_senderProtocolId(senderProtocolId),
			_senderProtocolType(senderProtocolType),
			_Id(Id),
			_msgData(msgData),
			_msgMeta(msgMeta)
		{
		}

		/**
		 * Constructor with default initialization.
		 *
		 * @param senderProtocolId		The sender protocol id to initialize the member with.
		 * @param Id					The remote object id to initialize the member with.
		 * @param msgData				The message data to initialize the member with.
		 * @param msgMeta				The message metadata to initialize the member with.
		 */
		InterProtocolMessage(ProtocolId senderProtocolId, RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta) :
			_senderProtocolId(senderProtocolId),
			_Id(Id),
			_msgMeta(msgMeta)
		{
			_msgData.payloadCopy(msgData);
		}

		/**
		 * Destructor.
		 */
		~InterProtocolMessage() {};

		/**
		 * Assignment operator
		 */
		InterProtocolMessage& operator=(const InterProtocolMessage& rhs)
		{
			if (this != &rhs)
			{
				_nodeId = rhs._nodeId;
				_senderProtocolId = rhs._senderProtocolId;
				_senderProtocolType = rhs._senderProtocolType;
				_Id = rhs._Id;
				_msgData.payloadCopy(rhs._msgData);
				_msgMeta = rhs._msgMeta;
			}

			return *this;
		}

		NodeId							_nodeId{ static_cast<NodeId>(INVALID_ADDRESS_VALUE) };
		ProtocolId						_senderProtocolId{ static_cast<ProtocolId>(INVALID_ADDRESS_VALUE) };
		ProtocolType					_senderProtocolType{ PT_Invalid };
		RemoteObjectIdentifier			_Id{ ROI_Invalid };
		RemoteObjectMessageData			_msgData;
		RemoteObjectMessageMetaInfo	_msgMeta;
	};

	/**
	 * Implementation of a node message to use with JUCE's message queue.
	 */
	struct NodeCallbackMessage : public Message
	{
		/**
		 * Constructor with default initialization.
		 * @param protocolMessage	The message data to encapsulate in this callback message
		 */
		NodeCallbackMessage(const InterProtocolMessage& protocolMessage) :
			_protocolMessage(protocolMessage) {};

		/**
		 * Destructor.
		 */
		~NodeCallbackMessage() override {};

		InterProtocolMessage	_protocolMessage;
	};

	/**
	 * Abstract embedded interface class for message data handling
	 */
	class NodeListener
	{
	public:
		NodeListener() {};
		virtual ~NodeListener() {};

		/**
		 * Method to be overloaded by ancestors to act as an interface
		 * for handling of received message data
		 */
		virtual void HandleNodeData(const NodeCallbackMessage* callbackMessage) = 0;
	};

public:
	ProcessingEngineNode();
	ProcessingEngineNode(ProcessingEngineNode::NodeListener* listener);
	~ProcessingEngineNode();

	void AddListener(ProcessingEngineNode::NodeListener* listener);

	NodeId GetId();
	Thread::ThreadID GetNodeThreadId();

	bool SendMessageTo(ProtocolId PId, RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) const;

	bool Start();
	bool Stop();
	bool IsRunning();

	//==============================================================================
	virtual std::unique_ptr<XmlElement> createStateXml() override;
	virtual bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	void OnProtocolMessageReceived(ProtocolProcessorBase* receiver, RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta = RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_None, -1)) override;

	//==============================================================================
	void handleMessage(const Message& msg) override;

	//==============================================================================
	void run() override;

	//==============================================================================
	ObjectDataHandling_Abstract* GetObjectDataHandling();

protected:
	/**
	 * Embedded class to safely handle message en-/dequeueing 
	 * between protocol callbacks and node thread.
	 */
	class InterProtocolMessageQueue
	{
	public:
		InterProtocolMessageQueue()
			: m_protocolMessagesInQueue(true), m_queueManager(1)
		{
			clear();
		};
		~InterProtocolMessageQueue()
		{};

		//==============================================================================
		/**
		 * Adds the given message ref contents to the queue.
		 * @param message	The message ref to take contents from and add to queue.
		 */
		void enqueueMessage(const InterProtocolMessage& message) 
		{
			const ScopedLock l(m_messageQueue.getLock());

			// check how full our queue is
			if (m_queueManager.getFreeSpace() == 0)
			{
				m_queueSize += 1024;
				m_messageQueue.resize(m_queueSize);
				m_queueManager.setTotalSize(m_queueSize);
			}

			// prepare enqueueing by determining where in the queue to put our message
			auto dataCountToWrite = 1;
			int writeIndex1, possibleItems1, writeIndex2, possibleItems2;
			m_queueManager.prepareToWrite(dataCountToWrite, writeIndex1, possibleItems1, writeIndex2, possibleItems2);
			// set the message into queue
			m_messageQueue.set(writeIndex1, message);
			// finish enqueueing
			m_queueManager.finishedWrite(dataCountToWrite);

			// signal to outside world, that data is available
			m_protocolMessagesInQueue.signal();
		};
		/**
		 * Gets a message from queue and fills it into the given msg struct.
		 * @param message	The message ref to be filled with next message content from queue
		 * @return True if a message was ready and filled into the ref, otherwise false.
		 */
		bool dequeueMessage(InterProtocolMessage& message)
		{
			const ScopedLock l(m_messageQueue.getLock());

			// check if queuemanager has items readready
			if (m_queueManager.getNumReady() > 0)
			{
				// prepare dequeueing by detemining where in the queue our item can be read
				auto requiredDataCount = 1;
				int readIndex1, availableItems1, readIndex2, availableItems2;
				m_queueManager.prepareToRead(requiredDataCount, readIndex1, availableItems1, readIndex2, availableItems2);
				// if availableitemcount matches what we requre, proceed with reading
				if ((availableItems1 + availableItems2) >= requiredDataCount)
				{
					// get the data item reference and copy into our local var
					message = m_messageQueue.getReference(readIndex1);
					// finish dequeueing
					m_queueManager.finishedRead(requiredDataCount);
				}

				// If manager has no more items readready, signal to outside world, that no more data is available
				if (m_queueManager.getNumReady() == 0)
					m_protocolMessagesInQueue.reset();

				return true;
			}
			else
				return false;
		};
		//==============================================================================
		void clear()
		{
			const ScopedLock l(m_messageQueue.getLock());

			m_queueManager.reset();
			m_protocolMessagesInQueue.reset();

			m_queueSize = 1024;
			m_messageQueue.resize(m_queueSize);
			m_queueManager.setTotalSize(m_queueSize);
		}

		//==============================================================================
		bool waitForMessage(int timeoutMilliseconds = -1) { return m_protocolMessagesInQueue.wait(timeoutMilliseconds); };

	private:
		//==============================================================================
		WaitableEvent									m_protocolMessagesInQueue;
		Array<InterProtocolMessage, CriticalSection>	m_messageQueue;
		AbstractFifo									m_queueManager;
		int												m_queueSize{ 1024 };

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InterProtocolMessageQueue)
	};

	//==============================================================================
	virtual std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>& GetTypeAProtocols();
	virtual std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>& GetTypeBProtocols();
	virtual void SetId(const NodeId id);

private:
	//==============================================================================
	ProtocolProcessorBase* CreateProtocolProcessor(ProtocolType type, int listenerPortNumber);
	//==============================================================================
	ObjectDataHandling_Abstract* CreateObjectDataHandling(ObjectHandlingMode mode);

	//==============================================================================
	std::unique_ptr<ObjectDataHandling_Abstract>					m_dataHandling;		/**< The object data handling object (to be initialized with instance of derived class). */

	NodeId															m_nodeId;			/**< The id of the bridging node object. */

	std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>	m_typeAProtocols;	/**< The remote protocols that act with role A of this node. */
	std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>	m_typeBProtocols;	/**< The remote protocols that act with role B of this node. */

	std::vector<ProcessingEngineNode::NodeListener*>				m_listeners;		/**< The listner objects, for e.g. logging message traffic. */

	bool															m_nodeRunning;
	WaitableEvent													m_threadRunning;

	InterProtocolMessageQueue										m_messageQueue;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingEngineNode)
};
