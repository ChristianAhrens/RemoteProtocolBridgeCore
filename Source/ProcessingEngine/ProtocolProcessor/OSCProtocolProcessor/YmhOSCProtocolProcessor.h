/*
  ==============================================================================

    YmhOSCProtocolProcessor.h
    Created: 18 Dec 2020 07:55:00am
    Author:  Christian Ahrens

  ==============================================================================
*/

#pragma once

#include "OSCProtocolProcessor.h"

#include <JuceHeader.h>

/**
 * Class YmhOSCProtocolProcessor is a derived class for special Yamaha OSC protocol interaction.
 */
class YmhOSCProtocolProcessor : public OSCProtocolProcessor
{
public:
    YmhOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
    virtual ~YmhOSCProtocolProcessor() override;

    void oscMessageReceived(const OSCMessage& message, const String& senderIPAddress, const int& senderPort) override;

    bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId) override;

    RemoteObjectAddressing ParsePositionXObjectAddress(const OSCMessage& message);

    static juce::String GetYmhRemoteObjectString(const RemoteObjectIdentifier roi);

    std::vector<ChannelId> GetActiveChannels();
};
