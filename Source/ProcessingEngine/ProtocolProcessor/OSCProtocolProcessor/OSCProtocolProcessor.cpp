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

#include "OSCProtocolProcessor.h"

#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class OSCProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
OSCProtocolProcessor::OSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
	: NetworkProtocolProcessorBase(parentNodeId), m_oscReceiver(listenerPortNumber)
{
	m_type = ProtocolType::PT_OSCProtocol;
	m_oscMsgRate = ET_DefaultPollingRate;

	// OSCProtocolProcessor derives from OSCReceiver::Listener
	m_oscReceiver.addListener(this);
}

/**
 * Destructor
 */
OSCProtocolProcessor::~OSCProtocolProcessor()
{
	Stop();

	m_oscReceiver.removeListener(this);
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool OSCProtocolProcessor::Start()
{
	bool successS = false;
	bool successR = false;

	// Connect both sender and receiver  
	successS = m_oscSender.connect(m_ipAddress, m_clientPort);
	jassert(successS);

	successR = m_oscReceiver.connect();
	jassert(successR);

	m_IsRunning = (successS && successR);

	return m_IsRunning;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool OSCProtocolProcessor::Stop()
{
	m_IsRunning = false;

	bool successS = false;
	bool successR = false;

	// Connect both sender and receiver  
	successS = m_oscSender.disconnect();
	jassert(successS);

	successR = m_oscReceiver.disconnect();
	jassert(successR);

	return (successS && successR);
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool OSCProtocolProcessor::setStateXml(XmlElement *stateXml)
{
	if (!NetworkProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		if (stateXml->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::USESACTIVEOBJ)) == 1)
		{
			auto activeObjsXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::ACTIVEOBJECTS));
			if (activeObjsXmlElement)
				SetRemoteObjectsActive(activeObjsXmlElement);
			else
				return false;
		}

		auto pollingIntervalXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::POLLINGINTERVAL));
		if (pollingIntervalXmlElement)
		{
			m_oscMsgRate = pollingIntervalXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::INTERVAL));
			return true;
		}
		else
			return false;
	}
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
void OSCProtocolProcessor::SetRemoteObjectsActive(XmlElement* activeObjsXmlElement)
{
	ProcessingEngineConfig::ReadActiveObjects(activeObjsXmlElement, m_activeRemoteObjects);

	// Start timer callback if objects are to be polled
	if (m_activeRemoteObjects.size() > 0)
	{
		startTimer(m_oscMsgRate);
	}
	else
	{
		stopTimer();
	}
}

/**
 * Setter for remote object channels to not forward for further processing.
 * This uses a helper method from engine config to get a list of 
 * object ids into the corresponding internal member.
 * 
 * @param mutedObjChsXmlElement	The xml element that has to be parsed to get the object data
 */
void OSCProtocolProcessor::SetRemoteObjectChannelsMuted(XmlElement* mutedObjChsXmlElement)
{
	ProcessingEngineConfig::ReadMutedObjectChannels(mutedObjChsXmlElement, m_mutedRemoteObjectChannels);
}

/**
 * Method to trigger sending of a message
 *
 * @param Id		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 */
