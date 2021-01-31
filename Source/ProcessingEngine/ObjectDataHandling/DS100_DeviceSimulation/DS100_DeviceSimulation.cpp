/*
  ==============================================================================

	DS100_DeviceSimulation.cpp
	Created: 4 Aug 2020 08:47:37am
	Author:  Christian Ahrens

  ==============================================================================
*/

#include "DS100_DeviceSimulation.h"

#include "../../ProcessingEngineNode.h"
#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class DS100_DeviceSimulation
// **************************************************************************************
/**
 * Constructor of class DS100_DeviceSimulation.
 *
 * @param parentNode	The objects' parent node that is used by derived objects to forward received message contents to.
 */
DS100_DeviceSimulation::DS100_DeviceSimulation(ProcessingEngineNode* parentNode)
	: ObjectDataHandling_Abstract(parentNode)
{
	SetMode(ObjectHandlingMode::OHM_DS100_DeviceSimulation);

	ScopedLock l(m_currentValLock);
	m_simulatedChCount = 0;
	m_simulatedMappingsCount = 0;

	m_refreshInterval = 50;
	m_simulationBaseValue = 0.0f;
}

/**
 * Destructor
 */
DS100_DeviceSimulation::~DS100_DeviceSimulation()
{
	stopTimerThread();
}

/**
 * Reimplemented to set the custom parts from configuration for the datahandling object.
 *
 * @param config	The overall configuration object that can be used to query config data from
 * @param NId		The node id of the parent node this data handling object is child of (needed to access data from config)
 */
bool DS100_DeviceSimulation::setStateXml(XmlElement* stateXml)
{
	if (!ObjectDataHandling_Abstract::setStateXml(stateXml))
		return false;

	if (stateXml->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE)) != ProcessingEngineConfig::ObjectHandlingModeToString(OHM_DS100_DeviceSimulation))
		return false;

	stopTimerThread();
	{
		ScopedLock l(m_currentValLock);

		auto simChCntXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::SIMCHCNT));
		if (simChCntXmlElement)
			m_simulatedChCount = simChCntXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::COUNT), 64);

		auto simMapingsCntXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::SIMMAPCNT));
		if (simMapingsCntXmlElement)
			m_simulatedMappingsCount = simMapingsCntXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::COUNT), 1);

		auto refreshIntervalXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::REFRESHINTERVAL));
		if (refreshIntervalXmlElement)
			m_refreshInterval = refreshIntervalXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::INTERVAL), 50);

		m_simulatedRemoteObjects = std::vector<RemoteObjectIdentifier>{ ROI_CoordinateMapping_SourcePosition_XY, ROI_CoordinateMapping_SourcePosition_X, ROI_CoordinateMapping_SourcePosition_Y, ROI_Positioning_SourceSpread, ROI_Positioning_SourceDelayMode, ROI_MatrixInput_ReverbSendGain};
	}
	InitDataValues();

	if (m_refreshInterval > 0)
		startTimerThread(m_refreshInterval, m_refreshInterval);

	return true;
}

/**
 * Method to be called by parent node on receiving data from node protocol with given id
 *
 * @param PId		The id of the protocol that received the data
 * @param Id		The object id to send a message for
 * @param msgData	The actual message value/content data
 * @return	True if successful sent/forwarded, false if not
 */
bool DS100_DeviceSimulation::OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData)
{
	auto parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (parentNode)
	{
		if (IsDataRequestPollMessage(Id, msgData))
		{
			return ReplyToDataRequest(PId, Id, msgData._addrVal);
		}
		else
		{
			SetDataValue(PId, Id, msgData);

			auto protocolAIter = std::find(GetProtocolAIds().begin(), GetProtocolAIds().end(), PId);
			if (protocolAIter != GetProtocolAIds().end())
			{
				// Send to all typeB protocols
				auto sendSuccess = true;
				for (auto const protocolB : GetProtocolBIds())
					sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolB, Id, msgData);

				return sendSuccess;

			}
			auto protocolBIter = std::find(GetProtocolBIds().begin(), GetProtocolBIds().end(), PId);
			if (protocolBIter != GetProtocolBIds().end())
			{
				// Send to all typeA protocols
				auto sendSuccess = true;
				for (auto const protocolA : GetProtocolAIds())
					sendSuccess = sendSuccess && parentNode->SendMessageTo(protocolA, Id, msgData);

				return sendSuccess;
			}

			return false;
		}
	}

	return false;
}

