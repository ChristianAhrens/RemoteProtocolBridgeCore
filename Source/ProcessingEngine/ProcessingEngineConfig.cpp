/*
===============================================================================

Copyright (C) 2019 d&b audiotechnik GmbH & Co. KG. All Rights Reserved.

This file is part of RemoteProtocolBridge.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY d&b audiotechnik GmbH & Co. KG "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===============================================================================
*/

#include "ProcessingEngineConfig.h"



// **************************************************************************************
//    class ProcessingEngineConfig
// **************************************************************************************

/**
 * The map of known object value ranges.
 * This is predefined here, but not const, nor is the access getter return ref,
 * to still be able to modify and update it during runtime, should this be required.
 */
std::map<RemoteObjectIdentifier, juce::Range<float>>	ProcessingEngineConfig::m_objectRanges = {
	// Soundobject ROIs
	std::make_pair(ROI_Positioning_SourceDelayMode, juce::Range<float>(0.0f, 2.0f)),
	std::make_pair(ROI_MatrixInput_ReverbSendGain, juce::Range<float>(-120.0f, 24.0f)),
	std::make_pair(ROI_Positioning_SourceSpread, juce::Range<float>(0.0f, 1.0f)),
	std::make_pair(ROI_CoordinateMapping_SourcePosition_X, juce::Range<float>(0.0f, 1.0f)),
	std::make_pair(ROI_CoordinateMapping_SourcePosition_Y, juce::Range<float>(0.0f, 1.0f)),
	std::make_pair(ROI_CoordinateMapping_SourcePosition_XY, juce::Range<float>(0.0f, 1.0f)),
	// MatrixInput ROIs
	std::make_pair(ROI_MatrixInput_LevelMeterPreMute, juce::Range<float>(-120.0f, 24.0f)),
	std::make_pair(ROI_MatrixInput_Gain, juce::Range<float>(-120.0f, 24.0f)),
	std::make_pair(ROI_MatrixInput_Mute, juce::Range<float>(0.0f, 1.0f)),
	// MatrixOutput ROIs
	std::make_pair(ROI_MatrixOutput_LevelMeterPostMute, juce::Range<float>(-120.0f, 10.0f)),
	std::make_pair(ROI_MatrixOutput_Gain, juce::Range<float>(-120.0f, 10.0f)),
	std::make_pair(ROI_MatrixOutput_Mute, juce::Range<float>(0.0f, 1.0f)),
	// MatrixSettings ROIs
	std::make_pair(ROI_MatrixSettings_ReverbRoomId, juce::Range<float>(0.0f, 9.0f)),
	std::make_pair(ROI_MatrixSettings_ReverbPredelayFactor, juce::Range<float>(0.2f, 2.0f)),
	std::make_pair(ROI_MatrixSettings_ReverbRearLevel, juce::Range<float>(-24.0f, 24.0f))};

/**
 * Constructs an object
 * and calls the InitConfiguration method
 */
ProcessingEngineConfig::ProcessingEngineConfig(const File& file)
	: JUCEAppBasics::AppConfigurationBase(file)
{

}

/**
 * Destructor for the object
 */
ProcessingEngineConfig::~ProcessingEngineConfig()
{
}

/**
 * Overridden from AppConfigurationBase to custom validate this xml configuration
 * @return True if the validation succeeded, false if the xml tree is not present or not corrupt
 */
bool ProcessingEngineConfig::isValid()
{
	if (!JUCEAppBasics::AppConfigurationBase::isValid())
		return false;

	XmlElement* rootChild = m_xml->getFirstChildElement();
	if (rootChild == nullptr)
		return false;

	while (rootChild != nullptr)
	{

		if (rootChild->getTagName() == getTagName(TagID::NODE))
		{
			XmlElement* nodeSectionElement = rootChild;
			ValidateUniqueId(nodeSectionElement->getIntAttribute(getAttributeName(AttributeID::ID)));

			XmlElement* objHandleSectionElement = nodeSectionElement->getChildByName(getTagName(TagID::OBJECTHANDLING));
			if (!objHandleSectionElement)
				return false;

			XmlElement* protocolASectionElement = nodeSectionElement->getChildByName(getTagName(TagID::PROTOCOLA));
			if (protocolASectionElement)
			{
                ValidateUniqueId(protocolASectionElement->getIntAttribute(getAttributeName(AttributeID::ID)));
                
				XmlElement* ipAddrSectionElement = protocolASectionElement->getChildByName(getTagName(TagID::IPADDRESS));
				if (!ipAddrSectionElement)
					return false;
				XmlElement* clientPortSectionElement = protocolASectionElement->getChildByName(getTagName(TagID::CLIENTPORT));
				if (!clientPortSectionElement)
					return false;
				XmlElement* hostPortSectionElement = protocolASectionElement->getChildByName(getTagName(TagID::HOSTPORT));
				if (!hostPortSectionElement)
					return false;
			}
			else
				return false;

			XmlElement* protocolBSectionElement = nodeSectionElement->getChildByName(getTagName(TagID::PROTOCOLB));
			if (protocolBSectionElement)
			{
                ValidateUniqueId(protocolBSectionElement->getIntAttribute(getAttributeName(AttributeID::ID)));
                
				XmlElement* ipAddrSectionElement = protocolBSectionElement->getChildByName(getTagName(TagID::IPADDRESS));
				if (!ipAddrSectionElement)
					return false;
				XmlElement* clientPortSectionElement = protocolBSectionElement->getChildByName(getTagName(TagID::CLIENTPORT));
				if (!clientPortSectionElement)
					return false;
				XmlElement* hostPortSectionElement = protocolBSectionElement->getChildByName(getTagName(TagID::HOSTPORT));
				if (!hostPortSectionElement)
					return false;
				XmlElement* pollingSectionElement = protocolBSectionElement->getChildByName(getTagName(TagID::POLLINGINTERVAL));
				if (!pollingSectionElement)
					return false;
				XmlElement* activeObjSectionElement = protocolBSectionElement->getChildByName(getTagName(TagID::ACTIVEOBJECTS));
				if (!activeObjSectionElement)
					return false;
			}
			// no else 'return false' here, since RoleB protocol is not mandatory!

		}
		else if (rootChild->getTagName() == getTagName(TagID::GLOBALCONFIG))
		{
			XmlElement* globalConfigSectionElement = rootChild;

			XmlElement* trafficLoggingSectionElement = globalConfigSectionElement->getChildByName(getTagName(TagID::TRAFFICLOGGING));
			if (!trafficLoggingSectionElement)
				return false;
			XmlElement* engineSectionElement = globalConfigSectionElement->getChildByName(getTagName(TagID::ENGINE));
			if (!engineSectionElement)
				return false;
		}
		else
			return false;

		rootChild = m_xml->getNextElement();
	}

	return true;
}

