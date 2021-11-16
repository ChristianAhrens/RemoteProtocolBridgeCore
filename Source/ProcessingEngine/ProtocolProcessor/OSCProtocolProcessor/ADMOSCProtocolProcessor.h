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
 * Class ADMOSCProtocolProcessor is a derived class for special ADM OSC protocol interaction.
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
		AOT_XYZPos,				// combined "xzy"
		AOT_CartesianCoords,	// "cartesian"
		AOT_Gain,				// "gain"
	};

public:
	ADMOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	virtual ~ADMOSCProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	static String GetADMMessageDomainString();
	static String GetADMMessageTypeString(const ADMOSCProtocolProcessor::ADMMessageType& msgType);
	static String GetADMObjectTypeString(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static ADMOSCProtocolProcessor::CoodinateSystem GetObjectTypeCoordinateSystem(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static ADMOSCProtocolProcessor::ADMMessageType GetObjectTypeMessageType(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static const juce::NormalisableRange<float> GetADMObjectRange(const ADMObjectType& objType);

	ADMOSCProtocolProcessor::ADMObjectType GetADMObjectType(const String& typeString);

	virtual void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

private:
	bool WriteToObjectCache(const ChannelId& channel, const ADMObjectType& objType, float objValue, bool syncPolarAndCartesian = false);
	bool WriteToObjectCache(const ChannelId& channel, const std::vector<ADMObjectType>& objTypes, const std::vector<float>& objValues, bool syncPolarAndCartesian = false);
	float ReadFromObjectCache(const ChannelId& channel, const ADMObjectType& objType);
	bool SyncCachedPolarToCartesianValues(const ChannelId& channel);
	bool SyncCachedCartesianToPolarValues(const ChannelId& channel);
	bool CreateMessageDataFromObjectCache(const RemoteObjectIdentifier& id, const ChannelId& channel, RemoteObjectMessageData& addressing);

	std::map<ChannelId, std::map<ADMObjectType, float>>	m_objectValueCache;				/**<	The cached object values, to be able to cross-calculate
																						 *		between coordinate systems, even if only single-val message is received. */
	MappingAreaId										m_mappingAreaId{ MAI_Invalid };	/**<	The DS100 mapping area to be used when converting
																						 *		incoming coords into relative messages.
																						 *		If this is MAI_Invalid, absolute messages will be generated. */

};
