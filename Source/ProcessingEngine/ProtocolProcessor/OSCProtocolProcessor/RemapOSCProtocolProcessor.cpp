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
					auto ROId = static_cast<RemoteObjectIdentifier>(i);
					if (oscRemappingXmlElement->getTagName() == ProcessingEngineConfig::GetObjectDescription(ROId).removeCharacters(" "))
					{
						auto oscRemappingTextElement = oscRemappingXmlElement->getFirstChildElement();
						if (oscRemappingTextElement && oscRemappingTextElement->isTextElement())
						{
							juce::String remapPattern = oscRemappingTextElement->getText();
							m_oscRemappings.insert(std::make_pair(ROId, remapPattern));
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
 * @param Id		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 */
bool RemapOSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	// do not send any values if the config forbids data sending
	if (m_dataSendindDisabled)
		return false;

	float floatValueSendBuffer[3] = { 0.0f, 0.0f, 0.0f };
	RemoteObjectMessageData remappedMsgData;

	// assemble the addressing string
	String addressString;// = GetADMMessageDomainString() + GetADMMessageTypeString(AMT_Object) + String(msgData._addrVal._first) + GetADMObjectTypeString(targetObjType);
	
	return SendAddressedMessage(addressString, remappedMsgData);
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

	RemoteObjectIdentifier newMsgId;
	RemoteObjectMessageData newMsgData;

	// Handle the incoming message contents.
	String addressString = message.getAddressPattern().toString();

	for (auto const& remapping : m_oscRemappings)
	{
		if (IsMatchingRemapping(remapping.second, addressString))
		{
			newMsgId = remapping.first;
			break;
		}
	}

	//// check if the osc message is actually one of ADM domain type
	//if (!addressString.startsWith(GetADMMessageDomainString()))
	//	return;
	//else
	//	addressString = addressString.fromFirstOccurrenceOf(GetADMMessageDomainString(), false, false);
	//
	//// check if the message is a object message (no handling of object config implemented)
	//if (addressString.startsWith(GetADMMessageTypeString(AMT_ObjectConfig)))
	//	return;
	//
	//// check if the message is a object message (no handling of object config implemented)
	//if (!addressString.startsWith(GetADMMessageTypeString(AMT_Object)))
	//	return;
	//else
	//{
	//	// Determine which parameter was changed depending on the incoming message's address pattern.
	//	auto admObjectType = GetADMObjectType(addressString.fromFirstOccurrenceOf(GetADMMessageTypeString(AMT_Object), false, false));
	//
	//	// process the admMessageType into an internal remoteobjectid for further handling
	//	auto targetedObjectId = ROI_Invalid;
	//	switch (admObjectType)
	//	{
	//	case AOT_Azimuth:
	//	case AOT_Elevation:
	//	case AOT_Distance:
	//	case AOT_AzimElevDist:
	//	case AOT_XPos:
	//	case AOT_YPos:
	//	case AOT_ZPos:
	//	case AOT_XYZPos:
	//		targetedObjectId = ROI_CoordinateMapping_SourcePosition_XY;
	//		break;
	//	case AOT_Width:
	//		targetedObjectId = ROI_Positioning_SourceSpread;
	//		break;
	//	case AOT_Gain:
	//		targetedObjectId = ROI_MatrixInput_Gain;
	//		break;
	//	case AOT_CartesianCoords:
	//	case AOT_Invalid:
	//	default:
	//		jassertfalse;
	//		break;
	//	}
	//
	//	auto channel = static_cast<ChannelId>(INVALID_ADDRESS_VALUE);
	//	auto record = static_cast<RecordId>(INVALID_ADDRESS_VALUE);
	//
	//	// get the channel info if the object is supposed to provide it
	//	if (ProcessingEngineConfig::IsChannelAddressingObject(targetedObjectId))
	//	{
	//		// Parse the Channel ID
	//		channel = static_cast<ChannelId>((addressString.fromLastOccurrenceOf(GetADMMessageTypeString(AMT_Object), false, true)).getIntValue());
	//		jassert(channel > 0);
	//		if (channel <= 0)
	//			return;
	//	}
	//
	//	// set the record info if the object needs it
	//	if (ProcessingEngineConfig::IsRecordAddressingObject(targetedObjectId))
	//		record = static_cast<RecordId>(m_mappingAreaId);
	//
	//	// assemble a remote object structure from the collected addressing data
	//	auto remoteObject = RemoteObject(targetedObjectId, RemoteObjectAddressing(channel, record));
	//	
	//	// create the remote object to be forwarded to processing node for further processing
	//	switch (admObjectType)
	//	{
	//	case AOT_Azimuth:
	//		WriteToObjectCache(channel, AOT_Azimuth, message[0].getFloat32(), true);
	//		break;
	//	case AOT_Elevation:
	//		WriteToObjectCache(channel, AOT_Elevation, message[0].getFloat32(), true);
	//		break;
	//	case AOT_Distance:
	//		WriteToObjectCache(channel, AOT_Distance, message[0].getFloat32(), true);
	//		break;
	//	case AOT_AzimElevDist:
	//		WriteToObjectCache(channel, std::vector<ADMObjectType>{AOT_Azimuth, AOT_Elevation, AOT_Distance}, std::vector<float>{message[0].getFloat32(), message[1].getFloat32(), message[2].getFloat32()}, true);
	//		break;
	//	case AOT_XPos:
	//		WriteToObjectCache(channel, AOT_XPos, message[0].getFloat32(), true);
	//		break;
	//	case AOT_YPos:
	//		WriteToObjectCache(channel, AOT_YPos, message[0].getFloat32(), true);
	//		break;
	//	case AOT_ZPos:
	//		WriteToObjectCache(channel, AOT_ZPos, message[0].getFloat32(), true);
	//		break;
	//	case AOT_XYZPos:
	//		WriteToObjectCache(channel, std::vector<ADMObjectType>{AOT_XPos, AOT_YPos, AOT_ZPos}, std::vector<float>{message[0].getFloat32(), message[1].getFloat32(), message[2].getFloat32()}, true);
	//		break;
	//	case AOT_Width:
	//		WriteToObjectCache(channel, AOT_Width, message[0].getFloat32());
	//		break;
	//	case AOT_Gain:
	//		WriteToObjectCache(channel, AOT_Gain, message[0].getFloat32());
	//		break;
	//	case AOT_CartesianCoords:
	//		if (SetExpectedCoordinateSystem(message[0].getInt32() == 1))
	//		{
	//			if (message[0].getInt32() == 1) // switch to cartesian coordinates - we take this as opportunity to sync current polar data to cartesian once
	//				SyncCachedPolarToCartesianValues(channel);
	//			else if (message[0].getInt32() == 0) // switch to polar coordinates - we take this as opportunity to sync current cartesian data to polar once
	//				SyncCachedCartesianToPolarValues(channel);
	//		}
	//		break;
	//	case AOT_Invalid:
	//	default:
	//		jassertfalse;
	//		break;
	//	}
	//
	//	// If the received object is set to muted, return without further processing
	//	if (IsRemoteObjectMuted(remoteObject))
	//		return;
	//
	//	// create a new message in internally known format
	//	auto newMsgData = RemoteObjectMessageData(remoteObject._Addr, ROVT_FLOAT, 0, nullptr, 0);
	//	if (!CreateMessageDataFromObjectCache(remoteObject._Id, channel, newMsgData))
	//		return;
	//
	//	// and provide that message to parent node
	//	if (m_messageListener)
	//		m_messageListener->OnProtocolMessageReceived(this, targetedObjectId, newMsgData);
	//}
}

/**
 * Protected helper to match a given osc string against a given remapping pattern
 * (containing e.g. wildcard placeholders, blanks, etc.)
 * @param	remapPattern	The pattern to match a given string against.
 * @param	patternToMatch	The string to match against the pattern.
 * @return	True if the matching was positive, false if not.
 */
bool RemapOSCProtocolProcessor::IsMatchingRemapping(const juce::String& remapPattern, const juce::String& oscStringToMatch)
{
	return false;
}

/**
 * Helper method to normalize a given value to a given range without clipping
 * @param	value				The value to normalize
 * @param	normalizationRange	The range to normalize the value to
 * @return	The normalized value.
 */
float RemapOSCProtocolProcessor::NormalizeValueByRange(float value, const juce::Range<float>& normalizationRange)
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
float RemapOSCProtocolProcessor::MapNormalizedValueToRange(float normalizedValue, const juce::Range<float>& range, bool invert)
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