/**
 * Getter for the list of node ids of current configuration
 *
 * @return	The list of node ids of current configuration
 */
Array<NodeId> ProcessingEngineConfig::GetNodeIds()
{
	auto currentConfig = getConfigState();
	if (!currentConfig)
		return Array<NodeId>{};

	Array<NodeId> NIds;
	XmlElement* nodeXmlElement = currentConfig->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE));
	while (nodeXmlElement != nullptr)
	{
		NIds.add(nodeXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID)));
		nodeXmlElement = nodeXmlElement->getNextElementWithTagName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE));
	}

	return NIds;
}

/**
 * Method to read the node configuration part regarding active objects per protocol
 *
 * @param activeObjectsElement	The xml element for the nodes' protocols' active objects in the DOM
 * @param remoteObjects			The remote objects list to fill according config contents
 * @return	True if remote objects were inserted, false if empty list is returned
 */
bool ProcessingEngineConfig::ReadActiveObjects(XmlElement* activeObjectsElement, std::vector<RemoteObject>& remoteObjects)
{
	if (!activeObjectsElement || activeObjectsElement->getTagName() != getTagName(TagID::ACTIVEOBJECTS))
		return false;

	if (IsDeprecatedObjectsFormat(activeObjectsElement))
		return ReadObjectsDeprecated(activeObjectsElement, remoteObjects);
	else
		return ReadObjects(activeObjectsElement, remoteObjects);
}

/**
 * Method to read the node configuration part regarding muted objects per protocol
 *
 * @param mutedObjectsElement	The xml element for the nodes' protocols' muted objects in the DOM
 * @param remoteObjects			The remote object list to fill according config contents
 * @return	True if remote objects were inserted, false if empty list is returned
 */
bool ProcessingEngineConfig::ReadMutedObjects(XmlElement* mutedObjectsElement, std::vector<RemoteObject>& remoteObjects)
{
	if (!mutedObjectsElement || mutedObjectsElement->getTagName() != getTagName(TagID::MUTEDOBJECTS))
		return false;

	if (IsDeprecatedObjectsFormat(mutedObjectsElement))
		return ReadObjectsDeprecated(mutedObjectsElement, remoteObjects);
	else
		return ReadObjects(mutedObjectsElement, remoteObjects);
}

/**
 * Deprecated method to read the node configuration part regarding muted/activated/... objects per protocol
 * This method does read a listing of channels and records per object as was used in older RPB standalone application
 *
 * @param objectsElement	The xml element for the nodes' protocols' muted objects in the DOM
 * @param remoteObjects		The remote object list to fill according config contents
 * @return	True if remote objects were inserted, false if empty list is returned
 */
bool ProcessingEngineConfig::ReadObjectsDeprecated(XmlElement* objectsElement, std::vector<RemoteObject>& remoteObjects)
{
	remoteObjects.clear(); // The incoming vector may contain remoteobjects, that can be regarded as obsolete. (the member vector of protocolprocessorbase e.g. is passed in here with the expectation of being filled only with the current objects after the call)

	XmlElement* objectChild = objectsElement->getFirstChildElement();
	RemoteObject obj;
	
	while (objectChild != nullptr)
	{
		Array<int> channels;
		String chStrToSplit = objectChild->getAttributeValue(0);
		StringArray chNumbers;
		chNumbers.addTokens(chStrToSplit, ", ", "");
		for (int j = 0; j < chNumbers.size(); ++j)
		{
			int chNum = chNumbers[j].getIntValue();
			if (chNum > 0)
				channels.add(chNum);
		}
	
		Array<int> records;
		String recStrToSplit = objectChild->getAttributeValue(1);
		StringArray recNumbers;
		recNumbers.addTokens(recStrToSplit, ", ", "");
		for (int j = 0; j < recNumbers.size(); ++j)
		{
			int recNum = recNumbers[j].getIntValue();
			if (recNum > 0)
				records.add(recNum);
		}
	
		for (int i = ROI_Invalid + 1; i < ROI_BridgingMAX; ++i)
		{
			RemoteObjectIdentifier ROId = (RemoteObjectIdentifier)i;
			if (objectChild->getTagName() == GetObjectDescription(ROId).removeCharacters(" "))
			{
				obj._Id = ROId;
	
				// now that we have all channels and records, recursively iterate through both arrays
				// to add an entry to our active objects list for every resulting ch/rec combi object
				for (int j = 0; j < channels.size(); ++j)
				{
					if (records.size() > 0)
					{
						for (int k = 0; k < records.size(); ++k)
						{
							obj._Addr._first = static_cast<ChannelId>(channels[j]);
							obj._Addr._second = static_cast<RecordId>(records[k]);
							remoteObjects.push_back(obj);
						}
					}
					else
					{
						obj._Addr._first = (int16)channels[j];
						obj._Addr._second = -1;
						remoteObjects.push_back(obj);
					}
				}
			}
		}
	
		objectChild = objectChild->getNextElement();
	}

	return !remoteObjects.empty();
}

/**
 * Private static helper method to read the node configuration part regarding muted/activated/... objects per protocol
 *
 * @param objectsElement	The xml element for the nodes' protocols' muted objects in the DOM
 * @param remoteObjects			The remote object list to fill according config contents
 * @return	True if remote objects were inserted, false if empty list is returned
 */
