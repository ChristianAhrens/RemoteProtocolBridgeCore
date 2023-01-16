/*
  ==============================================================================

	RemapOSCProtocolProcessor.cpp
	Created: 28 Dec 2022 15:26:00pm
	Author:  Christian Ahrens

  ==============================================================================
*/

#include "RemapOSCProtocolProcessor.h"


// **************************************************************************************
//    class RemapOSCProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
RemapOSCProtocolProcessor::RemapOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
	: OSCProtocolProcessor(parentNodeId, listenerPortNumber)
{
	m_type = ProtocolType::PT_RemapOSCProtocol;
}

/**
 * Destructor
 */
RemapOSCProtocolProcessor::~RemapOSCProtocolProcessor()
{
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool RemapOSCProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	if (!NetworkProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		auto oscRemappingsXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::REMAPPINGS));
		if (oscRemappingsXmlElement)
		{
			m_oscRemappings.clear();
			auto oscRemappingXmlElement = oscRemappingsXmlElement->getFirstChildElement();
			while (nullptr != oscRemappingXmlElement)
			{
				for (int i = ROI_Invalid + 1; i < ROI_BridgingMAX; ++i)
				{
					auto roid = static_cast<RemoteObjectIdentifier>(i);
					if (oscRemappingXmlElement->getTagName() == ProcessingEngineConfig::GetObjectDescription(roid).removeCharacters(" "))
					{
						auto oscRemappingTextElement = oscRemappingXmlElement->getFirstChildElement();
						if (oscRemappingTextElement && oscRemappingTextElement->isTextElement())
						{
							auto remapPattern = oscRemappingTextElement->getText();
							auto minVal = static_cast<float>(oscRemappingXmlElement->getDoubleAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MINVALUE)));
							auto maxVal = static_cast<float>(oscRemappingXmlElement->getDoubleAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MAXVALUE)));
							m_oscRemappings.insert(std::make_pair(roid, std::make_pair(remapPattern, juce::Range<float>(minVal, maxVal))));
						}
					}
				}

				oscRemappingXmlElement = oscRemappingXmlElement->getNextElement();
			}
		}
		
		auto dataSendingDisabledXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATASENDINGDISABLED));
		if (dataSendingDisabledXmlElement)
			m_dataSendindDisabled = 1 == dataSendingDisabledXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::STATE));
		
		if (oscRemappingsXmlElement && dataSendingDisabledXmlElement)
			return true;
		else
			return false;
	}
}

/**
 * Method to trigger sending of a message
 *
 * @param roid		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 */
bool RemapOSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier roid, const RemoteObjectMessageData& msgData)
{
	// do not send any values if the config forbids data sending
	if (m_dataSendindDisabled)
		return false;

	// we cannot send anything if no remapping for the given roi is configured
	if (m_oscRemappings.count(roid) < 1)
		return false;

	// assemble the addressing string
	auto addressString = m_oscRemappings.at(roid).first;
	if (addressString.contains(juce::StringRef("%1")))
	{
		addressString.replace("%1", "%i");
		addressString = juce::String::formatted(addressString, msgData._addrVal._first);
	}
	if (addressString.contains(juce::StringRef("%2")))
	{
		addressString.replace("%2", "%i");
		addressString = juce::String::formatted(addressString, msgData._addrVal._second);
	}

	// map the value range to target range
	auto remapRange = m_oscRemappings.at(roid).second;
	if (!remapRange.isEmpty())
	{
		RemoteObjectMessageData remappedData;
		if (!MapMessageDataToTargetRangeAndType(msgData, ProcessingEngineConfig::GetRemoteObjectRange(roid), remapRange, RemoteObjectValueType::ROVT_NONE, remappedData))
			return false;
		else
			return SendAddressedMessage(addressString, remappedData);
	}
	// or just send the data if no range is specified
	else
		return SendAddressedMessage(addressString, msgData);
}

/**
 * Called when the OSCReceiver receives a new OSC message and parses its contents to
 * pass the received data to parent node for further handling
 *
 * @param message			The received OSC message.
 * @param senderIPAddress	The ip the message originates from.
 * @param senderPort		The port this message was received on.
 */
