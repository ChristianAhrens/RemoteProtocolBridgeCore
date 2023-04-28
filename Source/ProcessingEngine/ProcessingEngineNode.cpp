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

#include "ProcessingEngineNode.h"

#include "ProcessingEngine.h"
#include "ProcessingEngineConfig.h"

#include "ObjectDataHandling/BypassHandling/BypassHandling.h"
#include "ObjectDataHandling/Remap_A_X_Y_to_B_XY_Handling/Remap_A_X_Y_to_B_XY_Handling.h"
#include "ObjectDataHandling/Mux_nA_to_mB/Mux_nA_to_mB.h"
#include "ObjectDataHandling/Forward_only_valueChanges/Forward_only_valueChanges.h"
#include "ObjectDataHandling/DS100_DeviceSimulation/DS100_DeviceSimulation.h"
#include "ObjectDataHandling/Forward_A_to_B_only/Forward_A_to_B_only.h"
#include "ObjectDataHandling/Reverse_B_to_A_only/Reverse_B_to_A_only.h"
#include "ObjectDataHandling/Mux_nA_to_mB_withValFilter/Mux_nA_to_mB_withValFilter.h"
#include "ObjectDataHandling/Mirror_dualA_withValFilter/Mirror_dualA_withValFilter.h"
#include "ObjectDataHandling/A1active_withValFilter/A1active_withValFilter.h"
#include "ObjectDataHandling/A2active_withValFilter/A2active_withValFilter.h"

#include "ProtocolProcessor/OCAProtocolProcessor/OCAProtocolProcessor.h"
#include "ProtocolProcessor/OSCProtocolProcessor/OSCProtocolProcessor.h"
#include "ProtocolProcessor/RTTrPMProtocolProcessor/RTTrPMProtocolProcessor.h"
#include "ProtocolProcessor/MIDIProtocolProcessor/MIDIProtocolProcessor.h"
#include "ProtocolProcessor/OSCProtocolProcessor/YmhOSCProtocolProcessor.h"
#include "ProtocolProcessor/OSCProtocolProcessor/ADMOSCProtocolProcessor.h"
#include "ProtocolProcessor/OSCProtocolProcessor/RemapOSCProtocolProcessor.h"

// **************************************************************************************
//    class ProcessingEngineNode
// **************************************************************************************
/**
 * Constructor
 */
ProcessingEngineNode::ProcessingEngineNode()
	: Thread("ProcessingEngingNode_Thread"),
	  m_dataHandling(nullptr),
      m_nodeId(0),
	  m_nodeRunning(false)
{
}

/**
 * Constructor including initialization of internal parent object
 *
 * @param parentEngine	The engine object to be used as internal parent
 */
ProcessingEngineNode::ProcessingEngineNode(ProcessingEngineNode::NodeListener* listener)
	: ProcessingEngineNode()
{
	AddListener(listener);
}

/**
 * Destructor
 */
ProcessingEngineNode::~ProcessingEngineNode()
{
	Stop();
}

/**
 * Method to register a listener object to be called when the node has received the respective data via a node protocol.
 * @param listener	The listener object to add to the internal list of listeners
 */
void ProcessingEngineNode::AddListener(ProcessingEngineNode::NodeListener* listener)
{
	if (listener)
		m_listeners.push_back(listener);
}

/**
 * Getter for the id of this processing node object
 *
 * @return The id of this processing node object
 */
NodeId ProcessingEngineNode::GetId()
{
	return m_nodeId;
}

/**
 * Getter for the id of the thread this node object instance is using.
 * 
 * @return	The thread id of this node object instance.
 */
Thread::ThreadID ProcessingEngineNode::GetNodeThreadId()
{
	return getThreadId();
}

/**
 * Starts the node object and therefor the internal protocol processing objects as well.
 *
 * @return	True if both internal processing objects have been started successfully. False if not.
 */