bool ProcessingEngineConfig::ReadObjects(XmlElement* objectsElement, std::vector<RemoteObject>& remoteObjects)
{
	remoteObjects.clear(); // The incoming vector may contain remoteobjects, that can be regarded as obsolete. (the member vector of protocolprocessorbase e.g. is passed in here with the expectation of being filled only with the current objects after the call)

	// iterate over all child elements (each must refer to a known object id with its tag name)
	for (auto const* element : objectsElement->getChildIterator())
	{
		if (!element)
			continue;

		// identify the referred object by iterating over all known ones and comparing tag name and object description string
		for (int i = ROI_Invalid + 1; i < ROI_BridgingMAX; ++i)
		{
			auto roi = static_cast<RemoteObjectIdentifier>(i);
			if (element->getTagName() == GetObjectDescription(roi).removeCharacters(" "))
			{
				// the child element of the element is expected to be of type text and contain the object addressings in format ch1,rec1;ch2,rec2;...
				auto objectListTextElement = element->getFirstChildElement();
				if (objectListTextElement && objectListTextElement->isTextElement())
				{
					auto objectAddressingList = RemoteObjectAddressing::createFromListString(objectListTextElement->getText());
					for (auto const& objectAddressing : objectAddressingList)
					{
						auto object = RemoteObject(roi, objectAddressing);
						if (std::find(remoteObjects.begin(), remoteObjects.end(), object) == remoteObjects.end())
							remoteObjects.push_back(object);
					}
				}
			}
		}
	}

	return !remoteObjects.empty();
}

/**
 * Method to read the node configuration part regarding polling interval per protocol.
 * Includes fixup to default if not found in xml.
 *
 * @param PollingIntervalElement	The xml element for the nodes' protocols' polling interval in the DOM
 * @param PollingInterval			The polling interval var to fill according config contents
 * @return	True if value was read from xml, false if default was used.
 */
bool ProcessingEngineConfig::ReadPollingInterval(XmlElement* PollingIntervalElement, int& PollingInterval)
{
	XmlElement* objectChild = PollingIntervalElement;
	PollingInterval = ET_DefaultPollingRate;

	if (objectChild != nullptr && objectChild->getAttributeName(0) == getTagName(TagID::POLLINGINTERVAL))
	{
		PollingInterval = objectChild->getAttributeValue(0).getIntValue();
		return true;
	}
	else
		return false;
}

/**
 * Method to write the node configuration part regarding active objects per protocol
 *
 * @param activeObjectsElement	The xml element for the nodes' protocols' active objects in the DOM
 * @param remoteObjects			The remote objects to set active in config
 * @return	True on success, false on failure
 */
bool ProcessingEngineConfig::WriteActiveObjects(XmlElement* activeObjectsElement, const std::vector<RemoteObject>& remoteObjects)
{
	if (!activeObjectsElement || activeObjectsElement->getTagName() != getTagName(TagID::ACTIVEOBJECTS))
		return false;

	return WriteObjects(activeObjectsElement, remoteObjects);
}

/**
 * Method to write the node configuration part regarding muted channels per protocol
 *
 * @param mutedObjectsElement	The xml element for the nodes' protocols' muted objects
 * @param remoteObjects			The remote objects to set active in config
 * @return	True on success, false on failure
 */
bool ProcessingEngineConfig::WriteMutedObjects(XmlElement* mutedObjectsElement, const std::vector<RemoteObject>& remoteObjects)
{
	if (!mutedObjectsElement || mutedObjectsElement->getTagName() != getTagName(TagID::MUTEDOBJECTS))
		return false;

	return WriteObjects(mutedObjectsElement, remoteObjects);
}

/**
 * Method to write the node configuration part regarding muted objects per protocol
 *
 * @param objectsElement	The xml element to write the given objects to
 * @param remoteObjects		The remote objects to write
 * @return	True on success, false on failure
 */
bool ProcessingEngineConfig::WriteObjects(XmlElement* objectsElement, const std::vector<RemoteObject>& remoteObjects)
{
	if (!objectsElement)
		return false;

	auto addressingsByROI = std::map<RemoteObjectIdentifier, std::vector<RemoteObjectAddressing>>();
	for (auto const& object : remoteObjects)
	{
		auto& addrMapElm = addressingsByROI[object._Id];
		if (std::find(addrMapElm.begin(), addrMapElm.end(), object._Addr) == addrMapElm.end())
			addrMapElm.push_back(object._Addr);
	}

	for (int i = ROI_Invalid + 1; i < ROI_BridgingMAX; ++i)
	{
		auto roi = static_cast<RemoteObjectIdentifier>(i);

			auto childElementName = GetObjectDescription(roi).removeCharacters(" ");
			auto objectElement = objectsElement->getChildByName(childElementName);
			if (!objectElement)
			{
				objectElement = objectsElement->createNewChildElement(childElementName);
				if (!objectElement)
					return false;

				if (0 != addressingsByROI.count(roi))
				{
					auto objectTextElement = objectElement->createTextElement(RemoteObjectAddressing::toString(addressingsByROI.at(roi)));
					if (!objectTextElement)
						return false;
				}
			}
			else
			{
				auto firstChild = objectElement->getFirstChildElement();
				if (1 == objectsElement->getNumChildElements() && firstChild && firstChild->isTextElement())
				{
					if (0 != addressingsByROI.count(roi))
						firstChild->setText(RemoteObjectAddressing::toString(addressingsByROI.at(roi)));
				}
				else
				{
					objectElement->deleteAllChildElements();
					objectElement->removeAllAttributes();
					if (0 != addressingsByROI.count(roi))
						objectElement->addTextElement(RemoteObjectAddressing::toString(addressingsByROI.at(roi)));
				}
			}
	}

	return true;
}

