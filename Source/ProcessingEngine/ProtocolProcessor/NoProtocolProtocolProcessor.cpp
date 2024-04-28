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

#include "NoProtocolProtocolProcessor.h"

#include "../../dbprProjectUtils.h"


// **************************************************************************************
//    class NoProtocolProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
NoProtocolProtocolProcessor::NoProtocolProtocolProcessor(const NodeId& parentNodeId, bool cacheInit)
	: ProtocolProcessorBase(parentNodeId)
{
	m_type = ProtocolType::PT_NoProtocol;

	SetActiveRemoteObjectsInterval(-1); // default value in ProtocolProcessorBase is 100 which we do not want to use, so invalidate it to overcome potential misunderstandings when reading code

    if (cacheInit)
    	InitializeObjectValueCache();
}

/**
 * Destructor
 */
NoProtocolProtocolProcessor::~NoProtocolProtocolProcessor()
{
	Stop();
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool NoProtocolProtocolProcessor::setStateXml(XmlElement* stateXml)
{
    if (nullptr == stateXml || !ProtocolProcessorBase::setStateXml(stateXml))
    {
        return false;
    }
    else
    {
        auto dbprDataXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DBPRDATA));
        if (nullptr != dbprDataXmlElement && 1 == dbprDataXmlElement->getNumChildElements() && dbprDataXmlElement->getFirstChildElement()->isTextElement())
        {
            auto projectData = ProjectData::FromString(dbprDataXmlElement->getFirstChildElement()->getAllSubText());
            if (!projectData.IsEmpty())
                InitializeObjectValueCache(projectData);
            else
                InitializeObjectValueCache();
        }
        
        m_animationMode = static_cast<AnimationMode>(stateXml->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE), AM_Off));

        if (AM_Rand == m_animationMode)
        {
            srand(static_cast <unsigned> (time(0)));
            for (int channel = INVALID_ADDRESS_VALUE; channel <= sc_chCnt; channel++)
            {
                m_channelRandomizedFactors[channel] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
                m_channelRandomizedScaleFactors[channel] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            }
            m_valueIdRandomizedFactors[0] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            m_valueIdRandomizedFactors[1] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            m_valueIdRandomizedFactors[2] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        }
        
        return true;
    }
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool NoProtocolProtocolProcessor::Start()
{
	m_IsRunning = true;

	startTimerThread(GetCallbackRate(), 100); // used for 4s heartbeat triggering

    TriggerSendingObjectValueCache();

	return true;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool NoProtocolProtocolProcessor::Stop()
{
	m_IsRunning = false;

	stopTimerThread();

	return true;;
}

/**
 * Method to trigger sending of a message
 *
 * @param roi			The id of the object to send a message for
 * @param msgData		The message payload and metadata
 * @param externalId	An optional external id for identification of replies, etc.
 */
bool NoProtocolProtocolProcessor::SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId)
{
	if (msgData._valCount == 0)
	{
        switch (roi)
        {
        case ROI_Scene_Previous:
        case ROI_Scene_Next:
            {
                auto ro = RemoteObject(ROI_Scene_SceneIndex, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE));
                if (GetValueCache().Contains(ro))
                {
                    auto sceneIndexMsgData = GetValueCache().GetValue(ro);
                    if (sceneIndexMsgData._payload != nullptr && sceneIndexMsgData._payloadSize != 0)
                    {
                        auto sceneIndex = juce::String(static_cast<char*>(sceneIndexMsgData._payload), sceneIndexMsgData._payloadSize).getFloatValue();
                        SetSceneIndexToCache(sceneIndex + (roi == ROI_Scene_Previous ? -1.0f : 1.0f));

                        m_messageListener->OnProtocolMessageReceived(this, ROI_Scene_SceneIndex,
                            GetValueCache().GetValue(ro),
                            RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, -1));
                    }
                }
            }
            break;
        default:
            {
                auto ro = RemoteObject(roi, msgData._addrVal);
                if (GetValueCache().Contains(ro))
                {
                    m_messageListener->OnProtocolMessageReceived(this, roi,
                        GetValueCache().GetValue(ro),
                        RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, externalId));
                }
                //else
                //    DBG(juce::String(__FUNCTION__) << " cache is missing " << ro._Id << " " << ro._Addr.toString());
            }
            break;
        }
	}
	else
	{
        float zeroPayload[3] = { 0.0f, 0.0f, 0.0f };
		auto msgsToReflect = std::vector<std::pair<RemoteObjectIdentifier, RemoteObjectMessageData>>();

		switch (roi)
		{
        case ROI_CoordinateMapping_SourcePosition:
            {
                if (msgData._valCount != 3 || msgData._payloadSize != 3 * sizeof(float))
                    return false;

                SetValue(RemoteObject(roi, msgData._addrVal), msgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_XY:
            {
                if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

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

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(refMsgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(refMsgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_X:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, refMsgData._payload, 2 * sizeof(float))));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_Y:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, refMsgData._payload, 2 * sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition:
            {
                if (msgData._valCount != 3 || msgData._payloadSize != 3 * sizeof(float))
                    return false;

                SetValue(RemoteObject(roi, msgData._addrVal), msgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
            }
            break;
        case ROI_Positioning_SourcePosition_XY:
            {
                if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

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

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(refMsgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(refMsgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition_X:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[0] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, refMsgData._payload, 2 * sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition_Y:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, refMsgData._payload, 2 * sizeof(float))));
            }
            break;
        case ROI_Scene_Recall:
            {
                if (msgData._valType == ROVT_STRING && msgData._payload != nullptr && msgData._payloadSize != 0)
                {
                    auto sceneIndex = juce::String(static_cast<char*>(msgData._payload), msgData._payloadSize).getFloatValue();
                    SetSceneIndexToCache(sceneIndex);
                }
                else if (msgData._valType == ROVT_FLOAT && msgData._payload != nullptr && msgData._payloadSize == sizeof(float))
                {
                    auto sceneIndex = static_cast<float*>(msgData._payload)[0];
                    SetSceneIndexToCache(sceneIndex);
                }
                else if (msgData._valType == ROVT_INT && msgData._payload != nullptr && msgData._payloadSize == 2 * sizeof(int))
                {
                    auto sceneIndexMajor = static_cast<int*>(msgData._payload)[0];
                    auto sceneIndexMinor = static_cast<int*>(msgData._payload)[1];
                    auto sceneIndexString = juce::String(sceneIndexMajor) + "." + juce::String(sceneIndexMinor);
                    SetSceneIndexToCache(sceneIndexString.getFloatValue());
                }
            }
            break;
		default:
            {
                SetValue(RemoteObject(roi, msgData._addrVal), msgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
            }
			break;
		}

		for (auto const& msgIdNData : msgsToReflect)
			m_messageListener->OnProtocolMessageReceived(this, msgIdNData.first, msgIdNData.second,
				RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_SetMessageAcknowledgement, externalId));
	}

	return true;
}

/**
 * TimerThreadBase callback to send keepalive queries cyclically and update potentially active animation
 */
void NoProtocolProtocolProcessor::timerThreadCallback()
{
	if (m_IsRunning)
	{
        if (IsHeartBeatCallback())
		    m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPong, RemoteObjectMessageData(), RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, -1));

        if (IsAnimationActive())
            StepAnimation();
	}

    BumpCallbackCount();
}

