/*
  ==============================================================================

	ADMOSCProtocolProcessor.cpp
	Created: 17 May 2021 16:50:00pm
	Author:  Christian Ahrens

  ==============================================================================
*/

#include "ADMOSCProtocolProcessor.h"


// **************************************************************************************
//    class ADMOSCProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
ADMOSCProtocolProcessor::ADMOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
	: OSCProtocolProcessor(parentNodeId, listenerPortNumber)
{
	m_type = ProtocolType::PT_ADMOSCProtocol;
}

/**
 * Destructor
 */
ADMOSCProtocolProcessor::~ADMOSCProtocolProcessor()
{
}


/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool ADMOSCProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	if (!NetworkProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		auto mappingAreaXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::MAPPINGAREA));
		if (mappingAreaXmlElement)
		{
			m_mappingAreaId = static_cast<MappingAreaId>(mappingAreaXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID)));
			return true;
		}
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
bool ADMOSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	// do not send any values if they relate to a mapping id differing from the one configured for this protocol
	if (msgData._addrVal._second != m_mappingAreaId)
		return false;
	// do not send any values if the addressing is not sane (ymh osc messages always use src #)
	if (msgData._addrVal._first <= INVALID_ADDRESS_VALUE)
		return false;

	//// assemble the addressing string
	//String addressString = GetADMObjectDomainString() + String(msgData._addrVal._first) + GetADMObjectParameterTypeString(Id);
	//
	//return SendAddressedMessage(addressString, msgData);
	ignoreUnused(Id);
	return false;
}

/**
 * Called when the OSCReceiver receives a new OSC message and parses its contents to
 * pass the received data to parent node for further handling
 *
 * @param message			The received OSC message.
 * @param senderIPAddress	The ip the message originates from.
 * @param senderPort			The port this message was received on.
 */