/**
 * Static helper method to test a given xml element regarding being of the old/deprecated channel list + record list format.
 * @param	objectsElement		The xml element to test 
 * @return	True in case the xml element contains deprecated format, false if new or invalid input.
 */
bool ProcessingEngineConfig::IsDeprecatedObjectsFormat(XmlElement* objectsElement)
{
	if (!objectsElement)
		return false;

	auto xmlChildToTest = objectsElement->getFirstChildElement();
	if (xmlChildToTest && (xmlChildToTest->hasAttribute("channels") || xmlChildToTest->hasAttribute("records")))
		return true;
	else
		return false;
}

/**
 * Method to generate next available unique id.
 * There is no cleanup / recycling of old ids available yet,
 * we only perform a ++ on a static counter
 */
int ProcessingEngineConfig::GetNextUniqueId()
{
    return ++s_uniqueIdCounter;
}

/**
 * Method to validate new external unique id to not be in conflict with internal id counter.
 * Internal counter is simply increased to not be in conflict with given new id.
 */
int ProcessingEngineConfig::ValidateUniqueId(int uniqueId)
{
	if (s_uniqueIdCounter < uniqueId)
		s_uniqueIdCounter = uniqueId;

	return uniqueId;
}

/**
 * Returns a default global config xml section
 */
std::unique_ptr<XmlElement>	ProcessingEngineConfig::GetDefaultGlobalConfig()
{
	auto globalConfigXmlElement = std::make_unique<XmlElement>(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::GLOBALCONFIG));

	auto trafficLoggingXmlElement = globalConfigXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::TRAFFICLOGGING));
	if (trafficLoggingXmlElement)
		trafficLoggingXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ALLOWED), 1);

	auto engineXmlElement = globalConfigXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::ENGINE));
	if (engineXmlElement)
		engineXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::AUTOSTART), 0);

    return globalConfigXmlElement;
}

/**
 * Adds a bridging node with default values to configuration object
 */
std::unique_ptr<XmlElement>	ProcessingEngineConfig::GetDefaultNode()
{
	auto nodeXmlElement = std::make_unique<XmlElement>(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::NODE));

	auto thisConfig = dynamic_cast<ProcessingEngineConfig*>(ProcessingEngineConfig::getInstance());
	if (thisConfig)
		nodeXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID), thisConfig->GetNextUniqueId());
	
	auto objectHandlingXmlElement = nodeXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::OBJECTHANDLING));
	if (objectHandlingXmlElement)
	{
		objectHandlingXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MODE), ProcessingEngineConfig::ObjectHandlingModeToString(OHM_Bypass));

		auto aChCntXmlElement = objectHandlingXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLACHCNT));
		if (aChCntXmlElement)
			aChCntXmlElement->addTextElement(String(0));

		auto bChCntXmlElement = objectHandlingXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLBCHCNT));
		if (bChCntXmlElement)
			bChCntXmlElement->addTextElement(String(0));

		auto precisionXmlElement = objectHandlingXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::DATAPRECISION));
		if (precisionXmlElement)
			precisionXmlElement->addTextElement(String(0.001f));
	}

	nodeXmlElement->addChildElement(GetDefaultProtocol(ProtocolRole::PR_A).release());
	nodeXmlElement->addChildElement(GetDefaultProtocol(ProtocolRole::PR_B).release());

    return nodeXmlElement;
}

/**
 * Creates a bridging node protocol xml element of specified role with default values
 *
 * @param role	The protocol role to use
 * @return The created xml element or nullptr
 */
std::unique_ptr<XmlElement> ProcessingEngineConfig::GetDefaultProtocol(ProtocolRole role)
{
	auto protocolXmlElement = std::make_unique<XmlElement>((role == ProtocolRole::PR_A) ? ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLA) : ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::PROTOCOLB));

	auto thisConfig = dynamic_cast<ProcessingEngineConfig*>(ProcessingEngineConfig::getInstance());
	if (thisConfig)
		protocolXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID), thisConfig->GetNextUniqueId());

	protocolXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::TYPE), ProcessingEngineConfig::ProtocolTypeToString(PT_OSCProtocol));
	protocolXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::USESACTIVEOBJ), 1);

	auto clientPortXmlElement = protocolXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::CLIENTPORT));
	if (clientPortXmlElement)
		clientPortXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT), 50010);

	auto hostPortXmlElement = protocolXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::HOSTPORT));
	if (hostPortXmlElement)
		hostPortXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT), 50011);

	// Active objects preparation
	std::vector<RemoteObject> activeObjects;
	RemoteObject objectX, objectY;

	objectX._Id = ROI_CoordinateMapping_SourcePosition_X;
	objectY._Id = ROI_CoordinateMapping_SourcePosition_Y;
	for (int16 i = 1; i <= 16; ++i)
	{
		RemoteObjectAddressing addr;
		addr._first = i; //channel = source
		addr._second = 1; //record = mapping

		objectX._Addr = addr;
		objectY._Addr = addr;

		activeObjects.push_back(objectX);
		activeObjects.push_back(objectY);
	}
	auto activeObjsXmlElement = protocolXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::ACTIVEOBJECTS));
	if (activeObjsXmlElement)
		ProcessingEngineConfig::WriteActiveObjects(activeObjsXmlElement, activeObjects);

	auto ipAdressXmlElement = protocolXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::IPADDRESS));
	if (ipAdressXmlElement)
		ipAdressXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ADRESS), "10.255.0.100");

	auto pollIntervalXmlElement = protocolXmlElement->createNewChildElement(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::POLLINGINTERVAL));
	if (pollIntervalXmlElement)
		pollIntervalXmlElement->setAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::INTERVAL), ET_DefaultPollingRate);

    return protocolXmlElement;
}

/**
 * Removes the bridging node with given NodeId from configuration object
 *
 * @param NId	The id of the node to remove
 */
bool ProcessingEngineConfig::RemoveNodeOrProtocol(int Id)
{
	auto nodeXmlElement = m_xml->getChildByAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID), String(Id));
	if (nodeXmlElement)
	{
		m_xml->removeChildElement(nodeXmlElement, true);
		triggerWatcherUpdate();
		return true;
	}
	else
		return false;
}