/**
 * Initializes internal cache with somewhat feasible dummy values.
 */
void NoProtocolProtocolProcessor::InitializeObjectValueCache()
{
    auto deviceName = juce::String("InternalSim");
    SetValue(
        RemoteObject(ROI_Settings_DeviceName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(deviceName.length()), deviceName.getCharPointer().getAddress(), static_cast<std::uint32_t>(deviceName.length())));

    // all scenes relevant values
    SetSceneIndexToCache(1.0f);
    SetSceneIndexToCache(2.0f);
    SetSceneIndexToCache(3.0f);
    SetSceneIndexToCache(4.0f);
    SetSceneIndexToCache(5.0f);
    SetSceneIndexToCache(10.0f);
    SetSceneIndexToCache(20.0f);
    SetSceneIndexToCache(30.0f);
    SetSceneIndexToCache(40.0f);
    SetSceneIndexToCache(50.0f);


    // all en-space relevant values
    auto oneFloat = 1.0f;
    auto oneInt = 1;
    SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRoomId, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_INT, 1, &oneInt, sizeof(int)));
    SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbPredelayFactor, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));
    SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRearLevel, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));


    // all other relevant values can be initialized through ProjectData
    ProjectData pd;

    // all input relevant values
    auto& ind = pd._inputNameData;
    for (int in = 1; in <= sc_chCnt; in++)
        ind[in] = juce::String("Input ") + juce::String(in);

    // all output relevant values
    auto& spd = pd._speakerPositionData;
    spd[1] = SpeakerPositionData::FromString("2.0,-2.0,0.0,135.0,0.0,0.0");
    spd[2] = SpeakerPositionData::FromString("2.0,0.0,0.0,180.0,0.0,0.0");
    spd[3] = SpeakerPositionData::FromString("2.0,2.0,0.0,225.0,0.0,0.0");
    spd[4] = SpeakerPositionData::FromString("0.0,2.0,0.0,270.0,0.0,0.0");
    spd[5] = SpeakerPositionData::FromString("-2.0,2.0,0.0,315.0,0.0,0.0");
    spd[6] = SpeakerPositionData::FromString("-2.0,0.0,0.0,0.0,0.0,0.0");
    spd[7] = SpeakerPositionData::FromString("-2.0,-2.0,0.0,45.0,0.0,0.0");
    spd[8] = SpeakerPositionData::FromString("0.0,-2.0,0.0,90.0,0.0,0.0");
    for (auto i = 9; i <= 64; i++)
        spd[i] = SpeakerPositionData::FromString("0.0,0.0,0.0,0.0,0.0,0.0");

    // all mapping settings relevant values
    auto& cmd = pd._coordinateMappingData;
    cmd[1] = CoordinateMappingData::FromString(juce::String("Example Mapping 1,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "-5,2,0,"     // rp1
        + "-2.5,2,0,"   // rp2
        + "-2.5,-2,0,"  // rp3
        + "-5,-2,0");   // rp4
    cmd[2] = CoordinateMappingData::FromString(juce::String("Example Mapping 2,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "2,5,0,"      // rp1
        + "2,2.5,0,"    // rp2
        + "-2,2.5,0,"   // rp3
        + "-2,5,0");    // rp4
    cmd[3] = CoordinateMappingData::FromString(juce::String("Example Mapping 3,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "5,-2,0,"     // rp1
        + "2.5,-2,0,"   // rp2
        + "2.5,2,0,"    // rp3
        + "5,2,0");     // rp4
    cmd[4] = CoordinateMappingData::FromString(juce::String("Example Mapping 4,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "-2,-5,0,"    // rp1
        + "-2,-2.5,0,"  // rp2
        + "2,-2.5,0,"   // rp3
        + "2,-5,0");    // rp4

    InitializeObjectValueCache(pd);
}

/**
 * Initializes internal cache with dummy values from given projectdata struct.
 * @param   projectData     The struct to initialize cache from
 */
void NoProtocolProtocolProcessor::InitializeObjectValueCache(const ProjectData& projectData)
{
    // helper function to avoid code duplicates
    std::function<void(const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record)> svtc
        = [=](const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record)
    {
        switch (roi)
        {
        case ROI_Positioning_SourcePosition:
            return SetValueToCache(roi, channel, record, { 0.0f, 0.0f, 0.0f });
        case ROI_CoordinateMapping_SourcePosition:
            return SetValueToCache(roi, channel, record, { 0.5f, 0.5f, 0.5f });
        case ROI_Positioning_SourcePosition_XY:
            return SetValueToCache(roi, channel, record, { 0.0f, 0.0f });
        case ROI_CoordinateMapping_SourcePosition_XY:
            return SetValueToCache(roi, channel, record, { 0.5f, 0.5f });
        default:
            return SetValueToCache(roi, channel, record, { 0.0f });
        };
    };

    // default init all bridging roi
    for (auto i = ROI_Invalid + 1; i < ROI_BridgingMAX; i++)
    {
        auto roi = static_cast<RemoteObjectIdentifier>(i);
        if (IsAnimatedObject(roi))
        {
            if (ProcessingEngineConfig::IsChannelAddressingObject(roi))
            {
                for (int channel = 1; channel <= sc_chCnt; channel++)
                {
                    if (ProcessingEngineConfig::IsRecordAddressingObject(roi))
                    {
                        for (int record = 1; record <= 4; record++)
                            svtc(roi, channel, static_cast<RecordId>(record));
                    }
                    else
                        svtc(roi, channel, INVALID_ADDRESS_VALUE);
                }
            }
        }
    }

    // all input relevant values
    for (auto const& inputNamesKV : projectData._inputNameData)
    {
        SetInputValuesToCache(inputNamesKV.first, inputNamesKV.second);
    }

    // all output relevant values
    for (auto const& speakerDataKV : projectData._speakerPositionData)
    {
        SetSpeakerPositionToCache(speakerDataKV.first, 
            float(speakerDataKV.second._x), float(speakerDataKV.second._y), float(speakerDataKV.second._z),
            float(speakerDataKV.second._hor), float(speakerDataKV.second._vrt), float(speakerDataKV.second._rot));
    }

    // all mapping settings relevant values
    for (auto const& coordMappingsKV : projectData._coordinateMappingData)
    {
        auto& mp = coordMappingsKV.first;
        auto& data = coordMappingsKV.second;

        auto name = data._name;

        float realp1[3] = { float(data._rp1x), float(data._rp1y), float(data._rp1z) };
        float realp2[3] = { float(data._rp2x), float(data._rp2y), float(data._rp2z) };
        float realp3[3] = { float(data._rp3x), float(data._rp3y), float(data._rp3z) };
        float realp4[3] = { float(data._rp4x), float(data._rp4y), float(data._rp4z) };
        float virtp1[3] = { float(data._vp1x), float(data._vp1y), float(data._vp1z) };
        float virtp3[3] = { float(data._vp3x), float(data._vp3y), float(data._vp3z) };
        int flip = data._flip;

        SetMappingSettingsToCache(mp, name, realp1, realp2, realp3, realp4, virtp1, virtp3, flip);
    }

    //// all scenes relevant values
    //SetSceneIndexToCache(1.0f);
    //
    //// all en-space relevant values
    //GetValueCache().SetValue(
    //    RemoteObject(ROI_MatrixSettings_ReverbRoomId, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
    //    RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_INT, 1, &oneInt, sizeof(int)));
    //GetValueCache().SetValue(
    //    RemoteObject(ROI_MatrixSettings_ReverbPredelayFactor, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
    //    RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));
    //GetValueCache().SetValue(
    //    RemoteObject(ROI_MatrixSettings_ReverbRearLevel, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
    //    RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));

}

/**
 * Triggers sending all values from internal cache.
 */
void NoProtocolProtocolProcessor::TriggerSendingObjectValueCache()
{
    for (auto const& value : GetValueCache().GetCachedValues())
    {
        auto& aro = GetActiveRemoteObjects();
        if (std::find(aro.begin(), aro.end(), value.first) != aro.end())
        {
            m_messageListener->OnProtocolMessageReceived(this, value.first._Id, value.second,
                RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, INVALID_EXTID));
        }
    }
}

/**
 * Helper method to set a given variant set of value as current value for a given remote object
 * @param   roi         The remote object id to set a value for
 * @param   channel     The channel of the object to set a value for
 * @param   record      The record of the object to set a value for
 * @param   vals        The value(s) to set
 */
void NoProtocolProtocolProcessor::SetValueToCache(const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const juce::Array<juce::var>& vals)
{
    int iVals[6] = { 0,0,0,0,0,0 };
    float fVals[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    auto addr = RemoteObjectAddressing(channel, record);

    auto valType = ROVT_NONE;
    auto valCount = std::uint16_t(0);

    switch (roi)
    {
    case ROI_MatrixInput_Mute:
        valType = ROVT_INT;
        valCount = 1;
        break;
    case ROI_MatrixInput_Gain:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixInput_Delay:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixInput_LevelMeterPreMute:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixInput_LevelMeterPostMute:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixOutput_Mute:
        valType = ROVT_INT;
        valCount = 1;
        break;
    case ROI_MatrixOutput_Gain:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixOutput_Delay:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixOutput_LevelMeterPreMute:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixOutput_LevelMeterPostMute:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_Positioning_SourceSpread:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_Positioning_SourceDelayMode:
        valType = ROVT_INT;
        valCount = 1;
        break;
    case ROI_Positioning_SourcePosition:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_Positioning_SourcePosition_XY:
        valType = ROVT_FLOAT;
        valCount = 2;
        break;
    case ROI_Positioning_SourcePosition_X:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_Positioning_SourcePosition_Y:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_CoordinateMapping_SourcePosition:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMapping_SourcePosition_XY:
        valType = ROVT_FLOAT;
        valCount = 2;
        break;
    case ROI_CoordinateMapping_SourcePosition_X:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_CoordinateMapping_SourcePosition_Y:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixSettings_ReverbRoomId:
        valType = ROVT_INT;
        valCount = 1;
        break;
    case ROI_MatrixSettings_ReverbPredelayFactor:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixSettings_ReverbRearLevel:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_MatrixInput_ReverbSendGain:
        valType = ROVT_FLOAT;
        valCount = 1;
        break;
    case ROI_CoordinateMappingSettings_P1real:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_P2real:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_P3real:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_P4real:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_P1virtual:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_P3virtual:
        valType = ROVT_FLOAT;
        valCount = 3;
        break;
    case ROI_CoordinateMappingSettings_Flip:
        valType = ROVT_INT;
        valCount = 1;
        break;
    case ROI_Positioning_SpeakerPosition:
        valType = ROVT_FLOAT;
        valCount = 6;
        break;
    default:
        jassertfalse; // not implemented
        break;
    }

    if (vals.size() != valCount)
        return;

    if (ROVT_FLOAT == valType)
    {
        for (int i = 0; i < vals.size(); i++)
            fVals[i] = vals[i];

        SetValue(
            RemoteObject(roi, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, valCount, fVals, valCount * sizeof(float)));
    }
    else if (ROVT_INT == valType)
    {
        for (int i = 0; i < vals.size(); i++)
            iVals[i] = vals[i];

        SetValue(
            RemoteObject(roi, addr),
            RemoteObjectMessageData(addr, ROVT_INT, valCount, iVals, valCount * sizeof(float)));
    }
    else
        jassertfalse; // not implemented
}

/**
 * Helper method to update the input related cached remote objects to
 * contain data matching a new scene index
 * @param
 */
void NoProtocolProtocolProcessor::SetInputValuesToCache(ChannelId channel, const juce::String& inputName)
{
    float pos[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float relpos[3] = { 0.5f, 0.5f, 0.0f };
    auto zeroFloat = 0.0f;
    auto oneInt = 1;

    auto addr = RemoteObjectAddressing(channel, INVALID_ADDRESS_VALUE);
    SetValue(
        RemoteObject(ROI_MatrixInput_ChannelName, addr),
        RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(inputName.length()), inputName.getCharPointer().getAddress(), static_cast<std::uint32_t>(inputName.length())));

    SetValue(
        RemoteObject(ROI_Positioning_SourcePosition, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, pos, 3 * sizeof(float)));

    for (int mp = 1; mp <= 4; mp++)
    {
        addr._second = RecordId(mp);
        SetValue(
            RemoteObject(ROI_CoordinateMapping_SourcePosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 3, relpos, 3 * sizeof(float)));
    }

    SetValue(
        RemoteObject(ROI_Positioning_SourceSpread, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 1, &zeroFloat, sizeof(float)));

    SetValue(
        RemoteObject(ROI_MatrixInput_ReverbSendGain, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 1, &zeroFloat, sizeof(float)));

    SetValue(
        RemoteObject(ROI_Positioning_SourceDelayMode, addr),
        RemoteObjectMessageData(addr, ROVT_INT, 1, &oneInt, sizeof(int)));
}

/**
 * Helper method to update the scene related cached remote objects to
 * contain data matching a new scene index
 * @param   sceneIndex  The new scene index the cached data should refer to
 */
void NoProtocolProtocolProcessor::SetSceneIndexToCache(std::float_t sceneIndex)
{
    if (sceneIndex < 1.0f)
        sceneIndex = 1.0f;

    auto sceneIndexMajor = static_cast<int>(sceneIndex);
    auto sceneIndexMinor = static_cast<int>(sceneIndex * 100.0f) % 100;

    auto sceneIndexString = juce::String(sceneIndexMajor) + "." + juce::String(sceneIndexMinor);
    SetValue(
        RemoteObject(ROI_Scene_SceneIndex, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneIndexString.length()), sceneIndexString.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneIndexString.length())));
    auto sceneName = juce::String("Example Scene ") + sceneIndexString;
    SetValue(
        RemoteObject(ROI_Scene_SceneName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneName.length()), sceneName.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneName.length())));
    auto sceneComment = juce::String("Example Scene Comment ") + sceneIndexString;
    SetValue(
        RemoteObject(ROI_Scene_SceneComment, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneComment.length()), sceneComment.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneComment.length())));
}