void ADMOSCProtocolProcessor::oscMessageReceived(const OSCMessage& message, const String& senderIPAddress, const int& senderPort)
{
	ignoreUnused(senderPort);
	if (senderIPAddress != m_ipAddress)
	{
#ifdef DEBUG
		DBG("NId" + String(m_parentNodeId)
			+ " PId" + String(m_protocolProcessorId) + ": ignore unexpected OSC message from "
			+ senderIPAddress + " (" + m_ipAddress + " expected)");
#endif
		return;
	}

	// sanity check
	if (!m_messageListener)
		return;

	// Handle the incoming message contents.
	String addressString = message.getAddressPattern().toString();

	// check if the osc message is actually one of ADM domain type
	if (!addressString.startsWith(GetADMMessageDomainString()))
		return;
	else
		addressString = addressString.fromFirstOccurrenceOf(GetADMMessageDomainString(), false, false);

	// check if the message is a object message (no handling of object config implemented)
	if (addressString.startsWith(GetADMMessageTypeString(AMT_ObjectConfig)))
		return;

	// check if the message is a object message (no handling of object config implemented)
	if (!addressString.startsWith(GetADMMessageTypeString(AMT_Object)))
		return;
	else
	{
		// Determine which parameter was changed depending on the incoming message's address pattern.
		auto admObjectType = GetADMObjectType(addressString.fromFirstOccurrenceOf(GetADMMessageTypeString(AMT_Object), false, false));

		// process the admMessageType into an internal remoteobject for further handling
		RemoteObjectMessageData newMsgData;
		newMsgData._addrVal._first = INVALID_ADDRESS_VALUE;
		newMsgData._addrVal._second = INVALID_ADDRESS_VALUE;
		newMsgData._valType = ROVT_NONE;
		newMsgData._valCount = 0;
		newMsgData._payload = 0;
		newMsgData._payloadSize = 0;
		auto newObjectId = ROI_Invalid;

		auto floatRange = juce::Range<float>();
		
		// create the remote object to be forwarded to processing node for further processing
		switch (admObjectType)
		{
		case AOT_Azimuth:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Azimuth] = message[0].getFloat32();
			break;
		case AOT_Elevation:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Elevation] = message[0].getFloat32();
			break;
		case AOT_Distance:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Distance] = message[0].getFloat32();
			break;
		case AOT_AzimElevDist:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Azimuth] = message[0].getFloat32();
			m_objectValueCache[AOT_Elevation] = message[1].getFloat32();
			m_objectValueCache[AOT_Distance] = message[2].getFloat32();
			break;
		case AOT_Width:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Width] = message[0].getFloat32();
			break;
		case AOT_XPos:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_XPos] = message[0].getFloat32();
			newObjectId = ROI_CoordinateMapping_SourcePosition_Y;
			floatRange = ProcessingEngineConfig::GetRemoteObjectRange(newObjectId);
			createRangeMappedFloatMessageData(message, newMsgData, floatRange.getStart(), floatRange.getEnd(), std::vector<float>{ -1.0f });
			break;
		case AOT_YPos:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_YPos] = message[0].getFloat32();
			newObjectId = ROI_CoordinateMapping_SourcePosition_X;
			floatRange = ProcessingEngineConfig::GetRemoteObjectRange(newObjectId);
			createRangeMappedFloatMessageData(message, newMsgData, floatRange.getStart(), floatRange.getEnd());
			break;
		case AOT_ZPos:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_ZPos] = message[0].getFloat32();
			break;
		case AOT_XYZPos:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_XPos] = message[0].getFloat32();
			m_objectValueCache[AOT_YPos] = message[1].getFloat32();
			m_objectValueCache[AOT_ZPos] = message[2].getFloat32();
			break;
		case AOT_Gain:
			// cache the incoming unaltered value
			m_objectValueCache[AOT_Gain] = message[0].getFloat32();
			newObjectId = ROI_MatrixInput_ReverbSendGain;
			floatRange = ProcessingEngineConfig::GetRemoteObjectRange(newObjectId);
			createRangeMappedFloatMessageData(message, newMsgData, floatRange.getStart(), floatRange.getEnd());
			break;
		case AOT_CartesianCoords:
			break;
		case AOT_Invalid:
		default:
			jassertfalse;
			break;
		}

		auto channelId = static_cast<ChannelId>(INVALID_ADDRESS_VALUE);
		auto recordId = static_cast<RecordId>(INVALID_ADDRESS_VALUE);

		// get the channel info if the object is supposed to provide it
		if (ProcessingEngineConfig::IsChannelAddressingObject(newObjectId))
		{
			// Parse the Channel ID
			channelId = static_cast<ChannelId>((addressString.fromLastOccurrenceOf(GetADMMessageDomainString() + GetADMMessageTypeString(AMT_Object), false, true)).getIntValue());
			jassert(channelId > 0);
			if (channelId <= 0)
				return;
		}

		// If the received channel (source) is set to muted, return without further processing
		if (IsChannelMuted(channelId))
			return;

		// set the record info if the object needs it
		if (ProcessingEngineConfig::IsRecordAddressingObject(newObjectId))
		{
			recordId = static_cast<RecordId>(m_mappingAreaId);
		}

		newMsgData._addrVal._first = channelId;
		newMsgData._addrVal._second = recordId;

		// provide the received message to parent node
		if (m_messageListener)
			m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
	}
}

/**
 * static method to get ADM OSC-domain specific preceding string
 * @return		The ADM specific OSC address preceding string
 */
String ADMOSCProtocolProcessor::GetADMMessageDomainString()
{
	return "/adm/";
}

/**
 * static method to get ADM OSC specific object def string
 * @return		The ADM specific OSC object addressing string fragment
 */
String ADMOSCProtocolProcessor::GetADMMessageTypeString(ADMMessageType type)
{
	switch (type)
	{
	case AMT_ObjectConfig:
		return "config/obj/1/";
	case AMT_Object:
		return "obj/";
	case AOT_Invalid:
	default:
		jassertfalse;
		return "";
	}
}

