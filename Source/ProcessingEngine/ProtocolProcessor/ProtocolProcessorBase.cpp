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

#include "ProtocolProcessorBase.h"

#include "../ProcessingEngineNode.h"

// **************************************************************************************
//    class ProtocolProcessorBase
// **************************************************************************************
/**
 * @fn bool ProtocolProcessorBase::Start()
 * Pure virtual function to start the derived processor object
 */

/**
 * @fn bool ProtocolProcessorBase::Stop()
 * Pure virtual function to stop the derived processor object
 */

/**
 * @fn bool ProtocolProcessorBase::SendMessage(RemoteObjectIdentifier roi, RemoteObjectMessageData& msgData)
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * Pure virtual function to trigger sending a message by derived processor object
 */

/**
 * Constructor of abstract class ProtocolProcessorBase.
 */
ProtocolProcessorBase::ProtocolProcessorBase(const NodeId& parentNodeId)
	: m_messageListener(nullptr),
      m_type(ProtocolType::PT_Invalid),
      m_parentNodeId(parentNodeId),
	  m_protocolProcessorId(0),
	  m_protocolProcessorRole(ProtocolRole::PR_Invalid),
      m_IsRunning(false),
      m_activeRemoteObjectsInterval(ET_DefaultPollingRate)
{
}

/**
 * Destructor
 */
ProtocolProcessorBase::~ProtocolProcessorBase()
{

}

/**
 * Getter for the value cache object member.
 * @return	The requested value cache object ref.
 */
RemoteObjectValueCache& ProtocolProcessorBase::GetValueCache()
{
	return m_valueCache;
}

/**
 * Sets the message listener object to be used for callback on message received.
 *
 * @param messageListener	The listener object
 */
void ProtocolProcessorBase::AddListener(Listener *messageListener)
{
	m_messageListener = messageListener;
}	

/**
 * Sets the configuration data for the protocol processor object.
 *
 * @param stateXml	The configuration data to parse and set active
 * @return True on success, false on failure
 */
bool ProtocolProcessorBase::setStateXml(XmlElement* stateXml)
{
	if (!stateXml)
		return false;

	auto retVal = true;

	auto isProtocolTypeA = stateXml->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLA);
	auto isProtocolTypeB = stateXml->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLB);

	if (!isProtocolTypeA && !isProtocolTypeB)
		retVal = false;

	m_protocolProcessorRole = stateXml->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLA) ? ProtocolRole::PR_A : ProtocolRole::PR_B;

	m_protocolProcessorId = stateXml->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID));


	auto pollingIntervalXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::POLLINGINTERVAL));
	if (pollingIntervalXmlElement)
		SetActiveRemoteObjectsInterval(pollingIntervalXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::INTERVAL)));

	if (stateXml->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::USESACTIVEOBJ)) == 1)
	{
		auto activeObjsXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::ACTIVEOBJECTS));
		if (activeObjsXmlElement)
			SetRemoteObjectsActive(activeObjsXmlElement);
		else
			return false;

		// special handling for heartbeats - this shall always be activated if active object usage is set to true
		m_activeRemoteObjects.push_back(RemoteObject(ROI_HeartbeatPing, RemoteObjectAddressing()));
		if (!isTimerThreadRunning() && m_activeRemoteObjectsInterval > 0)
			startTimerThread(m_activeRemoteObjectsInterval);
	}

	auto mutedObjsXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::MUTEDOBJECTS));
	if (mutedObjsXmlElement)
		SetRemoteObjectsMuted(mutedObjsXmlElement);

	return retVal;
}

/**
 * Setter for the active objects polling interval member value.
 * @param	interval	The polling interval for active objects.
 */
void ProtocolProcessorBase::SetActiveRemoteObjectsInterval(int interval)
{
	m_activeRemoteObjectsInterval = interval;
}

/**
 * Getter for the active objects polling interval member value.
 * @return	The polling interval for active objects.
 */
int ProtocolProcessorBase::GetActiveRemoteObjectsInterval()
{
	return m_activeRemoteObjectsInterval;
}