/**
 * Helper method to update the speaker position related cached remote objects to
 * contain data matching a new scene index
 * @param   
 */
void NoProtocolProtocolProcessor::SetSpeakerPositionToCache(ChannelId channel, float x, float y, float z, float hor, float vrt, float rot)
{
    float spos[6] = { x, y, z, hor, vrt, rot };

    auto addr1 = RemoteObjectAddressing(channel, INVALID_ADDRESS_VALUE);
    SetValue(
        RemoteObject(ROI_Positioning_SpeakerPosition, addr1),
        RemoteObjectMessageData(addr1, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));
}

/**
 * Helper method to update the coordinate mapping settings cached remote objects to
 * contain data matching a new scene index
 * @param   mapping     The mapping number to which the updated values refer
 * @param   mappingName The name of the mapping to which the values are updated
 * @param   realp1      P1 coords in real world values
 * @param   realp2      P2 coords in real world values
 * @param   realp3      P3 coords in real world values
 * @param   realp4      P4 coords in real world values
 * @param   virtp1      P1 coords in virtual (normalized) values
 * @param   virtp3      P3 coords in virtual (normalized) values
 */
void NoProtocolProtocolProcessor::SetMappingSettingsToCache(ChannelId mapping, const juce::String& mappingName, float realp1[3], float realp2[3], float realp3[3], float realp4[3], float virtp1[3], float virtp3[3], int flip)
{
    auto addr = RemoteObjectAddressing(mapping, INVALID_ADDRESS_VALUE);
    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_Name, addr),
        RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(mappingName.length()), mappingName.getCharPointer().getAddress(), static_cast<std::uint32_t>(mappingName.length())));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P1real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp1, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P2real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp2, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P3real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp3, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P4real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp4, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P1virtual, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, virtp1, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P3virtual, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, virtp3, 3 * sizeof(float)));

    SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_Flip, addr),
        RemoteObjectMessageData(addr, ROVT_INT, 1, &flip, sizeof(int)));
}

