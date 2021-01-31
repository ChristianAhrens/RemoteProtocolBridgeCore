/*
  ==============================================================================

	YmhOSCProtocolProcessor.cpp
	Created: 18 Dec 2020 07:55:00am
	Author:  Christian Ahrens

  ==============================================================================
*/

#include "YmhOSCProtocolProcessor.h"


// **************************************************************************************
//    class YmhOSCProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
YmhOSCProtocolProcessor::YmhOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
	: OSCProtocolProcessor(parentNodeId, listenerPortNumber)
{
	m_type = ProtocolType::PT_YamahaOSCProtocol;
}

/**
 * Destructor
 */
YmhOSCProtocolProcessor::~YmhOSCProtocolProcessor()
{
}


/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool YmhOSCProtocolProcessor::setStateXml(XmlElement* stateXml)
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
bool YmhOSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	// do not send any values if they relate to a mapping id differing from the one configured for this protocol
	if (msgData._addrVal._second != m_mappingAreaId)
		return false;
	// do not send any values if the addressing is not sane (ymh osc messages always use src #)
	if (msgData._addrVal._first <= INVALID_ADDRESS_VALUE)
		return false;

	// assemble the addressing string
	String addressString = GetRemoteObjectDomainString() + String(msgData._addrVal._first) + GetRemoteObjectParameterTypeString(Id);

	return SendAddressedMessage(addressString, msgData);
}

/**
 * Called when the OSCReceiver receives a new OSC message and parses its contents to
 * pass the received data to parent node for further handling
 *
 * @param message			The received OSC message.
 * @param senderIPAddress	The ip the message originates from.
 * @param senderPort			The port this message was received on.
 */
