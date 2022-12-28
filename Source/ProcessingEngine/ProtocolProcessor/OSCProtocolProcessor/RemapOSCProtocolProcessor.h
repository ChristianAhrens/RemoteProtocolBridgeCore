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

	virtual void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

protected:
	bool IsMatchingRemapping(const juce::String& remapPattern, const juce::String& oscStringToMatch);

private:
	float NormalizeValueByRange(float value, const juce::Range<float>& normalizationRange);
	float MapNormalizedValueToRange(float normalizedValue, const juce::Range<float>& range, bool invert = false);

	bool	m_dataSendindDisabled{ false };	/**< Bool flag to indicate if incoming message send requests from bridging node shall be ignored. */

	std::map<RemoteObjectIdentifier, juce::String>	m_oscRemappings;

};
