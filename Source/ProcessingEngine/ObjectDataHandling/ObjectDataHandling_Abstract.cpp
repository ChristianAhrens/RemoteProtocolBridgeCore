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

#include "ObjectDataHandling_Abstract.h"

#include "../ProcessingEngineNode.h"
#include "../ProcessingEngineConfig.h"


// **************************************************************************************
//    class ObjectDataHandling_Abstract
// **************************************************************************************

/**
 * @fn void ObjectDataHandling_Abstract::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
 * @param PId	The protocol id that a message was received on
 * @param Id	The id of the remote object that was received
 * @param msgData	The message that was received
 * Pure virtual function to be implemented by data handling objects to handle received protocol data.
 */

/**
 * Constructor of abstract class ObjectDataHandling_Abstract.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
ObjectDataHandling_Abstract::ObjectDataHandling_Abstract(ProcessingEngineNode* parentNode)
{
	m_parentNode = parentNode;
	m_mode = ObjectHandlingMode::OHM_Invalid;
	m_protocolReactionTimeout = 1000;
}

/**
 * Destructor
 */
ObjectDataHandling_Abstract::~ObjectDataHandling_Abstract()
{
	for (auto const& listener : m_stateListeners)
	{
		listener->ClearProtocolState();
	}
}

/**
 *
 */
std::unique_ptr<XmlElement> ObjectDataHandling_Abstract::createStateXml()
{
	return nullptr;
}

/**
 * Sets the configuration for the protocol processor object.
 * 
 * @param config	The overall configuration object that can be used to query protocol specific config data from
 * @param NId		The node id of the parent node this protocol processing object is child of (needed to access data from config)
 */
bool ObjectDataHandling_Abstract::setStateXml(XmlElement* stateXml)
{
	if (!stateXml || stateXml->getTagName() != ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::OBJECTHANDLING))
		return false;
	
	startTimer(static_cast<int>(m_protocolReactionTimeout));

	return true;
}

/**
 * Method to add a new id of a protocol of typeA to internal list of typeA protocolIds
 *
 * @param PAId	Protocol Id of typeA protocol to add.
 */
void ObjectDataHandling_Abstract::AddProtocolAId(ProtocolId PAId)
{
	m_protocolAIds.push_back(PAId);
	SetChangedProtocolState(PAId, OHS_Protocol_Down);
}

/**
 * Method to add a new id of a protocol of typeB to internal list of typeB protocolIds
 *
 * @param PBId	Protocol Id of typeB protocol to add.
 */
void ObjectDataHandling_Abstract::AddProtocolBId(ProtocolId PBId)
{
	m_protocolBIds.push_back(PBId);
	SetChangedProtocolState(PBId, OHS_Protocol_Down);
}

/**
 * Method to clear internal lists of both typeA and typeB protocolIds.
 * Also clears the map of last-seen-active timestamps per protocol.
 */
void ObjectDataHandling_Abstract::ClearProtocolIds()
{
	m_protocolAIds.clear();
	m_protocolBIds.clear();

	ScopedLock	tsAccessLock(m_protocolReactionTSLock);
	m_protocolReactionTSMap.clear();
}

/**
 * Setter for the protocol reaction timeout value member.
 * @param	timeout		The new value to set as reaction timeout (ms)
 */
void ObjectDataHandling_Abstract::SetProtocolReactionTimeout(double timeout)
{
	m_protocolReactionTimeout = timeout;
}

/**
 * Getter for the protocol reaction timeout value member.
 * @return	The current value for reaction timeout (ms)
 */
double ObjectDataHandling_Abstract::GetProtocolReactionTimeout()
{
	return m_protocolReactionTimeout;
}

/**
 * Getter for the parentNode member.
 * @return The parentNode pointer, can be nullptr.
 */
const ProcessingEngineNode* ObjectDataHandling_Abstract::GetParentNode()
{
	return m_parentNode;
}

/**
 * Getter for the mode member.
 * @return The object handling mode of this object.
 */
ObjectHandlingMode ObjectDataHandling_Abstract::GetMode()
{
	return m_mode;
}

/**
 * Setter for the mode member.
 * @param mode The object handling mode to set for this object.
 */
void  ObjectDataHandling_Abstract::SetMode(ObjectHandlingMode mode)
{
	m_mode = mode;
}

/**
 * Getter for the parent node id member.
 * @return The parentNode Id.
 */