bool OSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	if (!m_IsRunning)
		return false;

	bool sendSuccess = false;

	String addressString = GetRemoteObjectString(Id);

	if (msgData.addrVal.second != INVALID_ADDRESS_VALUE)
		addressString += String::formatted("/%d", msgData.addrVal.second);

	if (msgData.addrVal.first != INVALID_ADDRESS_VALUE)
		addressString += String::formatted("/%d", msgData.addrVal.first);

	uint16 valSize;
	switch (msgData.valType)
	{
	case ROVT_INT:
		valSize = sizeof(int);
		break;
	case ROVT_FLOAT:
		valSize = sizeof(float);
		break;
	case ROVT_STRING:
		jassertfalse; // String not (yet?) supported
		valSize = 0;
		break;
	case ROVT_NONE:
	default:
		valSize = 0;
		break;
	}

	jassert((msgData.valCount*valSize) == msgData.payloadSize);

	switch (msgData.valType)
	{
	case ROVT_INT:
		{
		jassert(msgData.valCount < 4); // max known d&b OSC msg val cnt would be positioning xyz
		int multivalues[3];

		for (int i = 0; i < msgData.valCount; ++i)
			multivalues[i] = ((int*)msgData.payload)[i];

		if (msgData.valCount == 1)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0]));
		else if (msgData.valCount == 2)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0], multivalues[1]));
		else if (msgData.valCount == 3)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0], multivalues[1], multivalues[2]));
		else
			sendSuccess = m_oscSender.send(OSCMessage(addressString));
		}
		break;
	case ROVT_FLOAT:
		{
		jassert(msgData.valCount < 4); // max known d&b OSC msg val cnt would be positioning xyz
		float multivalues[3];

		for (int i = 0; i < msgData.valCount; ++i)
			multivalues[i] = ((float*)msgData.payload)[i];

		if (msgData.valCount == 1)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0]));
		else if (msgData.valCount == 2)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0], multivalues[1]));
		else if (msgData.valCount == 3)
			sendSuccess = m_oscSender.send(OSCMessage(addressString, multivalues[0], multivalues[1], multivalues[2]));
		else
			sendSuccess = m_oscSender.send(OSCMessage(addressString));
		}
		break;
	case ROVT_NONE:
		sendSuccess = m_oscSender.send(OSCMessage(addressString));
		break;
	case ROVT_STRING:
	default:
		break;
	}

	return sendSuccess;
}

/**
* Called when the OSCReceiver receives a new OSC bundle.
* The bundle is processed and all contained individual messages passed on
* to oscMessageReceived for further handling.
*
* @param bundle				The received OSC bundle.
* @param senderIPAddress	The ip the bundle originates from.
* @param senderPort			The port this bundle was received on.
*/
void OSCProtocolProcessor::oscBundleReceived(const OSCBundle &bundle, const String& senderIPAddress, const int& senderPort)
{
	if (senderIPAddress != m_ipAddress)
	{
#ifdef DEBUG
		DBG("NId"+String(m_parentNodeId) 
			+ " PId"+String(m_protocolProcessorId) + ": ignore unexpected OSC bundle from " 
			+ senderIPAddress + " (" + m_ipAddress + " expected)");
#endif
		return;
	}

	for (int i = 0; i < bundle.size(); ++i)
	{
		if (bundle[i].isBundle())
			oscBundleReceived(bundle[i].getBundle(), senderIPAddress, senderPort);
		else if (bundle[i].isMessage())
			oscMessageReceived(bundle[i].getMessage(), senderIPAddress, senderPort);
	}
}

/**
 * Called when the OSCReceiver receives a new OSC message and parses its contents to
 * pass the received data to parent node for further handling
 *
 * @param message			The received OSC message.
* @param senderIPAddress	The ip the message originates from.
* @param senderPort			The port this message was received on.
 */
