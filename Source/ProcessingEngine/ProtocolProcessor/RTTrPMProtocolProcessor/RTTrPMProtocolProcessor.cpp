/* Copyright (c) 2020-2023, Christian Ahrens
 *
 * This file is part of RemoteProtocolBridgeCore <https://github.com/ChristianAhrens/RemoteProtocolBridgeCore>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "RTTrPMProtocolProcessor.h"

#include "../../../ProcessingEngine/ProcessingEngineConfig.h"


// **************************************************************************************
//    class RTTrPMProtocolProcessor
// **************************************************************************************
/**
 * Derived RTTrPM remote protocol processing class
 */
RTTrPMProtocolProcessor::RTTrPMProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
	: NetworkProtocolProcessorBase(parentNodeId)
{
	m_type = ProtocolType::PT_RTTrPMProtocol;

	// RTTrPMProtocolProcessor derives from RTTrPMReceiver::RealtimeListener
	m_rttrpmReceiver = std::make_unique<RTTrPMReceiver>(listenerPortNumber);
	m_rttrpmReceiver->addListener(this);
}

/**
 * Destructor
 */
RTTrPMProtocolProcessor::~RTTrPMProtocolProcessor()
{
	Stop();

	if (m_rttrpmReceiver)
		m_rttrpmReceiver->removeListener(this);
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool RTTrPMProtocolProcessor::Start()
{
	if (m_rttrpmReceiver)
		m_IsRunning = m_rttrpmReceiver->start();

	return m_IsRunning;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool RTTrPMProtocolProcessor::Stop()
{
	if (m_rttrpmReceiver)
		m_IsRunning = !m_rttrpmReceiver->stop();

	return !m_IsRunning;
}

/**
 * Reimplemented setter for the internal host port member,
 * to be able to reinstantiate the rttrpm receiver object with the changed port.
 * @param	hostPort	The value to set for host port member.
 */
void RTTrPMProtocolProcessor::SetHostPort(std::int32_t hostPort)
{
	if (hostPort != GetHostPort())
	{
		NetworkProtocolProcessorBase::SetHostPort(hostPort);

		if (m_IsRunning && m_rttrpmReceiver)
			m_rttrpmReceiver->stop();
		m_rttrpmReceiver = std::make_unique<RTTrPMReceiver>(hostPort);
		m_rttrpmReceiver->addListener(this);
		if (m_IsRunning && m_rttrpmReceiver)
			m_rttrpmReceiver->start();
	}
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool RTTrPMProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	auto stateXmlUpdateSuccess = true;
	if (!ProtocolProcessorBase::setStateXml(stateXml))
		stateXmlUpdateSuccess = false;
	else
	{
		auto hostPortXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::HOSTPORT));
		if (hostPortXmlElement)
			SetHostPort(hostPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT)));
		else
			stateXmlUpdateSuccess = false;

		auto mappingAreaXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::MAPPINGAREA));
		if (mappingAreaXmlElement)
			m_mappingAreaId = static_cast<MappingAreaId>(mappingAreaXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID)));
		else
			stateXmlUpdateSuccess = false;

		m_packetModuleTypesForPositioning.clear();
		auto moduleTypeForPositioningXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PACKETMODULE));
		if (moduleTypeForPositioningXmlElement)
		{
			auto moduleTypeIdentifier = moduleTypeForPositioningXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::TYPE));
			for (auto const& moduleType : PacketModule::PacketModuleTypes)
			{
				if (moduleTypeIdentifier.contains(GetRTTrPMModuleString(moduleType)))
				{
					m_packetModuleTypesForPositioning.add(moduleType);
				}
			}
			stateXmlUpdateSuccess = false;
		}
		else
			stateXmlUpdateSuccess = false;

		auto mappingAreaRescaleXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::MAPPINGAREARESCALE));
		if (mappingAreaRescaleXmlElement)
		{
			auto mappingAreaRescaleTextElement = mappingAreaRescaleXmlElement->getFirstChildElement();
			if (mappingAreaRescaleTextElement && mappingAreaRescaleTextElement->isTextElement())
			{
				auto rangeRescaleValues = StringArray();
				if (4 != rangeRescaleValues.addTokens(mappingAreaRescaleTextElement->getText(), ";", ""))
				{
					m_mappingAreaRescaleRangeX = juce::Range<float>(0.0f, 0.0f);
					m_mappingAreaRescaleRangeY = juce::Range<float>(0.0f, 0.0f);
				}
				else
				{
					auto minX = rangeRescaleValues[0].getFloatValue();
					auto maxX = rangeRescaleValues[1].getFloatValue();
					auto minY = rangeRescaleValues[2].getFloatValue();
					auto maxY = rangeRescaleValues[3].getFloatValue();

					m_mappingAreaRescaleRangeX = juce::Range<float>(minX, maxX);
					m_mappingAreaRescaleRangeY = juce::Range<float>(minY, maxY);
				}
			}
			else
				stateXmlUpdateSuccess = false;
		}
		else
			stateXmlUpdateSuccess = false;

		auto beaconIdxRemappingsXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::REMAPPINGS));
		if (beaconIdxRemappingsXmlElement)
		{
			m_beaconIdxToChannelMap.clear();
			auto beaconIdxRemappingXmlElement = beaconIdxRemappingsXmlElement->getFirstChildElement();
			while (nullptr != beaconIdxRemappingXmlElement)
			{
				if (beaconIdxRemappingXmlElement->getTagName() == ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::REMAPPINGS))
				{
					auto beaconIdxRemappingTextElement = beaconIdxRemappingXmlElement->getFirstChildElement();
					if (beaconIdxRemappingTextElement && beaconIdxRemappingTextElement->isTextElement())
					{
						auto beaconIdx = beaconIdxRemappingXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID), -1);
						auto channel = ChannelId(beaconIdxRemappingTextElement->getText().getIntValue());
						m_beaconIdxToChannelMap.insert(std::make_pair(beaconIdx, channel));
					}
				}

				beaconIdxRemappingXmlElement = beaconIdxRemappingXmlElement->getNextElement();
			}
		}
		else
			stateXmlUpdateSuccess = true; // beaconIdx remapping is not mandatory!
	}
	return stateXmlUpdateSuccess;
}

