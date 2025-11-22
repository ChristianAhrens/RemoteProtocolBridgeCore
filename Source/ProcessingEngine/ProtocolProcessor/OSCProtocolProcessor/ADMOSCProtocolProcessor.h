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

#define v04 // ADM-OSC v0.4 should be used --> comment out to use higher spec

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
		AOT_Azimuth,			// "azim" (v0.4)
		AOT_Elevation,			// "elev" (v0.4)
		AOT_Distance,			// "dist" (v0.4)
		AOT_AzimElevDist,		// combined "aed" (v0.4)
		AOT_Width,				// "w"
		AOT_WidthDeg,			// "widthDeg"
		AOT_XPos,				// "x" (v0.4)
		AOT_YPos,				// "y" (v0.4)
		AOT_ZPos,				// "z" (v0.4)
		AOT_XYZPos,				// combined "xzy" (v0.4)
		AOT_XYPos,				// combined "xy"
		AOT_Gain,				// "gain"
		AOT_Mute				// "mute"
#ifndef v04		
		AOT_CartesianCoords,	// "cartesian"
#endif
	};

public:
	ADMOSCProtocolProcessor(const NodeId& parentNodeId, int listenerPortNumber);
	virtual ~ADMOSCProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool Stop() override;

	bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId = -1) override;

	bool ReturnEarly(const RemoteObjectIdentifier& roi, const RemoteObjectMessageData& msgData);

	static String GetADMMessageDomainString();
	static String GetADMMessageTypeString(const ADMOSCProtocolProcessor::ADMMessageType& msgType);
	static String GetADMObjectTypeString(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static ADMOSCProtocolProcessor::CoodinateSystem GetObjectTypeCoordinateSystem(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static ADMOSCProtocolProcessor::ADMMessageType GetObjectTypeMessageType(const ADMOSCProtocolProcessor::ADMObjectType& objType);
	static const juce::Range<float> GetADMObjectRange(const ADMObjectType& objType);

	ADMOSCProtocolProcessor::ADMObjectType GetADMObjectType(const String& typeString);

	virtual void oscMessageReceived(const OSCMessage &message, const String& senderIPAddress, const int& senderPort) override;

private:
	// Note: integer values like mute are using a separate cache directly
	bool WriteToObjectCache(const ChannelId& channel, const ADMObjectType& objType, float objValue, bool syncPolarAndCartesian = false);
	bool WriteToObjectCache(const ChannelId& channel, const std::vector<ADMObjectType>& objTypes, const std::vector<float>& objValues, bool syncPolarAndCartesian = false);
	float ReadFromObjectCache(const ChannelId& channel, const ADMObjectType& objType);
	bool SetExpectedCoordinateSystem(bool cartesian);
	bool SyncCachedPolarToCartesianValues(const ChannelId& channel);
	bool SyncCachedCartesianToPolarValues(const ChannelId& channel);
	bool CreateMessageDataFromObjectCache(const RemoteObjectIdentifier& id, const ChannelId& channel, RemoteObjectMessageData& addressing);
	ADMObjectType WriteMessageDataToObjectCache(const RemoteObjectIdentifier& id, const RemoteObjectMessageData& messageData);

	std::map<ChannelId, std::map<ADMObjectType, int>>	m_objectValueCacheInt;			/**< Cached object values for integer values, used for e.g. mute */
	std::map<ChannelId, std::map<ADMObjectType, float>>	m_objectValueCache;				/**< The cached object values, to be able to cross-calculate
																						 *	 between coordinate systems, even if only single-val message is received. */
	MappingAreaId	m_mappingAreaId{ MAI_Invalid };	/**< The DS100 mapping area to be used when converting incoming coords into relative messages.
													 *	 If this is MAI_Invalid, absolute messages will be generated. */
	bool			m_xAxisInverted{ false };		/**< Bool flag to indicate if the x-Axis values between adm and d&b world shall be inverted. */
	bool			m_yAxisInverted{ false };		/**< Bool flag to indicate if the y-Axis values between adm and d&b world shall be inverted. */
	bool			m_xyAxisSwapped{ false };		/**< Bool flag to indicate if the x/y-Axis values between adm and d&b world shall be swapped. */
	bool			m_dataSendindDisabled{ false };	/**< Bool flag to indicate if incoming message send requests from bridging node shall be ignored. */
	bool			m_xyMessageCombined{ false };	/**< Bool flag to indicate if sending out changed xy parameter messages shall be done as single xy or separate x and y messages. */
	bool			m_widthIgnored{ false };			/**< Bool flag to indicate if sending out source spread (width) should be ignored or not. */
	bool			m_gainIgnored{ false };			/**< Bool flag to indicate if sending out changed gain should be ignored or not. */
	bool			m_muteIgnored{ false };			/**< Bool flag to indicate if sending out changed mute should be ignored or not. */
	CoodinateSystem	m_expectedCoordinateSystem{ CoodinateSystem::CS_Invalid };

};