bool ProcessingEngineNode::Start()
{
	// start our thread loop
	startThread();

	// Startup protocols
	bool successfullyStartedA = m_typeAProtocols.size() > 0;
	bool successfullyStartedB = true;

	for (auto const& protocolA : m_typeAProtocols)
	{
		successfullyStartedA = (protocolA.second && protocolA.second->Start());

		if (!successfullyStartedA)
			break;
	}

	for (auto const& protocolB : m_typeBProtocols)
	{
		successfullyStartedB = (protocolB.second && protocolB.second->Start());

		if (!successfullyStartedB)
			break;
	}

	// if one of the protocol processors did not start successfully,
	// enshure the other is not running without purpose
	m_nodeRunning = successfullyStartedA && successfullyStartedB && m_threadRunning.wait(1);
	if (!m_nodeRunning)
		Stop();

	return m_nodeRunning;
}

/**
 * Stops the node object and therefor the internal protocol processing objects as well.
 *
 * @return	True if both internal processing objects have been stopped successfully. False if not.
 */
bool ProcessingEngineNode::Stop()
{
	// Shutdown protocols
	bool successfullyStoppedA = true;
	bool successfullyStoppedB = true;

	for (auto const& typeAProtocol : m_typeAProtocols)
		successfullyStoppedA = successfullyStoppedA && typeAProtocol.second->Stop();

	for (auto const& typeBProtocol : m_typeBProtocols)
		successfullyStoppedB = successfullyStoppedB && typeBProtocol.second->Stop();

	// reset the running bool indicator
	auto protocolsRunning = !(successfullyStoppedA && successfullyStoppedB);

	// wait for thread termination
	auto threadShutdownSuccess = stopThread(100);

	// clear pending messages from queue
	m_messageQueue.clear();

	m_nodeRunning = !(!protocolsRunning && threadShutdownSuccess);

	return !m_nodeRunning;
}

/**
 * Gets the current running state bool value.
 *
 * @return	True if internal processing is running. False if not.
 */
bool ProcessingEngineNode::IsRunning()
{
	return m_nodeRunning;
}

/**
 *
 */
std::unique_ptr<XmlElement> ProcessingEngineNode::createStateXml()
{
	auto nodeXmlElement = std::make_unique<XmlElement>(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE));
	nodeXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID), static_cast<int>(m_nodeId));

	if (m_dataHandling)
	{
		auto dataHandlingXmlElement = m_dataHandling->createStateXml();
		if (dataHandlingXmlElement)
			nodeXmlElement->addChildElement(dataHandlingXmlElement.release());
	}
	
    return nodeXmlElement;
}

/**
 * Setter for the configuration of this node object.
 * Includes initializing the internal node id that is used to access relevant config info of the given config object.
 *
 * @param config	The application configuration object to use to access config data
 * @param NId	The node id to use for this node object and to access data from config object
 */
