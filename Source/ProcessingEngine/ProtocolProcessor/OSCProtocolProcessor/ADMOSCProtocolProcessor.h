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
	enum CoodinateSystem
	{
		CS_Invalid,
		CS_Polar,
		CS_Cartesian
	};

	enum ADMMessageType
	{
		AMT_Invalid,
		AMT_Object,			// "obj/"
		AMT_ObjectConfig,	// "obj/config/"
	};

	enum ADMObjectType
	{
		AOT_Invalid,
		AOT_Azimuth,			// "azim"
		AOT_Elevation,			// "elev"
		AOT_Distance,			// "dist"
		AOT_AzimElevDist,		// combined "aed"
		AOT_Width,				// "w"
		AOT_XPos,				// "x"
		AOT_YPos,				// "y"
		AOT_ZPos,				// "z"
		AOT_XYZPos,			// combined "xzy"
		AOT_CartesianCoords,	// "cartesian"
		AOT_Gain,				// "gain"
	};

public:
	ADMOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	virtual ~ADMOSCProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	static String GetADMMessageDomainString();
	static String GetADMMessageTypeString(ADMMessageType type);
	static String GetADMObjectTypeString(ADMObjectType type);

	ADMObjectType GetADMObjectType(const String& typeString);

	virtual void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

private:
	void createRangeMappedFloatMessageData(const OSCMessage& messageInput, RemoteObjectMessageData& newMessageData, float mappingRangeMin, float mappingRangeMax, const std::vector<float>& valueFactors = std::vector<float>());

	std::map<ADMObjectType, float>	m_objectValueCache;				/**< The cached object values, to be able to cross-calculate between coordinate systems, even if only single-val message is received. */
	MappingAreaId					m_mappingAreaId{ MAI_Invalid };	/**< The DS100 mapping area to be used when converting incoming coords into relative messages. If this is MAI_Invalid, absolute messages will be generated. */

};