/**
 * Method to trigger sending of a message
 *
 * @param Id		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 * @param externalId	An optional external id for identification of replies, etc. 
 *						(unused in this protocolprocessor impl)
 */
bool RTTrPMProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData, const int externalId)
{
	ignoreUnused(externalId);
	ignoreUnused(Id);
	ignoreUnused(msgData);

	// currently, RTTrPM Protocol implementation does not support sending data
	return false;
}

/**
 * Called when the RTTrPM server receives a new RTTrPM packet module
 *
 * @param module			The received RTTrPM module.
 * @param senderIPAddress	The ip the message originates from.
 * @param senderPort			The port this message was received on.
 */
void RTTrPMProtocolProcessor::RTTrPMModuleReceived(const RTTrPMReceiver::RTTrPMMessage& rttrpmMessage, const String& senderIPAddress, const int& senderPort)
{
	if (rttrpmMessage.header.GetPacketSize() == 0)
	{
		std::stringstream ssdbg;
		ssdbg << __FUNCTION__ << " ERROR: empty RTTrPM message header";
		std::cout << ssdbg.str() << std::endl;
		DBG(ssdbg.str());
		return;
	}

	if (!rttrpmMessage.header.IsLittleEndian())
	{
		std::stringstream ssdbg;
		ssdbg << __FUNCTION__ << " ERROR: only LittleEndian RTTrPM encoding supported";
		std::cout << ssdbg.str() << std::endl;
		DBG(ssdbg.str());
		return;
	}

	ignoreUnused(senderPort);
	if (!GetIpAddress().empty() && senderIPAddress != String(GetIpAddress()))
	{
#ifdef DEBUG
		DBG("NId" + String(m_parentNodeId)
			+ " PId" + String(m_protocolProcessorId) + ": ignore unexpected RTTrPM message from " 
			+ senderIPAddress + " (" + String(GetIpAddress()) + " expected)");
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
	
	std::vector<float> newDualFloatValue = { 0.0f, 0.0f };

	for (auto const& RTTrPMmodule : rttrpmMessage.modules)
	{
		if (RTTrPMmodule->isValid())
		{
			switch (RTTrPMmodule->GetModuleType())
			{
			case PacketModule::WithTimestamp:
			case PacketModule::WithoutTimestamp:
				{
					const PacketModuleTrackable* trackableModule = dynamic_cast<const PacketModuleTrackable*>(RTTrPMmodule.get());
					if (trackableModule)
					{
						//DBG("PacketModuleTrackable('" + String(trackableModule->GetName()) + "'): seqNo" 
						//	+ String(trackableModule->GetSeqNumber()) + " with " + String(trackableModule->GetNumberOfSubModules()) + " submodules");

						auto beaconIdx = String(trackableModule->GetName()).getIntValue();
						// in case the beaconIdx to channel remapping contains the incoming index, we use that remapping
						if (m_beaconIdxToChannelMap.find(beaconIdx) != m_beaconIdxToChannelMap.end())
							newMsgData._addrVal._first = m_beaconIdxToChannelMap.at(beaconIdx);
						// otherwise the generic channel equals beaconIdx scheme is applied
						else
							newMsgData._addrVal._first = ChannelId(beaconIdx);
						newMsgData._addrVal._second = static_cast<RecordId>(m_mappingAreaId);
					}
				}
				break;
			case PacketModule::CentroidPosition:
				{
					const CentroidPositionModule* centroidPositionModule = dynamic_cast<const CentroidPositionModule*>(RTTrPMmodule.get());
					if (centroidPositionModule)
					{
						//DBG("CentroidPositionModule: latency" 
						//	+ String(centroidPositionModule->GetLatency()) 
						//	+ String::formatted(" x%f,y%f,z%f", centroidPositionModule->GetX(), centroidPositionModule->GetY(), centroidPositionModule->GetZ()));

						if (m_packetModuleTypesForPositioning.contains(PacketModule::CentroidPosition))
						{
							if (m_mappingAreaId == MAI_Invalid)
							{
								newObjectId = ROI_Positioning_SourcePosition_XY;
								newDualFloatValue = { static_cast<float>(centroidPositionModule->GetX()), static_cast<float>(centroidPositionModule->GetY()) };
							}
							else
							{
								newObjectId = ROI_CoordinateMapping_SourcePosition_XY;
								newDualFloatValue = GetMappedPosition({ static_cast<float>(centroidPositionModule->GetX()), static_cast<float>(centroidPositionModule->GetY()) });
							}

							newMsgData._valType = ROVT_FLOAT;
							newMsgData._valCount = 2;
							newMsgData._payload = newDualFloatValue.data();
							newMsgData._payloadSize = 2 * sizeof(float);

							// If the received data targets a muted object, dont forward the message
							if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
								continue;

							// provide the received message to parent node
							else if (m_messageListener)
								m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
						}
					}
				}
				break;
			case PacketModule::TrackedPointPosition:
				{
					const TrackedPointPositionModule* trackedPointPositionModule = dynamic_cast<const TrackedPointPositionModule*>(RTTrPMmodule.get());
					if (trackedPointPositionModule)
					{
						//DBG("TrackedPointPositionModule: idx" 
						//	+ String(trackedPointPositionModule->GetPointIndex()) 
						//	+ " latency" + String(trackedPointPositionModule->GetLatency()) 
						//	+ String::formatted(" x%f,y%f,z%f", trackedPointPositionModule->GetX(), trackedPointPositionModule->GetY(), trackedPointPositionModule->GetZ()));

						if (m_packetModuleTypesForPositioning.contains(PacketModule::TrackedPointPosition))
						{
							if (m_mappingAreaId == MAI_Invalid)
							{
								newObjectId = ROI_Positioning_SourcePosition_XY;
								newDualFloatValue = { static_cast<float>(trackedPointPositionModule->GetX()), static_cast<float>(trackedPointPositionModule->GetY()) };
							}
							else
							{
								newObjectId = ROI_CoordinateMapping_SourcePosition_XY;
								newDualFloatValue = GetMappedPosition({ static_cast<float>(trackedPointPositionModule->GetX()), static_cast<float>(trackedPointPositionModule->GetY()) });
							}

							newMsgData._valType = ROVT_FLOAT;
							newMsgData._valCount = 2;
							newMsgData._payload = newDualFloatValue.data();
							newMsgData._payloadSize = 2 * sizeof(float);

							// If the received data targets a muted object, dont forward the message
							if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
								continue;

							// provide the received message to parent node
							else if (m_messageListener)
								m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
						}
					}
				}
				break;
			case PacketModule::OrientationQuaternion:
				{
					const OrientationQuaternionModule* orientationQuaternionModule = dynamic_cast<const OrientationQuaternionModule*>(RTTrPMmodule.get());
					if (orientationQuaternionModule)
					{
						//DBG("OrientationQuaternionModule: latency" 
						//	+ String(orientationQuaternionModule->GetLatency()) 
						//	+ String::formatted(" qx%f,qy%f,qz%f,qw%f", orientationQuaternionModule->GetQx(), orientationQuaternionModule->GetQy(), orientationQuaternionModule->GetQz(), orientationQuaternionModule->GetQw()));
					}
				}
				break;
			case PacketModule::OrientationEuler:
				{
					const OrientationEulerModule* orientationEulerModule = dynamic_cast<const OrientationEulerModule*>(RTTrPMmodule.get());
					if (orientationEulerModule)
					{
						//DBG("OrientationEulerModule: latency" 
						//	+ String(orientationEulerModule->GetLatency()) + " order" 
						//	+ String(orientationEulerModule->GetOrder()) 
						//	+ String::formatted(" r1%f,r2%f,r3%f", orientationEulerModule->GetR1(), orientationEulerModule->GetR2(), orientationEulerModule->GetR3()));
					}
				}
				break;
			case PacketModule::CentroidAccelerationAndVelocity:
				{
					const CentroidAccelAndVeloModule* centroidAccelAndVeloModule = dynamic_cast<const CentroidAccelAndVeloModule*>(RTTrPMmodule.get());
					if (centroidAccelAndVeloModule)
					{
						//DBG("CentroidAccelAndVeloModule:"
						//	+ String::formatted(" xc%f,yc%f,zc%f", centroidAccelAndVeloModule->GetXCoordinate(), centroidAccelAndVeloModule->GetYCoordinate(), centroidAccelAndVeloModule->GetZCoordinate())
						//	+ String::formatted(" xa%f,ya%f,za%f", centroidAccelAndVeloModule->GetXAcceleration(), centroidAccelAndVeloModule->GetYAcceleration(), centroidAccelAndVeloModule->GetZAcceleration())
						//	+ String::formatted(" xv%f,yv%f,zv%f", centroidAccelAndVeloModule->GetXVelocity(), centroidAccelAndVeloModule->GetYVelocity(), centroidAccelAndVeloModule->GetZVelocity()));

						if (m_packetModuleTypesForPositioning.contains(PacketModule::CentroidAccelerationAndVelocity))
						{
							if (m_mappingAreaId == MAI_Invalid)
							{
								newObjectId = ROI_Positioning_SourcePosition_XY;
								newDualFloatValue = { static_cast<float>(centroidAccelAndVeloModule->GetXCoordinate()), static_cast<float>(centroidAccelAndVeloModule->GetYCoordinate()) };
							}
							else
							{
								newObjectId = ROI_CoordinateMapping_SourcePosition_XY;
								newDualFloatValue = GetMappedPosition({ static_cast<float>(centroidAccelAndVeloModule->GetXCoordinate()), static_cast<float>(centroidAccelAndVeloModule->GetYCoordinate()) });
							}

							newMsgData._valType = ROVT_FLOAT;
							newMsgData._valCount = 2;
							newMsgData._payload = newDualFloatValue.data();
							newMsgData._payloadSize = 2 * sizeof(float);

							// If the received data targets a muted object, dont forward the message
							if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
								continue;

							// provide the received message to parent node
							else if (m_messageListener)
								m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
						}
					}
				}
				break;
			case PacketModule::TrackedPointAccelerationAndVelocity:
				{
					const TrackedPointAccelAndVeloModule* trackedPointAccelAndVeloModule = dynamic_cast<const TrackedPointAccelAndVeloModule*>(RTTrPMmodule.get());
					if (trackedPointAccelAndVeloModule)
					{
						//DBG("TrackedPointAccelAndVeloModule: idx" + String(trackedPointAccelAndVeloModule->GetPointIndex())
						//	+ String::formatted(" xc%f,yc%f,zc%f", trackedPointAccelAndVeloModule->GetXCoordinate(), trackedPointAccelAndVeloModule->GetYCoordinate(), trackedPointAccelAndVeloModule->GetZCoordinate())
						//	+ String::formatted(" xa%f,ya%f,za%f", trackedPointAccelAndVeloModule->GetXAcceleration(), trackedPointAccelAndVeloModule->GetYAcceleration(), trackedPointAccelAndVeloModule->GetZAcceleration())
						//	+ String::formatted(" xv%f,yv%f,zv%f", trackedPointAccelAndVeloModule->GetXVelocity(), trackedPointAccelAndVeloModule->GetYVelocity(), trackedPointAccelAndVeloModule->GetZVelocity()));

						if (m_packetModuleTypesForPositioning.contains(PacketModule::TrackedPointAccelerationAndVelocity))
						{
							if (m_mappingAreaId == MAI_Invalid)
							{
								newObjectId = ROI_Positioning_SourcePosition_XY;
								newDualFloatValue = { static_cast<float>(trackedPointAccelAndVeloModule->GetXCoordinate()), static_cast<float>(trackedPointAccelAndVeloModule->GetYCoordinate()) };
							}
							else
							{
								newObjectId = ROI_CoordinateMapping_SourcePosition_XY;
								newDualFloatValue = GetMappedPosition({ static_cast<float>(trackedPointAccelAndVeloModule->GetXCoordinate()), static_cast<float>(trackedPointAccelAndVeloModule->GetYCoordinate()) });
							}

							newMsgData._valType = ROVT_FLOAT;
							newMsgData._valCount = 2;
							newMsgData._payload = newDualFloatValue.data();
							newMsgData._payloadSize = 2 * sizeof(float);

							// If the received data targets a muted object, dont forward the message
							if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
								continue;

							// provide the received message to parent node
							else if (m_messageListener)
								m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
						}
					}
				}
				break;
			case PacketModule::ZoneCollisionDetection:
				{
					const ZoneCollisionDetectionModule* zoneCollisionDetectionModule = dynamic_cast<const ZoneCollisionDetectionModule*>(RTTrPMmodule.get());
					if (zoneCollisionDetectionModule)
					{
						//DBG("ZoneCollisionDetectionModule: contains " + String(zoneCollisionDetectionModule->GetZoneSubModules().size()) + "submodules");
					}
				}
				break;
			case PacketModule::Invalid:
				DBG("Invalid PacketModule!");
				break;
			default:
				break;
			}
		}
		else
		{
			newObjectId = ROI_Invalid;
		}
	}
}

/**
 * Static helper method that can be used to convert a given module type int value
 * to its string representation to be used in config or to use for human readable interfaces.
 * @param	moduleType	The type of the packet module to get a string representation for.
 * @return	The string representation for the given packet module type or empty if invalid.
 */
const juce::String RTTrPMProtocolProcessor::GetRTTrPMModuleString(PacketModule::PacketModuleType moduleType)
{
	switch(moduleType)
	{
	case PacketModule::WithTimestamp:
		return "WithTimestamp";
	case PacketModule::WithoutTimestamp:
		return "WithoutTimestamp";
	case PacketModule::CentroidPosition:
		return "CentroidPosition";
	case PacketModule::TrackedPointPosition:
		return "TrackedPointPosition";
	case PacketModule::OrientationQuaternion:
		return "OrientationQuaternion";
	case PacketModule::OrientationEuler:
		return "OrientationEuler";
	case PacketModule::CentroidAccelerationAndVelocity:
		return "CentroidAccelerationAndVelocity";
	case PacketModule::TrackedPointAccelerationAndVelocity:
		return "TrackedPointAccelerationAndVelocity";
	case PacketModule::ZoneCollisionDetection:
		return "ZoneCollisionDetection";
	case PacketModule::Invalid:
	default:
		return "";
	}
}

/**
 * Helper method to map a given xy position to the configured rescale origin/range and return this.
 * @param	moduleDataPosition	The xy position to be mapped
 * @return	The mapped xy position value
 */
std::vector<float> RTTrPMProtocolProcessor::GetMappedPosition(const std::vector<float>& moduleDataPosition)
{
	jassert(2 == moduleDataPosition.size());
	if (2 != moduleDataPosition.size())
		return {0.0f, 0.0f};

	jassert(0.0f != m_mappingAreaRescaleRangeX.getLength() && 0.0f != m_mappingAreaRescaleRangeY.getLength());
	if (0.0f == m_mappingAreaRescaleRangeX.getLength() || 0.0f == m_mappingAreaRescaleRangeY.getLength())
		return { 0.0f, 0.0f };

	std::vector<float> mappedPosition(2);
	mappedPosition[0] = (moduleDataPosition.at(0) - m_mappingAreaRescaleRangeX.getStart()) / m_mappingAreaRescaleRangeX.getLength();
	mappedPosition[1] = (moduleDataPosition.at(1) - m_mappingAreaRescaleRangeY.getStart()) / m_mappingAreaRescaleRangeY.getLength();

	return mappedPosition;
}
