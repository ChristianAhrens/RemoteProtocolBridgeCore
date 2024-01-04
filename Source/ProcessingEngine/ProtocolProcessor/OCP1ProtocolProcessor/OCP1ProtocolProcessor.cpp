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
    case ROI_CoordinateMappingSettings_P1real:
        return "CoordinateMappingSettings_P1real";
    case ROI_CoordinateMappingSettings_P2real:
        return "CoordinateMappingSettings_P2real";
    case ROI_CoordinateMappingSettings_P3real:
        return "CoordinateMappingSettings_P3real";
    case ROI_CoordinateMappingSettings_P4real:
        return "CoordinateMappingSettings_P4real";
    case ROI_CoordinateMappingSettings_P1virtual:
        return "CoordinateMappingSettings_P1virtual";
    case ROI_CoordinateMappingSettings_P3virtual:
        return "CoordinateMappingSettings_P3virtual";
    case ROI_CoordinateMappingSettings_Flip:
        return "CoordinateMappingSettings_Flip";
    case ROI_CoordinateMappingSettings_Name:
        return "CoordinateMappingSettings_Name";
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
    case ROI_MatrixInput_ChannelName:
        return "MatrixInput_ChannelName";
    case ROI_MatrixInput_LevelMeterPreMute:
        return "MatrixInput_LevelMeterPreMute";
    case ROI_MatrixOutput_Mute:
        return "MatrixOutput_Mute";
    case ROI_MatrixOutput_Gain:
        return "MatrixOutput_Gain";
    case ROI_MatrixOutput_ChannelName:
        return "MatrixOutput_ChannelName";
    case ROI_MatrixOutput_LevelMeterPreMute:
        return "MatrixInput_LevelMeterPreMute";
    case ROI_MatrixOutput_LevelMeterPostMute:
        return "MatrixInput_LevelMeterPostMute";
    case ROI_Positioning_SpeakerPosition:
        return "Positioning_Speaker_Position";
    default:
        return "?";
    }
}

/**
 *  @brief  Get and eventually initialize RemoteObject position data
 *  @param[in]  targetObj    Object to check and possibly initialize the cache
 *  @param[out] msgDataToSet Cached value of the object
 *  @returns                 True if the Position data has three floats
 */
bool OCP1ProtocolProcessor::PreparePositionMessageData(const RemoteObject& targetObj, RemoteObjectMessageData& msgDataToSet)
{
    float zeroPayload[3] = { 0.0f, 0.0f, 0.0f };
    if (!GetValueCache().Contains(targetObj))
        GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

    // get the xyz data from cache to insert the new x data and send the xyz out
    msgDataToSet = GetValueCache().GetValue(targetObj);
    return (msgDataToSet._valCount == 3 && msgDataToSet._payloadSize == 3 * sizeof(float));
}