/**
* Helper to resolve ROI to human readable string.
*
* @param roi	The remote object id to be resolved to a string.
*/
String ProcessingEngineConfig::GetObjectDescription(const RemoteObjectIdentifier roi)
{
	switch (roi)
	{
	case ROI_HeartbeatPing:
		return "PING";
	case ROI_HeartbeatPong:
		return "PONG";
	case ROI_Settings_DeviceName:
		return "Device Name";
	case ROI_Error_GnrlErr:
		return "General Error";
	case ROI_Error_ErrorText:
		return "Error Text";
	case ROI_Status_StatusText:
		return "Status Text";
	case ROI_MatrixInput_Select:
		return "Matrix Input Select";
	case ROI_MatrixInput_Mute:
		return "Matrix Input Mute";
	case ROI_MatrixInput_Gain:
		return "Matrix Input Gain";
	case ROI_MatrixInput_Delay:
		return "Matrix Input Delay";
	case ROI_MatrixInput_DelayEnable:
		return "Matrix Input DelayEnable";
	case ROI_MatrixInput_EqEnable:
		return "Matrix Input EqEnable";
	case ROI_MatrixInput_Polarity:
		return "Matrix Input Polarity";
	case ROI_MatrixInput_ChannelName:
		return "Matrix Input ChannelName";
	case ROI_MatrixInput_LevelMeterPreMute:
		return "Matrix Input LevelMeterPreMute";
	case ROI_MatrixInput_LevelMeterPostMute:
		return "Matrix Input LevelMeterPostMute";
	case ROI_MatrixNode_Enable:
		return "Matrix Node Enable";
	case ROI_MatrixNode_Gain:
		return "Matrix Node Gain";
	case ROI_MatrixNode_DelayEnable:
		return "Matrix Node DelayEnable";
	case ROI_MatrixNode_Delay:
		return "Matrix Node Delay";
	case ROI_MatrixOutput_Mute:
		return "Matrix Output Mute";
	case ROI_MatrixOutput_Gain:
		return "Matrix Output Gain";
	case ROI_MatrixOutput_Delay:
		return "Matrix Output Delay";
	case ROI_MatrixOutput_DelayEnable:
		return "Matrix Output DelayEnable";
	case ROI_MatrixOutput_EqEnable:
		return "Matrix Output EqEnable";
	case ROI_MatrixOutput_Polarity:
		return "Matrix Output Polarity";
	case ROI_MatrixOutput_ChannelName:
		return "Matrix Output ChannelName";
	case ROI_MatrixOutput_LevelMeterPreMute:
		return "Matrix Output LevelMeterPreMute";
	case ROI_MatrixOutput_LevelMeterPostMute:
		return "Matrix Output LevelMeterPostMute";
	case ROI_Positioning_SourceSpread:
		return "Sound Object Spread";
	case ROI_Positioning_SourceDelayMode:
		return "Sound Object Delay Mode";
	case ROI_Positioning_SourcePosition:
		return "Absolute Sound Object Position XYZ";
	case ROI_Positioning_SourcePosition_XY:
		return "Absolute Sound Object Position XY";
	case ROI_Positioning_SourcePosition_X:
		return "Absolute Sound Object Position X";
	case ROI_Positioning_SourcePosition_Y:
		return "Absolute Sound Object Position Y";
	case ROI_CoordinateMapping_SourcePosition:
		return "Mapped Sound Object Position XYZ";
	case ROI_CoordinateMapping_SourcePosition_XY:
		return "Mapped Sound Object Position XY";
	case ROI_CoordinateMapping_SourcePosition_X:
		return "Mapped Sound Object Position X";
	case ROI_CoordinateMapping_SourcePosition_Y:
		return "Mapped Sound Object Position Y";
	case ROI_MatrixSettings_ReverbRoomId:
		return "Matrix Settings ReverbRoomId";
	case ROI_MatrixSettings_ReverbPredelayFactor:
		return "Matrix Settings ReverbPredelayFactor";
	case ROI_MatrixSettings_ReverbRearLevel:
		return "Matrix Settings ReverbRearLevel";
	case ROI_MatrixInput_ReverbSendGain:
		return "Matrix Input ReverbSendGain";
	case ROI_ReverbInput_Gain:
		return "Reverb Input Gain";
	case ROI_ReverbInputProcessing_Mute:
		return "Reverb Input Processing Mute";
	case ROI_ReverbInputProcessing_Gain:
		return "Reverb Input Processing Gain";
	case ROI_ReverbInputProcessing_LevelMeter:
		return "Reverb Input Processing LevelMeter";
	case ROI_ReverbInputProcessing_EqEnable:
		return "Reverb Input Processing EqEnable";
	case ROI_Device_Clear:
		return "Device Clear";
	case ROI_Scene_Previous:
		return "Scene Previous";
	case ROI_Scene_Next:
		return "Scene Next";
	case ROI_Scene_Recall:
		return "Scene Recall";
	case ROI_Scene_SceneIndex:
		return "Scene SceneIndex";
	case ROI_Scene_SceneName:
		return "Scene SceneName";
	case ROI_Scene_SceneComment:
		return "Scene SceneComment";
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
		return "RPB Sound Object Select";
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
		return "RPB UI Element Select";
	case ROI_RemoteProtocolBridge_GetAllKnownValues:
		return "RPB get all known values";
	case ROI_RemoteProtocolBridge_SoundObjectGroupSelect:
		return "RPB SO Selection Select";
	case ROI_RemoteProtocolBridge_MatrixInputGroupSelect:
		return "RPB MI Selection Select";
	case ROI_RemoteProtocolBridge_MatrixOutputGroupSelect:
		return "RPB MO Selection Select";
	case ROI_CoordinateMappingSettings_P1real:
		return "Mapping Area P1 real";
	case ROI_CoordinateMappingSettings_P2real:
		return "Mapping Area P2 real";
	case ROI_CoordinateMappingSettings_P3real:
		return "Mapping Area P3 real";
	case ROI_CoordinateMappingSettings_P4real:
		return "Mapping Area P4 real";
	case ROI_CoordinateMappingSettings_P1virtual:
		return "Mapping Area P1 virt";
	case ROI_CoordinateMappingSettings_P3virtual:
		return "Mapping Area P3 virt";
	case ROI_CoordinateMappingSettings_Flip:
		return "Mapping Area flip";
	case ROI_CoordinateMappingSettings_Name:
		return "Mapping Area name";
	case ROI_Positioning_SpeakerPosition:
		return "Speaker Position";
	case ROI_Invalid:
		return "INVALID";
	default:
		jassertfalse;
		return "";
	}
}

