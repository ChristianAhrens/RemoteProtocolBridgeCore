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

#include <NanoOcp1.h>
#include <Ocp1ObjectDefinitions.h>


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
	if (m_nanoOcp && m_nanoOcp->start())
	{
		startTimerThread(GetActiveRemoteObjectsInterval(), 100);

		// Assign the callback functions AFTER internal handling is set up to not already get called before that is done
		m_nanoOcp->onDataReceived = [=](const juce::MemoryBlock& data) {
			return ocp1MessageReceived(data);
		};
		m_nanoOcp->onConnectionEstablished = [=]() {
			m_IsRunning = true;
		};
		m_nanoOcp->onConnectionLost = [=]() {
			m_IsRunning = false;
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

/**
 * Overloaded method to stop to protocol processing object.
 */
bool OCP1ProtocolProcessor::Stop()
{
	m_nanoOcp->onDataReceived = std::function<bool(const juce::MemoryBlock & data)>();
	m_nanoOcp->onConnectionEstablished = std::function<void()>();
	m_nanoOcp->onConnectionLost = std::function<void()>();

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
 * Private method to get OCA object specific ObjectName string
 * 
 * @param id	The object id to get the OCA specific object name
 * @return		The OCA specific object name
 */
String OCP1ProtocolProcessor::GetRemoteObjectString(RemoteObjectIdentifier id)
{
	switch (id)
	{
	case ROI_CoordinateMapping_SourcePosition_X:
		return "CoordinateMapping_Source_Position_X";
	case ROI_CoordinateMapping_SourcePosition_Y:
		return "CoordinateMapping_Source_Position_Y";
	case ROI_CoordinateMapping_SourcePosition_XY:
		return "CoordinateMapping_Source_Position_XY";
	case ROI_Positioning_SourcePosition_X:
		return "Positioning_Source_Position_X";
	case ROI_Positioning_SourcePosition_Y:
		return "Positioning_Source_Position_Y";
	case ROI_Positioning_SourcePosition_XY:
		return "Positioning_Source_Position_XY";
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

            DBG("Got an OCA notification from ONo 0x" << juce::String::toHexString(notifObj->GetEmitterOno()));

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
            //else
            //{
            //    DBG("Got an OCA notification from UNKNOWN object ONo 0x" << juce::String::toHexString(notifObj->GetEmitterOno()));
            //}

            return true;
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
