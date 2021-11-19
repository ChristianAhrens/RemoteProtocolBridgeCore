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

	auto chacheInitSuccess = true;
	auto chacheInitTypes = std::vector<ADMObjectType>{ AOT_XPos, AOT_YPos, AOT_ZPos, AOT_Azimuth, AOT_Elevation, AOT_Distance };
	auto chacheInitValues = std::vector<float>{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	for (int i = 1; i <= 128; i++)
		if (!WriteToObjectCache(static_cast<ChannelId>(i), chacheInitTypes, chacheInitValues, true))
			chacheInitSuccess = false;
	jassert(chacheInitSuccess);
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
			m_mappingAreaId = static_cast<MappingAreaId>(mappingAreaXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID)));

		auto xAxisInvertedXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::XINVERTED));
		if (xAxisInvertedXmlElement)
			m_xAxisInverted = 1 == xAxisInvertedXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::STATE));

		auto yAxisInvertedXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::YINVERTED));
		if (yAxisInvertedXmlElement)
			m_yAxisInverted = 1 == yAxisInvertedXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::STATE));

		auto xyAxisSwappedXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::XYSWAPPED));
		if (xyAxisSwappedXmlElement)
			m_xyAxisSwapped = 1 == xyAxisSwappedXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::STATE));

		auto dataSendingDisabledXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATASENDINGDISABLED));
		if (dataSendingDisabledXmlElement)
			m_dataSendindDisabled = 1 == dataSendingDisabledXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::STATE));

		if (mappingAreaXmlElement && xAxisInvertedXmlElement && yAxisInvertedXmlElement && xyAxisSwappedXmlElement && dataSendingDisabledXmlElement)
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
bool ADMOSCProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	// do not send any values if the config forbids data sending
	if (m_dataSendindDisabled)
		return false;
	// do not send any values if they relate to a mapping id differing from the one configured for this protocol
	if (msgData._addrVal._second != m_mappingAreaId)
		return false;
	// do not send any values if the addressing is not sane (ymh osc messages always use src #)
	if (msgData._addrVal._first <= INVALID_ADDRESS_VALUE)
		return false;

	ADMObjectType targetObjType = WriteMessageDataToObjectCache(Id, msgData);
	if(targetObjType == ADMObjectType::AOT_Invalid)
		return false;

	// Send object config message to make sender aware of us sending only cartesian coordinate values
	if (m_expectedCoordinateSystem != CS_Cartesian)
	{
		String addressString = GetADMMessageDomainString() + GetADMMessageTypeString(AMT_ObjectConfig) + GetADMObjectTypeString(AOT_CartesianCoords);

		int intValue[1] = { 1 };

		RemoteObjectMessageData coordSysMsgData;
		coordSysMsgData._valType = ROVT_INT;
		coordSysMsgData._valCount = 1;
		coordSysMsgData._payload = intValue;
		coordSysMsgData._payloadSize = sizeof(int);
		if (!SendAddressedMessage(addressString, coordSysMsgData))
			return false;
	}

	float floatValueSendBuffer[3] = { 0.0f, 0.0f, 0.0f };
	ADMObjectType::AOT_Invalid;
	RemoteObjectMessageData admConvertedMsgData;

	switch (targetObjType)
	{
	case ADMObjectType::AOT_XPos:
	case ADMObjectType::AOT_YPos:
	case ADMObjectType::AOT_Gain:
	case ADMObjectType::AOT_Width:
		{
			floatValueSendBuffer[0] = ReadFromObjectCache(msgData._addrVal._first, targetObjType);

			admConvertedMsgData._valType = ROVT_FLOAT;
			admConvertedMsgData._valCount = 1;
			admConvertedMsgData._payload = floatValueSendBuffer;
			admConvertedMsgData._payloadSize = sizeof(float);
		}
		break;
	case ADMObjectType::AOT_XYZPos:
		{
			floatValueSendBuffer[0] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_XPos);
			floatValueSendBuffer[1] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_YPos);
			floatValueSendBuffer[2] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_ZPos);

			admConvertedMsgData._valType = ROVT_FLOAT;
			admConvertedMsgData._valCount = 3;
			admConvertedMsgData._payload = floatValueSendBuffer;
			admConvertedMsgData._payloadSize = 3 * sizeof(float);
		}
		break;
	case ADMObjectType::AOT_AzimElevDist:
		{
			floatValueSendBuffer[0] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_Azimuth);
			floatValueSendBuffer[1] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_Elevation);
			floatValueSendBuffer[2] = ReadFromObjectCache(msgData._addrVal._first, ADMObjectType::AOT_Distance);

			admConvertedMsgData._valType = ROVT_FLOAT;
			admConvertedMsgData._valCount = 3;
			admConvertedMsgData._payload = floatValueSendBuffer;
			admConvertedMsgData._payloadSize = 3 * sizeof(float);
		}
		break;
	case ADMObjectType::AOT_Azimuth:
	case ADMObjectType::AOT_CartesianCoords:
	case ADMObjectType::AOT_Distance:
	case ADMObjectType::AOT_Elevation:
	case ADMObjectType::AOT_Invalid:
	default:
		jassertfalse; // unsupported object
		return false;
	}

	// assemble the addressing string
	String addressString = GetADMMessageDomainString() + GetADMMessageTypeString(AMT_Object) + String(msgData._addrVal._first) + GetADMObjectTypeString(targetObjType);
	
	return SendAddressedMessage(addressString, admConvertedMsgData);
}