/**
* Helper to resolve ROI to short human readable string.
*
* @param roi	The remote object id to be resolved to a string.
*/
String ProcessingEngineConfig::GetObjectShortDescription(const RemoteObjectIdentifier roi)
{
	switch (roi)
	{
	case ROI_HeartbeatPing:
		return "PING";
	case ROI_HeartbeatPong:
		return "PONG";
	case ROI_Settings_DeviceName:
		return "Dev. Name";
	case ROI_Error_GnrlErr:
		return "Gnrl Err";
	case ROI_Error_ErrorText:
		return "Err Txt";
	case ROI_Status_StatusText:
		return "Stat Txt";
	case ROI_MatrixInput_Select:
		return "Mtrx In Sel";
	case ROI_MatrixInput_Mute:
		return "Mtrx In Mute";
	case ROI_MatrixInput_Gain:
		return "Mtrx In Gain";
	case ROI_MatrixInput_Delay:
		return "Mtrx In Dly";
	case ROI_MatrixInput_DelayEnable:
		return "Mtrx In DlyEnable";
	case ROI_MatrixInput_EqEnable:
		return "Mtrx In EqEnable";
	case ROI_MatrixInput_Polarity:
		return "Mtrx In Pol";
	case ROI_MatrixInput_ChannelName:
		return "Mtrx In ChName";
	case ROI_MatrixInput_LevelMeterPreMute:
		return "Mtrx In LvlPreMute";
	case ROI_MatrixInput_LevelMeterPostMute:
		return "Mtrx In LvlPostMute";
	case ROI_MatrixNode_Enable:
		return "Mtrx Nd Enable";
	case ROI_MatrixNode_Gain:
		return "Mtrx Nd Gain";
	case ROI_MatrixNode_DelayEnable:
		return "Mtrx Nd DlyEnable";
	case ROI_MatrixNode_Delay:
		return "Mtrx Nd Dly";
	case ROI_MatrixOutput_Mute:
		return "Mtrx Out Mute";
	case ROI_MatrixOutput_Gain:
		return "Mtrx Out Gain";
	case ROI_MatrixOutput_Delay:
		return "Mtrx Out Dly";
	case ROI_MatrixOutput_DelayEnable:
		return "Mtrx Out DlyEnable";
	case ROI_MatrixOutput_EqEnable:
		return "Mtrx Out EqEnable";
	case ROI_MatrixOutput_Polarity:
		return "Mtrx Out Pol";
	case ROI_MatrixOutput_ChannelName:
		return "Mtrx Out ChName";
	case ROI_MatrixOutput_LevelMeterPreMute:
		return "Mtrx Out LvlPreMute";
	case ROI_MatrixOutput_LevelMeterPostMute:
		return "Mtrx Out LvlPostMute";
	case ROI_Positioning_SourceSpread:
		return "Sound Object Spread";
	case ROI_Positioning_SourceDelayMode:
		return "Obj Dly Mode";
	case ROI_Positioning_SourcePosition:
		return "Abs. Obj. Pos. XYZ";
	case ROI_Positioning_SourcePosition_XY:
		return "Abs. Obj. Pos. XY";
	case ROI_Positioning_SourcePosition_X:
		return "Abs. Obj. Pos. X";
	case ROI_Positioning_SourcePosition_Y:
		return "Abs. Obj. Pos. Y";
	case ROI_CoordinateMapping_SourcePosition:
		return "Rel. Obj. Pos. XYZ";
	case ROI_CoordinateMapping_SourcePosition_XY:
		return "Rel. Obj. Pos. XY";
	case ROI_CoordinateMapping_SourcePosition_X:
		return "Rel. Obj. Pos. X";
	case ROI_CoordinateMapping_SourcePosition_Y:
		return "Rel. Obj. Pos. Y";
	case ROI_MatrixSettings_ReverbRoomId:
		return "Mtrx Set. RevRoomId";
	case ROI_MatrixSettings_ReverbPredelayFactor:
		return "Mtrx Set. RevPredelay";
	case ROI_MatrixSettings_ReverbRearLevel:
		return "Mtrx Set. RevRearLevel";
	case ROI_MatrixInput_ReverbSendGain:
		return "Mtrx In RevSndGain";
	case ROI_ReverbInput_Gain:
		return "Rev. In Gain";
	case ROI_ReverbInputProcessing_Mute:
		return "Rev. In Proc. Mute";
	case ROI_ReverbInputProcessing_Gain:
		return "Rev. In Proc. Gain";
	case ROI_ReverbInputProcessing_LevelMeter:
		return "Rev. In Proc. Lvl";
	case ROI_ReverbInputProcessing_EqEnable:
		return "Rev In Proc. EqEnable";
	case ROI_Device_Clear:
		return "Dev. Clr";
	case ROI_Scene_Previous:
		return "Scn Prev.";
	case ROI_Scene_Next:
		return "Scn Next";
	case ROI_Scene_Recall:
		return "Scn Recall";
	case ROI_Scene_SceneIndex:
		return "Scn Idx";
	case ROI_Scene_SceneName:
		return "Scn Name";
	case ROI_Scene_SceneComment:
		return "Scn Comment";
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
		return "RPB Obj. Sel.";
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
		return "RPB UI Elm. Sel.";
	case ROI_RemoteProtocolBridge_GetAllKnownValues:
		return "RPB get vals";
	case ROI_RemoteProtocolBridge_SoundObjectGroupSelect:
		return "RPB SO Sel. Sel.";
	case ROI_RemoteProtocolBridge_MatrixInputGroupSelect:
		return "RPB MI Sel. Sel.";
	case ROI_RemoteProtocolBridge_MatrixOutputGroupSelect:
		return "RPB MO Sel. Sel.";
	case ROI_CoordinateMappingSettings_P1real:
		return "Map P1 real";
	case ROI_CoordinateMappingSettings_P2real:
		return "Map P2 real";
	case ROI_CoordinateMappingSettings_P3real:
		return "Map P3 real";
	case ROI_CoordinateMappingSettings_P4real:
		return "Map P4 real";
	case ROI_CoordinateMappingSettings_P1virtual:
		return "Map P1 virt";
	case ROI_CoordinateMappingSettings_P3virtual:
		return "Map P3 virt";
	case ROI_CoordinateMappingSettings_Flip:
		return "Map flip";
	case ROI_CoordinateMappingSettings_Name:
		return "Map name";
	case ROI_Positioning_SpeakerPosition:
		return "Spkr Pos";
	case ROI_Invalid:
		return "INVLD";
	default:
		jassertfalse;
		return "";
	}
}

