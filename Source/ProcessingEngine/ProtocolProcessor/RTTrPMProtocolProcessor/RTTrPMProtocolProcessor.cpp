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
	: NetworkProtocolProcessorBase(parentNodeId), m_rttrpmReceiver(listenerPortNumber)
{
	m_type = ProtocolType::PT_RTTrPMProtocol;

	// RTTrPMProtocolProcessor derives from RTTrPMReceiver::RealtimeListener
	m_rttrpmReceiver.addListener(this);
}

/**
 * Destructor
 */
RTTrPMProtocolProcessor::~RTTrPMProtocolProcessor()
{
	Stop();

	m_rttrpmReceiver.removeListener(this);
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool RTTrPMProtocolProcessor::Start()
{
	m_IsRunning = m_rttrpmReceiver.start();

	return m_IsRunning;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool RTTrPMProtocolProcessor::Stop()
{
	m_IsRunning = !m_rttrpmReceiver.stop();

	return !m_IsRunning;
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool RTTrPMProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	if (!ProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		auto hostPortXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::HOSTPORT));
		if (hostPortXmlElement)
			SetHostPort(hostPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT)));
		else
			return false;

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
bool RTTrPMProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
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
		return;

	ignoreUnused(senderPort);
	if (!GetIpAddress().empty() && senderIPAddress != String(GetIpAddress()))
	{
#ifdef DEBUG
		DBG("NId" + String(m_parentNodeId)
			+ " PId" + String(m_protocolProcessorId) + ": ignore unexpected OSC message from " 
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
	
	//float newFloatValue;
	float newDualFloatValue[2];
	//int newIntValue;

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

						ChannelId channelId = String(trackableModule->GetName()).getIntValue();

						newMsgData._addrVal._first = channelId;
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

						if (m_mappingAreaId == MAI_Invalid)
							newObjectId = ROI_Positioning_SourcePosition_XY;
						else
							newObjectId = ROI_CoordinateMapping_SourcePosition_XY;

						newDualFloatValue[0] = static_cast<float>(trackedPointPositionModule->GetX());
						newDualFloatValue[1] = static_cast<float>(trackedPointPositionModule->GetY());

						newMsgData._valType = ROVT_FLOAT;
						newMsgData._valCount = 2;
						newMsgData._payload = &newDualFloatValue;
						newMsgData._payloadSize = 2 * sizeof(float);

						// If the received data targets a muted object, dont forward the message
						if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
							return;

						// provide the received message to parent node
						else if (m_messageListener)
							m_messageListener->OnProtocolMessageReceived(this, newObjectId, newMsgData);
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
						//	+ String::formatted(" xc%f,yc%f,zc%f", CentroidAccelAndVeloModule->GetXCoordinate(), CentroidAccelAndVeloModule->GetYCoordinate(), CentroidAccelAndVeloModule->GetZCoordinate())
						//	+ String::formatted(" xa%f,ya%f,za%f", CentroidAccelAndVeloModule->GetXAcceleration(), CentroidAccelAndVeloModule->GetYAcceleration(), CentroidAccelAndVeloModule->GetZAcceleration())
						//	+ String::formatted(" xv%f,yv%f,zv%f", CentroidAccelAndVeloModule->GetXVelocity(), CentroidAccelAndVeloModule->GetYVelocity(), CentroidAccelAndVeloModule->GetZVelocity()));
					}
				}
				break;
			case PacketModule::TrackedPointAccelerationandVelocity:
				{
					const TrackedPointAccelAndVeloModule* trackedPointAccelAndVeloModule = dynamic_cast<const TrackedPointAccelAndVeloModule*>(RTTrPMmodule.get());
					if (trackedPointAccelAndVeloModule)
					{
						//DBG("TrackedPointAccelAndVeloModule: idx" + String(TrackedPointAccelAndVeloModule->GetPointIndex())
						//	+ String::formatted(" xc%f,yc%f,zc%f", TrackedPointAccelAndVeloModule->GetXCoordinate(), TrackedPointAccelAndVeloModule->GetYCoordinate(), TrackedPointAccelAndVeloModule->GetZCoordinate())
						//	+ String::formatted(" xa%f,ya%f,za%f", TrackedPointAccelAndVeloModule->GetXAcceleration(), TrackedPointAccelAndVeloModule->GetYAcceleration(), TrackedPointAccelAndVeloModule->GetZAcceleration())
						//	+ String::formatted(" xv%f,yv%f,zv%f", TrackedPointAccelAndVeloModule->GetXVelocity(), TrackedPointAccelAndVeloModule->GetYVelocity(), TrackedPointAccelAndVeloModule->GetZVelocity()));
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