void OSCProtocolProcessor::oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort)
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
	newMsgData.addrVal.first = INVALID_ADDRESS_VALUE;
	newMsgData.addrVal.second = INVALID_ADDRESS_VALUE;
	newMsgData.valType = ROVT_NONE;
	newMsgData.valCount = 0;
	newMsgData.payload = 0;
	newMsgData.payloadSize = 0;

	String addressString = message.getAddressPattern().toString();
	// Check if the incoming message is a response to a sent "ping" heartbeat.
	if (addressString.startsWith(GetRemoteObjectString(ROI_HeartbeatPong)) && m_messageListener)
		m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPong, newMsgData);
	// Check if the incoming message is a response to a sent "pong" heartbeat.
	else if (addressString.startsWith(GetRemoteObjectString(ROI_HeartbeatPing)) && m_messageListener)
		m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPing, newMsgData);
	// Handle the incoming message contents.
	else
	{
		RemoteObjectIdentifier newObjectId = ROI_Invalid;
		SourceId channelId = INVALID_ADDRESS_VALUE;
		MappingId recordId = INVALID_ADDRESS_VALUE;

		// Determine which parameter was changed depending on the incoming message's address pattern.
		if (addressString.startsWith(GetRemoteObjectString(ROI_Settings_DeviceName)))
			newObjectId = ROI_Settings_DeviceName;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Error_GnrlErr)))
			newObjectId = ROI_Error_GnrlErr;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Error_ErrorText)))
			newObjectId = ROI_Error_ErrorText;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Status_StatusText)))
			newObjectId = ROI_Status_StatusText;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Select)))
			newObjectId = ROI_MatrixInput_Select;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Mute)))
			newObjectId = ROI_MatrixInput_Mute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Gain)))
			newObjectId = ROI_MatrixInput_Gain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Delay)))
			newObjectId = ROI_MatrixInput_Delay;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_DelayEnable)))
			newObjectId = ROI_MatrixInput_DelayEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_EqEnable)))
			newObjectId = ROI_MatrixInput_EqEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Polarity)))
			newObjectId = ROI_MatrixInput_Polarity;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_ChannelName)))
			newObjectId = ROI_MatrixInput_ChannelName;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_LevelMeterPreMute)))
			newObjectId = ROI_MatrixInput_LevelMeterPreMute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_LevelMeterPostMute)))
			newObjectId = ROI_MatrixInput_LevelMeterPostMute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixNode_Enable)))
			newObjectId = ROI_MatrixNode_Enable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixNode_Gain)))
			newObjectId = ROI_MatrixNode_Gain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixNode_DelayEnable)))
			newObjectId = ROI_MatrixNode_DelayEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_Polarity)))
			newObjectId = ROI_MatrixInput_Polarity;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixNode_Delay)))
			newObjectId = ROI_MatrixNode_Delay;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_Mute)))
			newObjectId = ROI_MatrixOutput_Mute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_Gain)))
			newObjectId = ROI_MatrixOutput_Gain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_Delay)))
			newObjectId = ROI_MatrixOutput_Delay;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_DelayEnable)))
			newObjectId = ROI_MatrixOutput_DelayEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_EqEnable)))
			newObjectId = ROI_MatrixOutput_EqEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_Polarity)))
			newObjectId = ROI_MatrixOutput_Polarity;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_ChannelName)))
			newObjectId = ROI_MatrixOutput_ChannelName;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_LevelMeterPreMute)))
			newObjectId = ROI_MatrixOutput_LevelMeterPreMute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixOutput_LevelMeterPostMute)))
			newObjectId = ROI_MatrixOutput_LevelMeterPostMute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourceSpread)))
			newObjectId = ROI_Positioning_SourceSpread;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourceDelayMode)))
			newObjectId = ROI_Positioning_SourceDelayMode;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourcePosition_XY)))
			newObjectId = ROI_Positioning_SourcePosition_XY;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourcePosition_X)))
			newObjectId = ROI_Positioning_SourcePosition_X;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourcePosition_Y)))
			newObjectId = ROI_Positioning_SourcePosition_Y;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Positioning_SourcePosition)))
			newObjectId = ROI_Positioning_SourcePosition;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_CoordinateMapping_SourcePosition_XY)))
			newObjectId = ROI_CoordinateMapping_SourcePosition_XY;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_CoordinateMapping_SourcePosition_X)))
			newObjectId = ROI_CoordinateMapping_SourcePosition_X;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_CoordinateMapping_SourcePosition_Y)))
			newObjectId = ROI_CoordinateMapping_SourcePosition_Y;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_CoordinateMapping_SourcePosition)))
			newObjectId = ROI_CoordinateMapping_SourcePosition;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixSettings_ReverbRoomId)))
			newObjectId = ROI_MatrixSettings_ReverbRoomId;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixSettings_ReverbPredelayFactor)))
			newObjectId = ROI_MatrixSettings_ReverbPredelayFactor;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixSettings_RevebRearLevel)))
			newObjectId = ROI_MatrixSettings_RevebRearLevel;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_MatrixInput_ReverbSendGain)))
			newObjectId = ROI_MatrixInput_ReverbSendGain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_ReverbInput_Gain)))
			newObjectId = ROI_ReverbInput_Gain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_ReverbInputProcessing_Mute)))
			newObjectId = ROI_ReverbInputProcessing_Mute;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_ReverbInputProcessing_Gain)))
			newObjectId = ROI_ReverbInputProcessing_Gain;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_ReverbInputProcessing_LevelMeter)))
			newObjectId = ROI_ReverbInputProcessing_LevelMeter;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_ReverbInputProcessing_EqEnable)))
			newObjectId = ROI_ReverbInputProcessing_EqEnable;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Device_Clear)))
			newObjectId = ROI_Device_Clear;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_Previous)))
			newObjectId = ROI_Scene_Previous;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_Next)))
			newObjectId = ROI_Scene_Next;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_Recall)))
			newObjectId = ROI_Scene_Recall;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_SceneIndex)))
			newObjectId = ROI_Scene_SceneIndex;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_SceneName)))
			newObjectId = ROI_Scene_SceneName;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_Scene_SceneComment)))
			newObjectId = ROI_Scene_SceneComment;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_RemoteProtocolBridge_SoundObjectSelect)))
			newObjectId = ROI_RemoteProtocolBridge_SoundObjectSelect;
		else if (addressString.startsWith(GetRemoteObjectString(ROI_RemoteProtocolBridge_UIElementIndexSelect)))
			newObjectId = ROI_RemoteProtocolBridge_UIElementIndexSelect;
		else
			newObjectId = ROI_Invalid;

		if (IsChannelAddressingObject(newObjectId))
		{
			// Parse the Channel ID
			channelId = static_cast<SourceId>((addressString.fromLastOccurrenceOf("/", false, true)).getIntValue());
			jassert(channelId > 0);
			if (channelId <= 0)
				return;
		}

		if (IsRecordAddressingObject(newObjectId))
		{
			// Parse the Record ID
			addressString = addressString.upToLastOccurrenceOf("/", false, true);
			recordId = static_cast<MappingId>((addressString.fromLastOccurrenceOf("/", false, true)).getIntValue());
			jassert(recordId > 0);
			if (recordId <= 0)
				return;
		}

		// If the received channel (source) is set to muted, return without further processing
		if (m_mutedRemoteObjectChannels.contains(channelId))
			return;

		newMsgData.addrVal.first = channelId;
		newMsgData.addrVal.second = recordId;
		newMsgData.valType = ROVT_FLOAT;

		switch (newObjectId)
		{
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
			createIntMessageData(message, newMsgData);
			break;
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
		case ROI_Positioning_SourceSpread:
		case ROI_Positioning_SourcePosition_XY:
		case ROI_Positioning_SourcePosition_X:
		case ROI_Positioning_SourcePosition_Y:
		case ROI_Positioning_SourcePosition:
		case ROI_MatrixSettings_ReverbPredelayFactor:
		case ROI_MatrixSettings_RevebRearLevel:
		case ROI_MatrixInput_ReverbSendGain:
		case ROI_ReverbInput_Gain:
		case ROI_ReverbInputProcessing_Gain:
		case ROI_ReverbInputProcessing_LevelMeter:
		case ROI_CoordinateMapping_SourcePosition_XY:
		case ROI_CoordinateMapping_SourcePosition_X:
		case ROI_CoordinateMapping_SourcePosition_Y:
		case ROI_CoordinateMapping_SourcePosition:
			createFloatMessageData(message, newMsgData);
			break;
		case ROI_Scene_SceneIndex:
		case ROI_Settings_DeviceName:
		case ROI_Error_ErrorText:
		case ROI_Status_StatusText:
		case ROI_MatrixInput_ChannelName:
		case ROI_MatrixOutput_ChannelName:
		case ROI_Scene_SceneName:
		case ROI_Scene_SceneComment:
			createStringMessageData(message, newMsgData);
			break;
		case ROI_Device_Clear:
		case ROI_Scene_Previous:
		case ROI_Scene_Next:
			break;
		default:
			jassertfalse;
			break;
		}

		// provide the received message to parent node
		if (m_messageListener)
			m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
	}
}

