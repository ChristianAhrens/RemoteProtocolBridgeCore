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

#include "ProcessingEngine.h"

#include "ProcessingEngineConfig.h"
#include "../LoggingTarget_Interface.h"

// **************************************************************************************
//    class ProcessingEngine
// **************************************************************************************
/**
 * Constructor of central processing engine
 */
ProcessingEngine::ProcessingEngine()
{
	m_IsRunning = false;
	m_LoggingEnabled = false;
	m_logTarget = 0;
}

/**
 * Destructor of central processing engine
 */
ProcessingEngine::~ProcessingEngine()
{
}

/**
 * Initialization+Startup method for the engine.
 * It is responsible to create, configure and start the child nodes
 * and sets the interenal running flag to true if startup was successful.
 *
 * @return	True if startup was successful, otherwise false
 */
bool ProcessingEngine::Start()
{
	bool startSuccess = !m_ProcessingNodes.empty();

	for(auto const & node : m_ProcessingNodes)
		startSuccess = startSuccess && node.second->Start();

	if (startSuccess)
		m_IsRunning = true;

	return startSuccess;
}

/**
 * Shuts down the engine and clears the running flag.
 * This includes shutting down the child nodes as well.
 *
 * @return	True if stopping was successful, otherwise false
 */
bool ProcessingEngine::Stop()
{
	auto stopSuccess = true;

	for (auto const& node : m_ProcessingNodes)
		stopSuccess = (stopSuccess && node.second->Stop());

	m_IsRunning = false;

	return stopSuccess;
}

/**
 *
 */
std::unique_ptr<XmlElement> ProcessingEngine::createStateXml()
{
	return nullptr;
}

/**
 *
 */
bool ProcessingEngine::setStateXml(XmlElement* stateXml)
{
	XmlElement* rootChild = stateXml->getFirstChildElement();
	while (rootChild != nullptr)
	{

		if (rootChild->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE))
		{
			XmlElement* nodeSectionElement = rootChild;
			auto nodeId = nodeSectionElement->getIntAttribute("Id", -1);

			if(m_ProcessingNodes.count(nodeId) == 0)
			{
				m_ProcessingNodes.insert(std::make_pair(nodeId, std::make_unique<ProcessingEngineNode>(this)));
			}

			m_ProcessingNodes.at(nodeId)->setStateXml(nodeSectionElement);
		}
		else if (rootChild->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::GLOBALCONFIG))
		{
			// nothing to be done here, global config is not relevant for engine
		}
		else
			return false;

		rootChild = rootChild->getNextElement();
	}

	return true;
}

/**
 * Getter for the is running flag.
 *
 * @return	True if the engine is in running state, otherwise false
 */
bool ProcessingEngine::IsRunning()
{
	return m_IsRunning;
}

/**
 * Setter for internal logging enabled flag.
 *
 * @param enable	The state (en-/disable) to set the internal flag to
 */
void ProcessingEngine::SetLoggingEnabled(bool enable)
{
	m_LoggingEnabled = enable;
}

/**
 * Getter for internal logging enabled flag
 *
 * @return	True if enabled, false if not
 */
bool ProcessingEngine::IsLoggingEnabled()
{
	return m_LoggingEnabled;
}

/**
* Setter for logging target object to be used to push messages to
*
* @param logTarget	The target object for logging data
*/
void ProcessingEngine::SetLoggingTarget(LoggingTarget_Interface* logTarget)
{
	m_logTarget = logTarget;
}

/**
 * Method overloaded to enqueue logging data regarding message traffic in the nodes.
 *
 * @param nodeId				The node the logging data originates from
 * @param senderProtocolId		The protocol processor that has received the message
 * @param senderProtocolType	The protocol type of the receiving protocol
 * @param objectId					The remote object id the message refers to
 * @param msgData				The remote object message data of the object that is currently sent by a node and therefor forwarded here to be logged
 */
void ProcessingEngine::HandleNodeData(const ProcessingEngineNode::NodeCallbackMessage* callbackMessage)
{
	if (!IsLoggingEnabled())
		return;

	if (m_logTarget)
	{
		m_logTarget->AddLogData(callbackMessage->_protocolMessage._nodeId, callbackMessage->_protocolMessage._senderProtocolId, callbackMessage->_protocolMessage._senderProtocolType, callbackMessage->_protocolMessage._Id, callbackMessage->_protocolMessage._msgData);
	}
}

void ProcessingEngine::onConfigUpdated()
{
	auto config = ProcessingEngineConfig::getInstance();
	if (config != nullptr)
		setStateXml(config->getConfigState().get());
}