void RemapOSCProtocolProcessor::oscMessageReceived(const OSCMessage& message, const juce::String& senderIPAddress, const int& senderPort)
{
	ignoreUnused(senderPort);
	if (senderIPAddress.toStdString() != GetIpAddress())
	{
#ifdef DEBUG
		DBG("NId" + String(m_parentNodeId)
			+ " PId" + String(m_protocolProcessorId) + ": ignore unexpected OSC message from "
			+ senderIPAddress + " (" + GetIpAddress() + " expected)");
#endif
		return;
	}

	// sanity check
	if (!m_messageListener)
		return;

	RemoteObjectIdentifier newObjectId = ROI_Invalid;
	ChannelId channelId = INVALID_ADDRESS_VALUE;
	RecordId recordId = INVALID_ADDRESS_VALUE;
	RemoteObjectMessageData newMsgData;

	// Match the incoming message contents against known mappings and derive the associated remote obejct id accordingly.
	auto addressString = message.getAddressPattern().toString();
	juce::Range<float>		valueRange;
	for (auto const& remapping : m_oscRemappings)
	{
		if (IsMatchingRemapping(remapping.second.first, addressString))
		{
			newObjectId = remapping.first;

			ExtractAddressingFromRemapping(remapping.second.first, addressString, channelId, recordId);

			valueRange = remapping.second.second;

			break;
		}
	}

	// if the incoming addressString could not be matched with any known mapping to remote object id, we cannot proceed
	if (ROI_Invalid == newObjectId)
		return;

	// Special handling for ping/pong (empty msg contents in any case)
	if (ROI_HeartbeatPong == newObjectId)
		m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPong, newMsgData);
	else if (ROI_HeartbeatPing == newObjectId)
		m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPing, newMsgData);
	else
	{
		// Invalidate channelid addressing if the object does not use channel info
		if (!ProcessingEngineConfig::IsChannelAddressingObject(newObjectId))
			channelId = INVALID_ADDRESS_VALUE;
		// or set a valid default channelid if the object requires it but none was received
		else if (channelId == INVALID_ADDRESS_VALUE)
			channelId = 1;

		// Invalidate recordid addressing if the object does not use record info
		if (!ProcessingEngineConfig::IsRecordAddressingObject(newObjectId))
			recordId = INVALID_ADDRESS_VALUE;
		// or set a valid default recordid if the object requires it but none was received
		else if (recordId == INVALID_ADDRESS_VALUE)
			recordId = 1;

		// If the received channel (source) is set to muted, return without further processing
		if (IsRemoteObjectMuted(RemoteObject(newObjectId, RemoteObjectAddressing(channelId, recordId))))
			return;

		newMsgData._addrVal._first = channelId;
		newMsgData._addrVal._second = recordId;

		if (valueRange.isEmpty())
			createMessageData(message, newObjectId, newMsgData);
		else
		{
			OSCMessage modMsg = message; // clone incoming const to be able to modify arguments
			modMsg.clear();
			for (auto const& arg : message)
			{
				if (arg.isFloat32())
					modMsg.addFloat32(MapNormalizedValueToRange(NormalizeValueByRange(arg.getFloat32(), valueRange), ProcessingEngineConfig::GetRemoteObjectRange(newObjectId)));
				else if (arg.isInt32())
					modMsg.addFloat32(MapNormalizedValueToRange(NormalizeValueByRange(static_cast<float>(arg.getInt32()), valueRange), ProcessingEngineConfig::GetRemoteObjectRange(newObjectId)));
				else
					DBG(String(__FUNCTION__) + String(" value range based mapping is supported for float/int values only."));
			}

			createMessageData(modMsg, newObjectId, newMsgData);
		}

		// provide the received message to parent node
		if (m_messageListener)
			m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
	}
}

/**
 * Static method to split the given remapPattern into three pieces, 
 * based on the contained '%' ch/rec format placeholder characters.
 * @param	remapPattern	The input string to dissect.
 * @param	startSection	The first part of the string up until the first occurance of '%i'
 * @param	firstSeparator	The first occuring '%i' string
 * @param	middleSection	The middle part of the string, framed by the two '%i' (or just one)
 * @param	secondSeparator	The second occuring '%i' string
 * @param	endSection		The last part of the string from the last occurance of '%i' on.
 */