/**
 * Private method to get OSC object specific ObjectName string
 *
 * @param id	The object id to get the OSC specific object name
 * @return		The OSC specific object name
 */
String OSCProtocolProcessor::GetRemoteObjectString(RemoteObjectIdentifier id)
{
	switch (id)
	{
	case ROI_HeartbeatPong:
		return "/pong";
	case ROI_HeartbeatPing:
		return "/ping";
	case ROI_Settings_DeviceName:
		return "/dbaudio1/settings/devicename";
	case ROI_Error_GnrlErr:
		return "/dbaudio1/error/gnrlerr";
	case ROI_Error_ErrorText:
		return "/dbaudio1/error/errortext";
	case ROI_Status_StatusText:
		return "/dbaudio1/status/statustext";
	case ROI_MatrixInput_Select:
		return "/dbaudio1/matrixinput/select";
	case ROI_MatrixInput_Mute:
		return "/dbaudio1/matrixinput/mute";
	case ROI_MatrixInput_Gain:
		return "/dbaudio1/matrixinput/gain";
	case ROI_MatrixInput_Delay:
		return "/dbaudio1/matrixinput/delay";
	case ROI_MatrixInput_DelayEnable:
		return "/dbaudio1/matrixinput/delayenable";
	case ROI_MatrixInput_EqEnable:
		return "/dbaudio1/matrixinput/eqenable";
	case ROI_MatrixInput_Polarity:
		return "/dbaudio1/matrixinput/polarity";
	case ROI_MatrixInput_ChannelName:
		return "/dbaudio1/matrixinput/channelname";
	case ROI_MatrixInput_LevelMeterPreMute:
		return "/dbaudio1/matrixinput/levelmeterpremute";
	case ROI_MatrixInput_LevelMeterPostMute:
		return "/dbaudio1/matrixinput/levelmeterpostmute";
	case ROI_MatrixNode_Enable:
		return "/dbaudio1/matrixnode/enable";
	case ROI_MatrixNode_Gain:
		return "/dbaudio1/matrixnode/gain";
	case ROI_MatrixNode_DelayEnable:
		return "/dbaudio1/matrixnode/delayenable";
	case ROI_MatrixNode_Delay:
		return "/dbaudio1/matrixnode/delay";
	case ROI_MatrixOutput_Mute:
		return "/dbaudio1/matrixoutput/mute";
	case ROI_MatrixOutput_Gain:
		return "/dbaudio1/matrixoutput/gain";
	case ROI_MatrixOutput_Delay:
		return "/dbaudio1/matrixoutput/delay";
	case ROI_MatrixOutput_DelayEnable:
		return "/dbaudio1/matrixoutput/delayenable";
	case ROI_MatrixOutput_EqEnable:
		return "/dbaudio1/matrixoutput/eqenable";
	case ROI_MatrixOutput_Polarity:
		return "/dbaudio1/matrixoutput/polarity";
	case ROI_MatrixOutput_ChannelName:
		return "/dbaudio1/matrixoutput/channelname";
	case ROI_MatrixOutput_LevelMeterPreMute:
		return "/dbaudio1/matrixoutput/levelmeterpremute";
	case ROI_MatrixOutput_LevelMeterPostMute:
		return "/dbaudio1/matrixoutput/levelmeterpostmute";
	case ROI_Positioning_SourceSpread:
		return "/dbaudio1/positioning/source_spread";
	case ROI_Positioning_SourceDelayMode:
		return "/dbaudio1/positioning/source_delaymode";
	case ROI_Positioning_SourcePosition:
		return "/dbaudio1/positioning/source_position";
	case ROI_Positioning_SourcePosition_XY:
		return "/dbaudio1/positioning/source_position_xy";
	case ROI_Positioning_SourcePosition_X:
		return "/dbaudio1/positioning/source_position_x";
	case ROI_Positioning_SourcePosition_Y:
		return "/dbaudio1/positioning/source_position_y";
	case ROI_CoordinateMapping_SourcePosition:
		return "/dbaudio1/coordinatemapping/source_position";
	case ROI_CoordinateMapping_SourcePosition_X:
		return "/dbaudio1/coordinatemapping/source_position_x";
	case ROI_CoordinateMapping_SourcePosition_Y:
		return "/dbaudio1/coordinatemapping/source_position_y";
	case ROI_CoordinateMapping_SourcePosition_XY:
		return "/dbaudio1/coordinatemapping/source_position_xy";
	case ROI_MatrixSettings_ReverbRoomId:
		return "/dbaudio1/matrixsettings/reverbroomid";
	case ROI_MatrixSettings_ReverbPredelayFactor:
		return "/dbaudio1/matrixsettings/reverbpredelayfactor";
	case ROI_MatrixSettings_RevebRearLevel:
		return "/dbaudio1/matrixsettings/reverbrearlevel";
	case ROI_MatrixInput_ReverbSendGain:
		return "/dbaudio1/matrixinput/reverbsendgain";
	case ROI_ReverbInput_Gain:
		return "/dbaudio1/reverbinput/gain";
	case ROI_ReverbInputProcessing_Mute:
		return "/dbaudio1/reverbinputprocessing/mute";
	case ROI_ReverbInputProcessing_Gain:
		return "/dbaudio1/reverbinputprocessing/gain";
	case ROI_ReverbInputProcessing_LevelMeter:
		return "/dbaudio1/reverbinputprocessing/levelmeter";
	case ROI_ReverbInputProcessing_EqEnable:
		return "/dbaudio1/reverbinputprocessing/eqenable";
	case ROI_Device_Clear:
		return "/dbaudio1/device/clear";
	case ROI_Scene_Previous:
		return "/dbaudio1/scene/previous";
	case ROI_Scene_Next:
		return "/dbaudio1/scene/next";
	case ROI_Scene_Recall:
		return "/dbaudio1/scene/recall";
	case ROI_Scene_SceneIndex:
		return "/dbaudio1/scene/sceneindex";
	case ROI_Scene_SceneName:
		return "/dbaudio1/scene/scenename";
	case ROI_Scene_SceneComment:
		return "/dbaudio1/scene/scenecomment";
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
		return "/RemoteProtocolBridge/SoundObjectSelect";
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
		return "/RemoteProtocolBridge/UIElementIndexSelect";
	default:
		return "";
	}
}

