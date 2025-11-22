/*
  ==============================================================================

	YmhOSCProtocolProcessor.cpp
	Created: 18 Dec 2020 07:55:00am
	Author:  Christian Ahrens

  ==============================================================================
*/

#include "YmhOSCProtocolProcessor.h"

// **************************************************************************************
// class YmhOSCProtocolProcessor
// **************************************************************************************

YmhOSCProtocolProcessor::YmhOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber)
    : OSCProtocolProcessor(parentNodeId, listenerPortNumber)
{
    m_type = ProtocolType::PT_YamahaOSCProtocol;
}

YmhOSCProtocolProcessor::~YmhOSCProtocolProcessor() { }

void YmhOSCProtocolProcessor::oscMessageReceived(const OSCMessage& message, const String& senderIPAddress, const int& senderPort)
{
    if (!IsIpAddressMatchingConfig(senderIPAddress))
        return;

    auto parsedXAddress = ParsePositionXObjectAddress(message);
    if (parsedXAddress._first != INVALID_ADDRESS_VALUE && parsedXAddress._second != INVALID_ADDRESS_VALUE)
    {
        RemoteObjectMessageData newMsgData;
        auto remObject = RemoteObject(ROI_CoordinateMapping_SourcePosition_X, RemoteObjectAddressing(parsedXAddress._first, parsedXAddress._second));

        GetValueCache().SetValue(remObject, newMsgData);
    }

    String addressString = message.getAddressPattern().toString();

    if (addressString.containsWholeWord(GetYmhRemoteObjectString(ROI_RemoteProtocolBridge_SoundObjectSelect)))
    {
        // If the entire remote object identifier is disabled, return without further processing
        if (IsRemoteObjectIdDisabled(ROI_RemoteProtocolBridge_SoundObjectSelect))
            return;

        // Parse the Channel ID
        ChannelId channelId = INVALID_ADDRESS_VALUE;
        if (m_messageListener && message.size() == 1)
        {
            // value be an int, but since some OSC appliances can only process floats,
            // we need to be prepared to optionally accept float as well
            if (message[0].isInt32())
                channelId = message[0].getInt32();
            else if (message[0].isFloat32())
                channelId = static_cast<int>(round(message[0].getFloat32()));
            else
                return;

            // Create a new message data
            RemoteObjectMessageData newMessageData;
            newMessageData._valType         = ROVT_INT;
            newMessageData._valCount        = 1;
            int selectionOn                 = 1;
            newMessageData._payload         = &selectionOn;
            newMessageData._payloadSize     = sizeof(int);
            newMessageData._addrVal._first  = channelId;
            newMessageData._addrVal._second = INVALID_ADDRESS_VALUE;

            // Selection - provide the received message to parent node
            m_messageListener->OnProtocolMessageReceived(this, ROI_RemoteProtocolBridge_SoundObjectSelect, newMessageData);

            // Deselect all other channels except the one that is currently being set
            int selectionOff        = 0;
            newMessageData._payload = &selectionOff;

            for (auto const& channel : GetActiveChannels())
            {
                newMessageData._addrVal._first = channel;
                if (channelId != channel)
                    m_messageListener->OnProtocolMessageReceived(this, ROI_RemoteProtocolBridge_SoundObjectSelect, newMessageData);
            }
        }
    }

    else
        OSCProtocolProcessor::oscMessageReceived(message, senderIPAddress, senderPort);
}

bool YmhOSCProtocolProcessor::SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData,
                                                      const int externalId)
{
    // Ensure to only send xy combined messages to Yamaha (because of performance reasons)
    if (roi == ROI_CoordinateMapping_SourcePosition_X || roi == ROI_CoordinateMapping_SourcePosition_Y || roi == ROI_CoordinateMapping_SourcePosition)
        return false;

    // Fill the cache if the provided incoming identifier matches the SourcePosition_XY object
    if (roi == ROI_CoordinateMapping_SourcePosition_XY)
    {
        auto& first     = msgData._addrVal._first;
        auto& second    = msgData._addrVal._second;
        auto  remObject = RemoteObject(ROI_CoordinateMapping_SourcePosition_XY, RemoteObjectAddressing(first, second));

        GetValueCache().SetValue(remObject, msgData);
    }
    
    // Multiselection is supported in the d&b system but not in the mixing desk interface. Therefore, multiselection commands are suppressed, as they have no effect on the console.
    static const std::unordered_set<RemoteObjectIdentifier> disabledROIs = { ROI_RemoteProtocolBridge_SoundObjectSelect, ROI_RemoteProtocolBridge_MatrixInputSelect,
                    ROI_RemoteProtocolBridge_MatrixOutputSelect, ROI_RemoteProtocolBridge_UIElementIndexSelect, ROI_RemoteProtocolBridge_SoundObjectGroupSelect,
                    ROI_RemoteProtocolBridge_MatrixInputGroupSelect, ROI_RemoteProtocolBridge_MatrixOutputGroupSelect };

    if (disabledROIs.count(roi) > 0)
        return false;

    return OSCProtocolProcessor::SendRemoteObjectMessage(roi, msgData, externalId);
}

RemoteObjectAddressing YmhOSCProtocolProcessor::ParsePositionXObjectAddress(const OSCMessage& message)
{
    String addressString = message.getAddressPattern().toString();
    if (addressString.containsWholeWord(GetRemoteObjectString(ROI_CoordinateMapping_SourcePosition_X)))
    {
        // Parse the Channel ID
        auto channelId = static_cast<ChannelId>((addressString.fromLastOccurrenceOf("/", false, true)).getIntValue());
        jassert(channelId > 0);
        if (channelId <= 0)
            return {};

        // Parse the Record ID
        addressString = addressString.upToLastOccurrenceOf("/", false, true);
        auto recordId = static_cast<RecordId>((addressString.fromLastOccurrenceOf("/", false, true)).getIntValue());
        jassert(recordId > 0);
        if (recordId <= 0)
            return {};

        return RemoteObjectAddressing(channelId, recordId);
    }

    else
        return {};
}

juce::String YmhOSCProtocolProcessor::GetYmhRemoteObjectString(const RemoteObjectIdentifier roi)
{
    switch (roi)
    {
    case ROI_RemoteProtocolBridge_SoundObjectSelect:
        return "/dbaudio1/appctrl/obj/select";

    default:
        return OSCProtocolProcessor::GetRemoteObjectString(roi);
        break;
    }
}

std::vector<ChannelId> YmhOSCProtocolProcessor::GetActiveChannels()
{
    std::vector<ChannelId> uniqueActiveChannels;
    std::unordered_set<ChannelId> encounteredChannels; // Track already seen channels to avoid duplicates

    for (const auto& cachedValue : GetValueCache().GetCachedValues())
    {
        if (cachedValue.first._Id == ROI_CoordinateMapping_SourcePosition_XY)
        {
            const auto& currentChannel = cachedValue.first._Addr._first;

            // Insert channel into the set, check if it's already been added
            if (encounteredChannels.insert(currentChannel).second)
                uniqueActiveChannels.push_back(currentChannel);
        }
    }

    return uniqueActiveChannels;
}