void RemapOSCProtocolProcessor::DissectRemappingPattern(const juce::String& remapPattern, juce::String& startSection, juce::String& firstSparator, juce::String& middleSection, juce::String& secondSparator, juce::String& endSection)
{
	startSection = remapPattern.upToFirstOccurrenceOf("%", false, false);
	firstSparator = remapPattern.fromFirstOccurrenceOf("%", true, false).substring(0, 2);
	middleSection = remapPattern.fromFirstOccurrenceOf("%", false, false).upToFirstOccurrenceOf("%", false, false).substring(1);
	secondSparator = remapPattern.fromFirstOccurrenceOf("%", false, false).substring(1).fromFirstOccurrenceOf("%", true, false).substring(0, 2);
	endSection = remapPattern.fromLastOccurrenceOf("%", false, false).substring(1);
}

/**
 * Protected helper to match a given osc string against a given remapping pattern
 * (containing one of, both or none of "%1" and "%2")
 * @param	remapPattern	The pattern to match a given string against.
 * @param	patternToMatch	The string to match against the pattern.
 * @return	True if the matching was positive, false if not.
 */
bool RemapOSCProtocolProcessor::IsMatchingRemapping(const juce::String& remapPattern, const juce::String& oscStringToMatch)
{
	juce::String startSection, firstSparator, middleSection, secondSparator, endSection;
	DissectRemappingPattern(remapPattern, startSection, firstSparator, middleSection, secondSparator, endSection);
#ifdef DEBUG
	auto startMatch = oscStringToMatch.startsWith(startSection);
	auto endMatch = oscStringToMatch.endsWith(endSection);
	auto middleMatch = oscStringToMatch.contains(middleSection);

	return startMatch && endMatch && middleMatch;
#else
	return oscStringToMatch.startsWith(startSection) && oscStringToMatch.endsWith(endSection) && oscStringToMatch.contains(middleSection);
#endif
}

/**
 * Protected helper to match a given osc string against a given remapping pattern
 * (containing one of, both or none of "%1" and "%2")
 * @param	remapPattern			The pattern to use as reference for extracting addressing values.
 * @param	oscStringToExtractFrom	The string to extract addressing from.
 * @param	channelId				The extracted channel addressing
 * @param	recordId				The extracted record addressing
 * @return	True if the extraction delivered useful values, false if not.
 */
bool RemapOSCProtocolProcessor::ExtractAddressingFromRemapping(const juce::String& remapPattern, const juce::String& oscStringToExtractFrom, ChannelId& channelId, RecordId& recordId)
{
	juce::String startSection, firstSparator, middleSection, secondSparator, endSection;
	DissectRemappingPattern(remapPattern, startSection, firstSparator, middleSection, secondSparator, endSection);

	auto middleSectionInclSeparators = oscStringToExtractFrom.substring(startSection.length(), oscStringToExtractFrom.length() - endSection.length());
	juce::String firstSeparatorVal = middleSectionInclSeparators.upToFirstOccurrenceOf(middleSection, false, false);
	juce::String secondSeparatorVal = middleSectionInclSeparators.fromFirstOccurrenceOf(middleSection, false, false);

	if (firstSparator == "%1" || secondSparator == "%2")
	{
		if (firstSeparatorVal.isNotEmpty())
			channelId = static_cast<ChannelId>(firstSeparatorVal.getIntValue());
		else
			channelId = INVALID_ADDRESS_VALUE;

		if (secondSeparatorVal.isNotEmpty())
			recordId = static_cast<RecordId>(secondSeparatorVal.getIntValue());
		else
			recordId = INVALID_ADDRESS_VALUE;

		return true;
	}
	else if (firstSparator == "%2" || secondSparator == "%1")
	{
		if (secondSeparatorVal.isNotEmpty())
			channelId = static_cast<ChannelId>(secondSeparatorVal.getIntValue());
		else
			channelId = INVALID_ADDRESS_VALUE;

		if (firstSeparatorVal.isNotEmpty())
			recordId = static_cast<RecordId>(firstSeparatorVal.getIntValue());
		else
			recordId = INVALID_ADDRESS_VALUE;

		return true;
	}
	else
	{
		channelId = INVALID_ADDRESS_VALUE;
		recordId = INVALID_ADDRESS_VALUE;

		return false;
	}
}