/**
 * Timer callback function, which will be called at regular intervals to
 * send out OSC poll messages.
 */
void OSCProtocolProcessor::timerCallback()
{
	int objectCount = m_activeRemoteObjects.size();
	for (int i = 0; i < objectCount; i++)
	{
		RemoteObjectMessageData msgData;
		msgData.addrVal = m_activeRemoteObjects[i].Addr;
		msgData.valCount = 0;
		msgData.valType = ROVT_NONE;
		msgData.payload = 0;
		msgData.payloadSize = 0;
		
		SendRemoteObjectMessage(m_activeRemoteObjects[i].Id, msgData);
	}
}

/**
 * Helper method to fill a new remote object message data struct with data from an osc message.
 * This method reads a single float from osc message and fills it into the message data struct.
 * @param messageInput	The osc input message to read from.
 * @param newMessageData	The message data struct to fill data into.
 */
void OSCProtocolProcessor::createFloatMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData)
{
	if (messageInput.size() == 1)
	{
		m_floatValueBuffer[0] = messageInput[0].getFloat32();

		newMessageData.valCount = 1;
		newMessageData.payload = m_floatValueBuffer;
		newMessageData.payloadSize = sizeof(float);
	}
	if (messageInput.size() == 2)
	{
		m_floatValueBuffer[0] = messageInput[0].getFloat32();
		m_floatValueBuffer[1] = messageInput[1].getFloat32();

		newMessageData.valCount = 2;
		newMessageData.payload = m_floatValueBuffer;
		newMessageData.payloadSize = 2 * sizeof(float);
	}
	if (messageInput.size() == 3)
	{
		m_floatValueBuffer[0] = messageInput[0].getFloat32();
		m_floatValueBuffer[1] = messageInput[1].getFloat32();
		m_floatValueBuffer[2] = messageInput[2].getFloat32();

		newMessageData.valCount = 3;
		newMessageData.payload = m_floatValueBuffer;
		newMessageData.payloadSize = 3 * sizeof(float);
	}
}

