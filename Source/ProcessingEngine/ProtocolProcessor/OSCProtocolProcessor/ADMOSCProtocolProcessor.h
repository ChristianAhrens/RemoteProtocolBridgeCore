/*
  ==============================================================================

    ADMOSCProtocolProcessor.h
	Created: 17 May 2021 16:50:00pm
    Author:  Christian Ahrens

  ==============================================================================
*/

#pragma once

#include "OSCProtocolProcessor.h"

#include <JuceHeader.h>

/**
 * Class ADMOSCProtocolProcessor is a derived class for special Yamaha RIVAGES OSC protocol interaction.
 */
class ADMOSCProtocolProcessor : public OSCProtocolProcessor
{
public:
	ADMOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	virtual ~ADMOSCProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	static String GetRemoteObjectDomainString();
	static String GetRemoteObjectParameterTypeString(RemoteObjectIdentifier id);

	virtual void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

private:
	void createRangeMappedFloatMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData, float mappingRangeMin, float mappingRangeMax);

	MappingAreaId	m_mappingAreaId{ MAI_Invalid };	/**< The DS100 mapping area to be used when converting incoming coords into relative messages. If this is MAI_Invalid, absolute messages will be generated. */

};