bool ProcessingEngineNode::setStateXml(XmlElement* stateXml)
{
	bool shouldBeRunning = m_nodeRunning;

	Stop();

	if (!stateXml || stateXml->getTagName() != ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE))
		return false;

	auto retVal = true;

	m_nodeId = stateXml->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID));

	auto objectHandlingStateXml = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::OBJECTHANDLING));
	if (objectHandlingStateXml)
	{
		auto &ohmName = objectHandlingStateXml->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE));
		m_dataHandling = std::unique_ptr<ObjectDataHandling_Abstract>(CreateObjectDataHandling(ProcessingEngineConfig::ObjectHandlingModeFromString(ohmName)));
		if (m_dataHandling)
			m_dataHandling->setStateXml(objectHandlingStateXml);
		else
			retVal = false;
	}

	std::vector<int> protocolIdsInNewConfig;
	// this relies on the first protocol xml element being of type A - if the first is of typeB, it will be lost...
	XmlElement* protocolXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLA));
	while (protocolXmlElement != nullptr)
	{
		// create the protocol processing objects of correct type as defined in config
		auto protocolId = protocolXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID));
		protocolIdsInNewConfig.push_back(protocolId);
		auto protocolType = ProcessingEngineConfig::ProtocolTypeFromString(protocolXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::TYPE)));
		auto hostPort = 0;
		auto hostPortXmlElement = protocolXmlElement->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::HOSTPORT));
		if (hostPortXmlElement)
			hostPort = hostPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT));

		auto protocolExistsAsA = (m_typeAProtocols.count(protocolId) != 0);
		auto protocolExistsAsB = (m_typeBProtocols.count(protocolId) != 0);
		auto protocolExists = protocolExistsAsA || protocolExistsAsB;
		auto currentProtocolTypeMatches = false;
		if (protocolExists)
		{
			if (protocolExistsAsA)
				currentProtocolTypeMatches = (m_typeAProtocols.at(protocolId)->GetType() == protocolType);
			else if (protocolExistsAsB)
				currentProtocolTypeMatches = (m_typeBProtocols.at(protocolId)->GetType() == protocolType);
		}

		if (protocolExists && currentProtocolTypeMatches)
		{
			if (protocolExistsAsA)
			{
				m_typeAProtocols.at(protocolId)->setStateXml(protocolXmlElement);

				// add the protocolnodetype A to datahandlings' list of ProtocolIds for A protocols
				if (m_dataHandling)
					m_dataHandling->AddProtocolAId(protocolId);
				else
					retVal = false;
			}
			else if (protocolExistsAsB)
			{
				m_typeBProtocols.at(protocolId)->setStateXml(protocolXmlElement);

				// add the protocolnodetype A to datahandlings' list of ProtocolIds for A protocols
				if (m_dataHandling)
					m_dataHandling->AddProtocolBId(protocolId);
				else
					retVal = false;
			}
		}
		else
		{
			auto protocol = std::unique_ptr<ProtocolProcessorBase>(CreateProtocolProcessor(protocolType, hostPort));

			// set up the protocol processing objects of correct type as defined in config
			if (protocol)
			{
				protocol->AddListener(this);
				protocol->setStateXml(protocolXmlElement);
				if (protocol->GetRole() == ProtocolRole::PR_A)
				{
					m_typeAProtocols.insert(std::make_pair(protocolId, std::move(protocol)));

					// add the protocolnodetype A to datahandlings' list of ProtocolIds for A protocols
					if (m_dataHandling)
						m_dataHandling->AddProtocolAId(protocolId);
					else
						retVal = false;
				}
				else if (protocol->GetRole() == ProtocolRole::PR_B)
				{
					m_typeBProtocols.insert(std::make_pair(protocolId, std::move(protocol)));

					// add the protocolnodetype B to datahandlings' list of ProtocolIds for B protocols
					if (m_dataHandling)
						m_dataHandling->AddProtocolBId(protocolId);
					else
						retVal = false;
				}

			}
			else
				retVal = false;
		}

		// now search the next type A or B protocol xml element for next loop
		auto nextProtocolXmlElement = protocolXmlElement->getNextElement();
		protocolXmlElement = nullptr;
		while (nextProtocolXmlElement)
		{
			if (nextProtocolXmlElement == nullptr)
			{
				break;
			}
			if (nextProtocolXmlElement->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLA))
			{
				protocolXmlElement = nextProtocolXmlElement;
				break;
			}
			else if (nextProtocolXmlElement->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLB))
			{
				protocolXmlElement = nextProtocolXmlElement;
				break;
			}

			nextProtocolXmlElement = nextProtocolXmlElement->getNextElement();
		}
	}

	// cleanup no longer used protocols
	std::vector<ProtocolId> protocolAIdsToRemove;
	for (auto const& pA : m_typeAProtocols)
		if (std::find(protocolIdsInNewConfig.begin(), protocolIdsInNewConfig.end(), pA.first) == protocolIdsInNewConfig.end())
			protocolAIdsToRemove.push_back(pA.first);
	for (auto id : protocolAIdsToRemove)
		m_typeAProtocols.erase(id);

	std::vector<ProtocolId> protocolBIdsToRemove;
	for (auto const& pB : m_typeBProtocols)
		if (std::find(protocolIdsInNewConfig.begin(), protocolIdsInNewConfig.end(), pB.first) == protocolIdsInNewConfig.end())
			protocolBIdsToRemove.push_back(pB.first);
	for (auto id : protocolBIdsToRemove)
		m_typeBProtocols.erase(id);

	// restore running state after config has been applied
	if(shouldBeRunning)
		Start();

	return retVal;
}

