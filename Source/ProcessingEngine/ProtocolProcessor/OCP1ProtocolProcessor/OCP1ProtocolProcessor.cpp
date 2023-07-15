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
            ClearPendingHandles();
            GetValueCache().Clear();
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
 * @param roi	The object id to get the OCA specific object name
 * @return		The OCA specific object name
 */
juce::String OCP1ProtocolProcessor::GetRemoteObjectString(const RemoteObjectIdentifier roi)
{
    switch (roi)
    {
    case ROI_CoordinateMapping_SourcePosition:
    case ROI_CoordinateMapping_SourcePosition_XY:
    case ROI_CoordinateMapping_SourcePosition_X:
    case ROI_CoordinateMapping_SourcePosition_Y:
        return "CoordinateMapping_Source_Position";
    case ROI_Positioning_SourcePosition:
    case ROI_Positioning_SourcePosition_XY:
    case ROI_Positioning_SourcePosition_X:
    case ROI_Positioning_SourcePosition_Y:
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
        return "?";
    }
}

/**
 * Method to trigger sending of a message
 * This method implements special handling for single or dual float value 
 * positioning ROIs by mapping them to the triple float value equivalent and uses
 * the value cache to fill in the missing data.
 *
 * @param roi		    The id of the object to send a message for
 * @param msgData	    The message payload and metadata
 * @param externalId	An optional external id for identification of replies, etc. ...
 */
