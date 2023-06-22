/* Copyright (c) 2023, Christian Ahrens
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

#include "OCP1ProtocolProcessor.h"

#include "../../ProcessingEngineConfig.h"

#include "Ocp1DS100ObjectDefinitions.h"


// **************************************************************************************
//    class OCP1ProtocolProcessor
// **************************************************************************************
/**
 * Derived OCP1 remote protocol processing class
 */
OCP1ProtocolProcessor::OCP1ProtocolProcessor(const NodeId& parentNodeId)
	: NetworkProtocolProcessorBase(parentNodeId)
{
	m_type = ProtocolType::PT_OCP1Protocol;

    SetActiveRemoteObjectsInterval(5000); // used as 5s KeepAlive interval when NanoOcp connection is established
    CreateKnownONosMap();
}

/**
 * Destructor
 */
OCP1ProtocolProcessor::~OCP1ProtocolProcessor()
{
	Stop();
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool OCP1ProtocolProcessor::Start()
{
    if (m_nanoOcp)
    {
        m_nanoOcp->start();
        // Assign the callback functions AFTER internal handling is set up to not already get called before that is done
        m_nanoOcp->onDataReceived = [=](const juce::MemoryBlock& data) {
            return ocp1MessageReceived(data);
        };
        m_nanoOcp->onConnectionEstablished = [=]() {
            startTimerThread(GetActiveRemoteObjectsInterval(), 100);
            m_IsRunning = true;
            CreateObjectSubscriptions();
            QueryObjectValues();
        };
        m_nanoOcp->onConnectionLost = [=]() {
            stopTimerThread();
            m_IsRunning = false;
            DeleteObjectSubscriptions();
        };

        return true;
    }
    else
        return false;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool OCP1ProtocolProcessor::Stop()
{
    if (m_nanoOcp)
    {
        m_nanoOcp->onDataReceived = std::function<bool(const juce::MemoryBlock & data)>();
        m_nanoOcp->onConnectionEstablished = std::function<void()>();
        m_nanoOcp->onConnectionLost = std::function<void()>();
    }

	m_IsRunning = false;

	// stop the send timer thread
	stopTimerThread();

	if (m_nanoOcp)
		return m_nanoOcp->stop();
	else
		return false;
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool OCP1ProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	if (!NetworkProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		auto ocp1ConnectionModeXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::OCP1CONNECTIONMODE));
		if (ocp1ConnectionModeXmlElement)
		{
			auto modeString = ocp1ConnectionModeXmlElement->getAllSubText();
			if (modeString == "server")
				m_nanoOcp = std::make_unique<NanoOcp1::NanoOcp1Server>(GetIpAddress(), GetClientPort());
			else if (modeString == "client")
				m_nanoOcp = std::make_unique<NanoOcp1::NanoOcp1Client>(GetIpAddress(), GetClientPort());
			else
				return false;

			return true;
		}
		else
			return false;
	}
}

/**
 * Public method to get OCA object specific ObjectName string
 *
 * @param id	The object id to get the OCA specific object name
 * @return		The OCA specific object name
 */
String OCP1ProtocolProcessor::GetRemoteObjectString(RemoteObjectIdentifier id)
{
    switch (id)
    {
    case ROI_CoordinateMapping_SourcePosition:
        return "CoordinateMapping_Source_Position";
    case ROI_Positioning_SourcePosition:
        return "Positioning_Source_Position";
    case ROI_Positioning_SourceSpread:
        return "Positioning_Source_Spread";
    case ROI_Positioning_SourceDelayMode:
        return "Positioning_Source_DelayMode";
    case ROI_MatrixInput_Mute:
        return "MatrixInput_Mute";
    case ROI_MatrixInput_Gain:
        return "MatrixInput_Gain";
    case ROI_MatrixInput_ReverbSendGain:
        return "MatrixInput_ReverbSendGain";
    default:
        return "";
    }
}

/**
 * Method to trigger sending of a message
 * NOT YET IMPLEMENTED
 *
 * @param Id		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 */
