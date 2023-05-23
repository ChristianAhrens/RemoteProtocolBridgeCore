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
        if (m_nanoOcp->start())
        {
            // Assign the callback functions AFTER internal handling is set up to not already get called before that is done
            m_nanoOcp->onDataReceived = [=](const juce::MemoryBlock& data) {
                return ocp1MessageReceived(data);
            };
            m_nanoOcp->onConnectionEstablished = [=]() {
                startTimerThread(GetActiveRemoteObjectsInterval(), 100);
                CreateObjectSubscriptions();
                m_IsRunning = true;
            };
            m_nanoOcp->onConnectionLost = [=]() {
                stopTimerThread();
                m_IsRunning = false;
                DeleteObjectSubscriptions();
            };

            return true;
        }
        else
        {
            m_nanoOcp->onDataReceived = std::function<bool(const juce::MemoryBlock & data)>();
            m_nanoOcp->onConnectionEstablished = std::function<void()>();
            m_nanoOcp->onConnectionLost = std::function<void()>();

            return false;
        }
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
 * Method to trigger sending of a message
 * NOT YET IMPLEMENTED
 *
 * @param Id		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 */
bool OCP1ProtocolProcessor::SendRemoteObjectMessage(RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData)
{
	ignoreUnused(Id);
	ignoreUnused(msgData);

	return false;
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
	case ROI_MatrixInput_ReverbSendGain:
		return "MatrixInput_ReverbSendGain";
	default:
		return "";
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
        m_ROIsToDefsMap[ROI_Positioning_SourceDelayMode][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Spread(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_ReverbSendGain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_ReverbSendGain(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Gain][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Gain(channel);
        m_ROIsToDefsMap[ROI_MatrixInput_Mute][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_MatrixInput_Mute(channel);
        for (record = RecordId(MappingAreaId::MAI_First); record <= MappingAreaId::MAI_Fourth; record++)
            m_ROIsToDefsMap[ROI_CoordinateMapping_SourcePosition][std::make_pair(record, channel)] = NanoOcp1::DS100::dbOcaObjectDef_CoordinateMapping_Source_Position(0x00, channel);
    }
}

/**
 * TimerThreadBase callback to send keepalive queries cyclically
 */
void OCP1ProtocolProcessor::timerThreadCallback()
{
    /*todo*/
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

            for (auto roisKV = m_ROIsToDefsMap.begin(); roisKV != m_ROIsToDefsMap.end(); roisKV++)
            {
                for (auto objDefKV = roisKV->second.begin(); objDefKV != roisKV->second.end(); objDefKV++)
                {
                    if (notifObj->MatchesObject(&objDefKV->second))
                    {
                        return true;
                    }
                }
            }

            //// Update the right GUI element according to the definition of the object 
            //// which triggered the notification.
            //if (notifObj->MatchesObject(m_pwrOnObjDef.get()))
            //{
            //    std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
            //    m_powerD40LED->setToggleState(switchSetting > 0, dontSendNotification);
            //}
            //else if (notifObj->MatchesObject(m_potiLevelObjDef.get()))
            //{
            //    std::float_t newGain = NanoOcp1::DataToFloat(notifObj->GetParameterData());
            //    m_gainSlider->setValue(newGain, dontSendNotification);
            //}

            DBG("Got an OCA notification from ONo 0x" << juce::String::toHexString(notifObj->GetEmitterOno()));

            return false;
        }
        case NanoOcp1::Ocp1Message::Response:
        {
            NanoOcp1::Ocp1Response* responseObj = static_cast<NanoOcp1::Ocp1Response*>(msgObj.get());

            // Get the objDef matching the obtained response handle.
            //const auto iter = m_ocaHandleMap.find(responseObj->GetResponseHandle());
            //if (iter != m_ocaHandleMap.end())
            //{
            //    if (responseObj->GetResponseStatus() != 0)
            //    {
            //        DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
            //            " with status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()));
            //    }
            //    else if (responseObj->GetParamCount() == 0)
            //    {
            //        DBG("Got an empty \"OK\" OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()));
            //    }
            //    else
            //    {
            //        DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
            //            " and paramCount " << juce::String(responseObj->GetParamCount()));
            //
            //        // Update the right GUI element according to the definition of the object 
            //        // which triggered the response.
            //        NanoOcp1::Ocp1CommandDefinition* objDef = iter->second;
            //        if (objDef == m_pwrOnObjDef.get())
            //        {
            //            std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
            //            m_powerD40LED->setToggleState(switchSetting > 0, dontSendNotification);
            //        }
            //        else if (objDef == m_potiLevelObjDef.get())
            //        {
            //            std::float_t newGain = NanoOcp1::DataToFloat(responseObj->GetParameterData());
            //            m_gainSlider->setValue(newGain, dontSendNotification);
            //        }
            //        else
            //        {
            //            DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
            //                " which could not be processed (unknown object)!");
            //        }
            //    }
            //
            //    // Finally remove handle from the list, as it has been processed.
            //    m_ocaHandleMap.erase(iter);
            //}
            //else
            {
                DBG("Got an OCA response for UNKNOWN handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
                    "; status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()) <<
                    "; paramCount " << juce::String(responseObj->GetParamCount()));
            }

            return true;
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

    bool success = true;

    std::uint32_t handle = 0;

    //// subscribe pwrOn
    //success = success && m_nanoOcp1Client->sendData(
    //    NanoOcp1::Ocp1CommandResponseRequired(
    //        NanoOcp1::Dy::dbOcaObjectDef_Settings_PwrOn().AddSubscriptionCommand(), handle).GetMemoryBlock());
    //AddPendingSubscriptionHandle(handle);
    //
    //// subscribe potilevel for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    success = success && m_nanoOcp1Client->sendData(
    //        NanoOcp1::Ocp1CommandResponseRequired(
    //            NanoOcp1::Dy::dbOcaObjectDef_Config_PotiLevel(ch).AddSubscriptionCommand(), handle).GetMemoryBlock());
    //    AddPendingSubscriptionHandle(handle);
    //}
    //
    //// subscribe mute for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    success = success && m_nanoOcp1Client->sendData(
    //        NanoOcp1::Ocp1CommandResponseRequired(
    //            NanoOcp1::Dy::dbOcaObjectDef_Config_Mute(ch).AddSubscriptionCommand(), handle).GetMemoryBlock());
    //    AddPendingSubscriptionHandle(handle);
    //}
    //
    //// subscribe isp for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    success = success && m_nanoOcp1Client->sendData(
    //        NanoOcp1::Ocp1CommandResponseRequired(
    //            NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Isp(ch).AddSubscriptionCommand(), handle).GetMemoryBlock());
    //    AddPendingSubscriptionHandle(handle);
    //}
    //
    //// subscribe gr for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    success = success && m_nanoOcp1Client->sendData(
    //        NanoOcp1::Ocp1CommandResponseRequired(
    //            NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Gr(ch).AddSubscriptionCommand(), handle).GetMemoryBlock());
    //    AddPendingSubscriptionHandle(handle);
    //}
    //
    //// subscribe ovl for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    success = success && m_nanoOcp1Client->sendData(
    //        NanoOcp1::Ocp1CommandResponseRequired(
    //            NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Ovl(ch).AddSubscriptionCommand(), handle).GetMemoryBlock());
    //    AddPendingSubscriptionHandle(handle);
    //}

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

    bool success = true;

    std::uint32_t handle = 0;

    //auto pwrCmdDef = NanoOcp1::Dy::dbOcaObjectDef_Settings_PwrOn();
    //// query pwrOn
    //success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(pwrCmdDef, handle).GetMemoryBlock());
    //AddPendingGetValueHandle(handle, pwrCmdDef.m_targetOno);
    //
    //// query potilevel for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto potCmdDef = NanoOcp1::Dy::dbOcaObjectDef_Config_PotiLevel(ch);
    //    success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(potCmdDef, handle).GetMemoryBlock());
    //    AddPendingGetValueHandle(handle, potCmdDef.m_targetOno);
    //}
    //
    //// query mute for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto muteCmdDef = NanoOcp1::Dy::dbOcaObjectDef_Config_Mute(ch);
    //    success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(muteCmdDef, handle).GetMemoryBlock());
    //    AddPendingGetValueHandle(handle, muteCmdDef.m_targetOno);
    //}
    //
    //// query isp for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto ispCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Isp(ch);
    //    success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(ispCmdDef, handle).GetMemoryBlock());
    //    AddPendingGetValueHandle(handle, ispCmdDef.m_targetOno);
    //}
    //
    //// query gr for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto grCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Gr(ch);
    //    success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(grCmdDef, handle).GetMemoryBlock());
    //    AddPendingGetValueHandle(handle, grCmdDef.m_targetOno);
    //}
    //
    //// query ovl for all channels
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto ovlCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Ovl(ch);
    //    success = success && m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(ovlCmdDef, handle).GetMemoryBlock());
    //    AddPendingGetValueHandle(handle, ovlCmdDef.m_targetOno);
    //}

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

bool OCP1ProtocolProcessor::UpdateObjectValues(const NanoOcp1::Ocp1Notification* notifObj)
{
    // Update the right GUI element according to the definition of the object 
    // which triggered the notification.

    //// Objects without any further addressing
    //auto cmdDef = NanoOcp1::Dy::dbOcaObjectDef_Settings_PwrOn();
    //if (notifObj->MatchesObject(&cmdDef))
    //{
    //    std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
    //
    //    if (onPwrOnOff)
    //        onPwrOnOff(switchSetting > 0);
    //
    //    return true;
    //}
    //
    //// Objects with additional channel addressing dimension
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    auto potCmdDef = NanoOcp1::Dy::dbOcaObjectDef_Config_PotiLevel(ch);
    //    auto muteCmdDef = NanoOcp1::Dy::dbOcaObjectDef_Config_Mute(ch);
    //    auto ispCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Isp(ch);
    //    auto grCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Gr(ch);
    //    auto ovlCmdDef = NanoOcp1::Dy::dbOcaObjectDef_ChStatus_Ovl(ch);
    //    if (notifObj->MatchesObject(&potCmdDef))
    //    {
    //        std::float_t newGain = NanoOcp1::DataToFloat(notifObj->GetParameterData());
    //
    //        if (onChannelGain)
    //            onChannelGain(ch, newGain);
    //
    //        return true;
    //    }
    //    else if (notifObj->MatchesObject(&muteCmdDef))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
    //
    //        if (onChannelMute)
    //            onChannelMute(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (notifObj->MatchesObject(&ispCmdDef))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
    //
    //        if (onChannelISP)
    //            onChannelISP(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (notifObj->MatchesObject(&grCmdDef))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
    //
    //        if (onChannelGR)
    //            onChannelGR(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (notifObj->MatchesObject(&ovlCmdDef))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
    //
    //        if (onChannelOVL)
    //            onChannelOVL(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //
    //    //// Objects with additional record addressing dimension
    //    //for (std::uint16_t rec = 1; rec <= GetAmpRecordCount(); ch++)
    //    //{
    //    //}
    //}

    return false;
}

bool OCP1ProtocolProcessor::UpdateObjectValues(const std::uint32_t ONo, const NanoOcp1::Ocp1Response* responseObj)
{
    // Update the right GUI element according to the definition of the object 
    // to which the response refers.

    //// Objects without any further addressing
    //if (ONo == NanoOcp1::GetONo(1, 0, 0, NanoOcp1::Dy::Settings_PwrOn))
    //{
    //    std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
    //
    //    if (onPwrOnOff)
    //        onPwrOnOff(switchSetting > 0);
    //
    //    return true;
    //}
    //
    //// Objects with additional channel addressing dimension
    //for (std::uint16_t ch = 1; ch <= GetAmpChannelCount(); ch++)
    //{
    //    if (ONo == NanoOcp1::GetONo(1, 0, ch, NanoOcp1::Dy::Config_PotiLevel))
    //    {
    //        std::float_t newGain = NanoOcp1::DataToFloat(responseObj->GetParameterData());
    //
    //        if (onChannelGain)
    //            onChannelGain(ch, newGain);
    //
    //        return true;
    //    }
    //    else if (ONo == NanoOcp1::GetONo(1, 0, ch, NanoOcp1::Dy::Config_Mute))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
    //
    //        if (onChannelMute)
    //            onChannelMute(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (ONo == NanoOcp1::GetONo(1, 0, ch, NanoOcp1::Dy::ChStatus_Isp))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
    //
    //        if (onChannelISP)
    //            onChannelISP(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (ONo == NanoOcp1::GetONo(1, 0, ch, NanoOcp1::Dy::ChStatus_Gr))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
    //
    //        if (onChannelGR)
    //            onChannelGR(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //    else if (ONo == NanoOcp1::GetONo(1, 0, ch, NanoOcp1::Dy::ChStatus_Ovl))
    //    {
    //        std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
    //
    //        if (onChannelOVL)
    //            onChannelOVL(ch, switchSetting == 1);
    //
    //        return true;
    //    }
    //
    //    //// Objects with additional record addressing dimension
    //    //for (std::uint16_t rec = 1; rec <= GetAmpRecordCount(); ch++)
    //    //{
    //    //}
    //}

    return false;
}