/**
 * @brief Method to trigger sending of a message
 * @details
 * This method implements special handling for single or dual float value 
 * positioning ROIs by mapping them to the triple float value equivalent and uses
 * the value cache to fill in the missing data.
 *
 * @param roi           The id of the object to send a message for
 * @param msgData       The message payload and metadata
 * @param externalId    An optional external id for identification of replies, etc. ...
 * @returns             True if sending the message succeeded
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
    if (msgData.isDataEmpty() && !(roi == ROI_Scene_Next || roi == ROI_Scene_Previous))
        return QueryObjectValue(roi, msgData._addrVal._first, msgData._addrVal._second);

    auto& channel = msgData._addrVal._first;
    auto& record = msgData._addrVal._second;

    auto handle = std::uint32_t(0x00);
    juce::var objValue;
    NanoOcp1::Ocp1CommandDefinition* objDef = nullptr;

    auto targetObj = RemoteObject(roi, RemoteObjectAddressing(channel, record));
    RemoteObjectMessageData msgDataToSet; // helper for coordinate data conversion

    switch (roi)
    {
    // Sensors cannot be set
    case ROI_Status_StatusText:
    case ROI_Status_AudioNetworkSampleStatus:
    case ROI_Error_GnrlErr:
    case ROI_Error_ErrorText:
    case ROI_MatrixInput_LevelMeterPreMute:
    case ROI_MatrixInput_LevelMeterPostMute:
    case ROI_MatrixOutput_LevelMeterPreMute:
    case ROI_MatrixOutput_LevelMeterPostMute:
    case ROI_ReverbInputProcessing_LevelMeter:
    case ROI_Scene_SceneIndex:
    case ROI_Scene_SceneName:
    case ROI_Scene_SceneComment:
        {
            DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> is a sensor and cannot be set!");
            return false;
        }
    // Objects that are writable, but writing is not allowed via bridging
    case ROI_FunctionGroup_Name:
    case ROI_Positioning_SpeakerPosition:
    case ROI_CoordinateMappingSettings_Name:
    case ROI_CoordinateMappingSettings_Flip:
    case ROI_CoordinateMappingSettings_P1real:
    case ROI_CoordinateMappingSettings_P2real:
    case ROI_CoordinateMappingSettings_P3real:
    case ROI_CoordinateMappingSettings_P4real:
    case ROI_CoordinateMappingSettings_P1virtual:
    case ROI_CoordinateMappingSettings_P3virtual:
        {
            DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> is read-only for bridging!");
            return false;
        }
    case ROI_Settings_DeviceName:
        {
            if(!CheckAndParseStringMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Settings_DeviceName();
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_XY:
        {
            if(!CheckMessagePayload<float>(2, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new xy data
            reinterpret_cast<float*>(msgDataToSet._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
            reinterpret_cast<float*>(msgDataToSet._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_X:
        {
            if(!CheckMessagePayload<float>(1, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new x data
            reinterpret_cast<float*>(msgDataToSet._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_Y:
        {
            if(!CheckMessagePayload<float>(1, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new x data
            reinterpret_cast<float*>(msgDataToSet._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition:
        {
            if(!CheckMessagePayload<float>(3, msgData))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            ParsePositionMessagePayload(msgData, objValue, objDef);
        }
        break;
    case ROI_Positioning_SourcePosition_XY:
        {
            if(!CheckMessagePayload<float>(2, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new xy data
            reinterpret_cast<float*>(msgDataToSet._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];
            reinterpret_cast<float*>(msgDataToSet._payload)[1] = reinterpret_cast<float*>(msgData._payload)[1];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_Positioning_SourcePosition_X:
        {
            if(!CheckMessagePayload<float>(1, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new x data
            reinterpret_cast<float*>(msgDataToSet._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_Positioning_SourcePosition_Y:
        {
            if(!CheckMessagePayload<float>(1, msgData))
                break;

            // override the targetObject
            targetObj = RemoteObject(ROI_Positioning_SourcePosition, RemoteObjectAddressing(channel, record));
            if(!PreparePositionMessageData(targetObj, msgDataToSet))
                break;

            // insert the new y data
            reinterpret_cast<float*>(msgDataToSet._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            ParsePositionMessagePayload(msgDataToSet, objValue, objDef);
        }
        break;
    case ROI_Positioning_SourcePosition:
        {
            if(!CheckMessagePayload<float>(3, msgData))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
            ParsePositionMessagePayload(msgData, objValue, objDef);
        }
        break;
    case ROI_Positioning_SourceSpread:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        }
        break;
    case ROI_Positioning_SourceDelayMode:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
        }
        break;
    case ROI_FunctionGroup_Delay:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Delay(channel);
        }
        break;
    case ROI_FunctionGroup_SpreadFactor:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_SpreadFactor(channel);
        }
        break;
    case ROI_MatrixInput_Mute:
        {
            if(!CheckAndParseMuteMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        }
        break;
    case ROI_MatrixInput_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        }
        break;
    case ROI_MatrixInput_Delay:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Delay(channel);
        }
        break;
    case ROI_MatrixInput_DelayEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_DelayEnable(channel);
        }
        break;
    case ROI_MatrixInput_EqEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_EqEnable(channel);
        }
        break;
    case ROI_MatrixInput_Polarity:
        {
            if(!CheckAndParsePolarityMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Polarity(channel);
        }
        break;
    case ROI_MatrixInput_ChannelName:
        {
            if(!CheckAndParseStringMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
        }
        break;
    case ROI_MatrixInput_ReverbSendGain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        }
        break;
    case ROI_MatrixNode_Enable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Enable(record, channel);
        }
        break;
    case ROI_MatrixNode_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Gain(record, channel);
        }
        break;
    case ROI_MatrixNode_Delay:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Delay(record, channel);
        }
        break;
    case ROI_MatrixNode_DelayEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_DelayEnable(record, channel);
        }
        break;
    case ROI_MatrixOutput_Mute:
        {
            if(!CheckAndParseMuteMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
        }
        break;
    case ROI_MatrixOutput_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
        }
        break;
    case ROI_MatrixOutput_Delay:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Delay(channel);
        }
        break;
    case ROI_MatrixOutput_DelayEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_DelayEnable(channel);
        }
        break;
    case ROI_MatrixOutput_EqEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_EqEnable(channel);
        }
        break;
    case ROI_MatrixOutput_Polarity:
        {
            if(!CheckAndParsePolarityMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Polarity(channel);
        }
        break;
    case ROI_MatrixOutput_ChannelName:
        {
            if(!CheckAndParseStringMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
        }
        break;
    case ROI_MatrixSettings_ReverbRoomId:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRoomId();
        }
        break;
    case ROI_MatrixSettings_ReverbPredelayFactor:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor();
        }
        break;
    case ROI_MatrixSettings_ReverbRearLevel:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRearLevel();
        }
        break;
    case ROI_ReverbInput_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInput_Gain(record, channel);
        }
        break;
    case ROI_ReverbInputProcessing_Mute:
        {
            if(!CheckAndParseMuteMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Mute(channel);
        }
        break;
    case ROI_ReverbInputProcessing_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Gain(channel);
        }
        break;
    case ROI_ReverbInputProcessing_EqEnable:
        {
            if(!CheckAndParseMessagePayload<int>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_EqEnable(channel);
        }
        break;
    case ROI_Scene_Recall:
        {
            // support either one parameter (major) or two parameters (major minor) for scene index
            std::uint16_t sceneIndex[2] = { 0, 0 };
            if (!CheckMessagePayload<int>(1, msgData))
            {
                if(!CheckMessagePayload<int>(2, msgData))
                    break;
                else
                {
                    // index 2 because we read uint16 instead of int(32) within the payload
                    sceneIndex[1] = static_cast<std::uint16_t*>(msgData._payload)[2];
                }
            }
            sceneIndex[0] = static_cast<std::uint16_t*>(msgData._payload)[0];

            // override the targetObject to redirect to SceneIndex
            targetObj = RemoteObject(ROI_Scene_SceneIndex, RemoteObjectAddressing(channel, record));
            GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_INT, 2, &sceneIndex, 2 * sizeof(int)));

            // Set object definition
            auto sceneAgent = NanoOcp1::DS100::dbOcaObjectDef_SceneAgent();

            // Very special handling in contrast to the other ROIs: use "ApplyCommand" on SceneAgent instead of "SetValueCommand"
            bool success = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(sceneAgent.ApplyCommand(sceneIndex[0], sceneIndex[1]), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, sceneAgent.m_targetOno, externalId);
            return success;
        }
    case ROI_Scene_Next:
        {
            // Set object definition
            auto sceneAgent = NanoOcp1::DS100::dbOcaObjectDef_SceneAgent();
            // Very special handling in contrast to the other ROIs: use "NextCommand" on SceneAgent instead of "SetValueCommand"
            bool success = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(sceneAgent.NextCommand(), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, sceneAgent.m_targetOno, externalId);
            return success;
        }
        break;
    case ROI_Scene_Previous:
        {
            // Set object definition
            auto sceneAgent = NanoOcp1::DS100::dbOcaObjectDef_SceneAgent();
            // Very special handling in contrast to the other ROIs: use "PreviousCommand" on SceneAgent instead of "SetValueCommand"
            bool success = m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired(sceneAgent.PreviousCommand(), handle).GetMemoryBlock());
            AddPendingSetValueHandle(handle, sceneAgent.m_targetOno, externalId);
            return success;
        }
        break;
    case ROI_SoundObjectRouting_Mute:
        {
            if(!CheckAndParseMuteMessagePayload(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Mute(record, channel);
        }
        break;
    case ROI_SoundObjectRouting_Gain:
        {
            if(!CheckAndParseMessagePayload<float>(msgData, objValue))
                break;
            objDef = new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Gain(record, channel);
        }
        break;
    default:
        DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> not implemented");
        break;
    }

    // Sanity checks
    jassert(objDef != nullptr); // Missing implementation!
    if (objDef == nullptr)
        return false;

    // Ensure automatic cleanup
    auto objDefPtr = std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(objDef);

    // Set the value to the cache (use the msgDataToSet if it contains data)
    GetValueCache().SetValue(targetObj, msgDataToSet.isDataEmpty() ? msgData : msgDataToSet);

    // Send SetValue command
    bool success = m_nanoOcp->sendData( NanoOcp1::Ocp1CommandResponseRequired( (*objDefPtr.get()).SetValueCommand(objValue), handle).GetMemoryBlock());
    AddPendingSetValueHandle(handle, (*objDefPtr.get()).m_targetOno, externalId);
    //DBG(juce::String(__FUNCTION__) + " " + ProcessingEngineConfig::GetObjectTagName(roi) + "(handle: " + NanoOcp1::HandleToString(handle) + ")");
    return success;
}

/**
 * Helper method to setup the internal map of remote object ids to ocp1 object ONos
 */