bool OCP1ProtocolProcessor::SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId)
{
    // if NanoOcp does not exist or the processor is not running, give up immediately
    if (!m_nanoOcp || !m_IsRunning)
        return false;

    // if we are dealing with the special ROI for heartbeat (Ocp1 Keepalive), send it right away
    if (roi == ROI_HeartbeatPing)
        return m_nanoOcp->sendData(NanoOcp1::Ocp1KeepAlive(static_cast<std::uint16_t>(GetActiveRemoteObjectsInterval() / 1000)).GetMemoryBlock());

    // if the ROI data is empty, it is a value request message that must be handled as such
    if (msgData.isDataEmpty())
        return QueryObjectValue(roi, msgData._addrVal._first, msgData._addrVal._second);

    auto& channel = msgData._addrVal._first;
    auto& record = msgData._addrVal._second;

    auto handle = std::uint32_t(0x00);
    auto sendSuccess = false;

    float zeroPayload[3] = { 0.0f, 0.0f, 0.0f };

    switch (roi)
    {
    case ROI_CoordinateMapping_SourcePosition:
        {
            if (msgData._valCount != 3 || msgData._payloadSize != 3 * sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(msgData._payload)[0],
                reinterpret_cast<float*>(msgData._payload)[1],
                reinterpret_cast<float*>(msgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_XY:
        {
            if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new xy data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
            reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0], 
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);

            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_X:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new x data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0],
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_Y:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new x data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0],
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourcePosition:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != 3 * sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(msgData._payload)[0],
                reinterpret_cast<float*>(msgData._payload)[1],
                reinterpret_cast<float*>(msgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourcePosition_XY:
        {
            if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new xy data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
            reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << roi << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0],
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourcePosition_X:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new xy data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << roi << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0],
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourcePosition_Y:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                return false;

            auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if (!GetValueCache().Contains(targetObj))
                GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

            // get the xyz data from cache to insert the new xy data and send the xyz out
            auto& refMsgData = GetValueCache().GetValue(targetObj);
            if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                return false;
            reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
            //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
            //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
            //DBG(juce::String(__FUNCTION__) << " ROI:" << roi << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

            GetValueCache().SetValue(targetObj, refMsgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            auto parameterData = NanoOcp1::DataFromPosition(
                reinterpret_cast<float*>(refMsgData._payload)[0],
                reinterpret_cast<float*>(refMsgData._payload)[1],
                reinterpret_cast<float*>(refMsgData._payload)[2]);
            auto posVar = objDef.ToVariant(3, parameterData);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(posVar), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourceSpread:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
            auto spreadValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(spreadValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_Positioning_SourceDelayMode:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(int))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
            auto delayModeValue = *static_cast<int*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(delayModeValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixInput_Mute:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(int))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
            auto muteValue = (*static_cast<int*>(msgData._payload) == 1) ? 1 : 2; // internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(muteValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixInput_ReverbSendGain:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
            auto gainValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(gainValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixInput_Gain:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
            auto gainValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(gainValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixInput_ChannelName:
        {
            if (msgData._valCount < 1 || msgData._payloadSize != msgData._valCount * sizeof(char))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
            auto stringValue = juce::String(static_cast<const char*>(msgData._payload), msgData._payloadSize);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(stringValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixInput_LevelMeterPreMute:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel);
            auto levelValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(levelValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
            break;
        }
        break;
    case ROI_MatrixOutput_Mute:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(int))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
            auto muteValue = (*static_cast<int*>(msgData._payload) == 1) ? 1 : 2; // internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(muteValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixOutput_Gain:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
            auto gainValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(gainValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixOutput_ChannelName:
        {
            if (msgData._valCount < 1 || msgData._payloadSize != msgData._valCount * sizeof(char))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
            auto stringValue = juce::String(static_cast<const char*>(msgData._payload), msgData._payloadSize);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(stringValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    case ROI_MatrixOutput_LevelMeterPreMute:
        {
            if (msgData._valCount != 1 || msgData._payloadSize != sizeof(float))
                return false;

            GetValueCache().SetValue(RemoteObject(roi, RemoteObjectAddressing(channel, record)), msgData);

            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel);
            auto levelValue = *static_cast<float*>(msgData._payload);
            sendSuccess = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(objDef.SetValueCommand(levelValue), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, objDef.m_targetOno, externalId);
        }
        break;
    default:
        DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> not implmented");
        break;
    }

    return sendSuccess;
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
        m_ROIsToDefsMap[ROI_MatrixInput_ChannelName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_LevelMeterPreMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_ChannelName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_LevelMeterPreMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel);
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

                auto externalId = -1;
                PopPendingSubscriptionHandle(handle);
                PopPendingGetValueHandle(handle);
                PopPendingSetValueHandle(handle, externalId);

                return false;
            }
            else if (PopPendingSubscriptionHandle(handle))
            {
                if (!HasPendingSubscriptions())
                {
                    // All subscriptions were confirmed
                    //DBG(juce::String(__FUNCTION__) << " All NanoOcp1 subscriptions were confirmed (handle:"
                    //    << NanoOcp1::HandleToString(handle) << ")");
                }
                return true;
            }
            else
            {
                auto GetValONo = PopPendingGetValueHandle(handle);
                if (0x00 != GetValONo)
                {
                    if (!UpdateObjectValue(GetValONo, responseObj))
                    {
                        DBG(juce::String(__FUNCTION__) << " Got an unhandled OCA getvalue response message (handle:" 
                            << NanoOcp1::HandleToString(handle) + ", targetONo:0x" << juce::String::toHexString(GetValONo) << ")");
                        return false;
                    }
                    else
                    {
                        if (!HasPendingGetValues())
                        {
                            // All subscriptions were confirmed
                            //DBG(juce::String(__FUNCTION__) << " All pending NanoOcp1 getvalue commands were confirmed (handle:"
                            //    << NanoOcp1::HandleToString(handle) << ")");
                        }
                        return true;
                    }
                }

                auto externalId = -1;
                auto SetValONo = PopPendingSetValueHandle(handle, externalId);
                if (0x00 != SetValONo)
                {
                    if (!HasPendingSetValues())
                    {
                        // All subscriptions were confirmed
                        //DBG(juce::String(__FUNCTION__) << " All pending NanoOcp1 setvalue commands were confirmed (handle:"
                        //    << NanoOcp1::HandleToString(handle) << ")");
                    }
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
                //DBG(juce::String(__FUNCTION__) << " ROI_CoordinateMapping_SourcePosition ("
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_Positioning_SourcePosition:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourcePosition (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_Positioning_SourceSpread:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourceSpread (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_Positioning_SourceDelayMode:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_Positioning_SourceDelayMode (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixInput_Mute:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_Mute (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixInput_ReverbSendGain:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_ReverbSendGain (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixInput_Gain:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_Gain (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixInput_ChannelName:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_ChannelName (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixInput_LevelMeterPreMute:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_LevelMeterPreMute (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixOutput_Mute:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixOutput_Mute (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixOutput_Gain:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixOutput_Gain (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixOutput_ChannelName:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixOutput_ChannelName (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_MatrixOutput_LevelMeterPreMute:
            {
                success = success && m_nanoOcp->sendData(
                    NanoOcp1::Ocp1CommandResponseRequired(
                        NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel).AddSubscriptionCommand(), handle).GetMemoryBlock());
                //DBG(juce::String(__FUNCTION__) << " ROI_MatrixInput_LevelMeterPreMute (" 
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel)
                //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_XY:
        case ROI_CoordinateMapping_SourcePosition_X:
        case ROI_CoordinateMapping_SourcePosition_Y:
            {
                //DBG(juce::String(__FUNCTION__) << " skipping ROI_CoordinateMapping_SourcePosition subscriptions that are not supported ("
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel) << ")");
            }
            break;
        case ROI_Positioning_SourcePosition_XY:
        case ROI_Positioning_SourcePosition_X:
        case ROI_Positioning_SourcePosition_Y:
            {            
                //DBG(juce::String(__FUNCTION__) << " skipping ROI_Positioning_SourcePosition subscriptions that are not supported ("
                //  << "record:" << juce::String(record)
                //  << " channel:" << juce::String(channel) << ")");
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

    auto success = true;

    for (auto const& activeObj : GetOcp1SupportedActiveRemoteObjects())
    {
        auto& roi = activeObj._Id;
        auto& channel = activeObj._Addr._first;
        auto& record = activeObj._Addr._second;

        success = QueryObjectValue(roi, channel, record) && success;
    }

    return success;
}

bool OCP1ProtocolProcessor::QueryObjectValue(const RemoteObjectIdentifier roi, const ChannelId& channel, const RecordId& record)
{
    auto handle = std::uint32_t(0);
    auto success = true;

    switch (roi)
    {
    case ROI_CoordinateMapping_SourcePosition:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_CoordinateMapping_SourcePosition (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_Positioning_SourcePosition:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourcePosition (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_Positioning_SourceSpread:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourceSpread (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_Positioning_SourceDelayMode:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_Positioning_SourceDelayMode (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixInput_Mute:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_Mute (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixInput_ReverbSendGain:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_ReverbSendGain (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixInput_Gain:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_Gain(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixInput_ChannelName:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_ChannelName(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixInput_LevelMeterPreMute:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixInput_LevelMeterPreMute(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixOutput_Mute:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixOutput_Mute (handle:" + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixOutput_Gain:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixOutput_Gain(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixOutput_ChannelName:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixOutput_ChannelName(handle: " + NanoOcp1::HandleToString(handle) + ")");
        }
        break;
    case ROI_MatrixOutput_LevelMeterPreMute:
        {
            auto objDef = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel);
            success = m_nanoOcp->sendData(
                NanoOcp1::Ocp1CommandResponseRequired(
                    objDef.GetValueCommand(), handle).GetMemoryBlock()) && success;
            AddPendingGetValueHandle(handle, objDef.m_targetOno);
            //DBG(juce::String(__FUNCTION__) + " ROI_MatrixOutput_LevelMeterPreMute(handle: " + NanoOcp1::HandleToString(handle) + ")");
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
        DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> not implmented");
        success = false;
    }

    return success;
}

void OCP1ProtocolProcessor::AddPendingSubscriptionHandle(const std::uint32_t handle)
{
    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle) << ")");
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
    return !m_pendingSubscriptionHandles.empty();
}

void OCP1ProtocolProcessor::AddPendingGetValueHandle(const std::uint32_t handle, const std::uint32_t ONo)
{
    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle)
    //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
    m_pendingGetValueHandlesWithONo.insert(std::make_pair(handle, ONo));
}

const std::uint32_t OCP1ProtocolProcessor::PopPendingGetValueHandle(const std::uint32_t handle)
{
    auto it = std::find_if(m_pendingGetValueHandlesWithONo.begin(), m_pendingGetValueHandlesWithONo.end(), [handle](const auto& val) { return val.first == handle; });
    if (it != m_pendingGetValueHandlesWithONo.end())
    {
        auto ONo = it->second;
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle)
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
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

void OCP1ProtocolProcessor::AddPendingSetValueHandle(const std::uint32_t handle, const std::uint32_t ONo, int externalId)
{
    //DBG(juce::String(__FUNCTION__)
    //    << " (handle:" << NanoOcp1::HandleToString(handle)
    //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
    m_pendingSetValueHandlesWithONo.insert(std::make_pair(handle, std::make_pair(ONo, externalId)));
}

const std::uint32_t OCP1ProtocolProcessor::PopPendingSetValueHandle(const std::uint32_t handle, int& externalId)
{
    auto it = std::find_if(m_pendingSetValueHandlesWithONo.begin(), m_pendingSetValueHandlesWithONo.end(), [handle](const auto& val) { return val.first == handle; });
    if (it != m_pendingSetValueHandlesWithONo.end())
    {
        auto ONo = it->second.first;
        externalId = it->second.second;
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle)
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
        m_pendingSetValueHandlesWithONo.erase(it);
        return ONo;
    }
    else
        return 0x00;
}

bool OCP1ProtocolProcessor::HasPendingSetValues()
{
    return m_pendingGetValueHandlesWithONo.empty();
}

const std::optional<std::pair<std::uint32_t, int>> OCP1ProtocolProcessor::HasPendingSetValue(const std::uint32_t ONo)
{
    auto it = std::find_if(m_pendingSetValueHandlesWithONo.begin(), m_pendingSetValueHandlesWithONo.end(), [ONo](const auto& val) { return val.second.first == ONo; });  
    if (it != m_pendingSetValueHandlesWithONo.end())
    {
        //auto ONo = it->second.first;
        //DBG(juce::String(__FUNCTION__)
        //    << " (handle:" << NanoOcp1::HandleToString(handle)
        //    << ", targetONo:0x" << juce::String::toHexString(ONo) << ")");
        return std::optional<std::pair<std::uint32_t, int>>(std::make_pair(it->first, it->second.second));
    }
    else
        return std::optional<std::pair<std::uint32_t, int>>();
}

void OCP1ProtocolProcessor::ClearPendingHandles()
{
    m_pendingSubscriptionHandles.clear();
    m_pendingGetValueHandlesWithONo.clear();
    m_pendingSetValueHandlesWithONo.clear();
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

    //DBG(juce::String(__FUNCTION__)
    //    << " (targetONo:0x" << juce::String::toHexString(cmdDef.m_targetOno) << ")");

    auto remObjMsgData = RemoteObjectMessageData();
    remObjMsgData._addrVal = RemoteObjectAddressing(objAddr.second, objAddr.first);

    auto objectsDataToForward = std::map<RemoteObjectIdentifier, RemoteObjectMessageData>();

    int newIntValue[3];
    float newFloatValue[3];
    juce::String newStringValue;

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

            //DBG(juce::String(__FUNCTION__) << " " << roi 
            //    << " (" << static_cast<int>(objAddr.first) << "," << static_cast<int>(objAddr.second) << ") " 
            //    << newFloatValue[0] << "," << newFloatValue[1] << "," << newFloatValue[2] << ";");

            objectsDataToForward[ROI_CoordinateMapping_SourcePosition].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 3, &newFloatValue, 3 * sizeof(float)));
            objectsDataToForward[ROI_CoordinateMapping_SourcePosition_XY].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 2, &newFloatValue, 2 * sizeof(float)));
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

            //DBG(juce::String(__FUNCTION__) << " " << roi
            //    << " (" << static_cast<int>(objAddr.first) << "," << static_cast<int>(objAddr.second) << ") "
            //    << newFloatValue[0] << "," << newFloatValue[1] << "," << newFloatValue[2] << ";");

            objectsDataToForward[ROI_Positioning_SourcePosition].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 3, &newFloatValue, 3 * sizeof(float)));
            objectsDataToForward[ROI_Positioning_SourcePosition_XY].payloadCopy(RemoteObjectMessageData(remObjMsgData._addrVal, ROVT_FLOAT, 2, &newFloatValue, 2 * sizeof(float)));
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
            *newIntValue = NanoOcp1::DataToUint16(msgObj->GetParameterData());  // off=0, tight=1, full=2

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
            auto ocaMuteValue = NanoOcp1::DataToUint8(msgObj->GetParameterData());

            // internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
            switch (ocaMuteValue)
            {
            case 2:
                *newIntValue = 0;
                break;
            case 1:
            default:
                *newIntValue = 1;
                break;
            }

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixInput_ChannelName:
        {
            newStringValue = NanoOcp1::DataToString(msgObj->GetParameterData());

            remObjMsgData._payloadSize = static_cast<std::uint32_t>(newStringValue.length() * sizeof(char));
            remObjMsgData._valCount = static_cast<std::uint16_t>(newStringValue.length());
            remObjMsgData._valType = ROVT_STRING;
            remObjMsgData._payload = newStringValue.getCharPointer().getAddress();

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixInput_LevelMeterPreMute:
        {
            *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(float);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixOutput_Gain:
        {
            *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(float);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixOutput_Mute:
        {
            auto ocaMuteValue = NanoOcp1::DataToUint8(msgObj->GetParameterData());

            // internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
            switch (ocaMuteValue)
            {
            case 2:
                *newIntValue = 0;
                break;
            case 1:
            default:
                *newIntValue = 1;
                break;
            }

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixOutput_ChannelName:
        {
            newStringValue = NanoOcp1::DataToString(msgObj->GetParameterData());

            remObjMsgData._payloadSize = static_cast<std::uint32_t>(newStringValue.length() * sizeof(char));
            remObjMsgData._valCount = static_cast<std::uint16_t>(newStringValue.length());
            remObjMsgData._valType = ROVT_STRING;
            remObjMsgData._payload = newStringValue.getCharPointer().getAddress();

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    case ROI_MatrixOutput_LevelMeterPreMute:
        {
            *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(float);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    default:
        DBG(juce::String(__FUNCTION__) << " unknown: " << ProcessingEngineConfig::GetObjectDescription(roi)
            << " (" << static_cast<int>(objAddr.first) << "," << static_cast<int>(objAddr.second) << ") ");
        return false;
    }

    for (auto const& objData : objectsDataToForward)
        GetValueCache().SetValue(RemoteObject(objData.first, objData.second._addrVal), objData.second);

    if (m_messageListener)
    {
        auto SetValueReplyInfo = HasPendingSetValue(cmdDef.m_targetOno);

        for (auto const& objData : objectsDataToForward)
        {
#ifdef DEBUG
            auto msgMetaInfo = RemoteObjectMessageMetaInfo();
            if (SetValueReplyInfo)
            {
                //DBG(juce::String(__FUNCTION__) << " forwarding " << GetRemoteObjectString(objData.first) << " flagged as SetMessageAcknowledgement");
                msgMetaInfo = RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement, SetValueReplyInfo.value().second);
            }
            else
            {
                //DBG(juce::String(__FUNCTION__) << " forwarding " << GetRemoteObjectString(objData.first) << " flagged as Unsolicited");
                msgMetaInfo = RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_UnsolicitedMessage, INVALID_EXTID);
            }
            m_messageListener->OnProtocolMessageReceived(this, objData.first, objData.second, msgMetaInfo);
#else
            m_messageListener->OnProtocolMessageReceived(this, objData.first, objData.second,
                SetValueReplyInfo ?
                RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_SetMessageAcknowledgement, SetValueReplyInfo.value().second) :
                RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MC_UnsolicitedMessage, INVALID_EXTID));
#endif
        }
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
        //auto& channel = activeObj._Addr._first;
        //auto& record = activeObj._Addr._second;

        switch (activeObj._Id)
        {
        case ROI_CoordinateMapping_SourcePosition_XY:
            {                
                //DBG(juce::String(__FUNCTION__) << " modifying unsupported ROI_CoordinateMapping_SourcePosition_XY to ROI_CoordinateMapping_SourcePosition in 'active' list ("
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel) << ")");

                auto modActiveObj = activeObj;
                modActiveObj._Id = ROI_CoordinateMapping_SourcePosition;
                ocp1SupportedActiveObjects.push_back(modActiveObj);
            }
            break;
        case ROI_Positioning_SourcePosition_XY:
            {
                //DBG(juce::String(__FUNCTION__) << " modifying unsupported ROI_Positioning_SourcePosition_XY to ROI_Positioning_SourcePosition in 'active' list ("
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel) << ")");

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
                //DBG(juce::String(__FUNCTION__) << " filtering unsupported ROI (" << activeObj._Id << ") from 'active' list ("
                //    << "record:" << juce::String(record)
                //    << " channel:" << juce::String(channel) << ")");
            }
            break;
        default:
            ocp1SupportedActiveObjects.push_back(activeObj);
            break;
        }
    }

    return ocp1SupportedActiveObjects;
}