void YmhOSCProtocolProcessor::oscMessageReceived(const OSCMessage& message, const String& senderIPAddress, const int& senderPort)
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

	RemoteObjectMessageData newMsgData;
	newMsgData._addrVal._first = INVALID_ADDRESS_VALUE;
	newMsgData._addrVal._second = INVALID_ADDRESS_VALUE;
	newMsgData._valType = ROVT_NONE;
	newMsgData._valCount = 0;
	newMsgData._payload = 0;
	newMsgData._payloadSize = 0;
	RemoteObjectIdentifier newObjectId = ROI_Invalid;

	String addressString = message.getAddressPattern().toString();

	// Handle the incoming message contents.

	SourceId channelId = INVALID_ADDRESS_VALUE;
	MappingId recordId = INVALID_ADDRESS_VALUE;

	// sanity check
	if (!m_messageListener)
		return;

	// check if the osc message is actually one of yamaha domain type
	if (!addressString.startsWith(GetRemoteObjectDomainString()))
		return;

	// Determine which parameter was changed depending on the incoming message's address pattern.
	if (addressString.endsWith(GetRemoteObjectParameterTypeString(ROI_Positioning_SourceSpread)))
		newObjectId = ROI_Positioning_SourceSpread;
	else if (addressString.endsWith(GetRemoteObjectParameterTypeString(ROI_CoordinateMapping_SourcePosition_X)))
		newObjectId = ROI_CoordinateMapping_SourcePosition_X;
	else if (addressString.endsWith(GetRemoteObjectParameterTypeString(ROI_CoordinateMapping_SourcePosition_Y)))
		newObjectId = ROI_CoordinateMapping_SourcePosition_Y;
	else if (addressString.endsWith(GetRemoteObjectParameterTypeString(ROI_MatrixInput_ReverbSendGain)))
		newObjectId = ROI_MatrixInput_ReverbSendGain;
	else
		newObjectId = ROI_Invalid;

	// get the channel info if the object is supposed to provide it
	if (ProcessingEngineConfig::IsChannelAddressingObject(newObjectId))
	{
		// Parse the Channel ID
		channelId = static_cast<SourceId>((addressString.fromLastOccurrenceOf(GetRemoteObjectDomainString(), false, true)).getIntValue());
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
		recordId = static_cast<MappingId>(m_mappingAreaId);
	}

	newMsgData._addrVal._first = channelId;
	newMsgData._addrVal._second = recordId;
	newMsgData._valType = ROVT_FLOAT;

	switch (newObjectId)
	{
	// objects spread, rvsend, mappedpos xy are supported
	case ROI_MatrixInput_ReverbSendGain:
		{
		auto rsgR = ProcessingEngineConfig::GetRemoteObjectRange(newObjectId);
		createRangeMappedFloatMessageData(message, newMsgData, rsgR.getStart(), rsgR.getEnd());
		}
		break;
	case ROI_Positioning_SourceSpread:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
		createFloatMessageData(message, newMsgData);
		break;
	// all other remote objects are not supported by Yamaha OSC
	case ROI_Error_GnrlErr:
	case ROI_MatrixInput_Select:
	case ROI_MatrixInput_Mute:
	case ROI_MatrixInput_DelayEnable:
	case ROI_MatrixInput_EqEnable:
	case ROI_MatrixInput_Polarity:
	case ROI_MatrixNode_Enable:
	case ROI_MatrixNode_DelayEnable:
	case ROI_MatrixOutput_Mute:
	case ROI_MatrixOutput_DelayEnable:
	case ROI_MatrixOutput_EqEnable:
	case ROI_MatrixOutput_Polarity:
	case ROI_Positioning_SourceDelayMode:
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_ReverbInputProcessing_Mute:
	case ROI_ReverbInputProcessing_EqEnable:
	case ROI_Scene_Recall:
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
	case ROI_MatrixInput_Gain:
	case ROI_MatrixInput_Delay:
	case ROI_MatrixInput_LevelMeterPreMute:
	case ROI_MatrixInput_LevelMeterPostMute:
	case ROI_MatrixNode_Gain:
	case ROI_MatrixNode_Delay:
	case ROI_MatrixOutput_Gain:
	case ROI_MatrixOutput_Delay:
	case ROI_MatrixOutput_LevelMeterPreMute:
	case ROI_MatrixOutput_LevelMeterPostMute:
	case ROI_Positioning_SourcePosition_XY:
	case ROI_Positioning_SourcePosition_X:
	case ROI_Positioning_SourcePosition_Y:
	case ROI_Positioning_SourcePosition:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_RevebRearLevel:
	case ROI_ReverbInput_Gain:
	case ROI_ReverbInputProcessing_Gain:
	case ROI_ReverbInputProcessing_LevelMeter:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_Scene_SceneIndex:
	case ROI_Settings_DeviceName:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixInput_ChannelName:
	case ROI_MatrixOutput_ChannelName:
	case ROI_Scene_SceneName:
	case ROI_Scene_SceneComment:
	case ROI_Device_Clear:
	case ROI_Scene_Previous:
	case ROI_Scene_Next:
	default:
		jassertfalse;
		break;
	}

	// provide the received message to parent node
	if (m_messageListener)
		m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
}

/**
 * static method to get Yamaha OSC-domain specific preceding string
 * @return		The Yamaha specific OSC address preceding string
 */
String YmhOSCProtocolProcessor::GetRemoteObjectDomainString()
{
	return "/ymh/src/";
}

/**
 * static method to get Yamaha specific OSC address string parameter definition trailer
 *
 * @param id	The object id to get the OSC specific parameter def string for
 * @return		The parameter defining string, empty if not available
 */
