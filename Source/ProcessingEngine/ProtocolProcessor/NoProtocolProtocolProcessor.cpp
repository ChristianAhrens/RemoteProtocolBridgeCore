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
NoProtocolProtocolProcessor::NoProtocolProtocolProcessor(const NodeId& parentNodeId)
	: ProtocolProcessorBase(parentNodeId)
{
	m_type = ProtocolType::PT_NoProtocol;

	SetActiveRemoteObjectsInterval(4000); // used as 4s KeepAlive interval

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
    if (!ProtocolProcessorBase::setStateXml(stateXml))
    {
        return false;
    }
    else if (1 == stateXml->getNumChildElements() && stateXml->getFirstChildElement()->isTextElement())
    {
        auto projectData = ProjectData::FromString(stateXml->getFirstChildElement()->getAllSubText());
        if (!projectData.IsEmpty())
            InitializeObjectValueCache(projectData);

        return true;
    }
    else
        return false;
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool NoProtocolProtocolProcessor::Start()
{
	m_IsRunning = true;

	startTimerThread(GetActiveRemoteObjectsInterval(), 100);

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

                GetValueCache().SetValue(RemoteObject(roi, msgData._addrVal), msgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_XY:
            {
                if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
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

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_X:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
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
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                GetValueCache().SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, msgData._payload, 2 * sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_CoordinateMapping_SourcePosition_Y:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_CoordinateMapping_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                GetValueCache().SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, msgData._payload, 2 * sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition:
            {
                if (msgData._valCount != 3 || msgData._payloadSize != 3 * sizeof(float))
                    return false;

                GetValueCache().SetValue(RemoteObject(roi, msgData._addrVal), msgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
            }
            break;
        case ROI_Positioning_SourcePosition_XY:
            {
                if (msgData._valCount != 2 || msgData._payloadSize != 2 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
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

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition_X:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
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
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                GetValueCache().SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, msgData._payload, 2 * sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_Y, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[1], sizeof(float))));
            }
            break;
        case ROI_Positioning_SourcePosition_Y:
            {
                if (msgData._valCount != 1 || msgData._payloadSize != 1 * sizeof(float))
                    return false;

                auto targetObj = RemoteObject(ROI_Positioning_SourcePosition, msgData._addrVal);
                if (!GetValueCache().Contains(targetObj))
                    GetValueCache().SetValue(targetObj, RemoteObjectMessageData(targetObj._Addr, ROVT_FLOAT, 3, &zeroPayload, 3 * sizeof(float)));

                // get the xyz data from cache to insert the new xy data and send the xyz out
                auto& refMsgData = GetValueCache().GetValue(targetObj);
                if (refMsgData._valCount != 3 || refMsgData._payloadSize != 3 * sizeof(float))
                    return false;
                reinterpret_cast<float*>(refMsgData._payload)[1] = reinterpret_cast<float*>(msgData._payload)[0];

                //auto x = reinterpret_cast<float*>(refMsgData._payload)[0];
                //auto y = reinterpret_cast<float*>(refMsgData._payload)[1];
                //auto z = reinterpret_cast<float*>(refMsgData._payload)[2];
                //DBG(juce::String(__FUNCTION__) << " ROI:" << id << " data from cache combined with incoming -> x:" << x << " y:" << y << " z:" << z);

                GetValueCache().SetValue(targetObj, refMsgData);

                msgsToReflect.push_back(std::make_pair(roi, msgData));
                msgsToReflect.push_back(std::make_pair(targetObj._Id, refMsgData));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_X, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 1, &reinterpret_cast<float*>(msgData._payload)[0], sizeof(float))));
                msgsToReflect.push_back(std::make_pair(ROI_Positioning_SourcePosition_XY, RemoteObjectMessageData(msgData._addrVal, ROVT_FLOAT, 2, msgData._payload, 2 * sizeof(float))));
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
			break;
		}

		for (auto const& msgIdNData : msgsToReflect)
			m_messageListener->OnProtocolMessageReceived(this, msgIdNData.first, msgIdNData.second,
				RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_SetMessageAcknowledgement, externalId));
	}

	return true;
}