void OCP1ProtocolProcessor::CreateKnownONosMap()
{
    auto record = static_cast<RecordId>(INVALID_ADDRESS_VALUE);
    auto channel = static_cast<ChannelId>(INVALID_ADDRESS_VALUE);

    // definitions without record and channel
    m_ROIsToDefsMap[ROI_Settings_DeviceName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Settings_DeviceName();
    m_ROIsToDefsMap[ROI_Status_StatusText][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Status_StatusText();
    m_ROIsToDefsMap[ROI_Status_AudioNetworkSampleStatus][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Status_AudioNetworkSampleStatus();
    m_ROIsToDefsMap[ROI_Error_GnrlErr][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Error_GnrlErr();
    m_ROIsToDefsMap[ROI_Error_ErrorText][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Error_ErrorText();

    m_ROIsToDefsMap[ROI_MatrixSettings_ReverbRoomId][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRoomId();
    m_ROIsToDefsMap[ROI_MatrixSettings_ReverbPredelayFactor][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor();
    m_ROIsToDefsMap[ROI_MatrixSettings_ReverbRearLevel][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRearLevel();
    m_ROIsToDefsMap[ROI_Scene_SceneIndex][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneIndex();
    m_ROIsToDefsMap[ROI_Scene_SceneName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneName();
    m_ROIsToDefsMap[ROI_Scene_SceneComment][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneComment();

    // definitions with channel
    for (channel = ChannelId(1); channel <= NanoOcp1::DS100::MaxChannelCount; channel++)
    {
        m_ROIsToDefsMap[ROI_Positioning_SpeakerPosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Speaker_Position(channel);
        m_ROIsToDefsMap[ROI_Positioning_SourcePosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        m_ROIsToDefsMap[ROI_Positioning_SourceSpread][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        m_ROIsToDefsMap[ROI_Positioning_SourceDelayMode][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Delay][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Delay(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_DelayEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_DelayEnable(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_EqEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_EqEnable(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Polarity][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Polarity(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_ChannelName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_LevelMeterPreMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_LevelMeterPostMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPostMute(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_ReverbSendGain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Delay][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Delay(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_DelayEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_DelayEnable(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_EqEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_EqEnable(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_Polarity][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Polarity(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_ChannelName][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_LevelMeterPreMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel);
        m_ROIsToDefsMap[ROI_MatrixOutput_LevelMeterPostMute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(channel);
        m_ROIsToDefsMap[ROI_FunctionGroup_Name][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Name(channel);
        m_ROIsToDefsMap[ROI_FunctionGroup_Delay][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Delay(channel);
        m_ROIsToDefsMap[ROI_FunctionGroup_SpreadFactor][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_SpreadFactor(channel);
        m_ROIsToDefsMap[ROI_ReverbInputProcessing_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Mute(channel);
        m_ROIsToDefsMap[ROI_ReverbInputProcessing_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Gain(channel);
        m_ROIsToDefsMap[ROI_ReverbInputProcessing_EqEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_EqEnable(channel);
        m_ROIsToDefsMap[ROI_ReverbInputProcessing_LevelMeter][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_LevelMeter(channel);
        // definitions with channel and record
        for (record = RecordId(MappingAreaId::MAI_First); record <= MappingAreaId::MAI_Fourth; record++)
        {
            m_ROIsToDefsMap[ROI_CoordinateMapping_SourcePosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
            m_ROIsToDefsMap[ROI_MatrixNode_Enable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Enable(record, channel);
            m_ROIsToDefsMap[ROI_MatrixNode_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Gain(record, channel);
            m_ROIsToDefsMap[ROI_MatrixNode_Delay][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Delay(record, channel);
            m_ROIsToDefsMap[ROI_MatrixNode_DelayEnable][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_DelayEnable(record, channel);
            m_ROIsToDefsMap[ROI_ReverbInput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_ReverbInput_Gain(record, channel);
            m_ROIsToDefsMap[ROI_SoundObjectRouting_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Mute(record, channel);
            m_ROIsToDefsMap[ROI_SoundObjectRouting_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Gain(record, channel);
        }
    }

    // definitions with record
    for (record = static_cast<RecordId>(MappingAreaId::MAI_First); record <= MappingAreaId::MAI_Fourth; record++)
    {
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P1real][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_real(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P2real][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P2_real(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P3real][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_real(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P4real][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P4_real(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P1virtual][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_P3virtual][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_Flip][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Flip(record);
        m_ROIsToDefsMap[ROI_CoordinateMappingSettings_Name][std::make_pair(record, INVALID_ADDRESS_VALUE)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Name(record);
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

/**
 * @brief  Helper to get the Pointer to Ocp1CommandDefinition for a specific RemoteObjectIdentifier, Record and Channel.
 * @warning This method does not take care of the returned pointer. You need to delete it on your own!
 * @param[in]	roi		The RemoteObjectIdentifier to resolve into object definition
 * @param[in]	record	The Record for the object definition
 * @param[in]	channel	The Channel for the object definition
 * @param[out]	objDef	Pointer to the object definition to be retured or nullptr
 * @returns				True if the RemoteObjectIdentifier was resolved to a Ocp1CommandDefinition
 */
bool OCP1ProtocolProcessor::GetObjectDefinition(const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, NanoOcp1::Ocp1CommandDefinition** objDef)
{
    // reset the objDef
    if (nullptr != objDef)
    {
        delete *objDef;
        *objDef = nullptr;
    }

    switch (roi)
    {
    case ROI_Settings_DeviceName:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Settings_DeviceName();
        }
        break;
    case ROI_Status_StatusText:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Status_StatusText();
        }
        break;
    case ROI_Status_AudioNetworkSampleStatus:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Status_AudioNetworkSampleStatus();
        }
        break;
    case ROI_Error_GnrlErr:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Error_GnrlErr();
        }
        break;
    case ROI_Error_ErrorText:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Error_ErrorText();
        }
        break;
    case ROI_CoordinateMappingSettings_Name:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Name(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_Flip:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_Flip(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P1real:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_real(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P2real:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P2_real(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P3real:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_real(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P4real:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P4_real(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P1virtual:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(channel);
        }
        break;
    case ROI_CoordinateMappingSettings_P3virtual:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(channel);
        }
        break;
    case ROI_Positioning_SourcePosition:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(channel);
        }
        break;
    case ROI_Positioning_SourcePosition_XY:
    case ROI_Positioning_SourcePosition_X:
    case ROI_Positioning_SourcePosition_Y:
        {
            DBG(juce::String(__FUNCTION__) + " skipping ROI_Positioning_SourcePosition X Y XY");
            return false;
        }
        break;
    case ROI_CoordinateMapping_SourcePosition:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(record, channel);
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_XY:
    case ROI_CoordinateMapping_SourcePosition_X:
    case ROI_CoordinateMapping_SourcePosition_Y:
        {
            DBG(juce::String(__FUNCTION__) + " skipping ROI_CoordinateMapping_SourcePosition X Y XY");
            return false;
        }
        break;
    case ROI_Positioning_SourceSpread:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        }
        break;
    case ROI_Positioning_SourceDelayMode:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_DelayMode(channel);
        }
        break;
    case ROI_Positioning_SpeakerPosition:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Positioning_Speaker_Position(channel);
        }
        break;
    case ROI_FunctionGroup_Name:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Name(channel);
        }
        break;
    case ROI_FunctionGroup_Delay:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_Delay(channel);
        }
        break;
    case ROI_FunctionGroup_SpreadFactor:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_FunctionGroup_SpreadFactor(channel);
        }
        break;
    case ROI_MatrixInput_Mute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        }
        break;
    case ROI_MatrixInput_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        }
        break;
    case ROI_MatrixInput_Delay:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Delay(channel);
        }
        break;
    case ROI_MatrixInput_DelayEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_DelayEnable(channel);
        }
        break;
    case ROI_MatrixInput_EqEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_EqEnable(channel);
        }
        break;
    case ROI_MatrixInput_Polarity:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Polarity(channel);
        }
        break;
    case ROI_MatrixInput_ChannelName:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ChannelName(channel);
        }
        break;
    case ROI_MatrixInput_LevelMeterPreMute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPreMute(channel);
        }
        break;
    case ROI_MatrixInput_LevelMeterPostMute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_LevelMeterPostMute(channel);
        }
        break;
    case ROI_MatrixInput_ReverbSendGain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        }
        break;
    case ROI_MatrixNode_Enable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Enable(record, channel);
        }
        break;
    case ROI_MatrixNode_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Gain(record, channel);
        }
        break;
    case ROI_MatrixNode_Delay:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_Delay(record, channel);
        }
        break;
    case ROI_MatrixNode_DelayEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixNode_DelayEnable(record, channel);
        }
        break;
    case ROI_MatrixOutput_Mute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Mute(channel);
        }
        break;
    case ROI_MatrixOutput_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Gain(channel);
        }
        break;
    case ROI_MatrixOutput_Delay:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Delay(channel);
        }
        break;
    case ROI_MatrixOutput_DelayEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_DelayEnable(channel);
        }
        break;
    case ROI_MatrixOutput_EqEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_EqEnable(channel);
        }
        break;
    case ROI_MatrixOutput_Polarity:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_Polarity(channel);
        }
        break;
    case ROI_MatrixOutput_ChannelName:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_ChannelName(channel);
        }
        break;
    case ROI_MatrixOutput_LevelMeterPreMute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(channel);
        }
        break;
    case ROI_MatrixOutput_LevelMeterPostMute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(channel);
        }
        break;
    case ROI_MatrixSettings_ReverbRoomId:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRoomId();
        }
        break;
    case ROI_MatrixSettings_ReverbPredelayFactor:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor();
        }
        break;
    case ROI_MatrixSettings_ReverbRearLevel:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_MatrixSettings_ReverbRearLevel();
        }
        break;
    case ROI_ReverbInput_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInput_Gain(record, channel);
        }
        break;
    case ROI_ReverbInputProcessing_Mute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Mute(channel);
        }
        break;
    case ROI_ReverbInputProcessing_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_Gain(channel);
        }
        break;
    case ROI_ReverbInputProcessing_EqEnable:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_EqEnable(channel);
        }
        break;
    case ROI_ReverbInputProcessing_LevelMeter:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_ReverbInputProcessing_LevelMeter(channel);
        }
        break;
    case ROI_Scene_SceneIndex:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneIndex();
        }
        break;
    case ROI_Scene_SceneName:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneName();
        }
        break;
    case ROI_Scene_Next:
    case ROI_Scene_Previous:
        return false;
    case ROI_Scene_SceneComment:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_Scene_SceneComment();
        }
        break;
    case ROI_SoundObjectRouting_Mute:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Mute(record, channel);
        }
        break;
    case ROI_SoundObjectRouting_Gain:
        {
            *objDef = new NanoOcp1::DS100::dbOcaObjectDef_SoundObjectRouting_Gain(record, channel);
        }
        break;
    default:
        DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectDescription(roi) << " -> not implmented");
    }

    jassert(*objDef != nullptr); // Missing implementation!
    return (*objDef != nullptr);
}

/**
 * @brief  Send subscribe commands for each supported active remote object
 * @returns True if all supported active remote objects were sucessfully subscribed
 */
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

        NanoOcp1::Ocp1CommandDefinition* objdef = nullptr;

        // Get the object definition
        if (false == GetObjectDefinition(activeObj._Id, channel, record, &objdef))
            continue;

        // Ensure automatic cleanup.
        auto objDefPtr = std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(objdef);

        success = success && m_nanoOcp->sendData(NanoOcp1::Ocp1CommandResponseRequired((*objDefPtr.get()).AddSubscriptionCommand(), handle).GetMemoryBlock());
        //DBG(juce::String(__FUNCTION__) << " " << ProcessingEngineConfig::GetObjectTagName(activeObj._Id) << "("
        //    << (record >= 0 ? (" record:" + juce::String(record)) : "")
        //    << (channel >= 0 ? (" channel:" + juce::String(channel)) : "")
        //    << " handle:" << NanoOcp1::HandleToString(handle) << ")");

        AddPendingSubscriptionHandle(handle);
    }

    return success;
}