/**
 * Setter for remote object to specifically activate.
 * For OSC processing this is used to activate internal polling
 * of the object values.
 * In case an empty list of objects is passed, polling is stopped and
 * the internal list is cleared.
 *
 * @param activeObjsXmlElement	The xml element that has to be parsed to get the object data
 */
void ProtocolProcessorBase::SetRemoteObjectsActive(XmlElement* activeObjsXmlElement)
{
	ScopedLock l(m_activeRemoteObjectsLock);
	ProcessingEngineConfig::ReadActiveObjects(activeObjsXmlElement, m_activeRemoteObjects);

	// Start timer callback if objects are to be polled
	if (m_IsRunning)
	{
		if (GetActiveRemoteObjects().size() > 0 && m_activeRemoteObjectsInterval > 0)
		{
			startTimerThread(m_activeRemoteObjectsInterval);
		}
		else
		{
			stopTimerThread();
		}
	}
}

/**
 * Setter for remote object channels to not forward for further processing.
 * This uses a helper method from engine config to get a list of
 * object ids into the corresponding internal member.
 *
 * @param mutedObjChsXmlElement	The xml element that has to be parsed to get the object data
 */
void ProtocolProcessorBase::SetRemoteObjectsMuted(XmlElement* mutedObjChsXmlElement)
{
	ProcessingEngineConfig::ReadMutedObjects(mutedObjChsXmlElement, m_mutedRemoteObjects);
}

/**
 * Getter for the mute state of a given remote object.
 * @param	roi	The remote object to look for.
 * @return	True if the channel is muted (contained in internal list of muted channels), false if not.
 */
bool ProtocolProcessorBase::IsRemoteObjectMuted(RemoteObject roi)
{
	return std::find(m_mutedRemoteObjects.begin(), m_mutedRemoteObjects.end(), roi) != m_mutedRemoteObjects.end();
}

/**
 * Getter for the type of this protocol processing object
 *
 * @return The type of this protocol processing object
 */
ProtocolType ProtocolProcessorBase::GetType()
{
	return m_type;
}

/**
 * Getter for the id of this protocol processing object
 *
 * @return The id of this protocol processing object
 */
ProtocolId ProtocolProcessorBase::GetId()
{
	return m_protocolProcessorId;
}

/**
 * Getter for the role of this protocol processing object
 *
 * @return The role of this protocol processing object
 */
ProtocolRole ProtocolProcessorBase::GetRole()
{
	return m_protocolProcessorRole;
}

/**
 * Timer callback function, which will be called at regular intervals to
 * send out OSC poll messages.
 */
void ProtocolProcessorBase::timerThreadCallback()
{
	RemoteObjectMessageData msgData;

	ScopedLock l(m_activeRemoteObjectsLock);
	for (auto const& obj : m_activeRemoteObjects)
	{
		msgData._addrVal = obj._Addr;
		SendRemoteObjectMessage(obj._Id, msgData);
	}
}


/**
 * Helper method to normalize a given value to a given range without clipping
 * @param	value				The value to normalize
 * @param	normalizationRange	The range to normalize the value to
 * @return	The normalized value.
 */
float ProtocolProcessorBase::NormalizeValueByRange(float value, const juce::Range<float>& normalizationRange)
{
	jassert(!normalizationRange.isEmpty());
	if (normalizationRange.isEmpty())
		return 0.0f;

	auto valueInRange = value - normalizationRange.getStart();
	auto normalizedInRangeValue = (valueInRange / normalizationRange.getLength());

	return normalizedInRangeValue;
}

/**
 * Helper method to map a normalized 0-1 value to a given range and optionally invert it.
 * @param	normalizedValue		The normalized 0-1 value to map to extrude to the given range.
 * @param	range				The range to map the value to.
 * @param	invert				Bool indicator if the value shall be inverted in addition to mapping to range.
 * @return	The mapped and optionally inverted value.
 */