/**
 * Called when the OSCReceiver receives a new OSC message and parses its contents to
 * pass the received data to parent node for further handling
 *
 * @param message			The received OSC message.
 * @param senderIPAddress	The ip the message originates from.
 * @param senderPort			The port this message was received on.
 */
void ADMOSCProtocolProcessor::oscMessageReceived(const OSCMessage& message, const juce::String& senderIPAddress, const int& senderPort)
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

		// process the admMessageType into an internal remoteobjectid for further handling
		auto targetedObjectId = ROI_Invalid;
		switch (admObjectType)
		{
		case AOT_Azimuth:
		case AOT_Elevation:
		case AOT_Distance:
		case AOT_AzimElevDist:
		case AOT_XPos:
		case AOT_YPos:
		case AOT_ZPos:
		case AOT_XYZPos:
			targetedObjectId = ROI_CoordinateMapping_SourcePosition_XY;
			break;
		case AOT_Width:
			targetedObjectId = ROI_Positioning_SourceSpread;
			break;
		case AOT_Gain:
			targetedObjectId = ROI_MatrixInput_Gain;
			break;
		case AOT_CartesianCoords:
		case AOT_Invalid:
		default:
			jassertfalse;
			break;
		}

		auto channel = static_cast<ChannelId>(INVALID_ADDRESS_VALUE);
		auto record = static_cast<RecordId>(INVALID_ADDRESS_VALUE);

		// get the channel info if the object is supposed to provide it
		if (ProcessingEngineConfig::IsChannelAddressingObject(targetedObjectId))
		{
			// Parse the Channel ID
			channel = static_cast<ChannelId>((addressString.fromLastOccurrenceOf(GetADMMessageTypeString(AMT_Object), false, true)).getIntValue());
			jassert(channel > 0);
			if (channel <= 0)
				return;
		}

		// set the record info if the object needs it
		if (ProcessingEngineConfig::IsRecordAddressingObject(targetedObjectId))
			record = static_cast<RecordId>(m_mappingAreaId);

		// assemble a remote object structure from the collected addressing data
		auto remoteObject = RemoteObject(targetedObjectId, RemoteObjectAddressing(channel, record));
		
		// create the remote object to be forwarded to processing node for further processing
		switch (admObjectType)
		{
		case AOT_Azimuth:
			WriteToObjectCache(channel, AOT_Azimuth, message[0].getFloat32(), true);
			break;
		case AOT_Elevation:
			WriteToObjectCache(channel, AOT_Elevation, message[0].getFloat32(), true);
			break;
		case AOT_Distance:
			WriteToObjectCache(channel, AOT_Distance, message[0].getFloat32(), true);
			break;
		case AOT_AzimElevDist:
			WriteToObjectCache(channel, std::vector<ADMObjectType>{AOT_Azimuth, AOT_Elevation, AOT_Distance}, std::vector<float>{message[0].getFloat32(), message[1].getFloat32(), message[2].getFloat32()}, true);
			break;
		case AOT_XPos:
			WriteToObjectCache(channel, AOT_XPos, message[0].getFloat32(), true);
			break;
		case AOT_YPos:
			WriteToObjectCache(channel, AOT_YPos, message[0].getFloat32(), true);
			break;
		case AOT_ZPos:
			WriteToObjectCache(channel, AOT_ZPos, message[0].getFloat32(), true);
			break;
		case AOT_XYZPos:
			WriteToObjectCache(channel, std::vector<ADMObjectType>{AOT_XPos, AOT_YPos, AOT_ZPos}, std::vector<float>{message[0].getFloat32(), message[1].getFloat32(), message[2].getFloat32()}, true);
			break;
		case AOT_Width:
			WriteToObjectCache(channel, AOT_Width, message[0].getFloat32());
			break;
		case AOT_Gain:
			WriteToObjectCache(channel, AOT_Gain, message[0].getFloat32());
			break;
		case AOT_CartesianCoords:
			if (SetExpectedCoordinateSystem(message[0].getInt32() == 1))
			{
				if (message[0].getInt32() == 1) // switch to cartesian coordinates - we take this as opportunity to sync current polar data to cartesian once
					SyncCachedPolarToCartesianValues(channel);
				else if (message[0].getInt32() == 0) // switch to polar coordinates - we take this as opportunity to sync current cartesian data to polar once
					SyncCachedCartesianToPolarValues(channel);
			}
			break;
		case AOT_Invalid:
		default:
			jassertfalse;
			break;
		}

		// If the received object is set to muted, return without further processing
		if (IsRemoteObjectMuted(remoteObject))
			return;

		// create a new message in internally known format
		auto newMsgData = RemoteObjectMessageData(remoteObject._Addr, ROVT_FLOAT, 0, nullptr, 0);
		if (!CreateMessageDataFromObjectCache(remoteObject._Id, channel, newMsgData))
			return;

		// and provide that message to parent node
		if (m_messageListener)
			m_messageListener->OnProtocolMessageReceived(this, targetedObjectId, newMsgData);
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
String ADMOSCProtocolProcessor::GetADMMessageTypeString(const ADMMessageType& msgType)
{
	switch (msgType)
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
 * @param type	The message type to get the OSC specific parameter def string for
 * @return		The parameter defining string, empty if not available
 */
String ADMOSCProtocolProcessor::GetADMObjectTypeString(const ADMObjectType& objType)
{
	switch (objType)
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
 * static helper method to get ADM specific coordinate system the given object type is associated with.
 * @param type	The object type to get the coordinate system for
 * @return		The coordinate system enum value for the object type
 */
ADMOSCProtocolProcessor::CoodinateSystem ADMOSCProtocolProcessor::GetObjectTypeCoordinateSystem(const ADMObjectType& objType)
{
	switch (objType)
	{
	case AOT_Azimuth:
	case AOT_Elevation:
	case AOT_Distance:
	case AOT_AzimElevDist:
		return ADMOSCProtocolProcessor::CoodinateSystem::CS_Polar;
	case AOT_XPos:
	case AOT_YPos:
	case AOT_ZPos:
	case AOT_XYZPos:
		return ADMOSCProtocolProcessor::CoodinateSystem::CS_Cartesian;
	case AOT_Width:
	case AOT_CartesianCoords:
	case AOT_Gain:
	case AOT_Invalid:
	default:
		return ADMOSCProtocolProcessor::CoodinateSystem::CS_Invalid;
	}
}

/**
 * static helper method to get ADM specific message type the given object type is associated with.
 * @param type	The object type to get the message type for
 * @return		The message type enum value for the object type
 */
ADMOSCProtocolProcessor::ADMMessageType ADMOSCProtocolProcessor::GetObjectTypeMessageType(const ADMObjectType& objType)
{
	switch (objType)
	{
	case AOT_Azimuth:
	case AOT_Elevation:
	case AOT_Distance:
	case AOT_AzimElevDist:
	case AOT_XPos:
	case AOT_YPos:
	case AOT_ZPos:
	case AOT_XYZPos:
	case AOT_Width:
	case AOT_Gain:
		return ADMOSCProtocolProcessor::ADMMessageType::AMT_Object;
	case AOT_CartesianCoords:
		return ADMOSCProtocolProcessor::ADMMessageType::AMT_ObjectConfig;
	case AOT_Invalid:
	default:
		return ADMOSCProtocolProcessor::ADMMessageType::AMT_Invalid;
	}
}

/**
 * Static method to get a valid value range for a given ADM object type.
 * @param	objType	The type of ADM object to get the range for.
 * @return	The requested range. Invalid (0.0f<->0.0f) if not supported.
 */
const juce::Range<float> ADMOSCProtocolProcessor::GetADMObjectRange(const ADMObjectType& objType)
{
	switch (objType)
	{
	case AOT_Azimuth:
		return juce::Range<float>(-180.0f, 180.0f);
	case AOT_Elevation:
		return juce::Range<float>(-90.0f, 90.0f);
	case AOT_Distance:
		return juce::Range<float>(0.0f, 1.0f);
	case AOT_XPos:
	case AOT_YPos:
	case AOT_ZPos:
		return juce::Range<float>(-1.0f, 1.0f);
	case AOT_Width:
		return juce::Range<float>(0.0f, 180.0f);
	case AOT_Gain:
		return juce::Range<float>(0.0f, 1.0f);
	case AOT_AzimElevDist:
	case AOT_XYZPos:
	case AOT_CartesianCoords:
	case AOT_Invalid:
	default:
		jassertfalse;
		return juce::Range<float>(0.0f, 0.0f);
	}
}

/**
* Method to write a given value to object value cache member and optionnally sync the values 
* to cached values for opposing coordinate system (cartesian vs. polar).
* @param	channel					The addressing to use to write to the cache.
* @param	objType					The incoming type of object.
* @param	objValue				The incoming object value.
* @param	syncPolarAndCartesian	Bool indicator if the incoming object value shall be synced
*									to its counter part in the opposing coordinate system.
* @return	False if syncing to opposing coordinate system was requested but failed. Otherwise true.
*/
bool ADMOSCProtocolProcessor::WriteToObjectCache(const ChannelId& channel, const ADMObjectType& objType, float objValue, bool syncPolarAndCartesian)
{
	m_objectValueCache[channel][objType] = objValue;

	if (syncPolarAndCartesian)
	{
		switch (GetObjectTypeCoordinateSystem(objType))
		{
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Cartesian:
			return SyncCachedCartesianToPolarValues(channel);
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Polar:
			return SyncCachedPolarToCartesianValues(channel);
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Invalid:
		default:
			return false;
		}
	}

	return true;
}

/**
* Method to write a given value to object value cache member and optionnally sync the values
* to cached values for opposing coordinate system (cartesian vs. polar). Syncing is only possible
* if the incoming object types refer to the same coordinate system. If they don't syncing is skipped
* and the method return falue is false (failure).
* @param	channel					The addressing to use to write to the cache.
* @param	objTypes				The incoming object types.
* @param	objValues				The incoming object values.
* @param	syncPolarAndCartesian	Bool indicator if the incoming object value shall be synced
*									to its counter part in the opposing coordinate system.
* @return	False if the incoming data is invalid or syncing to opposing coordinate system was requested but failed. Otherwise true.
*/
bool ADMOSCProtocolProcessor::WriteToObjectCache(const ChannelId& channel, const std::vector<ADMObjectType>& objTypes, const std::vector<float>& objValues, bool syncPolarAndCartesian)
{
	if (objTypes.size() != objValues.size() || objTypes.empty())
		return false;

	auto commonCoordinateSystem = GetObjectTypeCoordinateSystem(objTypes.front());
	for (int i = 0; i < objTypes.size(); i++)
	{
		auto const& objType = objTypes.at(i);
		auto const& objValue = objValues.at(i);

		auto objTypeCoordSystem = GetObjectTypeCoordinateSystem(objType);
		if (objTypeCoordSystem != commonCoordinateSystem)
			commonCoordinateSystem = ADMOSCProtocolProcessor::CoodinateSystem::CS_Invalid;

		m_objectValueCache[channel][objType] = objValue;
	}

	if (syncPolarAndCartesian && commonCoordinateSystem != ADMOSCProtocolProcessor::CoodinateSystem::CS_Invalid)
	{
		switch (commonCoordinateSystem)
		{
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Cartesian:
			return SyncCachedCartesianToPolarValues(channel);
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Polar:
			return SyncCachedPolarToCartesianValues(channel);
		case ADMOSCProtocolProcessor::CoodinateSystem::CS_Invalid:
		default:
			return false;
		}
	}

	return true;
}

/**
 * Read the value for a given object type from value cache.
 * @param	channel	The addressing to use to read from the cache.
 * @param	objType	The type of object to read the valu for.
 * @return	The requested object value.
 */
float ADMOSCProtocolProcessor::ReadFromObjectCache(const ChannelId& channel, const ADMObjectType& objType)
{
	return m_objectValueCache[channel][objType];
}

/**
 * Method to set the expected coordinate system member field to cartesian or polar.
 * @param	cartesian	Bool indicator if member field shall be set to cartesian (true) or polar (false).
 * @return	True if the parameter value was successfully set. False if the member field was already configured the expected way before.
 */
bool ADMOSCProtocolProcessor::SetExpectedCoordinateSystem(bool cartesian)
{
	if (cartesian && m_expectedCoordinateSystem != CS_Cartesian)
	{
		m_expectedCoordinateSystem = CS_Cartesian;
		return true;
	}
	else if (!cartesian && m_expectedCoordinateSystem != CS_Polar)
	{
		m_expectedCoordinateSystem = CS_Polar;
		return true;
	}
	else
		return false;
}

/**
 * Method to sync the cached azimuth and elevation angles and the distance from origin values
 * into x, y, z adm cached coordinate values for a given channel.
 * @param	channel	The channel number to sync the values in cache for.
 * @return	True on success, false if syncing failed due to something.
 */
bool ADMOSCProtocolProcessor::SyncCachedPolarToCartesianValues(const ChannelId& channel)
{
	/******************** Read values *********************/
	auto admAzimuth = ReadFromObjectCache(channel, AOT_Azimuth);
	auto admElevation = ReadFromObjectCache(channel, AOT_Elevation);
	auto admDistance = ReadFromObjectCache(channel, AOT_Distance);
	auto admOrigin = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);

	/********************* Conversion *********************/
	auto admPos = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);

	auto admAzimuthRad = (admAzimuth / 180) * juce::MathConstants<float>::pi;
	auto admElevationRad = (admElevation / 180)* juce::MathConstants<float>::pi;
	admPos.z = std::cos(admElevationRad) * admDistance;
	auto admXYAbs = std::sqrt((admDistance * admDistance) - (admPos.z * admPos.z));
	admPos.x = std::sin(admAzimuthRad) * admXYAbs;
	admPos.y = std::cos(admAzimuthRad) * admXYAbs;

	/******************** Write values ********************/
	WriteToObjectCache(channel, AOT_XPos, admPos.x);
	WriteToObjectCache(channel, AOT_YPos, admPos.y);
	WriteToObjectCache(channel, AOT_ZPos, admPos.z);

	return true;
}