/**
 * Helper method to fill a new remote object message data struct with data from an osc message.
 * This method reads two ints from osc message and fills them into the message data struct.
 * @param messageInput	The osc input message to read from.
 * @param newMessageData	The message data struct to fill data into.
 */
void OSCProtocolProcessor::createIntMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData)
{
	if (messageInput.size() == 1)
	{
		// value be an int, but since some OSC appliances can only process floats,
		// we need to be prepared to optionally accept float as well
		if (messageInput[0].isInt32())
			m_intValueBuffer[0] = messageInput[0].getInt32();
		else if (messageInput[0].isFloat32())
			m_intValueBuffer[0] = static_cast<int>(round(messageInput[0].getFloat32()));

		newMessageData.valType = ROVT_INT;
		newMessageData.valCount = 1;
		newMessageData.payload = m_intValueBuffer;
		newMessageData.payloadSize = sizeof(int);
	}
	else if (messageInput.size() == 2)
	{
		// value be an int, but since some OSC appliances can only process floats,
		// we need to be prepared to optionally accept float as well
		if (messageInput[0].isInt32())
			m_intValueBuffer[0] = messageInput[0].getInt32();
		else if (messageInput[0].isFloat32())
			m_intValueBuffer[0] = (int)round(messageInput[0].getFloat32());
		if (messageInput[1].isInt32())
			m_intValueBuffer[1] = messageInput[1].getInt32();
		else if (messageInput[1].isFloat32())
			m_intValueBuffer[1] = (int)round(messageInput[1].getFloat32());

		newMessageData.valType = ROVT_INT;
		newMessageData.valCount = 2;
		newMessageData.payload = m_intValueBuffer;
		newMessageData.payloadSize = 2 * sizeof(int);
	}
}