float ProtocolProcessorBase::MapNormalizedValueToRange(float normalizedValue, const juce::Range<float>& range, bool invert)
{
	auto mappedValue = range.getStart() + normalizedValue * (range.getEnd() - range.getStart());

	if (invert)
	{
		auto invertedMappedValue = range.getStart() + (range.getEnd() - mappedValue);
		return invertedMappedValue;
	}
	else
	{
		return mappedValue;
	}
}

/**
 * Helper to map a given MessageData struct with a given incoming range to another MessageData struct and range.
 * @param	sourceData		The incoming MessageData struct
 * @param	sourceRange		The value range the incoming message data values refer to
 * @param	targetRange		The value range the outgoing message data values shall be mapped to
 * @param	targetType		The dataType the outgoing message data values shall be of
 * @param	targetData		The outgoing MessageData struct
 * @return	True if mapping was successful. False if the target datatype was not supported for range-mapping. In the latter case, the targetData output will be empty.
 */
bool ProtocolProcessorBase::MapMessageDataToTargetRangeAndType(const RemoteObjectMessageData& sourceData, const juce::Range<float>& sourceRange, const juce::Range<float>& targetRange, const RemoteObjectValueType targetType, RemoteObjectMessageData& targetData)
{
	// Normalize the incoming message data values against the incoming source range
	auto normalizedObjValues = std::vector<float>();
	switch (sourceData._valType)
	{
	case ROVT_FLOAT:
		jassert(sizeof(float) * sourceData._valCount == sourceData._payloadSize);
		for (auto i = 0; i < sourceData._valCount; i++)
		{
			auto objectValue = static_cast<float*>(sourceData._payload)[i];
			normalizedObjValues.push_back(NormalizeValueByRange(objectValue, sourceRange));
		}
		break;
	case ROVT_INT:
		jassert(sizeof(int) * sourceData._valCount == sourceData._payloadSize);
		for (auto i = 0; i < sourceData._valCount; i++)
		{
			auto objectValue = static_cast<int*>(sourceData._payload)[i];
			normalizedObjValues.push_back(NormalizeValueByRange(static_cast<float>(objectValue), sourceRange));
		}
		break;
	case ROVT_NONE:
	case ROVT_STRING:
	default:
		break;
	}

	// map the normalized message data on the incoming target range
	auto targetRangeMappedObjValues = std::vector<float>();
	for (auto const& normalizedValue : normalizedObjValues)
		targetRangeMappedObjValues.push_back(MapNormalizedValueToRange(normalizedValue, targetRange));

	// dump the mapped data into outgoing message data struct
	targetData = sourceData;
	switch (targetType)
	{
	case ROVT_FLOAT:
		targetData._valType = targetType;
		targetData._valCount = static_cast<std::uint16_t>(targetRangeMappedObjValues.size());
		targetData._payload = new float[targetData._valCount];
		targetData._payloadSize = sizeof(float) * targetData._valCount;
		for (auto i = 0; i < targetData._valCount; i++)
			static_cast<float*>(targetData._payload)[i] = targetRangeMappedObjValues.at(i);
		return true;
	case ROVT_INT:
		targetData._valType = targetType;
		targetData._valCount = static_cast<std::uint16_t>(targetRangeMappedObjValues.size());
		targetData._payload = new int[targetData._valCount];
		targetData._payloadSize = sizeof(int) * targetData._valCount;
		for (auto i = 0; i < targetData._valCount; i++)
			static_cast<int*>(targetData._payload)[i] = static_cast<int>(targetRangeMappedObjValues.at(i));
		return true;
	case ROVT_NONE:
	case ROVT_STRING:
	default:
		targetData._valType = ROVT_NONE;
		targetData._valCount = 0;
		targetData._payload = nullptr;
		targetData._payloadSize = 0;
		break;
	}

	return false;
}

/**
 * Getter for the internal list of remote objects to actively handle.
 * @return		The requested reference to internal list.
 */
const std::vector<RemoteObject>& ProtocolProcessorBase::GetActiveRemoteObjects()
{
	return m_activeRemoteObjects;
}
