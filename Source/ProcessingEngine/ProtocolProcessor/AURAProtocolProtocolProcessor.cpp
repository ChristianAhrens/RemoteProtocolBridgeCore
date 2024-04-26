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
        
        return true;
    }
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool AURAProtocolProtocolProcessor::Start()
{
    return NoProtocolProtocolProcessor::Start();
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool AURAProtocolProtocolProcessor::Stop()
{
    return NoProtocolProtocolProcessor::Stop();
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

    // all output relevant values
    auto& spd = pd._speakerPositionData;
    spd[1] = SpeakerPositionData::FromString("5.0,5.0,0.0,0.0,90.0,0.0");
    for (auto i = 2; i <= 64; i++)
        spd[i] = SpeakerPositionData::FromString("0.0,0.0,0.0,0.0,0.0,0.0");

    // all mapping settings relevant values
    auto& cmd = pd._coordinateMappingData;
    cmd[1] = CoordinateMappingData::FromString(juce::String("Area 10m x 10m,")
        + "0,"          // flip
        + "1,1,0,"      // vp1
        + "0,0,0,"      // vp3
        + "0,10,0,"     // rp1
        + "10,10,0,"   // rp2
        + "10,0,0,"  // rp3
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