/**
 * Helper method to fill a new remote object message data struct with data from an osc message.
 * This method reads a string from osc message and fills it into the message data struct.
 * @param messageInput	The osc input message to read from.
 * @param newMessageData	The message data struct to fill data into.
 */
void OSCProtocolProcessor::createStringMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData)
{
	if (messageInput.size() == 1)
	{
		// value be an int, but since some OSC appliances can only process floats,
		// we need to be prepared to optionally accept float as well
		if (messageInput[0].isString())
			m_stringValueBuffer = messageInput[0].getString();

		newMessageData.valType = ROVT_INT;
		newMessageData.valCount = 1;
		newMessageData.payload = m_stringValueBuffer.getCharPointer().getAddress();
		newMessageData.payloadSize = m_stringValueBuffer.length();
	}
}

/**
 * Helper method to check if a given remote object relates to channel/soundsourcid info.
 * @param objectId	The remote object id to check.
 * @return True if the object relates to channels, false if not.
 */
bool OSCProtocolProcessor::IsChannelAddressingObject(RemoteObjectIdentifier objectId)
{
	switch (objectId)
	{
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
	case ROI_ReverbInputProcessing_Mute:
	case ROI_ReverbInputProcessing_EqEnable:
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
	case ROI_Positioning_SourceSpread:
	case ROI_Positioning_SourcePosition_XY:
	case ROI_Positioning_SourcePosition_X:
	case ROI_Positioning_SourcePosition_Y:
	case ROI_Positioning_SourcePosition:
	case ROI_MatrixInput_ReverbSendGain:
	case ROI_ReverbInput_Gain:
	case ROI_ReverbInputProcessing_Gain:
	case ROI_ReverbInputProcessing_LevelMeter:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_MatrixInput_ChannelName:
	case ROI_MatrixOutput_ChannelName:
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
		return true;
	case ROI_Settings_DeviceName:
	case ROI_Error_GnrlErr:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_RevebRearLevel:
	case ROI_Device_Clear:
	case ROI_Scene_Previous:
	case ROI_Scene_Next:
	case ROI_Scene_Recall:
	case ROI_Scene_SceneIndex:
	case ROI_Scene_SceneName:
	case ROI_Scene_SceneComment:
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
	default:
		return false;
	}
}

/**
 * Helper method to check if a given remote object relates to record/mappingid info.
 * @param objectId	The remote object id to check.
 * @return True if the object relates to records, false if not.
 */
bool OSCProtocolProcessor::IsRecordAddressingObject(RemoteObjectIdentifier objectId)
{
	switch (objectId)
	{
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition:
		return true;
	default:
		return false;
	}
}