/**
 * @brief  Remove all subscriptions
 * @note	not implemented yet.
 * @returns True
 */
bool OCP1ProtocolProcessor::DeleteObjectSubscriptions()
{
    return false;
}

/**
 * @brief  Send GetValue command for each supported active remote object
 * @returns True if all supported active remote objects were sucessfully requested
 */
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

/**
 * @brief  Send GetValue command to the device
 * @param[in]	roi		The RemoteObjectIdentifier to resolve into object definition for the get command
 * @param[in]	channel	The channel of the object
 * @param[in]	record	The record of the object
 * @returns				True if the GetValue command was sent sucessfully
 */
bool OCP1ProtocolProcessor::QueryObjectValue(const RemoteObjectIdentifier roi, const ChannelId& channel, const RecordId& record)
{
    NanoOcp1::Ocp1CommandDefinition* objdef = nullptr;
    auto handle = std::uint32_t(0);

    // Get the object definition
    if (false == GetObjectDefinition(roi, channel, record, &objdef))
        return false;

    // Ensure automatic cleanup.
    auto objDefPtr = std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>(objdef); 

    // Send GetValue command
    bool success = m_nanoOcp->sendData( NanoOcp1::Ocp1CommandResponseRequired( (*objDefPtr.get()).GetValueCommand(), handle).GetMemoryBlock());
    AddPendingGetValueHandle(handle, (*objDefPtr.get()).m_targetOno);
    //DBG(juce::String(__FUNCTION__) + " " + ProcessingEngineConfig::GetObjectTagName(roi) + "(handle: " + NanoOcp1::HandleToString(handle) + ")");
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
    float newFloatValue[6];
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
    case ROI_Positioning_SpeakerPosition:
        {
            if (!NanoOcp1::VariantToPositionAndRotation(cmdDef.ToVariant(6, msgObj->GetParameterData()), 
                newFloatValue[3], newFloatValue[4], newFloatValue[5],   // RPBC expects position values first
                newFloatValue[0], newFloatValue[1], newFloatValue[2]))  // and rotation values second - OCP1 provides them the other way around, so switching is done here
            {
                return false;
            }
            remObjMsgData._payloadSize = 6 * sizeof(float);
            remObjMsgData._valCount = 6;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            //DBG(juce::String(__FUNCTION__) << " " << roi 
            //    << " (" << static_cast<int>(objAddr.first) << "," << static_cast<int>(objAddr.second) << ") " 
            //    << newFloatValue[0] << "," << newFloatValue[1] << "," << newFloatValue[2] << ","
            //    << newFloatValue[3] << "," << newFloatValue[4] << "," << newFloatValue[5] << ";");

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
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
    // OcaInt32Sensor / OcaInt32Actuator
    case ROI_Status_AudioNetworkSampleStatus:
        {
            *newIntValue = NanoOcp1::DataToInt32(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // OcaBoolean
    case ROI_Error_GnrlErr:
        {
            *newIntValue = NanoOcp1::DataToUint8(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // OcaPolarity
    case ROI_MatrixInput_Polarity:
    case ROI_MatrixOutput_Polarity:
        {
            remObjMsgData._addrVal._second = static_cast<RecordId>(INVALID_ADDRESS_VALUE);

            // internal value 0=normal, 1=inverted; OcaPolarity uses 1=normal, 2=inverted
            *newIntValue = NanoOcp1::DataToUint8(msgObj->GetParameterData()) - 1;

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // OcaSwitch
    case ROI_CoordinateMappingSettings_Flip:
        {
            // CoordinateMappingSettings address mappingarea, RPBC expect this value as channel - OCP1 holds it as record
            remObjMsgData._addrVal = RemoteObjectAddressing(
                static_cast<ChannelId>(objAddr.first), 
                static_cast<RecordId>(objAddr.second));
        }
        [[fallthrough]]; // intentional fallthrough
    case ROI_MatrixInput_DelayEnable:
    case ROI_MatrixInput_EqEnable:
    case ROI_MatrixNode_Enable:
    case ROI_MatrixNode_DelayEnable:
    case ROI_MatrixOutput_DelayEnable:
    case ROI_MatrixOutput_EqEnable:
    case ROI_Positioning_SourceDelayMode:
    case ROI_MatrixSettings_ReverbRoomId:
    case ROI_ReverbInputProcessing_EqEnable:
        {
            *newIntValue = NanoOcp1::DataToUint16(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(int);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_INT;
            remObjMsgData._payload = &newIntValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // OcaMute
    case ROI_MatrixInput_Mute:
    case ROI_MatrixOutput_Mute:
    case ROI_ReverbInputProcessing_Mute:
    case ROI_SoundObjectRouting_Mute:
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
    // OcaFloat32Sensor / OcaFloat32Actuator
    case ROI_Positioning_SourceSpread:
    case ROI_MatrixInput_ReverbSendGain:
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
    case ROI_MatrixSettings_ReverbPredelayFactor:
    case ROI_MatrixSettings_ReverbRearLevel:
    case ROI_FunctionGroup_Delay:
    case ROI_FunctionGroup_SpreadFactor:
    case ROI_ReverbInput_Gain:
    case ROI_ReverbInputProcessing_Gain:
    case ROI_ReverbInputProcessing_LevelMeter:
    case ROI_SoundObjectRouting_Gain:
        {
            *newFloatValue = NanoOcp1::DataToFloat(msgObj->GetParameterData());

            remObjMsgData._payloadSize = sizeof(float);
            remObjMsgData._valCount = 1;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // OcaStringSensor / OcaStringActuator
    case ROI_CoordinateMappingSettings_Name:
        {
            // CoordinateMappingSettings address mappingarea, RPBC expect this value as channel - OCP1 holds it as record
            remObjMsgData._addrVal = RemoteObjectAddressing(
                static_cast<ChannelId>(objAddr.first),
                static_cast<RecordId>(objAddr.second)); 
        }
        [[fallthrough]]; // intentional fallthrough
    case ROI_Settings_DeviceName:
    case ROI_Status_StatusText:
    case ROI_Error_ErrorText:
    case ROI_MatrixInput_ChannelName:
    case ROI_MatrixOutput_ChannelName:
    case ROI_Scene_SceneIndex:
    case ROI_Scene_SceneName:
    case ROI_Scene_SceneComment:
    case ROI_FunctionGroup_Name:
        {
            newStringValue = NanoOcp1::DataToString(msgObj->GetParameterData());

            remObjMsgData._payloadSize = static_cast<std::uint32_t>(newStringValue.length() * sizeof(char));
            remObjMsgData._valCount = static_cast<std::uint16_t>(newStringValue.length());
            remObjMsgData._valType = ROVT_STRING;
            remObjMsgData._payload = newStringValue.getCharPointer().getAddress();

            objectsDataToForward.insert(std::make_pair(roi, remObjMsgData));
        }
        break;
    // dbOcaPositionAgentDeprecated
    case ROI_CoordinateMappingSettings_P1real:
    case ROI_CoordinateMappingSettings_P2real:
    case ROI_CoordinateMappingSettings_P3real:
    case ROI_CoordinateMappingSettings_P4real:
    case ROI_CoordinateMappingSettings_P1virtual:
    case ROI_CoordinateMappingSettings_P3virtual:
        {
            if (!NanoOcp1::VariantToPosition(cmdDef.ToVariant(3, msgObj->GetParameterData()), newFloatValue[0], newFloatValue[1], newFloatValue[2]))
                return false;
            remObjMsgData._addrVal = RemoteObjectAddressing(
                static_cast<ChannelId>(objAddr.first),
                static_cast<RecordId>(objAddr.second)); // CoordinateMappingSettings address mappingarea, RPBC expect this value as channel - OCP1 holds it as record
            remObjMsgData._payloadSize = 3 * sizeof(float);
            remObjMsgData._valCount = 3;
            remObjMsgData._valType = ROVT_FLOAT;
            remObjMsgData._payload = &newFloatValue;

            //DBG(juce::String(__FUNCTION__) << " " << roi 
            //    << " (" << static_cast<int>(objAddr.first) << "," << static_cast<int>(objAddr.second) << ") " 
            //    << newFloatValue[0] << "," << newFloatValue[1] << "," << newFloatValue[2] << ";");

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
        case ROI_HeartbeatPing:
        case ROI_HeartbeatPong:
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

/**
 *  @brief  Helper to check message payload size and parse msgData as string into value
 *  @param[in]  msgData The input message data to check and parse
 *  @param[out] value   The output juce String as juce::var
 *  @returns            True if the msgData check succeded
 */
bool OCP1ProtocolProcessor::CheckAndParseStringMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value)
{
    if (msgData._valCount < 1 || msgData._payloadSize != msgData._valCount * sizeof(char))
        return false;

    value = juce::var(juce::String(static_cast<const char*>(msgData._payload), msgData._payloadSize));
    return true;
}

/**
 *  @brief  Helper to check message payload size and parse msgData as mute state into value
 *  @note internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
 *  @param[in]  msgData The input message data to check and parse
 *  @param[out] value   The output int mute state as juce::var
 *  @returns            True if the msgData check succeded
 */
bool OCP1ProtocolProcessor::CheckAndParseMuteMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value)
{
    if (!CheckMessagePayload<int>(1, msgData))
        return false;

    // internal value 0=unmute, 1=mute; OcaMute uses 2=unmute, 1=mute
    value = juce::var((*static_cast<int*>(msgData._payload) == 1) ? 1 : 2);
    return true;
}

/**
 *  @brief  Helper to check message payload size and parse msgData as polarity state into value
 *  @note internal value 0=normal, 1=inverted; OcaPolarity uses 1=normal, 2=inverted
 *  @param[in]  msgData The input message data to check and parse
 *  @param[out] value   The output int polarity state as juce::var
 *  @returns            True if the msgData check succeded
 */
bool OCP1ProtocolProcessor::CheckAndParsePolarityMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value)
{
    if (!CheckMessagePayload<int>(1, msgData))
        return false;

    // internal value 0=normal, 1=inverted; OcaPolarity uses 1=normal, 2=inverted
    value = juce::var((*static_cast<int*>(msgData._payload) == 1) ? 2 : 1);
    return true;
}

/**
 *  @brief  Helper to check message payload size and parse msgData as positional message into value
 *  @param[in]  msgData The input message data to check and parse
 *  @param[out] value   The output 3 position coordinates packed as juce::var
 *  @param[in]  objDef  The object def required to use its ToVariant method
 *  @returns            True if the msgData check succeded
 */
bool OCP1ProtocolProcessor::ParsePositionMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value, NanoOcp1::Ocp1CommandDefinition* objDef)
{
    if (!CheckMessagePayload<float>(3, msgData))
        return false;

    if (nullptr == objDef)
        return false;

    auto parameterData = NanoOcp1::DataFromPosition(
        reinterpret_cast<float*>(msgData._payload)[0],
        reinterpret_cast<float*>(msgData._payload)[1],
        reinterpret_cast<float*>(msgData._payload)[2]);
    value = objDef->ToVariant(3, parameterData);
    return true;
}

/**
 *  @brief  Helper to check message payload size and parse msgData as position and rotation message into value
 *  @param[in]  msgData The input message data to check and parse
 *  @param[out] value   The output 3 position coordinates and 3 rotation values packed as juce::var
 *  @param[in]  objDef  The object def required to use its ToVariant method
 *  @returns            True if the msgData check succeded
 */
bool OCP1ProtocolProcessor::ParsePositionAndRotationMessagePayload(const RemoteObjectMessageData& msgData, juce::var& value, NanoOcp1::Ocp1CommandDefinition* objDef)
{
    if (!CheckMessagePayload<float>(6, msgData))
        return false;

    if (nullptr == objDef)
        return false;

    auto parameterData = NanoOcp1::DataFromPositionAndRotation(
        reinterpret_cast<float*>(msgData._payload)[0],
        reinterpret_cast<float*>(msgData._payload)[1],
        reinterpret_cast<float*>(msgData._payload)[2],
        reinterpret_cast<float*>(msgData._payload)[3],
        reinterpret_cast<float*>(msgData._payload)[4],
        reinterpret_cast<float*>(msgData._payload)[5]);
    value = objDef->ToVariant(6, parameterData);
    return true;
}