/**
 * Helper method to set a value to valuecache and allow derived classes to reimplement and add additional functionality
 * @param	ro			The remote object to set the value for
 * @param	valueData	The value to set
 */
void NoProtocolProtocolProcessor::SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData)
{
    GetValueCache().SetValue(ro, valueData);
}

/**
 * Helper method to calculate a next animation step.
 */
void NoProtocolProtocolProcessor::StepAnimation()
{
    for (auto const& valueKV : GetValueCache().GetCachedValues())
    {
        auto& objInfo = valueKV.first;
        auto& objData = valueKV.second;

        auto& roi = objInfo._Id;

        if (!IsAnimatedObject(roi))
            continue;

        auto& channel = objInfo._Addr._first;
        auto& record = objInfo._Addr._second;
        auto& dataCount = objData._valCount;
        auto& dataType = objData._valType;

        switch (dataType)
        {
        case ROVT_INT:
            if (nullptr != objData._payload && objData._payloadSize == sizeof(int) * dataCount)
            {
                for (auto i = 0; i < dataCount; i++)
                {
                    auto intValPtr = &static_cast<int*>(objData._payload)[i];
                    *intValPtr = CalculateValueStep(*intValPtr, roi, channel, record, i);
                }
            }
            break;
        case ROVT_FLOAT:
            if (nullptr != objData._payload && objData._payloadSize == sizeof(float) * dataCount)
            {       
                for (auto i = 0; i < dataCount; i++)
                {
                    auto floatValPtr = &static_cast<float*>(objData._payload)[i];
                    *floatValPtr = CalculateValueStep(*floatValPtr, roi, channel, record, i);
                }
            }
            break;
        default:
            break;
        }

        auto msgsToReflect = std::vector<std::pair<RemoteObjectIdentifier, RemoteObjectMessageData>>();
        msgsToReflect.push_back(std::make_pair(roi, objData));

        switch (roi)
        {
        case ROI_CoordinateMapping_SourcePosition:
            msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectMessageData(objData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(objData._payload)[0], sizeof(float))));
            msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_Y, RemoteObjectMessageData(objData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(objData._payload)[1], sizeof(float))));
            msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectMessageData(objData._addrVal, ROVT_FLOAT, 2, &reinterpret_cast<float*>(objData._payload)[0], 2 * sizeof(float))));
            break;
        default:
            break;
        }

        for (auto const& msgIdNData : msgsToReflect)
        {
            auto& aro = GetActiveRemoteObjects();
            if (std::find(aro.begin(), aro.end(), RemoteObject(msgIdNData.first, msgIdNData.second._addrVal)) != aro.end())
            {
                m_messageListener->OnProtocolMessageReceived(this, msgIdNData.first, msgIdNData.second,
                    RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_SetMessageAcknowledgement, -1));
            }
        }

    }

}