/**
 * Method to sync x, y, z adm cached coordinate values into cached 
 * azimuth and elevation angles and the distance from origin values 
 * for a given channel.
 * @param	channel	The channel number to sync the values in cache for.
 * @return	True on success, false if syncing failed due to something.
 */
bool ADMOSCProtocolProcessor::SyncCachedCartesianToPolarValues(const ChannelId& channel)
{
	/******************** Read values *********************/
	auto admPos = juce::Vector3D<float>(ReadFromObjectCache(channel, AOT_XPos), ReadFromObjectCache(channel, AOT_YPos), ReadFromObjectCache(channel, AOT_ZPos));
	auto admOrigin = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);

	/********************* Conversion *********************/
	auto admAzimuth = 0.0f;
	auto admElevation = 0.0f;
	auto admDistance = 0.0f;

	auto admPosAbs = admPos.length();
	if (admPos.y != 0.0f && admPosAbs != 0.0f)
	{
		auto admAzimuthRad = std::atan(admPos.x / admPos.y);
		auto admElevationRad = std::acos(admPos.z / admPosAbs);
		admAzimuth = admAzimuthRad * (180 / juce::MathConstants<float>::pi);
		admElevation = admElevationRad * (180 / juce::MathConstants<float>::pi);
		admDistance = admPosAbs;
	}

	/******************** Write values ********************/
	WriteToObjectCache(channel, AOT_Azimuth, admAzimuth);
	WriteToObjectCache(channel, AOT_Elevation, admElevation);
	WriteToObjectCache(channel, AOT_Distance, admDistance);

	return true;
}