/**
 * Helper method to detect if incoming message is an osc message adressing a valid object without
 * actual data value (meaning a reply with the current data value of the object is expected).
 *
 * @param Id	The ROI that was received and has to be checked
 * @param msgData	The received message data that has to be checked
 * @return True if the object is valid and no data is contained, false if not
 */
bool DS100_DeviceSimulation::IsDataRequestPollMessage(const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	bool remoteObjectRequiresReply = false;
	switch (Id)
	{
	case ROI_HeartbeatPing:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_Positioning_SourceSpread:
	case ROI_Positioning_SourceDelayMode:
	case ROI_MatrixInput_ReverbSendGain:
		remoteObjectRequiresReply = true;
		break;
	case ROI_HeartbeatPong:
	case ROI_Invalid:
	default:
		remoteObjectRequiresReply = false;
		break;
	}

	if (remoteObjectRequiresReply)
	{
		if (msgData._valCount == 0)
			return true;
		else
			return false;
	}
	else
		return false;
}

/**
 * Helper method print debug output for a given message id/data pair
 */
void DS100_DeviceSimulation::PrintDataInfo(const String& actionName, const std::pair<RemoteObjectIdentifier, RemoteObjectMessageData>& idDataKV)
{
	auto payloadPtrStr = "0x" + String::toHexString(reinterpret_cast<std::uint64_t>(idDataKV.second._payload));

	switch (idDataKV.first)
	{
	case ROI_HeartbeatPong:
		DBG(actionName + " Pong value.");
		break;
	case ROI_CoordinateMapping_SourcePosition_XY:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ "Rec" + String(idDataKV.second._addrVal._second) 
			+ " XY position value (" + String(static_cast<float*>(idDataKV.second._payload)[0]) + ", " + String(static_cast<float*>(idDataKV.second._payload)[1])
			+ " at " + payloadPtrStr + ").");
		break;
	case ROI_CoordinateMapping_SourcePosition_X:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ "Rec" + String(idDataKV.second._addrVal._second) 
			+ " X position value (" + String(static_cast<float*>(idDataKV.second._payload)[0])
			+ " at " + payloadPtrStr + ").");
		break;
	case ROI_CoordinateMapping_SourcePosition_Y:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ "Rec" + String(idDataKV.second._addrVal._second) 
			+ " Y position value (" + String(static_cast<float*>(idDataKV.second._payload)[0])
			+ " at " + payloadPtrStr + ").");
		break;
	case ROI_Positioning_SourceSpread:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ " spread value (" + String(static_cast<float*>(idDataKV.second._payload)[0])
			+ " at " + payloadPtrStr + ").");
		break;
	case ROI_MatrixInput_ReverbSendGain:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ " EnSpace gain value (" + String(static_cast<float*>(idDataKV.second._payload)[0])
			+ " at " + payloadPtrStr + ").");
		break;
	case ROI_Positioning_SourceDelayMode:
		DBG(actionName + " Ch" + String(idDataKV.second._addrVal._first)
			+ " delaymode value (" + String(static_cast<int*>(idDataKV.second._payload)[0]) 
			+ " at " + payloadPtrStr + ").");
		break;
	default:
		return;
	}
}

/**
 * Helper method to reply the correct current simulated data to a received request.
 *
 * @param PId		The id of the protocol that received the request and needs to be sent back the current value
 * @param Id		The ROI that was received and has to be checked
 * @param adressing	The adressing (ch+rec) that was queried and requires answering
 * @return True if the requested reply was successful, false if not
 */