bool OCP1ProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData)
{
    if (!m_nanoOcp || !m_IsRunning)
        return false;

    auto& channel = msgData._addrVal._first;
    auto& record = msgData._addrVal._second;

    std::uint32_t handle;

    switch (id)
    {
    case ROI_CoordinateMapping_SourcePosition:
    {
        if (msgData._valCount != 3 || msgData._payloadSize != 3 * sizeof(float))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(msgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_CoordinateMapping_SourcePosition_XY:
    {
        if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new xy data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
        reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(refMsgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_CoordinateMapping_SourcePosition_X:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new x data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(refMsgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_CoordinateMapping_SourcePosition_Y:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new x data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(refMsgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourcePosition:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != 3 * sizeof(float))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(msgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourcePosition_XY:
    {
        if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new xy data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
        reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(msgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourcePosition_X:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new xy data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(msgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourcePosition_Y:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
            return false;

        // get the xyz data from cache to insert the new xy data and send the xyz out
        auto& refMsgData = GetValueCache().GetValue(RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record)));
        if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
            return false;
        reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        auto parameterData = std::vector<std::uint8_t>(3 * sizeof(float), *static_cast<std::uint8_t*>(msgData._payload));
        auto posVar = objDef.ToVariant(3, parameterData);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourceSpread:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(*static_cast<float*>(msgData._payload)), handle).GetMemoryBlock());
    }
    case ROI_Positioning_SourceDelayMode:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != sizeof(int))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(*static_cast<int*>(msgData._payload)), handle).GetMemoryBlock());
    }
    case ROI_MatrixInput_Mute:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != sizeof(int))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(*static_cast<int*>(msgData._payload)), handle).GetMemoryBlock());
    }
    case ROI_MatrixInput_ReverbSendGain:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(*static_cast<float*>(msgData._payload)), handle).GetMemoryBlock());
    }
    case ROI_MatrixInput_Gain:
    {
        if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
            return false;

        GetValueCache().SetValue(RemoteObject(id, RemoteObjectAddressing(channel, record)), msgData);

        auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        return m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(*static_cast<float*>(msgData._payload)), handle).GetMemoryBlock());
    }
    case ROI_HeartbeatPing:
    {
        return m_nanoOcp->sendData(NanoOcp1::Ocp1KeepAlive(static_cast<std::uint16_t>(GetActiveRemoteObjectsInterval()/1000)).GetMemoryBlock());
    }
    default:
        return false;
    }
}

/**
 * Helper method to setup the internal map of remote object ids to ocp1 object ONos
 */
