/* Copyright (c) 2024, Christian Ahrens
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

#include "AURAProtocolProtocolProcessor.h"

#include "../../dbprProjectUtils.h"


// **************************************************************************************
//    class AURAProtocolProtocolProcessor
// **************************************************************************************
/**
 * Derived OSC remote protocol processing class
 */
AURAProtocolProtocolProcessor::AURAProtocolProtocolProcessor(const NodeId& parentNodeId)
	: NoProtocolProtocolProcessor(parentNodeId, false)
{
	m_type = ProtocolType::PT_AURAProtocol;

    SetActiveRemoteObjectsInterval(-1); // default value in ProtocolProcessorBase is 100 which we do not want to use, so invalidate it to overcome potential misunderstandings when reading code

    InitializeObjectValueCache();

    m_networkConnection = std::make_unique<AURAConnection>();
}

/**
 * Destructor
 */
AURAProtocolProtocolProcessor::~AURAProtocolProtocolProcessor()
{
	Stop();
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool AURAProtocolProtocolProcessor::setStateXml(XmlElement* stateXml)
{
    if (nullptr == stateXml || !ProtocolProcessorBase::setStateXml(stateXml))
    {
        return false;
    }
    else
    {
        auto retVal = true;
        auto listenerPosXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::POSITION));
        if (listenerPosXmlElement)
        {
            auto positionValues = StringArray();
            if (3 == positionValues.addTokens(listenerPosXmlElement->getAllSubText(), ";", ""))
                SetListenerPosition({ positionValues[0].getFloatValue(), positionValues[1].getFloatValue(), positionValues[2].getFloatValue() });
        }
        else
            retVal = false;

        auto areaXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::AREA));
        if (areaXmlElement)
        {
            auto areaValues = StringArray();
            if (2 == areaValues.addTokens(areaXmlElement->getAllSubText(), ";", ""))
                SetArea({ areaValues[0].getFloatValue(), areaValues[1].getFloatValue() });
        }
        else
            retVal = false;

        auto ipAdressXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::IPADDRESS));
        if (ipAdressXmlElement)
        {
            SetIpAddress(juce::IPAddress(ipAdressXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ADRESS)).toStdString()));
        }
        else
            retVal = false;
        
        auto clientPortXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::CLIENTPORT));
        if (clientPortXmlElement)
        {
            SetPort(clientPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT)));
        }
        else
            retVal = false;

        return retVal;
    }
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool AURAProtocolProtocolProcessor::Start()
{
    if (m_networkConnection)
    {
        // assign lambdas for connection status tracking first
        m_networkConnection->onConnectionEstablished = [=]() {
            stopTimer();

            auto info = "AURA connection established.";
            DBG(info);
            std::cout << info << std::endl;

            SendListenerPositionToAURA();
            SendKnownSourcePositionsToAURA();
        };
        m_networkConnection->onConnectionLost = [=]() {
            auto info = "AURA connection lost.";
            DBG(info);
            std::cout << info << std::endl;

            startTimer(500); // start trying to reestablish connection
        };

        // then fire up connection
        auto info = (juce::String("Try to connect to AURA on ") + m_ipAddress.toString() + ":" + juce::String(m_port)).toStdString();
        DBG(info);
        std::cout << info << std::endl;
        if (!m_networkConnection->connect(m_ipAddress, m_port))
            startTimer(500); // start trying to establish connection
        else
            startTimer(5000); // connection established, switch to 5s keepalive

        // assign the lambda for data processing callback AFTER internal handling is set up to not already get called before that is done
        m_networkConnection->onDataReceived = [=](const juce::MemoryBlock& data) {
            ignoreUnused(data);
            return true;
        };
    }

    return NoProtocolProtocolProcessor::Start();
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool AURAProtocolProtocolProcessor::Stop()
{
    if (m_networkConnection)
    {
        m_networkConnection->onDataReceived = std::function<bool(const juce::MemoryBlock & data)>();
        m_networkConnection->onConnectionEstablished = std::function<void()>();
        m_networkConnection->onConnectionLost = std::function<void()>();

        if (m_networkConnection->isConnected())
            m_networkConnection->disconnect();
    }

    return NoProtocolProtocolProcessor::Stop();
}

void AURAProtocolProtocolProcessor::timerCallback()
{
    if (m_networkConnection->isConnected())
    {
        SendKeepaliveToAURA();
    }
    else if (m_networkConnection->connect(m_ipAddress, m_port, 50))
        startTimer(5000); // connection established, switch to 5s keepalive
}

void AURAProtocolProtocolProcessor::SetIpAddress(const juce::IPAddress& ipAddress)
{
    m_ipAddress = ipAddress;
}

void AURAProtocolProtocolProcessor::SetPort(int port)
{
    m_port = port;
}

/**
 * Setter for the listener position member
 * @param pos  The position to copy to internal member
 */
void AURAProtocolProtocolProcessor::SetListenerPosition(const juce::Vector3D<float>& pos)
{
    m_listenerPosition = pos;

    InitializeObjectValueCache();
}

/**
 * Setter for the area member
 * @param area  The area to copy to internal member
 */
void AURAProtocolProtocolProcessor::SetArea(const juce::Rectangle<float>& area)
{
    m_area = area;

    InitializeObjectValueCache();
}

/**
 * Initializes internal cache with somewhat feasible dummy values.
 */
void AURAProtocolProtocolProcessor::InitializeObjectValueCache()
{
    auto deviceName = juce::String("AURAInterface");
    GetValueCache().SetValue(
        RemoteObject(ROI_Settings_DeviceName, RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE)),
        RemoteObjectMessageData(RemoteObjectAddressing(INVALID_ADDRESS_VALUE, INVALID_ADDRESS_VALUE), ROVT_STRING, static_cast<std::uint16_t>(deviceName.length()), deviceName.getCharPointer().getAddress(), static_cast<std::uint32_t>(deviceName.length())));


    // all other relevant values can be initialized through ProjectData
    ProjectData pd;

    // all input relevant values
    auto& ind = pd._inputNameData;
    for (int in = 1; in <= sc_chCnt; in++)
        ind[in] = juce::String("Input ") + juce::String(in);

    auto w = juce::String(m_area.getWidth());
    auto h = juce::String(m_area.getHeight());
    auto px = juce::String(m_listenerPosition.x);
    auto py = juce::String(m_area.getHeight() - m_listenerPosition.y);

    // all output relevant values
    auto& spd = pd._speakerPositionData;
    spd[1] = SpeakerPositionData::FromString(py + "," + px + ",0.0,0.0,90.0,0.0");
    for (auto i = 2; i <= 64; i++)
        spd[i] = SpeakerPositionData::FromString("0.0,0.0,0.0,0.0,0.0,0.0");

    // all mapping settings relevant values
    auto& cmd = pd._coordinateMappingData;
    cmd[1] = CoordinateMappingData::FromString(juce::String("AURA Area " + w + "m x " + h + "m,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "0," + w + ",0,"     // rp1
        + h + "," + w + ",0,"   // rp2
        + h + ",0,0,"  // rp3
        + "0,0,0");   // rp4
    cmd[2] = CoordinateMappingData::FromString(juce::String("2,")
        + "0,"          // flip
        + "0,0,0,"      // vp1
        + "0,0,0,"      // vp3
        + "0,0,0,"     // rp1
        + "0,0,0,"   // rp2
        + "0,0,0,"  // rp3
        + "0,0,0");   // rp4
    cmd[3] = CoordinateMappingData::FromString(juce::String("3,")
        + "0,"          // flip
        + "0,0,0,"      // vp1
        + "0,0,0,"      // vp3
        + "0,0,0,"     // rp1
        + "0,0,0,"   // rp2
        + "0,0,0,"  // rp3
        + "0,0,0");   // rp4
    cmd[4] = CoordinateMappingData::FromString(juce::String("4,")
        + "0,"          // flip
        + "0,0,0,"      // vp1
        + "0,0,0,"      // vp3
        + "0,0,0,"     // rp1
        + "0,0,0,"   // rp2
        + "0,0,0,"  // rp3
        + "0,0,0");   // rp4

    NoProtocolProtocolProcessor::InitializeObjectValueCache(pd);
}

/**
 * Reimplemented helper method to set a value to valuecache 
 * and process relative position values to be forwarded to AURA as absolute
 * @param	ro			The remote object to set the value for
 * @param	valueData	The value to set
 */
void AURAProtocolProtocolProcessor::SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData)
{
    NoProtocolProtocolProcessor::SetValue(ro, valueData);

    auto sourceId = ro._Addr._first;
    auto position = juce::Vector3D<float>(0.0f, 0.0f, 0.0f);

    switch (ro._Id)
    {
    case ROI_Positioning_SourcePosition:
    case ROI_Positioning_SourcePosition_XY:
    case ROI_Positioning_SourcePosition_X:
    case ROI_Positioning_SourcePosition_Y:
        // nix spezielles mit absoluten positionen
        break;
    case ROI_CoordinateMapping_SourcePosition:
        {
            auto xyzVal = GetValueCache().GetTripleFloatValues(RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(sourceId, 1)));
            position = juce::Vector3D<float>(std::get<0>(xyzVal), std::get<1>(xyzVal), std::get<2>(xyzVal));

            SendSourcePositionToAURA(sourceId, RelativeToAbsolutePosition(position));
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_XY:
        {
            auto xyVal = GetValueCache().GetDualFloatValues(RemoteObject(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectAddressing(sourceId, 1)));
            position.x = std::get<0>(xyVal);
            position.y = std::get<1>(xyVal);

            SendSourcePositionToAURA(sourceId, RelativeToAbsolutePosition(position));
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_X:
        {
            auto xVal = GetValueCache().GetFloatValue(RemoteObject(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectAddressing(sourceId, 1)));
            position.x = xVal;

            SendSourcePositionToAURA(sourceId, RelativeToAbsolutePosition(position));
        }
        break;
    case ROI_CoordinateMapping_SourcePosition_Y:
        {
            auto yVal = GetValueCache().GetFloatValue(RemoteObject(ROI_CoordinateMapping_SourcePosition_Y, RemoteObjectAddressing(sourceId, 1)));
            position.y = yVal;

            SendSourcePositionToAURA(sourceId, RelativeToAbsolutePosition(position));
        }
        break;
    default:
        break;
    }
}

juce::Vector3D<float> AURAProtocolProtocolProcessor::RelativeToAbsolutePosition(const juce::Vector3D<float>& relativePosition)
{
    return juce::Vector3D<float>(
        m_area.getWidth() * relativePosition.x,
        m_area.getHeight() * relativePosition.y,
        0.0f
    );
}

bool AURAProtocolProtocolProcessor::SendListenerPositionToAURA()
{
    if (!m_networkConnection || !m_networkConnection->isConnected())
        return false;
    else
    {
        auto info = (juce::String("listener pos > AURA: ") + juce::String(m_listenerPosition.x) + ";" + juce::String(m_listenerPosition.y) + ";" + juce::String(m_listenerPosition.z)).toStdString();
        DBG(info);
        std::cout << info << std::endl;

        assert(sizeof(std::uint32_t) == sizeof(std::float_t));
        std::uint32_t packetId = APT_ListenerPosition; // id 1
        std::uint32_t xInt = *(std::uint32_t*)&m_listenerPosition.x;
        std::uint32_t yInt = *(std::uint32_t*)&m_listenerPosition.y;
        std::uint32_t zInt = *(std::uint32_t*)&m_listenerPosition.z;

        auto data = std::vector<std::uint8_t>
            ({
                static_cast<std::uint8_t>(packetId >> 24),
                static_cast<std::uint8_t>(packetId >> 16),
                static_cast<std::uint8_t>(packetId >> 8),
                static_cast<std::uint8_t>(packetId),
                static_cast<std::uint8_t>(xInt >> 24),
                static_cast<std::uint8_t>(xInt >> 16),
                static_cast<std::uint8_t>(xInt >> 8),
                static_cast<std::uint8_t>(xInt),
                static_cast<std::uint8_t>(yInt >> 24),
                static_cast<std::uint8_t>(yInt >> 16),
                static_cast<std::uint8_t>(yInt >> 8),
                static_cast<std::uint8_t>(yInt),
                static_cast<std::uint8_t>(zInt >> 24),
                static_cast<std::uint8_t>(zInt >> 16),
                static_cast<std::uint8_t>(zInt >> 8),
                static_cast<std::uint8_t>(zInt),
                });

        return m_networkConnection->sendMessage(juce::MemoryBlock(static_cast<void*>(data.data()), data.size()));
    }
}

bool AURAProtocolProtocolProcessor::SendSourcePositionToAURA(std::int32_t sourceId, const juce::Vector3D<float>& sourcePosition)
{
    if (!m_networkConnection || !m_networkConnection->isConnected())
        return false;
    else
    {
        auto info = (juce::String("obj pos ") + juce::String(sourceId) + " > AURA: " + juce::String(sourcePosition.x) + "; " + juce::String(sourcePosition.y) + "; " + juce::String(sourcePosition.z)).toStdString();
        DBG(info);
        std::cout << info << std::endl;

        assert(sizeof(std::uint32_t) == sizeof(std::float_t));
        std::uint32_t packetId = APT_ObjectPosition; // id 2
        std::uint32_t xInt = *(std::uint32_t*)&sourcePosition.x;
        std::uint32_t yInt = *(std::uint32_t*)&sourcePosition.y;
        std::uint32_t zInt = *(std::uint32_t*)&sourcePosition.z;

        auto data = std::vector<std::uint8_t>
            ({
                static_cast<std::uint8_t>(packetId >> 24),
                static_cast<std::uint8_t>(packetId >> 16),
                static_cast<std::uint8_t>(packetId >> 8),
                static_cast<std::uint8_t>(packetId),
                static_cast<std::uint8_t>(sourceId >> 24),
                static_cast<std::uint8_t>(sourceId >> 16),
                static_cast<std::uint8_t>(sourceId >> 8),
                static_cast<std::uint8_t>(sourceId),
                static_cast<std::uint8_t>(xInt >> 24),
                static_cast<std::uint8_t>(xInt >> 16),
                static_cast<std::uint8_t>(xInt >> 8),
                static_cast<std::uint8_t>(xInt),
                static_cast<std::uint8_t>(yInt >> 24),
                static_cast<std::uint8_t>(yInt >> 16),
                static_cast<std::uint8_t>(yInt >> 8),
                static_cast<std::uint8_t>(yInt),
                static_cast<std::uint8_t>(zInt >> 24),
                static_cast<std::uint8_t>(zInt >> 16),
                static_cast<std::uint8_t>(zInt >> 8),
                static_cast<std::uint8_t>(zInt),
                });

        return m_networkConnection->sendMessage(juce::MemoryBlock(static_cast<void*>(data.data()), data.size()));
    }
}

bool AURAProtocolProtocolProcessor::SendKnownSourcePositionsToAURA()
{
    auto retVal = true;
    for (int sourceId = 1; sourceId <= sc_chCnt; sourceId++)
    {
        auto ro = RemoteObject(ROI_CoordinateMapping_SourcePosition, RemoteObjectAddressing(sourceId, 1));
        if (GetValueCache().Contains(ro))
        {
            auto xyzVal = GetValueCache().GetTripleFloatValues(ro);
            retVal = retVal && SendSourcePositionToAURA(sourceId, juce::Vector3D<float>(std::get<0>(xyzVal), std::get<1>(xyzVal), std::get<2>(xyzVal)));
        }
    }
    return retVal;
}

bool AURAProtocolProtocolProcessor::SendKeepaliveToAURA()
{
    if (!m_networkConnection || !m_networkConnection->isConnected())
        return false;
    else
    {
        auto info = juce::String("keepalive > AURA").toStdString();
        DBG(info);
        std::cout << info << std::endl;

        assert(sizeof(std::uint32_t) == sizeof(std::float_t));
        std::uint32_t packetId = APT_Keepalive; // id 3

        auto data = std::vector<std::uint8_t>
            ({
                static_cast<std::uint8_t>(packetId >> 24),
                static_cast<std::uint8_t>(packetId >> 16),
                static_cast<std::uint8_t>(packetId >> 8),
                static_cast<std::uint8_t>(packetId),
                });

        return m_networkConnection->sendMessage(juce::MemoryBlock(static_cast<void*>(data.data()), data.size()));
    }
}