/**
 * Helper to get the info if a given object is part of what can be animated if configured.
 * @param   roi     The remote object id to get the info for
 * @return  True or false depending if the object is animatable or not.
 */
bool NoProtocolProtocolProcessor::IsAnimatedObject(const RemoteObjectIdentifier& roi)
{
    switch (roi)
    {
    case ROI_MatrixInput_Mute:
    case ROI_MatrixInput_Gain:
    case ROI_MatrixInput_Delay:
    case ROI_MatrixInput_LevelMeterPreMute:
    case ROI_MatrixInput_LevelMeterPostMute:
    case ROI_MatrixOutput_Mute:
    case ROI_MatrixOutput_Gain:
    case ROI_MatrixOutput_Delay:
    case ROI_MatrixOutput_LevelMeterPreMute:
    case ROI_MatrixOutput_LevelMeterPostMute:
    case ROI_Positioning_SourceSpread:
    case ROI_Positioning_SourceDelayMode:
    case ROI_Positioning_SourcePosition:
    case ROI_Positioning_SourcePosition_XY:
    case ROI_Positioning_SourcePosition_X:
    case ROI_Positioning_SourcePosition_Y:
    case ROI_CoordinateMapping_SourcePosition:		
    case ROI_CoordinateMapping_SourcePosition_XY:	
    case ROI_CoordinateMapping_SourcePosition_X:		
    case ROI_CoordinateMapping_SourcePosition_Y:
    case ROI_MatrixSettings_ReverbRoomId:
    case ROI_MatrixSettings_ReverbPredelayFactor:
    case ROI_MatrixSettings_ReverbRearLevel:
    case ROI_MatrixInput_ReverbSendGain:
        return true;
    default:
        return false;
    }
}

