/*
  ==============================================================================

    RemapOSCProtocolProcessor.h
	Created: 28 Dec 2022 15:26:00pm
    Author:  Christian Ahrens

  ==============================================================================
*/

#pragma once

#include "OSCProtocolProcessor.h"

#include <JuceHeader.h>

/**
 * Class RemapOSCProtocolProcessor is a derived class for special ADM OSC protocol interaction.
 */
class RemapOSCProtocolProcessor : public OSCProtocolProcessor
{
public:
	RemapOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	virtual ~RemapOSCProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

	static void DissectRemappingPattern(const juce::String& remapPattern, juce::String& startSection, juce::String& firstSparator, juce::String& middleSection, juce::String& secondSparator, juce::String& endSection);

protected:
	bool IsMatchingRemapping(const juce::String& remapPattern, const juce::String& oscStringToMatch);
	bool ExtractAddressingFromRemapping(const juce::String& remapPattern, const juce::String& oscStringToExtractFrom, ChannelId& channelId, RecordId& recordId);

private:

	bool	m_dataSendindDisabled{ false };	/**< Bool flag to indicate if incoming message send requests from bridging node shall be ignored. */

	std::map<RemoteObjectIdentifier, juce::String>	m_oscRemappings;

};