bool DS100_DeviceSimulation::ReplyToDataRequest(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectAddressing adressing)
{
	const ProcessingEngineNode* parentNode = ObjectDataHandling_Abstract::GetParentNode();
	if (!parentNode)
		return false;

	ScopedLock l(m_currentValLock);
	
	if (m_currentValues.count(Id) == 0)
		return false;
	if (m_currentValues.at(Id).count(adressing) == 0)
		return false;

	auto dataReplyId = Id;
	auto& dataReplyValue = m_currentValues.at(Id).at(adressing);

	switch (Id)
	{
	case ROI_HeartbeatPing:
		jassert(dataReplyValue._valType == ROVT_NONE);
		jassert(dataReplyValue._addrVal == adressing);
		dataReplyId = ROI_HeartbeatPong;
		break;
	case ROI_CoordinateMapping_SourcePosition_XY:
		jassert(dataReplyValue._valCount == 2);
		jassert(dataReplyValue._valType == ROVT_FLOAT);
		jassert(dataReplyValue._addrVal == adressing);
		break;
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_Positioning_SourceSpread:
	case ROI_MatrixInput_ReverbSendGain:
		jassert(dataReplyValue._valCount == 1);
		jassert(dataReplyValue._valType == ROVT_FLOAT);
		jassert(dataReplyValue._addrVal == adressing);
		break;
	case ROI_Positioning_SourceDelayMode:
		jassert(dataReplyValue._valCount == 1);
		jassert(dataReplyValue._valType == ROVT_INT);
		jassert(dataReplyValue._addrVal == adressing);
		break;
	case ROI_HeartbeatPong:
	case ROI_Invalid:
	default:
		return false;
	}

#ifdef DEBUG
	PrintDataInfo("Sending", std::make_pair(dataReplyId, dataReplyValue));
#endif

	return parentNode->SendMessageTo(PId, dataReplyId, dataReplyValue);
}

/**
 * Helper method to set a new RemoteObjectMessageData obj. to internal map of current values.
 * Takes care of cleaning up previously stored data if required.
 *
 * @param Id	The ROI that shall be stored
 * @param msgData	The message data that shall be stored
 */