/**
 * Method that creates the protocol processing object corresponding to the given type.
 *
 * @param type					The protocol type to create the corresponding processor for
 * @param listenerPortNumber	The port for the processor to listen on for client interaction
 * @return	The created processor object
 */
ProtocolProcessorBase *ProcessingEngineNode::CreateProtocolProcessor(ProtocolType type, int listenerPortNumber)
{
	switch(type)
	{
		case PT_OSCProtocol:
			return new OSCProtocolProcessor(m_nodeId, listenerPortNumber);
		case PT_OCAProtocol:
			return new OCAProtocolProcessor(m_nodeId);
		case PT_RTTrPMProtocol:
			return new RTTrPMProtocolProcessor(m_nodeId, listenerPortNumber);
		case PT_MidiProtocol:
			return new MIDIProtocolProcessor(m_nodeId);
		case PT_YamahaOSCProtocol:
			return new YmhOSCProtocolProcessor(m_nodeId, listenerPortNumber);
		case PT_ADMOSCProtocol:
			return new ADMOSCProtocolProcessor(m_nodeId, listenerPortNumber);
		case PT_RemapOSCProtocol:
			return new RemapOSCProtocolProcessor(m_nodeId, listenerPortNumber);
		default:
			return nullptr;
	}
}

/**
 * Method that creates the data handling object corresponding to the given handling mode.
 *
 * @param mode	The data handling mode to create the corresponding handling object for
 * @return	The created data handling object
 */
ObjectDataHandling_Abstract* ProcessingEngineNode::CreateObjectDataHandling(ObjectHandlingMode mode)
{
	switch (mode)
	{
	case OHM_Reverse_B_to_A_only:
		return new Reverse_B_to_A_only(this);
	case OHM_Forward_A_to_B_only:
		return new Forward_A_to_B_only(this);
	case OHM_Forward_only_valueChanges:
		return new Forward_only_valueChanges(this);
	case OHM_Remap_A_X_Y_to_B_XY:
		return new Remap_A_X_Y_to_B_XY_Handling(this);
	case OHM_Mux_nA_to_mB:
		return new Mux_nA_to_mB(this);
	case OHM_Bypass:
		return new BypassHandling(this);
	case OHM_DS100_DeviceSimulation:
		return new DS100_DeviceSimulation(this);
	case OHM_Mux_nA_to_mB_withValFilter:
		return new Mux_nA_to_mB_withValFilter(this);
	case OHM_Mirror_dualA_withValFilter:
		return new Mirror_dualA_withValFilter(this);
	case OHM_A1active_withValFilter:
		return new A1active_withValFilter(this);
	case OHM_A2active_withValFilter:
		return new A2active_withValFilter(this);
	case OHM_Invalid:
	default:
		return nullptr;
	}
}

/**
 * Convenience method to externally access object data handling.
 * This is meant to be used to be able to access custom data handling objects that allow
 * specialized interaction, e.g. visualization data interfacing.
 * 
 * @return	The object data hadnling object pointer or nullptr if none exists.
 */
ObjectDataHandling_Abstract* ProcessingEngineNode::GetObjectDataHandling()
{
	return m_dataHandling.get();
}

/**
 * Method to handle incoming message data from the processing protocol objects (they are members of the node object).
 * This is achieved by the member processing protocol objects accessing their parent with this handling method.
 *
 * @param receiver	The protocol processing object that has received the message
 * @param id		The message object id that corresponds to the received message
 * @param msgData	The actual message data that was received
 */