/**
 * Method to create a message data struct from internally cached ADM object values.
 * If the given RemoteObjectIdentifier is not supported, an empty struct is returned.
 * @param	id				The remote object identification to create a data struct for.
 * @param	channel			The remote object channel to use to access the cache.
 * @param	newMessageData	The message data struct as requested. Unaltered if other input parameters are invalid.
 * @return	True on success.
 */
bool ADMOSCProtocolProcessor::CreateMessageDataFromObjectCache(const RemoteObjectIdentifier& id, const ChannelId& channel, RemoteObjectMessageData& newMessageData)
{
	auto valRange = ProcessingEngineConfig::GetRemoteObjectRange(id);

	switch (id)
	{
	case ROI_CoordinateMapping_SourcePosition:
		{
			auto admRange = GetADMObjectRange(AOT_XPos);

			auto admXValue = ReadFromObjectCache(channel, AOT_YPos);
			auto admYValue = ReadFromObjectCache(channel, AOT_XPos);
			auto admZValue = ReadFromObjectCache(channel, AOT_ZPos);

			auto admNormXValue = NormalizeValueByRange(admXValue, admRange);
			auto admNormYValue = NormalizeValueByRange(admYValue, admRange);
			auto admNormZValue = NormalizeValueByRange(admZValue, admRange);

			auto xValue = MapNormalizedValueToRange(admNormXValue, valRange, m_xAxisInverted);
			auto yValue = MapNormalizedValueToRange(admNormYValue, valRange, m_yAxisInverted);
			auto zValue = MapNormalizedValueToRange(admNormZValue, valRange);

			m_floatValueBuffer[0] = m_xyAxisSwapped ? yValue : xValue;
			m_floatValueBuffer[1] = m_xyAxisSwapped ? xValue : yValue;
			m_floatValueBuffer[2] = zValue;
			
			newMessageData._valCount = 3;
			newMessageData._payload = m_floatValueBuffer;
			newMessageData._payloadSize = 3 * sizeof(float);
		}
		return true;
	case ROI_CoordinateMapping_SourcePosition_X:
		{
			auto admRange = GetADMObjectRange(AOT_XPos);

			auto admXValue = ReadFromObjectCache(channel, AOT_YPos);
			auto admYValue = ReadFromObjectCache(channel, AOT_XPos);

			auto admNormXValue = NormalizeValueByRange(admXValue, admRange);
			auto admNormYValue = NormalizeValueByRange(admYValue, admRange);

			auto xValue = MapNormalizedValueToRange(admNormXValue, valRange, m_xAxisInverted);
			auto yValue = MapNormalizedValueToRange(admNormYValue, valRange, m_yAxisInverted);

			m_floatValueBuffer[0] = m_xyAxisSwapped ? yValue : xValue;

			newMessageData._valCount = 1;
			newMessageData._payload = m_floatValueBuffer;
			newMessageData._payloadSize = 1 * sizeof(float);
		}
		return true;
	case ROI_CoordinateMapping_SourcePosition_Y:
		{
			auto admRange = GetADMObjectRange(AOT_XPos);

			auto admXValue = ReadFromObjectCache(channel, AOT_YPos);
			auto admYValue = ReadFromObjectCache(channel, AOT_XPos);

			auto admNormXValue = NormalizeValueByRange(admXValue, admRange);
			auto admNormYValue = NormalizeValueByRange(admYValue, admRange);

			auto xValue = MapNormalizedValueToRange(admNormXValue, valRange, m_xAxisInverted);
			auto yValue = MapNormalizedValueToRange(admNormYValue, valRange, m_yAxisInverted);

			m_floatValueBuffer[0] = m_xyAxisSwapped ? xValue : yValue;

			newMessageData._valCount = 1;
			newMessageData._payload = m_floatValueBuffer;
			newMessageData._payloadSize = 1 * sizeof(float);
		}
		return true;
	case ROI_CoordinateMapping_SourcePosition_XY:
		{
			auto admRange = GetADMObjectRange(AOT_XPos);

			auto admXValue = ReadFromObjectCache(channel, AOT_YPos);
			auto admYValue = ReadFromObjectCache(channel, AOT_XPos);

auto admNormXValue = NormalizeValueByRange(admXValue, admRange);
auto admNormYValue = NormalizeValueByRange(admYValue, admRange);

auto xValue = MapNormalizedValueToRange(admNormXValue, valRange, m_xAxisInverted);
auto yValue = MapNormalizedValueToRange(admNormYValue, valRange, m_yAxisInverted);

m_floatValueBuffer[0] = m_xyAxisSwapped ? yValue : xValue;
m_floatValueBuffer[1] = m_xyAxisSwapped ? xValue : yValue;

newMessageData._valCount = 2;
newMessageData._payload = m_floatValueBuffer;
newMessageData._payloadSize = 2 * sizeof(float);
		}
		return true;
	case ROI_MatrixInput_Gain:
	{
		auto admRange = GetADMObjectRange(AOT_Gain);

		auto admValue = ReadFromObjectCache(channel, AOT_Gain);

		auto admNormValue = NormalizeValueByRange(admValue, admRange);

		m_floatValueBuffer[0] = MapNormalizedValueToRange(admNormValue, valRange);

		newMessageData._valCount = 1;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 1 * sizeof(float);
	}
	return true;
	case ROI_Positioning_SourceSpread:
	{
		auto admRange = GetADMObjectRange(AOT_Width);

		auto admValue = ReadFromObjectCache(channel, AOT_Width);

		auto admNormValue = NormalizeValueByRange(admValue, admRange);

		m_floatValueBuffer[0] = MapNormalizedValueToRange(admNormValue, valRange);

		newMessageData._valCount = 1;
		newMessageData._payload = m_floatValueBuffer;
		newMessageData._payloadSize = 1 * sizeof(float);
	}
	return true;
	case ROI_HeartbeatPong:
	case ROI_HeartbeatPing:
	case ROI_Settings_DeviceName:
	case ROI_Error_GnrlErr:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixInput_Select:
	case ROI_MatrixInput_Mute:
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
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_ReverbRearLevel:
	case ROI_MatrixInput_ReverbSendGain:
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
		break;
	}

	return false;
}