String YmhOSCProtocolProcessor::GetRemoteObjectParameterTypeString(RemoteObjectIdentifier id)
{
	switch (id)
	{
	case ROI_Positioning_SourceSpread:
		return "/w";								// width parameter (0-1, can be mapped directly to SO spread)
	case ROI_CoordinateMapping_SourcePosition_X:
		return "/p";								// pan parameter (0-1, cartesian, can be mapped directly to X SO position)
	case ROI_CoordinateMapping_SourcePosition_Y:
		return "/d";								//distance parameter (0-1, cartesian, can be mapped directly to Y SO position)
	case ROI_MatrixInput_ReverbSendGain:
		return "/s";								// aux send (0-1, linear, need log and scale to -120 +24 to be in dB mapped to EnSpace send)
	case ROI_HeartbeatPong:
	case ROI_HeartbeatPing:
	case ROI_Settings_DeviceName:
	case ROI_Error_GnrlErr:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixInput_Select:
	case ROI_MatrixInput_Mute:
	case ROI_MatrixInput_Gain:
	case ROI_MatrixInput_Delay:
	case ROI_MatrixInput_DelayEnable:
	case ROI_MatrixInput_EqEnable:
	case ROI_MatrixInput_Polarity:
	case ROI_MatrixInput_ChannelName:
	case ROI_MatrixInput_LevelMeterPreMute:
	case ROI_MatrixInput_LevelMeterPostMute:
	case ROI_MatrixNode_Enable:
	case ROI_MatrixNode_Gain:
	case ROI_MatrixNode_DelayEnable:
	case ROI_MatrixNode_Delay:
	case ROI_MatrixOutput_Mute:
	case ROI_MatrixOutput_Gain:
	case ROI_MatrixOutput_Delay:
	case ROI_MatrixOutput_DelayEnable:
	case ROI_MatrixOutput_EqEnable:
	case ROI_MatrixOutput_Polarity:
	case ROI_MatrixOutput_ChannelName:
	case ROI_MatrixOutput_LevelMeterPreMute:
	case ROI_MatrixOutput_LevelMeterPostMute:
	case ROI_Positioning_SourceDelayMode:
	case ROI_Positioning_SourcePosition:
	case ROI_Positioning_SourcePosition_XY:
	case ROI_Positioning_SourcePosition_X:
	case ROI_Positioning_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_RevebRearLevel:
	case ROI_ReverbInput_Gain:
	case ROI_ReverbInputProcessing_Mute:
	case ROI_ReverbInputProcessing_Gain:
	case ROI_ReverbInputProcessing_LevelMeter:
	case ROI_ReverbInputProcessing_EqEnable:
	case ROI_Device_Clear:
	case ROI_Scene_Previous:
	case ROI_Scene_Next:
	case ROI_Scene_Recall:
	case ROI_Scene_SceneIndex:
	case ROI_Scene_SceneName:
	case ROI_Scene_SceneComment:
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
	default:
		jassertfalse;
		return "";
	}
}

/**
 * Helper method to fill a new remote object message data struct with data from an osc message.
 * This method reads a single float from osc message, maps it to the given min/max range and fills it into the message data struct.
 * @param messageInput	The osc input message to read from.
 * @param newMessageData	The message data struct to fill data into.
 */
void YmhOSCProtocolProcessor::createRangeMappedFloatMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData, float mappingRangeMin, float mappingRangeMax)
{
	if (messageInput.size() == 1)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax);

		newMessageData._valCount = 1;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = sizeof(float);
	}
	if (messageInput.size() == 2)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax);
		m_floatValueBuffer[1] = jmap(jlimit(0.0f, 1.0f, messageInput[1].getFloat32()), mappingRangeMin, mappingRangeMax);

		newMessageData._valCount = 2;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 2 * sizeof(float);
	}
	if (messageInput.size() == 3)
	{
		m_floatValueBuffer[0] = jmap(jlimit(0.0f, 1.0f, messageInput[0].getFloat32()), mappingRangeMin, mappingRangeMax);
		m_floatValueBuffer[1] = jmap(jlimit(0.0f, 1.0f, messageInput[1].getFloat32()), mappingRangeMin, mappingRangeMax);
		m_floatValueBuffer[2] = jmap(jlimit(0.0f, 1.0f, messageInput[2].getFloat32()), mappingRangeMin, mappingRangeMax);

		newMessageData._valCount = 3;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 3 * sizeof(float);
	}
}