void OCP1ProtocolProcessor::CreateKnownONosMap()
{
    for (auto channel = ChannelId(1); channel <= NanoOcp1::DS100::MaxChannelCount; channel++)
    {
        auto record = RecordId(0);
        m_ROIsToDefsMap[ROI_Positioning_SourcePosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);       
        m_ROIsToDefsMap[ROI_Positioning_SourceSpread][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        m_ROIsToDefsMap[ROI_Positioning_SourceDelayMode][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_ReverbSendGain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        for (record = RecordId(MappingAreaId::MAI_First); record <= MappingAreaId::MAI_Fourth; record++)
            m_ROIsToDefsMap[ROI_CoordinateMapping_SourcePosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
    }
}

/**
 * TimerThreadBase callback to send keepalive queries cyclically
 */
void OCP1ProtocolProcessor::timerThreadCallback()
{
    if (m_IsRunning)
    {
        if (!SendRemoteObjectMessage(ROI_HeartbeatPing, RemoteObjectMessageData()))
            DBG(juce::String(__FUNCTION__) + " sending Ocp1 heartbeat failed.");
    }
}

bool OCP1ProtocolProcessor::ocp1MessageReceived(const juce::MemoryBlock& data)
{
    std::unique_ptr<NanoOcp1::Ocp1Message> msgObj = NanoOcp1::Ocp1Message::UnmarshalOcp1Message(data);
    if (msgObj)
    {
        switch (msgObj->GetMessageType())
        {
        case NanoOcp1::Ocp1Message::Notification:
        {
            NanoOcp1::Ocp1Notification* notifObj = static_cast<NanoOcp1::Ocp1Notification*>(msgObj.get());

            if (UpdateObjectValue(notifObj))
                return true;

            DBG(juce::String(__FUNCTION__) << " Got an unhandled OCA notification for ONo 0x" 
                << juce::String::toHexString(notifObj->GetEmitterOno()));
            return false;
        }
        case NanoOcp1::Ocp1Message::Response:
        {
            NanoOcp1::Ocp1Response* responseObj = static_cast<NanoOcp1::Ocp1Response*>(msgObj.get());

            auto handle = responseObj->GetResponseHandle();
            if (responseObj->GetResponseStatus() != 0)
            {
                DBG(juce::String(__FUNCTION__) << " Got an OCA response (handle:" << NanoOcp1::HandleToString(handle) <<
                    ") with status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()));
                return false;
            }
            else if (PopPendingSubscriptionHandle(handle) && !HasPendingSubscriptions())
            {
                // All subscriptions were confirmed
                DBG(juce::String(__FUNCTION__) << " All NanoOcp1 subscriptions were confirmed (handle:" 
                    << NanoOcp1::HandleToString(handle) << ")");
                return true;
            }
            else
            {
                auto ONo = PopPendingGetValueHandle(handle);
                if (0x00 != ONo)
                {
                    if (!UpdateObjectValue(ONo, responseObj))
                    {
                        DBG(juce::String(__FUNCTION__) << " Got an unhandled OCA getvalue response message (handle:" 
                            << NanoOcp1::HandleToString(handle) + ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
                        return false;
                    }
                    else
                        return true;
                }

                DBG(juce::String(__FUNCTION__) << " Got an OCA response for UNKNOWN handle " << NanoOcp1::HandleToString(handle) <<
                    "; status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()) <<
                    "; paramCount " << juce::String(responseObj->GetParamCount()));

                return false;
            }
        }
        case NanoOcp1::Ocp1Message::KeepAlive:
        {
            // provide the received message to parent node
            if (m_messageListener)
            {
                m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPong, RemoteObjectMessageData());
                return true;
            }
            else
                return false;
        }
        default:
            break;
        }
    }

    return false;
}

bool OCP1ProtocolProcessor::CreateObjectSubscriptions()
{
    if (!m_nanoOcp || !m_IsRunning)
        return false;

    auto handle = std::uint32_t(0);
    auto success = true;

    for (auto const& activeObj : GetOcp1SupportedActiveRemoteObjects())
    {
        auto& channel = activeObj._Addr._first;
        auto& record = activeObj._Addr._second;

        switch (activeObj._Id)
        {
        case ROI_CoordinateMapping_SourcePosition:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_CoordinateMapping_SourcePosition ("
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_Positioning_SourcePosition:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourcePosition (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_Positioning_SourceSpread:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourceSpread (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_Positioning_SourceDelayMode:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourceDelayMode (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_MatrixInput_Mute:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_Mute (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_MatrixInput_ReverbSendGain:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_ReverbSendGain (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_MatrixInput_Gain:
        {
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
            DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_Gain (" 
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel)
                << " handle:" << NanoOcp1::HandleToString(handle) << ")");
        }
        break;
        case ROI_CoordinateMapping_SourcePosition_XY:
        case ROI_CoordinateMapping_SourcePosition_X:
        case ROI_CoordinateMapping_SourcePosition_Y:
        {
            DBG(juce::String(__FUNCTION__) << " skipping ROI_CoordinateMapping_SourcePosition subscriptions that are not supported ("
                << "record:" << juce::String(record)
                << " channel:" << juce::String(channel) << ")");
        }
        break;
        case ROI_Positioning_SourcePosition_XY:
        case ROI_Positioning_SourcePosition_X:
        case ROI_Positioning_SourcePosition_Y:
        {            
            DBG(juce::String(__FUNCTION__) << " skipping ROI_Positioning_SourcePosition subscriptions that are not supported ("
            << "record:" << juce::String(record)
            << " channel:" << juce::String(channel) << ")");
        }
        break;
        default:
            continue; // skip adding a pending subscription handle below
        }

        AddPendingSubscriptionHandle(handle);
    }

    return success;
}

bool OCP1ProtocolProcessor::DeleteObjectSubscriptions()
{
    return false;
}

bool OCP1ProtocolProcessor::QueryObjectValues()
{
    if (!m_nanoOcp || !m_IsRunning)
        return false;

    auto handle = std::uint32_t(0);
    auto success = true;

    for (auto const& activeObj : GetOcp1SupportedActiveRemoteObjects())
    {
        auto& channel = activeObj._Addr._first;
        auto& record = activeObj._Addr._second;

        switch (activeObj._Id)
        {
        case ROI_CoordinateMapping_SourcePosition:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_CoordinateMapping_SourcePosition (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_Positioning_SourcePosition:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourcePosition (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_Positioning_SourceSpread:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourceSpread (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_Positioning_SourceDelayMode:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourceDelayMode (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_MatrixInput_Mute:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_Mute (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_MatrixInput_ReverbSendGain:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_ReverbSendGain (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_MatrixInput_Gain:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
            success = success && m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock());
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_Gain(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
        case ROI_CoordinateMapping_SourcePosition_XY:
        case ROI_CoordinateMapping_SourcePosition_X:
        case ROI_CoordinateMapping_SourcePosition_Y:
        {
            DBG(juce::String(__FUNCTION__) + " skipping ROI_CoordinateMapping_SourcePosition X Y XY");
        }
        break;
        case ROI_Positioning_SourcePosition_XY:
        case ROI_Positioning_SourcePosition_X:
        case ROI_Positioning_SourcePosition_Y:
        {
            DBG(juce::String(__FUNCTION__) + " skipping ROI_Positioning_SourcePosition X Y XY");
        }
        break;
        default:
            continue; // skip adding a pending subscription handle below
        }
    }

    return success;
}

void OCP1ProtocolProcessor::AddPendingSubscriptionHandle(const std::uint32_t handle)
{
    m_pendingSubscriptionHandles.push_back(handle);
}

bool OCP1ProtocolProcessor::PopPendingSubscriptionHandle(const std::uint32_t handle)
{
    auto it = std::find(m_pendingSubscriptionHandles.begin(), m_pendingSubscriptionHandles.end(), handle);
    if (it != m_pendingSubscriptionHandles.end())
    {
        m_pendingSubscriptionHandles.erase(it);
        return true;
    }
    else
        return false;
}

bool OCP1ProtocolProcessor::HasPendingSubscriptions()
{
    return m_pendingSubscriptionHandles.empty();
}

void OCP1ProtocolProcessor::AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo)
{
    DBG(juce::String(__FUNCTION__)
        << " (handle:" << NanoOcp1::HandleToString(handle)
        << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
    m_pendingGetValueHandlesWithONo.insert(std::make_pair(handle, ONo));
}

const std::uint32_t OCP1ProtocolProcessor::PopPendingGetValueHandle(const std::uint32_t handle)
{
    auto it = std::find_if(m_pendingGetValueHandlesWithONo.begin(), m_pendingGetValueHandlesWithONo.end(), [handle](const auto& val) { return val.first == handle; });
    if (it != m_pendingGetValueHandlesWithONo.end())
    {
        auto ONo = it->second;
        m_pendingGetValueHandlesWithONo.erase(it);
        return ONo;
    }
    else
        return 0x00;
}

bool OCP1ProtocolProcessor::HasPendingGetValues()
{
    return m_pendingGetValueHandlesWithONo.empty();
}

bool OCP1ProtocolProcessor::UpdateObjectValue(NanoOcp1::Ocp1Notification* notifObj)
{
    for (auto roisKV = m_ROIsToDefsMap.begin(); roisKV != m_ROIsToDefsMap.end(); roisKV++)
    {
        for (auto objDefKV = roisKV->second.begin(); objDefKV != roisKV->second.end(); objDefKV++)
        {
            if (notifObj->MatchesObject(&objDefKV->second))
            {
                return UpdateObjectValue(roisKV->first, dynamic_cast<NanoOcp1::Ocp1Message*>(notifObj), *objDefKV);
            }
        }
    }

    return false;
}

bool OCP1ProtocolProcessor::UpdateObjectValue(const std::uint32_t ONo, NanoOcp1::Ocp1Response* responseObj)
{
    for (auto roisKV = m_ROIsToDefsMap.begin(); roisKV != m_ROIsToDefsMap.end(); roisKV++)
    {
        for (auto objDefKV = roisKV->second.begin(); objDefKV != roisKV->second.end(); objDefKV++)
        {
            if (objDefKV->second.m_targetOno == ONo)
            {
                return UpdateObjectValue(roisKV->first, dynamic_cast<NanoOcp1::Ocp1Message*>(responseObj), *objDefKV);
            }
        }
    }

    return false;
}

bool OCP1ProtocolProcessor::UpdateObjectValue(const RemoteObjectIdentifier roi, NanoOcp1::Ocp1Message* msgObj, const std::pair<std::pair<RecordId, ChannelId>, NanoOcp1::Ocp1CommandDefinition>& objectDetails)
{
    auto objAddr = objectDetails.first;
    auto cmdDef = objectDetails.second;

    auto remObjMsgData = RemoteObjectMessageData();
    remObjMsgData._addrVal = RemoteObjectAddressing(objAddr.second, objAddr.first);

    auto objectsDataToForward = std::map<RemoteObjectIdentifier, RemoteObjectMessageData>();

    int newIntValue[3];
    float newFloatValue[3];

    switch (roi)
    {
    case ROI_CoordinateMapping_SourcePosition:
    {
        if (!NanoOcp1::VariantToPosition(cmdDef.ToVariant(3, msgObj->GetParameterData()), newFloatValue[0], newFloatValue[1], newFloatValue[2]))
            return false;
        remObjMsgData._payloadSize = 3 * sizeof(float);
        remObjMsgData._valCount = 3;
        remObjMsgData._valType = ROVT_FLOAT;
        remObjMsgData._payload = &newFloatValue;

        objectsDataToForward[ROI_CoordinateMapping_SourcePosition_XY] = RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 2, &newFloatValue, 2 * sizeof(float));
        objectsDataToForward[ROI_CoordinateMapping_SourcePosition_X].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 1, &newFloatValue, sizeof(float)));
        newFloatValue[0] = newFloatValue[1]; // hacky bit - move the y value to the first (x) position to be able to reuse the newFloatValue array for Y forwarding
        objectsDataToForward[ROI_CoordinateMapping_SourcePosition_Y] = RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 1, &newFloatValue, sizeof(float));
    }
    break;
    case ROI_Positioning_SourcePosition:
    {
        if (!NanoOcp1::VariantToPosition(cmdDef.ToVariant(3, msgObj->GetParameterData()), newFloatValue[0], newFloatValue[1], newFloatValue[2]))
            return false;
        remObjMsgData._payloadSize = 3 * sizeof(float);
        remObjMsgData._valCount = 3;
        remObjMsgData._valType = ROVT_FLOAT;
        remObjMsgData._payload = &newFloatValue;

        objectsDataToForward[ROI_Positioning_SourcePosition_XY] = RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 2, &newFloatValue, 2 * sizeof(float));
        objectsDataToForward[ROI_Positioning_SourcePosition_X].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 1, &newFloatValue, sizeof(float)));
        newFloatValue[0] = newFloatValue[1]; // hacky bit - move the y value to the first (x) position to be able to reuse the newFloatValue array for Y forwarding
        objectsDataToForward[ROI_Positioning_SourcePosition_Y] = RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 1, &newFloatValue, sizeof(float));
    }
    break;
    case ROI_Positioning_SourceSpread:
    {
        *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

        remObjMsgData._payloadSize = sizeof(float);
        remObjMsgData._valCount = 1;
        remObjMsgData._valType = ROVT_FLOAT;
        remObjMsgData._payload = &newFloatValue;

        objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
    }
    break;
    case ROI_Positioning_SourceDelayMode:
    {
        *newIntValue = NanoOcp1::DataToUint16(msgObj->GetParameterData());

        remObjMsgData._payloadSize = sizeof(int);
        remObjMsgData._valCount = 1;
        remObjMsgData._valType = ROVT_INT;
        remObjMsgData._payload = &newIntValue;

        objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
    }
    break;
    case ROI_MatrixInput_ReverbSendGain:
    {
        *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

        remObjMsgData._payloadSize = sizeof(float);
        remObjMsgData._valCount = 1;
        remObjMsgData._valType = ROVT_FLOAT;
        remObjMsgData._payload = &newFloatValue;

        objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
    }
    break;
    case ROI_MatrixInput_Gain:
    {
        *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

        remObjMsgData._payloadSize = sizeof(float);
        remObjMsgData._valCount = 1;
        remObjMsgData._valType = ROVT_FLOAT;
        remObjMsgData._payload = &newFloatValue;

        objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
    }
    break;
    case ROI_MatrixInput_Mute:
    {
        *newIntValue = NanoOcp1::DataToUint16(msgObj->GetParameterData());

        remObjMsgData._payloadSize = sizeof(int);
        remObjMsgData._valCount = 1;
        remObjMsgData._valType = ROVT_INT;
        remObjMsgData._payload = &newIntValue;

        objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
    }
    break;
    default:
        return false;
    }

    GetValueCache().SetValue(RemoteObject(roi, remObjMsgData._addrVal), remObjMsgData);

    if (m_messageListener)
    {
        for (auto const& objData : objectsDataToForward)
            m_messageListener->OnProtocolMessageReceived(this, objData.first, objData.second);
        return true;
    }
    else
        return false;
}

const std::vector<RemoteObject> OCP1ProtocolProcessor::GetOcp1SupportedActiveRemoteObjects()
{
    auto ocp1SupportedActiveObjects = std::vector<RemoteObject>();

    for (auto const& activeObj : GetActiveRemoteObjects())
    {
        auto& channel = activeObj._Addr._first;
        auto& record = activeObj._Addr._second;

        switch (activeObj._Id)
        {
        case ROI_CoordinateMapping_SourcePosition_XY:
            {                
                DBG(juce::String(__FUNCTION__) << " modifying unsupported ROI_CoordinateMapping_SourcePosition_XY to ROI_CoordinateMapping_SourcePosition in 'active' list ("
                    << "record:" << juce::String(record)
                    << " channel:" << juce::String(channel) << ")");

                auto modActiveObj = activeObj;
                modActiveObj._Id = ROI_CoordinateMapping_SourcePosition;
                ocp1SupportedActiveObjects.push_back(modActiveObj);
            }
            break;
        case ROI_Positioning_SourcePosition_XY:
            {
                DBG(juce::String(__FUNCTION__) << " modifying unsupported ROI_Positioning_SourcePosition_XY to ROI_Positioning_SourcePosition in 'active' list ("
                    << "record:" << juce::String(record)
                    << " channel:" << juce::String(channel) << ")");

                auto modActiveObj = activeObj;
                modActiveObj._Id = ROI_Positioning_SourcePosition;
                ocp1SupportedActiveObjects.push_back(modActiveObj);
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_X:
        case ROI_CoordinateMapping_SourcePosition_Y:
        case ROI_Positioning_SourcePosition_X:
        case ROI_Positioning_SourcePosition_Y:
            {
                DBG(juce::String(__FUNCTION__) << " filtering unsupported ROI (" << activeObj._Id << ") from 'active' list ("
                    << "record:" << juce::String(record)
                    << " channel:" << juce::String(channel) << ")");
            }
            break;
        default:
            ocp1SupportedActiveObjects.push_back(activeObj);
            break;
        }
    }

    return ocp1SupportedActiveObjects;
}