void DS100_DeviceSimulation::InitDataValues()
{
	RemoteObjectMessageData emptyReplyMessageData;
	{
		emptyReplyMessageData._payload = nullptr;
		emptyReplyMessageData._payloadSize = 0;
		emptyReplyMessageData._payloadOwned = false;
		emptyReplyMessageData._valCount = 0;
		emptyReplyMessageData._valType = ROVT_NONE;
		emptyReplyMessageData._addrVal._first = INVALID_ADDRESS_VALUE;
		emptyReplyMessageData._addrVal._second = INVALID_ADDRESS_VALUE;
		ScopedLock l(m_currentValLock);
		m_currentValues[ROI_HeartbeatPing].insert(std::make_pair(emptyReplyMessageData._addrVal, emptyReplyMessageData));
		m_currentValues[ROI_HeartbeatPong].insert(std::make_pair(emptyReplyMessageData._addrVal, emptyReplyMessageData));
	}

	for (auto const& roi : m_simulatedRemoteObjects)
	{
		ScopedLock l(m_currentValLock);
		auto& remoteAddressValueMap = m_currentValues[roi];

		MappingId mapping = 1;
		auto mappingsCount = m_simulatedMappingsCount;
		if (!ProcessingEngineConfig::IsRecordAddressingObject(roi))
		{
			mapping = INVALID_ADDRESS_VALUE;
			mappingsCount = 0;
		}

		for (; mapping <= mappingsCount && mapping != 0; mapping++)
		{
			SourceId channel = 1;
			auto channelCount = m_simulatedChCount;
			if (!ProcessingEngineConfig::IsChannelAddressingObject(roi))
			{
				channel = INVALID_ADDRESS_VALUE;
				channelCount = 0;
			}

			for (; channel <= channelCount && channel != 0; channel++)
			{
				// add the empty but already addressed remote data entry to map
				auto remoteData = RemoteObjectMessageData();
				remoteData._addrVal = RemoteObjectAddressing(channel, mapping);
				//remoteAddressValueMap.insert(std::make_pair(remoteData._addrVal, remoteData));
				remoteAddressValueMap[remoteData._addrVal] = remoteData;

				// Take a reference to the entry for further data generation.
				// This avoids a local object with payload memory allocated to go out of scope 
				// and due to _payloadOwned==true auto delete the memory. (See RemoteObjectMessageData destructor for details)
				auto& remoteValue = remoteAddressValueMap.at(remoteData._addrVal);

				jassert(remoteValue._addrVal._first == channel);
				jassert(remoteValue._addrVal._second == mapping);
				jassert(remoteValue._valCount == 0);
				jassert(remoteValue._valType == ROVT_NONE);
				jassert(remoteValue._payloadSize == 0);
				jassert(remoteValue._payload == 0);

				switch (roi)
				{
				case ROI_CoordinateMapping_SourcePosition_XY:
					remoteValue._valCount = 2;
					remoteValue._valType = ROVT_FLOAT;
					jassert(remoteValue._payload == nullptr);
					remoteValue._payload = new float[2];
					remoteValue._payloadOwned = true;
					static_cast<float*>(remoteValue._payload)[0] = 0.0f;
					static_cast<float*>(remoteValue._payload)[1] = 0.0f;
					remoteValue._payloadSize = 2 * sizeof(float);
					break;
				case ROI_CoordinateMapping_SourcePosition_X:
				case ROI_CoordinateMapping_SourcePosition_Y:
				case ROI_Positioning_SourceSpread:
				case ROI_MatrixInput_ReverbSendGain:
					remoteValue._valCount = 1;
					remoteValue._valType = ROVT_FLOAT;
					jassert(remoteValue._payload == nullptr);
					remoteValue._payload = new float;
					remoteValue._payloadOwned = true;
					static_cast<float*>(remoteValue._payload)[0] = 0.0f;
					remoteValue._payloadSize = sizeof(float);
					break;
				case ROI_Positioning_SourceDelayMode:
					remoteValue._valCount = 1;
					remoteValue._valType = ROVT_INT;
					jassert(remoteValue._payload == nullptr);
					remoteValue._payload = new int;
					remoteValue._payloadOwned = true;
					static_cast<int*>(remoteValue._payload)[0] = 0;
					remoteValue._payloadSize = sizeof(int);
					break;
				case ROI_HeartbeatPing:
				case ROI_HeartbeatPong:
				case ROI_Invalid:
				default:
					remoteValue._valCount = 0;
					remoteValue._valType = ROVT_NONE;
					remoteValue._payload = nullptr;
					remoteValue._payloadSize = 0;
					break;
				}

#ifdef DEBUG
				PrintDataInfo("Initializing", std::make_pair(roi, remoteValue));
#endif
			}
		}
	}

	return;
}

/**
 * Helper method to set the data from an incoming message to internal map of simulated values.
 * (If timer is active, this will be overwritten on next timer timeout - otherwise 
 * a new poll request will trigger an answer with the value set in this method.)
 *
 * @param PId	The id of the protocol that received the message.
 * @param Id	The ROI that was received
 * @param msgData	The received message data from which the value shall be taken
 */