/**
 * Helper method to check if a given remote object relates to channel/soundsourcid info.
 * @param objectId	The remote object id to check.
 * @return True if the object relates to channels, false if not.
 */
bool ProcessingEngineConfig::IsChannelAddressingObject(RemoteObjectIdentifier objectId)
{
	switch (objectId)
	{
	case ROI_MatrixInput_Select:
	case ROI_MatrixInput_Mute:
	case ROI_MatrixInput_DelayEnable:
	case ROI_MatrixInput_EqEnable:
	case ROI_MatrixInput_Polarity:
	case ROI_MatrixNode_Enable:
	case ROI_MatrixNode_DelayEnable:
	case ROI_MatrixOutput_Mute:
	case ROI_MatrixOutput_DelayEnable:
	case ROI_MatrixOutput_EqEnable:
	case ROI_MatrixOutput_Polarity:
	case ROI_Positioning_SourceDelayMode:
	case ROI_ReverbInputProcessing_Mute:
	case ROI_ReverbInputProcessing_EqEnable:
	case ROI_MatrixInput_Gain:
	case ROI_MatrixInput_Delay:
	case ROI_MatrixInput_LevelMeterPreMute:
	case ROI_MatrixInput_LevelMeterPostMute:
	case ROI_MatrixNode_Gain:
	case ROI_MatrixNode_Delay:
	case ROI_MatrixOutput_Gain:
	case ROI_MatrixOutput_Delay:
	case ROI_MatrixOutput_LevelMeterPreMute:
	case ROI_MatrixOutput_LevelMeterPostMute:
	case ROI_Positioning_SourceSpread:
	case ROI_Positioning_SourcePosition_XY:
	case ROI_Positioning_SourcePosition_X:
	case ROI_Positioning_SourcePosition_Y:
	case ROI_Positioning_SourcePosition:
	case ROI_MatrixInput_ReverbSendGain:
	case ROI_ReverbInput_Gain:
	case ROI_ReverbInputProcessing_Gain:
	case ROI_ReverbInputProcessing_LevelMeter:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_MatrixInput_ChannelName:
	case ROI_MatrixOutput_ChannelName:
	case ROI_RemoteProtocolBridge_SoundObjectSelect:
	case ROI_RemoteProtocolBridge_SoundObjectGroupSelect:
	case ROI_RemoteProtocolBridge_MatrixInputGroupSelect:
	case ROI_RemoteProtocolBridge_MatrixOutputGroupSelect:
	case ROI_Positioning_SpeakerPosition:
	// CoordinateMappingSettings address a mappingarea (usually record)
	// but only that for the following objects, which makes the objects
	// channel addressing objects from RPBC's perspective
	case ROI_CoordinateMappingSettings_P1real:
	case ROI_CoordinateMappingSettings_P2real:
	case ROI_CoordinateMappingSettings_P3real:
	case ROI_CoordinateMappingSettings_P4real:
	case ROI_CoordinateMappingSettings_P1virtual:
	case ROI_CoordinateMappingSettings_P3virtual:
	case ROI_CoordinateMappingSettings_Flip:
	case ROI_CoordinateMappingSettings_Name:
		return true;
	case ROI_Settings_DeviceName:
	case ROI_Error_GnrlErr:
	case ROI_Error_ErrorText:
	case ROI_Status_StatusText:
	case ROI_MatrixSettings_ReverbRoomId:
	case ROI_MatrixSettings_ReverbPredelayFactor:
	case ROI_MatrixSettings_ReverbRearLevel:
	case ROI_Device_Clear:
	case ROI_Scene_Previous:
	case ROI_Scene_Next:
	case ROI_Scene_Recall:
	case ROI_Scene_SceneIndex:
	case ROI_Scene_SceneName:
	case ROI_Scene_SceneComment:
	case ROI_RemoteProtocolBridge_UIElementIndexSelect:
	case ROI_RemoteProtocolBridge_GetAllKnownValues:
	default:
		return false;
	}
}

/**
 * Helper method to check if a given remote object relates to record/mappingid info.
 * @param objectId	The remote object id to check.
 * @return True if the object relates to records, false if not.
 */
bool ProcessingEngineConfig::IsRecordAddressingObject(RemoteObjectIdentifier objectId)
{
	switch (objectId)
	{
	case ROI_MatrixNode_Enable:
	case ROI_MatrixNode_Gain:
	case ROI_MatrixNode_Delay:
	case ROI_MatrixNode_DelayEnable:
	case ROI_CoordinateMapping_SourcePosition_XY:
	case ROI_CoordinateMapping_SourcePosition_X:
	case ROI_CoordinateMapping_SourcePosition_Y:
	case ROI_CoordinateMapping_SourcePosition:
	case ROI_ReverbInput_Gain:
		return true;
	default:
		return false;
	}
}