/**
 * Method to write contents of a message data struct to internally cached ADM object values.
 * @param	id				The remote object identification to write data struct contents to cache from.
 * @param	messageData	The source message data struct.
 * @return	The resulting targeted ADM specific object type. Invalid type on failure.
 */
ADMOSCProtocolProcessor::ADMObjectType ADMOSCProtocolProcessor::WriteMessageDataToObjectCache(const RemoteObjectIdentifier& id, const RemoteObjectMessageData& messageData)
{
	ADMObjectType resultingTargetObjType = ADMObjectType::AOT_Invalid;

	// sanity checking first
	const ChannelId& channel = messageData._addrVal._first;
	if (channel == static_cast<ChannelId>(INVALID_ADDRESS_VALUE))
		return resultingTargetObjType;
	if (messageData._valCount != 1)
		return resultingTargetObjType;
	if (messageData._valType != ROVT_FLOAT)
		return resultingTargetObjType;
	if (messageData._payloadSize != sizeof(float))
		return resultingTargetObjType;

	auto remoteObjectRange = ProcessingEngineConfig::GetRemoteObjectRange(id);
	auto remoteObjectValue = *static_cast<float*>(messageData._payload);
	auto normalizedRemoteObjectValue = NormalizeValueByRange(remoteObjectValue, remoteObjectRange);

	switch (id)
	{
	case ROI_CoordinateMapping_SourcePosition_X:
		{
			resultingTargetObjType = m_xyAxisSwapped ? ADMObjectType::AOT_XPos : ADMObjectType::AOT_YPos;

			auto admRange = GetADMObjectRange(resultingTargetObjType);
			auto admValue = MapNormalizedValueToRange(normalizedRemoteObjectValue, admRange, m_xyAxisSwapped ? m_yAxisInverted : m_xAxisInverted);

			WriteToObjectCache(channel, resultingTargetObjType, admValue, true);
		}
		break;
	case ROI_CoordinateMapping_SourcePosition_Y:
		{
			resultingTargetObjType = m_xyAxisSwapped ? ADMObjectType::AOT_YPos : ADMObjectType::AOT_XPos;

			auto admRange = GetADMObjectRange(resultingTargetObjType);
			auto admValue = MapNormalizedValueToRange(normalizedRemoteObjectValue, admRange, m_xyAxisSwapped ? m_xAxisInverted : m_yAxisInverted);

			WriteToObjectCache(channel, resultingTargetObjType, admValue, true);
		}
		break;
	case ROI_MatrixInput_Gain:
		{
			resultingTargetObjType = ADMObjectType::AOT_Gain;

			auto admRange = GetADMObjectRange(resultingTargetObjType);
			auto admValue = MapNormalizedValueToRange(normalizedRemoteObjectValue, admRange);

			WriteToObjectCache(channel, resultingTargetObjType, admValue);
		}
		break;
	case ROI_Positioning_SourceSpread:
		{
			resultingTargetObjType = ADMObjectType::AOT_Width;

			auto admRange = GetADMObjectRange(resultingTargetObjType);
			auto admValue = MapNormalizedValueToRange(normalizedRemoteObjectValue, admRange);

			WriteToObjectCache(channel, resultingTargetObjType, admValue);
		}
		break;
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_HeartbeatPong:
	case ROI_HeartbeatPing:
	case ROI_Settings_DeviceName:
	case ROI_Error_GnrlErr:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixInput_Select:
	case ROI_MatrixInput_Mute:
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
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_ReverbRearLevel:
	case ROI_MatrixInput_ReverbSendGain:
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
		break;
	}

	return resultingTargetObjType;
}

/**
 * Helper method to normalize a given value to a given range without clipping
 * @param	value				The value to normalize
 * @param	normalizationRange	The range to normalize the value to
 * @return	The normalized value.
 */
float ADMOSCProtocolProcessor::NormalizeValueByRange(float value, const juce::Range<float>& normalizationRange)
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
float ADMOSCProtocolProcessor::MapNormalizedValueToRange(float normalizedValue, const juce::Range<float>& range, bool invert)
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