void ProcessingEngineNode::OnProtocolMessageReceived(ProtocolProcessorBase* receiver, RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData)
{
	m_messageQueue.enqueueMessage(InterProtocolMessage(this->GetId(), receiver->GetId(), receiver->GetType(), id, msgData));
}

/**
 * Method to forward a message to member protocol with given id
 *
 * @param PId		The id of the protocol to send the RemoteObject to
 * @param Id		The message object id that corresponds to the message to be sent
 * @param msgData	The actual message data that was received
 */
bool ProcessingEngineNode::SendMessageTo(ProtocolId PId, RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData) const
{
	if (m_typeAProtocols.count(PId))
		return m_typeAProtocols.at(PId)->SendRemoteObjectMessage(Id, msgData);
	else if (m_typeBProtocols.count(PId))
		return m_typeBProtocols.at(PId)->SendRemoteObjectMessage(Id, msgData);
	else
		return false;
}

/**
 * Reimplmented from MessageListener.
 * @param msg	The message data to handle.
 */
void ProcessingEngineNode::handleMessage(const Message& msg)
{
	if (auto* callbackMessage = dynamic_cast<const NodeCallbackMessage*> (&msg))
	{
		// broadcast received data to all listeners
		for (auto listener : m_listeners)
			listener->HandleNodeData(callbackMessage);
	}
}

/**
 * Main thread loop reimplementation from JUCE thread.
 * This implementation waits for InterprotocolMessages being posted to
 * internal message queue, dequeues them and posts them as NodeCallbackMessage
 * to JUCE's message queue to asynchronously be forwarded to subscribed listeners.
 * Afterwards, the message is synchronously handed over to datahandling module.
 */
void ProcessingEngineNode::run()
{
	InterProtocolMessage protocolMessage;

	while (!threadShouldExit())
	{
		m_threadRunning.signal();
		if (m_messageQueue.waitForMessage(25) && m_messageQueue.dequeueMessage(protocolMessage))
		{
			// send the message data to any listeners - asynchronous
			postMessage(new NodeCallbackMessage(protocolMessage));

			// perform internal bridging forwarding of message - synchronous
			auto isBridgingObject = (protocolMessage._Id < ROI_BridgingMAX);
			auto isOHMCtrlObject = (protocolMessage._Id == ROI_RemoteProtocolBridge_GetAllKnownValues);
			if (m_dataHandling && (isBridgingObject || isOHMCtrlObject))
				m_dataHandling->OnReceivedMessageFromProtocol(protocolMessage._senderProtocolId, protocolMessage._Id, protocolMessage._msgData);
		}
	}

	m_threadRunning.reset();
}

/**
 * Protected getter for the internal list of type A protocols.
 * This is useful for moc'ing the ProcessingEngineNode in a UnitTesting environment
 * and e.g. exposing the method access publicly in this special scenario.
 * @return	The internal list of type A protocols.
 */
std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>& ProcessingEngineNode::GetTypeAProtocols()
{
	return m_typeAProtocols;
}

/**
 * Protected getter for the internal list of type B protocols.
 * This is useful for moc'ing the ProcessingEngineNode in a UnitTesting environment
 * and e.g. exposing the method access publicly in this special scenario.
 * @return	The internal list of type B protocols.
 */
std::map<ProtocolId, std::unique_ptr<ProtocolProcessorBase>>& ProcessingEngineNode::GetTypeBProtocols()
{
	return m_typeBProtocols;
}

/**
 * Protected setter for the internal id of this node, ususally set through the incoming
 * stateXml when the node is set up regularly.
 * This is useful for moc'ing the ProcessingEngineNode in a UnitTesting environment
 * and e.g. exposing the method access publicly in this special scenario.
 * @param	id	The nodeId value to set to internal member variable.
 */
void ProcessingEngineNode::SetId(const NodeId id)
{
	m_nodeId = id;
}