/**
 * Method to calculate the next value in the chain of animation for a given object.
 * @param   lastValue   The last calculated float value
 * @param   roi         The remote object id to calculate for
 * @param   channel     The channel of the object to calculate
 * @param   record      The record of the object to calculate
 * @param   valueIndex  The index of the value to calculate
 * @return  The calculated next object value.
 */
float NoProtocolProtocolProcessor::CalculateValueStep(const float& lastValue, const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const int& valueIndex)
{
    //auto val1 = (sin((0.1f * GetCallbackCount()) + (channel * 0.1f)) + 1.0f) * 0.5f;
    //auto val2 = (cos((0.1f * GetCallbackCount()) + (channel * 0.1f)) + 1.0f) * 0.5f;
    ignoreUnused(record);

    auto normalizedValue = 0.0f;
    
    if (AM_Circle == GetAnimationMode())
    {
        normalizedValue = (sin((0.1f * GetCallbackCount()) + (channel * 0.1f) + (valueIndex * juce::MathConstants<float>::halfPi)) + 1.0f) * 0.5f;
    }
    else if (AM_Rand == GetAnimationMode())
    {
        auto randomizedValue = static_cast<float>((sin((0.1f * GetCallbackCount()) + (m_channelRandomizedFactors[channel] * channel * 0.1f) + (m_valueIdRandomizedFactors[valueIndex] * valueIndex * juce::MathConstants<float>::halfPi)) + 1.0f) * m_channelRandomizedScaleFactors[channel]);
        normalizedValue = jlimit(0.0f, 1.0f, randomizedValue);
    }
    else
    {
        jassertfalse;
        return lastValue;
    }

    auto valueRange = ProcessingEngineConfig::GetRemoteObjectRange(roi);
    if (valueRange.isEmpty())
        return normalizedValue;
    else
        return juce::jmap(normalizedValue, valueRange.getStart(), valueRange.getEnd());
}