NodeId ObjectDataHandling_Abstract::GetParentNodeId()
{
	return m_parentNodeId;
}

/**
 * Getter for the type a protocols array member.
 * @return The type a protocols.
 */
const std::vector<ProtocolId>& ObjectDataHandling_Abstract::GetProtocolAIds()
{
	return m_protocolAIds;
}

/**
 * Getter for the type b protocols array member.
 * @return The type b protocols.
 */
const std::vector<ProtocolId>& ObjectDataHandling_Abstract::GetProtocolBIds()
{
	return m_protocolBIds;
}

/**
 * Method to distribute a changed protocol status to all registered listeners.
 * @param	id		The id of the protocol the state change refers to.
 * @param	state	The changed state value.
 */
void ObjectDataHandling_Abstract::SetChangedProtocolState(ProtocolId id, ObjectHandlingState state)
{
	for (auto const& listener : m_stateListeners)
	{
		listener->SetChangedProtocolState(id, state);
	}
}

/**
 * Adds an object derived from embedded statelistener class
 * to internal list of listener objects.
 * @param	listener	The listener object to add.
 */
void ObjectDataHandling_Abstract::AddStateListener(ObjectDataHandling_Abstract::StateListener* listener)
{
	m_stateListeners.push_back(listener);
}

/**
 * Removes an object derived from embedded statuslistener class
 * from internal list of listener objects.
 * @param	listener	The listener object to add.
 */
void ObjectDataHandling_Abstract::RemoveStateListener(ObjectDataHandling_Abstract::StateListener* listener)
{
	auto listenerIter = std::find(m_stateListeners.begin(), m_stateListeners.end(), listener);
	if (listenerIter != m_stateListeners.end())
		m_stateListeners.erase(listenerIter);
}

/**
 * Reimplemented from Timer to update the status of all active protocols.
 * Currently this implementation only takes care of online status based on TS map.
 */
void ObjectDataHandling_Abstract::timerCallback()
{
	for (auto const& id : GetProtocolAIds())
	{
		ScopedLock	tsAccessLock(m_protocolReactionTSLock);
		// if the protocol is not present in map, continue
		if (m_protocolReactionTSMap.count(id) <= 0)
			continue;
		// if the protocol has not received data in more than what is specified in timeouttime, set it to down
		else if (Time::getMillisecondCounterHiRes() - GetLastProtocolReactionTSMap().at(id) > m_protocolReactionTimeout)
			SetChangedProtocolState(id, OHS_Protocol_Down);
	}
	for (auto const& id : GetProtocolBIds())
	{
		ScopedLock	tsAccessLock(m_protocolReactionTSLock);
		// if the protocol is not present in map, continue
		if (m_protocolReactionTSMap.count(id) <= 0)
			continue;
		// if the protocol has not received data in more than what is specified in timeouttime, set it to down
		else if (Time::getMillisecondCounterHiRes() - GetLastProtocolReactionTSMap().at(id) > m_protocolReactionTimeout)
			SetChangedProtocolState(id, OHS_Protocol_Down);
	}
}

/**
 * Method to update internal 'last seen' timestamp map for a given protocol.
 * This also updates protocol status according to timestamping info.
 * @param	id	The protocol to update the online state for
 */
void ObjectDataHandling_Abstract::UpdateOnlineState(ProtocolId id)
{
	ScopedLock	tsAccessLock(m_protocolReactionTSLock);

	// if the protocol is not present in map and therefor seems to have initially received data, set its status to UP
	if (m_protocolReactionTSMap.count(id) <= 0)
		SetChangedProtocolState(id, OHS_Protocol_Up);
	// if the protocol is receiving data the first time after 1s of silence, set its status to UP
	else if (Time::getMillisecondCounterHiRes() - GetLastProtocolReactionTSMap().at(id) > m_protocolReactionTimeout)
		SetChangedProtocolState(id, OHS_Protocol_Up);

	m_protocolReactionTSMap[id] = Time::getMillisecondCounterHiRes();
}

/**
 * Getter for the internal hash of last-seen timestamps per protocol
 * @return	The requested map reference
 */
const std::map<ProtocolId, double>& ObjectDataHandling_Abstract::GetLastProtocolReactionTSMap()
{
	// no locking of m_lastProtocolReactionTSLock here, since this supposed to only be called from the node processing thread

	return m_protocolReactionTSMap;
}