/**
 * static method to get ADM specific OSC address string parameter definition trailer
 *
 * @param type	The message type to get the OSC specific parameter def string for
 * @return		The parameter defining string, empty if not available
 */
String ADMOSCProtocolProcessor::GetADMObjectTypeString(ADMObjectType type)
{
	switch (type)
	{
	case AOT_Azimuth:
		return "/azim";
	case AOT_Elevation:
		return "/elev";
	case AOT_Distance:
		return "/dist";
	case AOT_AzimElevDist:
		return "/aed";
	case AOT_Width:
		return "/w";			
	case AOT_XPos:
		return "/x";			
	case AOT_YPos:
		return "/y";			
	case AOT_ZPos:
		return "/z";
	case AOT_XYZPos:
		return "/xyz";
	case AOT_CartesianCoords:
		return "/cartesian";
	case AOT_Gain:
		return "/gain";			
	case AOT_Invalid:
	default:
		jassertfalse;
		return "";
	}
}

/**
 * static helper method to derive a ADM object type definition from a given string
 * @param	messageTypeString	The string to derive an ADM object type definition from
 * @return	The derived type or invalid if none was found
 */
ADMOSCProtocolProcessor::ADMObjectType ADMOSCProtocolProcessor::GetADMObjectType(const String& typeString)
{
	if (typeString.endsWith(GetADMObjectTypeString(AOT_Azimuth)))
		return AOT_Azimuth;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_Elevation)))
		return AOT_Elevation;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_Distance)))
		return AOT_Distance;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_AzimElevDist)))
		return AOT_AzimElevDist;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_Width)))
		return AOT_Width;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_XPos)))
		return AOT_XPos;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_YPos)))
		return AOT_YPos;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_ZPos)))
		return AOT_ZPos;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_XYZPos)))
		return AOT_XYZPos;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_CartesianCoords)))
		return AOT_CartesianCoords;
	else if (typeString.endsWith(GetADMObjectTypeString(AOT_Gain)))
		return AOT_Gain;
	else
		return AOT_Invalid;
}

/**
 * Helper method to fill a new remote object message data struct with data from an osc message.
 * This method reads a single float from osc message, maps it to the given min/max range and fills it into the message data struct.
 * @param messageInput	The osc input message to read from.
 * @param newMessageData	The message data struct to fill data into.
 * @param mappingRangeMin	Min range value to map onto
 * @param mappingRangeMax	Max range value to map onto
 * @param valueFactors		Set of factors to apply to the float values. Must be empty or equal to message value contents size to be taken into account!
 */
void ADMOSCProtocolProcessor::createRangeMappedFloatMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData, float mappingRangeMin, float mappingRangeMax, const std::vector<float>& valueFactors)
{
	if (messageInput.size() == 1)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 1 ? valueFactors.at(0) : 1.0f);

		newMessageData._valCount = 1;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = sizeof(float);
	}
	if (messageInput.size() == 2)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 2 ? valueFactors.at(0) : 1.0f);
		m_floatValueBuffer[1] = jmap(jlimit(0.0f, 1.0f, messageInput[1].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 2 ? valueFactors.at(1) : 1.0f);

		newMessageData._valCount = 2;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 2 * sizeof(float);
	}
	if (messageInput.size() == 3)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 3 ? valueFactors.at(0) : 1.0f);
		m_floatValueBuffer[1] = jmap(jlimit(0.0f, 1.0f, messageInput[1].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 3 ? valueFactors.at(1) : 1.0f);
		m_floatValueBuffer[2] = jmap(jlimit(0.0f, 1.0f, messageInput[2].getFloat32()), mappingRangeMin, mappingRangeMax) * (valueFactors.size() >= 3 ? valueFactors.at(2) : 1.0f);

		newMessageData._valCount = 3;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 3 * sizeof(float);
	}
}