/**
 * Helper method to check if a given remote object is involved in keepalive transmission and must not be filtered out e.g. based on value change detection.
 * @param objectId    The remote object id to check.
 * @return True if the object is involved in keepalive transmission, false if not.
 */
bool ProcessingEngineConfig::IsKeepaliveObject(RemoteObjectIdentifier objectId)
{
    switch (objectId)
    {
    case ROI_HeartbeatPing:
    case ROI_HeartbeatPong:
        return true;
    default:
        return false;
    }
}

/**
 * Helper method to get an internal defined value range for a given remote object.
 * @param	roi		The remote object id to get the value range for
 * @return	The float value range as requested. Empty range for unknown object.
 */
juce::Range<float>& ProcessingEngineConfig::GetRemoteObjectRange(const RemoteObjectIdentifier roi)
{
	return m_objectRanges[roi];
}

/**
* Convenience function to resolve enum to sth. human readable (e.g. in config file)
*/
String  ProcessingEngineConfig::ProtocolTypeToString(ProtocolType pt)
{
	switch (pt)
	{
	case PT_OCP1Protocol:
		return "OCP1";
	case PT_OSCProtocol:
		return "OSC";
	case PT_YamahaOSCProtocol:
		return "Yamaha";
	case PT_ADMOSCProtocol:
		return "ADM";
	case PT_RemapOSCProtocol:
		return "Remap";
	case PT_RTTrPMProtocol:
		return "RTTrPM";
	case PT_MidiProtocol:
		return "MIDI";
	case PT_Invalid:
		return "Invalid";
	case PT_NoProtocol:
		return "NoProtocol";
	default:
		return "";
	}
}

/**
* Convenience function to resolve string to enum
*/
ProtocolType  ProcessingEngineConfig::ProtocolTypeFromString(String type)
{
	if (type == ProtocolTypeToString(PT_OCP1Protocol))
		return PT_OCP1Protocol;
	if (type == ProtocolTypeToString(PT_OSCProtocol))
		return PT_OSCProtocol;
	if (type == ProtocolTypeToString(PT_MidiProtocol))
		return PT_MidiProtocol;
	if (type == ProtocolTypeToString(PT_RTTrPMProtocol))
		return PT_RTTrPMProtocol;
	if (type == ProtocolTypeToString(PT_YamahaOSCProtocol))
		return PT_YamahaOSCProtocol;
	if (type == ProtocolTypeToString(PT_ADMOSCProtocol))
		return PT_ADMOSCProtocol;
	if (type == ProtocolTypeToString(PT_RemapOSCProtocol))
		return PT_RemapOSCProtocol;
	if (type == ProtocolTypeToString(PT_NoProtocol))
		return PT_NoProtocol;

	return PT_Invalid;
}

/**
* Convenience function to resolve enum to sth. human readable (e.g. in config file)
*/
String ProcessingEngineConfig::ObjectHandlingModeToString(ObjectHandlingMode ohm)
{
	switch (ohm)
	{
	case OHM_Bypass:
		return "Bypass (A<->B)";
	case OHM_Remap_A_X_Y_to_B_XY:
		return "Reroute single A (x), (y) to combi B (xy)";
	case OHM_Mux_nA_to_mB:
		return "Multiplex multiple n-ch. A to m-ch. B protocols";
	case OHM_Forward_only_valueChanges:
		return "Forward value changes only";
    case OHM_Forward_A_to_B_only:
        return "Forward data only (A->B)";
    case OHM_Reverse_B_to_A_only:
        return "Reverse data only (B->A)";
	case OHM_DS100_DeviceSimulation:
		return "Simulate DS100 object poll answers";
	case OHM_Mux_nA_to_mB_withValFilter:
		return "Multiplex mult. n-ch. A to m-ch. B (fwd. val. changes only)";
	case OHM_Mirror_dualA_withValFilter:
		return "Mirror dual A and fwd. to B (val. changes only)";
	case OHM_A1active_withValFilter:
		return "A1 forwarding only (val. changes only)";
	case OHM_A2active_withValFilter:
		return "A2 forwarding only (val. changes only)";
	default:
		return "";
	}
}

/**
* Convenience function to resolve string to enum
*/
ObjectHandlingMode ProcessingEngineConfig::ObjectHandlingModeFromString(String mode)
{
	if (mode == ObjectHandlingModeToString(OHM_Bypass))
		return OHM_Bypass;
	if (mode == ObjectHandlingModeToString(OHM_Remap_A_X_Y_to_B_XY))
		return OHM_Remap_A_X_Y_to_B_XY;
	if (mode == ObjectHandlingModeToString(OHM_Mux_nA_to_mB))
		return OHM_Mux_nA_to_mB;
	if (mode == ObjectHandlingModeToString(OHM_Forward_only_valueChanges))
		return OHM_Forward_only_valueChanges;
    if (mode == ObjectHandlingModeToString(OHM_Forward_A_to_B_only))
        return OHM_Forward_A_to_B_only;
    if (mode == ObjectHandlingModeToString(OHM_Reverse_B_to_A_only))
        return OHM_Reverse_B_to_A_only;
	if (mode == ObjectHandlingModeToString(OHM_DS100_DeviceSimulation))
		return OHM_DS100_DeviceSimulation;
	if (mode == ObjectHandlingModeToString(OHM_Mux_nA_to_mB_withValFilter))
		return OHM_Mux_nA_to_mB_withValFilter;
	if (mode == ObjectHandlingModeToString(OHM_Mirror_dualA_withValFilter))
		return OHM_Mirror_dualA_withValFilter;
	if (mode == ObjectHandlingModeToString(OHM_A1active_withValFilter))
		return OHM_A1active_withValFilter;
	if (mode == ObjectHandlingModeToString(OHM_A2active_withValFilter))
		return OHM_A2active_withValFilter;

	return OHM_Invalid;
}