/**
 * Method to calculate the next value in the chain of animation for a given object.
 * @param   lastValue   The last calculated float value
 * @param   roi         The remote object id to calculate for
 * @param   channel     The channel of the object to calculate
 * @param   record      The record of the object to calculate
 * @param   valueIndex  The index of the value to calculate
 * @return  The calculated next object value.
 */
int NoProtocolProtocolProcessor::CalculateValueStep(const int& lastValue, const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const int& valueIndex)
{
    //auto val1 = (sin((0.1f * GetCallbackCount()) + (channel * 0.1f)) + 1.0f) * 0.5f;
    //auto val2 = (cos((0.1f * GetCallbackCount()) + (channel * 0.1f)) + 1.0f) * 0.5f;
    ignoreUnused(record);

    auto normalizedValue = 0.0f;

    if (AM_Circle == GetAnimationMode())
    {
        normalizedValue = (sin((0.1f * GetCallbackCount()) + (channel * 0.1f) + (valueIndex * juce::MathConstants<float>::halfPi)) + 1.0f) * 0.6f;
    }
    else if (AM_Rand == GetAnimationMode())
    {
        auto randomizedValue = static_cast<float>((sin((0.1f * GetCallbackCount()) + (m_channelRandomizedFactors[channel] * channel * 0.1f) + (m_valueIdRandomizedFactors[valueIndex] * valueIndex * juce::MathConstants<float>::halfPi)) + 1.0f) * m_channelRandomizedScaleFactors[channel]);
        normalizedValue = jlimit(0.0f, 1.0f, randomizedValue);
    }
    else
    {
        jassertfalse;
        return lastValue;
    }

    auto valueRange = ProcessingEngineConfig::GetRemoteObjectRange(roi);
    if (valueRange.isEmpty())
        return static_cast<int>(normalizedValue + 0.5f);
    else
        return static_cast<int>(juce::jmap(normalizedValue, valueRange.getStart(), valueRange.getEnd()));
}