/**
 * TimerThreadBase callback to send keepalive queries cyclically
 */
void NoProtocolProtocolProcessor::timerThreadCallback()
{
	if (m_IsRunning)
	{
		m_messageListener->OnProtocolMessageReceived(this, ROI_HeartbeatPong, RemoteObjectMessageData(), RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, -1));
	}
}

/**
 * Initializes internal cache with somewhat feasible dummy values.
 */
void NoProtocolProtocolProcessor::InitializeObjectValueCache()
{
    auto deviceName = juce::String("InternalSim");
    GetValueCache().SetValue(
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
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRoomId, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_INT, 1, &oneInt, sizeof(int)));
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbPredelayFactor, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRearLevel, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));


    // all other relevant values can be initialized through ProjectData
    ProjectData pd;

    // all input relevant values
    auto& ind = pd._inputNameData;
    for (int in = 1; in <= 128; in++)
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
        m_messageListener->OnProtocolMessageReceived(this, value.first._Id, value.second,
            RemoteObjectMessageMetaInfo(RemoteObjectMessageMetaInfo::MessageCategory::MC_UnsolicitedMessage, -1));
    }
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
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixInput_ChannelName, addr),
        RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(inputName.length()), inputName.getCharPointer().getAddress(), static_cast<std::uint32_t>(inputName.length())));

    GetValueCache().SetValue(
        RemoteObject(ROI_Positioning_SourcePosition, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, pos, 3 * sizeof(float)));

    for (int mp = 1; mp <= 4; mp++)
    {
        addr._second = RecordId(mp);
        GetValueCache().SetValue(
            RemoteObject(ROI_CoordinateMapping_SourcePosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 3, relpos, 3 * sizeof(float)));
    }

    GetValueCache().SetValue(
        RemoteObject(ROI_Positioning_SourceSpread, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 1, &zeroFloat, sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixInput_ReverbSendGain, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 1, &zeroFloat, sizeof(float)));

    GetValueCache().SetValue(
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
    GetValueCache().SetValue(
        RemoteObject(ROI_Scene_SceneIndex, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneIndexString.length()), sceneIndexString.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneIndexString.length())));
    auto sceneName = juce::String("Example Scene ") + sceneIndexString;
    GetValueCache().SetValue(
        RemoteObject(ROI_Scene_SceneName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneName.length()), sceneName.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneName.length())));
    auto sceneComment = juce::String("Example Scene Comment ") + sceneIndexString;
    GetValueCache().SetValue(
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
    GetValueCache().SetValue(
        RemoteObject(ROI_Positioning_SpeakerPosition, addr1),
        RemoteObjectMessageData(addr1, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));
}

/**
 * Helper method to update the coordinate mapping settings cached remote objects to
 * contain data matching a new scene index
 * @param   
 */
void NoProtocolProtocolProcessor::SetMappingSettingsToCache(ChannelId mapping, const juce::String& mappingName, float realp1[3], float realp2[3], float realp3[3], float realp4[3], float virtp1[3], float virtp3[3], int flip)
{
    auto addr = RemoteObjectAddressing(mapping, INVALID_ADDRESS_VALUE);
    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_Name, addr),
        RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(mappingName.length()), mappingName.getCharPointer().getAddress(), static_cast<std::uint32_t>(mappingName.length())));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P1real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp1, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P2real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp2, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P3real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp3, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P4real, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, realp4, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P1virtual, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, virtp1, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_P3virtual, addr),
        RemoteObjectMessageData(addr, ROVT_FLOAT, 3, virtp3, 3 * sizeof(float)));

    GetValueCache().SetValue(
        RemoteObject(ROI_CoordinateMappingSettings_Flip, addr),
        RemoteObjectMessageData(addr, ROVT_INT, 1, &flip, sizeof(int)));
}