void DS100_DeviceSimulation::SetDataValue(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	ignoreUnused(PId);

	auto newMsgData = msgData;

	ScopedLock l(m_currentValLock);

	// special handling for some remote objects - e.g. incoming x value shall also be inserted as new value to combined xy remote object
	switch (Id)
	{
	case ROI_CoordinateMapping_SourcePosition_X:
		// we require the xy object to already be present in current values for the ch/rec addressing to be able to set the single x value
		if (m_currentValues.count(ROI_CoordinateMapping_SourcePosition_XY) > 0 
			&& m_currentValues.at(ROI_CoordinateMapping_SourcePosition_XY).count(msgData._addrVal) > 0)
		{
			// check data sanity before accessing the payload and copying the x value to xy
			auto& xyData = m_currentValues.at(ROI_CoordinateMapping_SourcePosition_XY).at(msgData._addrVal);
			if ((xyData._valType == ROVT_FLOAT && msgData._valType == ROVT_FLOAT)
				&& (xyData._valCount == 2 && msgData._valCount == 1)
				&& (xyData._payloadSize == 2 *sizeof(float) && msgData._payloadSize == sizeof(float)))
			{
				static_cast<float*>(xyData._payload)[0] = static_cast<float*>(msgData._payload)[0];
			}
		}
		break;
	case ROI_CoordinateMapping_SourcePosition_Y:
		// we require the xy object to already be present in current values for the ch/rec addressing to be able to set the single x value
		if (m_currentValues.count(ROI_CoordinateMapping_SourcePosition_XY) > 0
			&& m_currentValues.at(ROI_CoordinateMapping_SourcePosition_XY).count(msgData._addrVal) > 0)
		{
			// check data sanity before accessing the payload and copying the x value to xy
			auto& xyData = m_currentValues.at(ROI_CoordinateMapping_SourcePosition_XY).at(msgData._addrVal);
			if ((xyData._valType == ROVT_FLOAT && msgData._valType == ROVT_FLOAT)
				&& (xyData._valCount == 2 && msgData._valCount == 1)
				&& (xyData._payloadSize == 2 * sizeof(float) && msgData._payloadSize == sizeof(float)))
			{
				static_cast<float*>(xyData._payload)[1] = static_cast<float*>(msgData._payload)[0];
			}
		}
		break;
	case ROI_CoordinateMapping_SourcePosition_XY:
		// we require both the x and y object to already be present in current values for the ch/rec addressing to be able to individually set the xy object's values
		if (m_currentValues.count(ROI_CoordinateMapping_SourcePosition_X) > 0
			&& m_currentValues.at(ROI_CoordinateMapping_SourcePosition_X).count(msgData._addrVal) > 0)
		{
			// check data sanity before accessing the payload and copying the value
			auto& xData = m_currentValues.at(ROI_CoordinateMapping_SourcePosition_X).at(msgData._addrVal);
			if ((xData._valType == ROVT_FLOAT && msgData._valType == ROVT_FLOAT)
				&& (xData._valCount == 1 && msgData._valCount == 2)
				&& (xData._payloadSize == sizeof(float) && msgData._payloadSize == 2 * sizeof(float)))
			{
				static_cast<float*>(xData._payload)[0] = static_cast<float*>(msgData._payload)[0];
			}
		}
		if (m_currentValues.count(ROI_CoordinateMapping_SourcePosition_Y) > 0
			&& m_currentValues.at(ROI_CoordinateMapping_SourcePosition_Y).count(msgData._addrVal) > 0)
		{
			// check data sanity before accessing the payload and copying the value
			auto& yData = m_currentValues.at(ROI_CoordinateMapping_SourcePosition_Y).at(msgData._addrVal);
			if ((yData._valType == ROVT_FLOAT && msgData._valType == ROVT_FLOAT)
				&& (yData._valCount == 1 && msgData._valCount == 2)
				&& (yData._payloadSize == sizeof(float) && msgData._payloadSize == 2 * sizeof(float)))
			{
				static_cast<float*>(yData._payload)[0] = static_cast<float*>(msgData._payload)[1];
			}
		}
		break;
	default:
		break;
	}

	// set the data for the actual incoming remote object
	if (m_currentValues.count(Id) > 0)
	{
		// if the data entry does not exist, insert the incoming data as placeholder
		if (m_currentValues.at(Id).count(msgData._addrVal) <= 0)
			m_currentValues.at(Id).insert(std::make_pair(msgData._addrVal, msgData));

		// if the entry existed or we just inserted the incoming data, perform a fully copy incl. payload anyways
		m_currentValues.at(Id).at(msgData._addrVal).payloadCopy(newMsgData);
	}
	else
	{
		m_currentValues.insert(std::pair<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, RemoteObjectMessageData>>(Id, std::map<RemoteObjectAddressing, RemoteObjectMessageData>()));
		m_currentValues.at(Id).insert(std::make_pair(msgData._addrVal, newMsgData));
		m_currentValues.at(Id).at(msgData._addrVal).payloadCopy(newMsgData);
	}
}

