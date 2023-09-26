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
    float pos[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float relpos[3] = { 0.5f, 0.5f, 0.0f };
    auto oneFloat = 1.0f;
    auto zeroFloat = 0.0f;
    auto oneInt = 1;
    auto zeroInt = 0;

    auto deviceName = juce::String("InternalSim");
    GetValueCache().SetValue(
        RemoteObject(ROI_Settings_DeviceName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(deviceName.length()), deviceName.getCharPointer().getAddress(), static_cast<std::uint32_t>(deviceName.length())));

    // all input relevant values
    for (int in = 1; in <= 128; in++)
    {
        auto name = juce::String("Input ") + juce::String(in);

        auto addr = RemoteObjectAddressing(in, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_MatrixInput_ChannelName, addr), 
            RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(name.length()), name.getCharPointer().getAddress(), static_cast<std::uint32_t>(name.length())));

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

    // all output relevant values
    float spos[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    auto& x = spos[0];
    auto& y = spos[1];
    auto& a = spos[3];

    x = -8.5f;
    y = -7.5f;
    a = 0.0f;
    for (int out = 1; out <= 16; out++)
    {
        auto addr = RemoteObjectAddressing(out, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_Positioning_SpeakerPosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));

        y += 1.0f;
    }
    x = -7.5f;
    y = 8.5f;
    a = 270.0f;
    for (int out = 17; out <= 32; out++)
    {
        auto addr = RemoteObjectAddressing(out, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_Positioning_SpeakerPosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));

        x += 1.0f;
    }
    x = 8.5f;
    y = -7.5f;
    a = 180.0f;
    for (int out = 33; out <= 48; out++)
    {
        auto addr = RemoteObjectAddressing(out, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_Positioning_SpeakerPosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));

        y += 1.0f;
    }
    x = -7.5f;
    y = -8.5f;
    a = 90.0f;
    for (int out = 49; out <= 64; out++)
    {
        auto addr = RemoteObjectAddressing(out, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_Positioning_SpeakerPosition, addr),
            RemoteObjectMessageData(addr, ROVT_FLOAT, 6, spos, 6 * sizeof(float)));

        x += 1.0f;
    }

    // all mapping settings relevant values
    float realp3[3] = { -1.0f, -6.0f, 0.0f };
    float realp2[3] = { -1.0f, -1.0f, 0.0f };
    float realp1[3] = { -6.0f, -1.0f, 0.0f };
    float realp4[3] = { -6.0f, -6.0f, 0.0f };
    float virtp1[3] = { 1.0f, 1.0f, 0.0f };
    float virtp3[3] = { 0.0f, 0.0f, 0.0f };
    for (int mp = 1; mp <= 4; mp++)
    {
        auto name = juce::String("Mapping Area ") + juce::String(mp);

        auto addr = RemoteObjectAddressing(mp, INVALID_ADDRESS_VALUE);
        GetValueCache().SetValue(
            RemoteObject(ROI_CoordinateMappingSettings_Name, addr),
            RemoteObjectMessageData(addr, ROVT_STRING, static_cast<std::uint16_t>(name.length()), name.getCharPointer().getAddress(), static_cast<std::uint32_t>(name.length())));

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
            RemoteObjectMessageData(addr, ROVT_INT, 1, &zeroInt, sizeof(int)));

        if (mp % 2 == 0)
        {
            realp1[1] -= 7.0f;
            realp2[1] -= 7.0f;
            realp3[1] -= 7.0f;
            realp4[1] -= 7.0f;

            realp1[0] += 7.0f;
            realp2[0] += 7.0f;
            realp3[0] += 7.0f;
            realp4[0] += 7.0f;
        }
        else
        {
            realp1[1] += 7.0f;
            realp2[1] += 7.0f;
            realp3[1] += 7.0f;
            realp4[1] += 7.0f;
        }
    }

    // all scenes relevant values
    SetSceneIndexToCache(1.0f);

    // all en-space relevant values
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRoomId, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_INT, 1, &oneInt, sizeof(int)));
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbPredelayFactor, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));
    GetValueCache().SetValue(
        RemoteObject(ROI_MatrixSettings_ReverbRearLevel, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_FLOAT, 1, &oneFloat, sizeof(float)));

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
    auto sceneName = juce::String("Dummy Scene ") + sceneIndexString;
    GetValueCache().SetValue(
        RemoteObject(ROI_Scene_SceneName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneName.length()), sceneName.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneName.length())));
    auto sceneComment = juce::String("Dummy Scene Comment ") + sceneIndexString;
    GetValueCache().SetValue(
        RemoteObject(ROI_Scene_SceneComment, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(sceneComment.length()), sceneComment.getCharPointer().getAddress(), static_cast<std::uint32_t>(sceneComment.length())));
}