/**
 * Method to be called cyclically to update the simulated values. 
 */
void DS100_DeviceSimulation::UpdateDataValues()
{
	{
		// tick our simulation base value once to be ready to generate next set of simulation values
		ScopedLock l(m_currentValLock);
		m_simulationBaseValue += 0.1f;
	}

	// iterate through all simulation relevant remote object ids to update simulation value updates
	for (auto const& roi : m_simulatedRemoteObjects)
	{
		ScopedLock l(m_currentValLock);
		jassert(m_currentValues.count(roi) > 0);
		auto& remoteAddressValueMap = m_currentValues.at(roi);

		MappingId mapping = 1;
		auto mappingsCount = m_simulatedMappingsCount;
		if (!ProcessingEngineConfig::IsRecordAddressingObject(roi))
		{
			mapping = INVALID_ADDRESS_VALUE;
			mappingsCount = 0;
		}

		for (; mapping <= mappingsCount && mapping != 0; mapping++)
		{
			SourceId channel = 1;
			auto channelCount = m_simulatedChCount;
			if (!ProcessingEngineConfig::IsChannelAddressingObject(roi))
			{
				channel = INVALID_ADDRESS_VALUE;
				channelCount = 0;
			}

			for (; channel <= channelCount && channel != 0; channel++)
			{
				RemoteObjectAddressing addressing(channel, mapping);
				jassert(remoteAddressValueMap.count(addressing) > 0);
				auto& remoteValue = remoteAddressValueMap.at(addressing);

				// update our two oscilating values
				auto val1 = (sin(m_simulationBaseValue + (channel * 0.1f)) + 1.0f) * 0.5f;
				auto val2 = (cos(m_simulationBaseValue + (channel * 0.1f)) + 1.0f) * 0.5f;

				jassert(remoteValue._payload != nullptr || remoteValue._valCount == 0);

				switch (remoteValue._valType)
				{
				case ROVT_FLOAT:
					if ((remoteValue._valCount == 1) && (remoteValue._payloadSize == sizeof(float)))
					{
						if (roi == ROI_MatrixInput_ReverbSendGain) // scale 0...1 value to gain specific -120...+24 dB range
						{
							auto rsgR = ProcessingEngineConfig::GetRemoteObjectRange(roi);
							static_cast<float*>(remoteValue._payload)[0] = (val1 * rsgR.getLength()) + rsgR.getStart();
						}
						else if (roi == ROI_CoordinateMapping_SourcePosition_Y) // use second value (cosinus) for y, to get a circle movement when visualizing single x and y values on a 2d surface ui
							static_cast<float*>(remoteValue._payload)[0] = val2;
						else
							static_cast<float*>(remoteValue._payload)[0] = val1;
					}
					else if ((remoteValue._valCount == 2) && (remoteValue._payloadSize == 2 * sizeof(float)))
					{
						static_cast<float*>(remoteValue._payload)[0] = val1;
						static_cast<float*>(remoteValue._payload)[1] = val2;
					}
					break;
				case ROVT_INT:
					if ((remoteValue._valCount == 1) && (remoteValue._payloadSize == sizeof(int)))
					{
						if (roi == ROI_Positioning_SourceDelayMode)
							static_cast<int*>(remoteValue._payload)[0] = static_cast<int>(val1 * 3.0f);
						else
							static_cast<int*>(remoteValue._payload)[0] = static_cast<int>(val1);
					}
					else if ((remoteValue._valCount == 2) && (remoteValue._payloadSize == 2 * sizeof(int)))
					{
						static_cast<int*>(remoteValue._payload)[0] = static_cast<int>(val1);
						static_cast<int*>(remoteValue._payload)[1] = static_cast<int>(val2);
					}
					break;
				case ROVT_STRING:
				case ROVT_NONE:
				default:
					break;
				}

#ifdef DEBUG
				PrintDataInfo("Updating", std::make_pair(roi, remoteValue));
#endif
			}
		}
	}
}

/**
 * Reimplemented from Timer to tick
 */
void DS100_DeviceSimulation::timerThreadCallback()
{
	UpdateDataValues();
}