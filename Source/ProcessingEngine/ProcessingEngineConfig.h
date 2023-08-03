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

#pragma once

#include "../RemoteProtocolBridgeCommon.h"

#include <AppConfigurationBase.h>
#include <JuceHeader.h>


/**
 * Class ProcessingEngineConfig is class for managing application runtime configuration.
 * It is used to be passed to different object wihtin application, that can then access it to
 * get specific config infos through its interface methods
 */
class ProcessingEngineConfig : public JUCEAppBasics::AppConfigurationBase
{

public:
	enum TagID
	{
		NODE,
		OBJECTHANDLING,
		PROTOCOLACHCNT,
		PROTOCOLBCHCNT,
		PROTOCOLA,
		PROTOCOLB,
		IPADDRESS,
		CLIENTPORT,
		HOSTPORT,
		POLLINGINTERVAL,
		ACTIVEOBJECTS,
		MUTEDOBJECTS,
		GLOBALCONFIG,
		TRAFFICLOGGING,
		ENGINE,
		SIMCHCNT,
		SIMMAPCNT,
		REFRESHINTERVAL,
		MAPPINGAREA,
		INPUTDEVICE,
		OUTPUTDEVICE,
		DATAPRECISION,
		FAILOVERTIME,
		XINVERTED,
		YINVERTED,
		XYSWAPPED,
		DATASENDINGDISABLED,
		REACTMONIPROTOS,
		VALTOCMDASSI,
		XYMESSAGECOMBINED,
		REMAPPINGS,
		PACKETMODULE,
		MAPPINGAREARESCALE,
		ORIGINOFFSET,
		OCP1CONNECTIONMODE,
	};
	static String getTagName(TagID Id)
	{
		switch (Id)
		{
		case NODE:
			return "Node";
		case OBJECTHANDLING:
			return "ObjectHandling";
		case PROTOCOLACHCNT:
			return "ProtocolAChCnt";
		case PROTOCOLBCHCNT:
			return "ProtocolBChCnt";
		case PROTOCOLA:
			return "ProtocolA";
		case PROTOCOLB:
			return "ProtocolB";
		case IPADDRESS:
			return "IpAddress";
		case CLIENTPORT:
			return "ClientPort";
		case HOSTPORT:
			return "HostPort";
		case POLLINGINTERVAL:
			return "PollingInterval";
		case ACTIVEOBJECTS:
			return "ActiveObjects";
		case MUTEDOBJECTS:
			return "MutedObjects";
		case GLOBALCONFIG:
			return "GlobalConfig";
		case TRAFFICLOGGING:
			return "TrafficLogging";
		case ENGINE:
			return "Engine";
		case SIMCHCNT:
			return "SimulatedChCount";
		case SIMMAPCNT:
			return "SimulatedMappingsCount";
		case REFRESHINTERVAL:
			return "RefreshInterval";
		case MAPPINGAREA:
			return "MappingArea";
		case INPUTDEVICE:
			return "InputDevice";
		case OUTPUTDEVICE:
			return "OutputDevice";
		case DATAPRECISION:
			return "DataPrecision";
		case FAILOVERTIME:
			return "FailoverTime";
		case XINVERTED:
			return "xInverted";
		case YINVERTED:
			return "yInverted";
		case XYSWAPPED:
			return "xySwapped";
		case DATASENDINGDISABLED:
			return "DataSendingDisabled";
		case REACTMONIPROTOS:
			return "ReactionMonitoredProtocols";
		case VALTOCMDASSI:
			return "ValToCmdAssi";
		case XYMESSAGECOMBINED:
			return "xyMessageCombined";
		case REMAPPINGS:
			return "Remappings";
		case PACKETMODULE:
			return "PacketModule";
		case MAPPINGAREARESCALE:
			return "MappingAreaRescale";
		case ORIGINOFFSET:
			return "OriginOffset";
		case OCP1CONNECTIONMODE:
			return "Ocp1ConnectionMode";
		default:
			return "INVALID";
		}
	};

	enum AttributeID
	{
		MODE,
		COUNT,
		ID,
		TYPE,
		USESACTIVEOBJ,
		ADRESS,
		PORT,
		INTERVAL,
		ALLOWED,
		AUTOSTART,
		DEVICEIDENTIFIER,
		STATE,
		VALUE,
		MULTIVALUE,
		MINVALUE,
		MAXVALUE,
	};
	static String getAttributeName(AttributeID Id)
	{
		switch (Id)
		{
		case MODE:
			return "Mode";
		case COUNT:
			return "Count";
		case ID:
			return "Id";
		case TYPE:
			return "Type";
		case USESACTIVEOBJ:
			return "UsesActiveRemoteObjects";
		case ADRESS:
			return "Address";
		case PORT:
			return "Port";
		case INTERVAL:
			return "Interval";
		case ALLOWED:
			return "Allowed";
		case AUTOSTART:
			return "Autostart";
		case DEVICEIDENTIFIER:
			return "DeviceIdentifier";
		case STATE:
			return "State";
		case VALUE:
			return "Value";
		case MULTIVALUE:
			return "MultiValue";
		case MINVALUE:
			return "MinValue";
		case MAXVALUE:
			return "MaxValue";
		default:
			return "INVALID";
		}
	};

public:
	ProcessingEngineConfig(const File& file);
	~ProcessingEngineConfig();

	Array<NodeId>		GetNodeIds();

	static std::unique_ptr<XmlElement>	GetDefaultGlobalConfig();
	static std::unique_ptr<XmlElement>	GetDefaultNode();
	static std::unique_ptr<XmlElement>	GetDefaultProtocol(ProtocolRole role);
	bool				RemoveNodeOrProtocol(int Id);

	static bool	ReadPollingInterval(XmlElement* pollingIntervalElement, int& PollingInterval);

	static bool	ReadActiveObjects(XmlElement* activeObjectsElement, std::vector<RemoteObject>& remoteObjects);
	static bool	ReadMutedObjects(XmlElement* mutedObjectsElement, std::vector<RemoteObject>& remoteObjects);

	static bool	WriteActiveObjects(XmlElement* activeObjectsElement, const std::vector<RemoteObject>& remoteObjects);
	static bool	WriteMutedObjects(XmlElement* mutedObjectsElement, const std::vector<RemoteObject>& remoteObjects);

	static bool	ReplaceActiveObjects(XmlElement* activeObjectsElement, const std::vector<RemoteObject>& remoteObjects);
	static bool	ReplaceMutedObjects(XmlElement* mutedObjectsElement, const std::vector<RemoteObject>& remoteObjects);

	static String				ProtocolTypeToString(ProtocolType pt);
	static ProtocolType			ProtocolTypeFromString(String type);
	static String				ObjectHandlingModeToString(ObjectHandlingMode ohm);
	static ObjectHandlingMode	ObjectHandlingModeFromString(String mode);

	static String GetObjectDescription(RemoteObjectIdentifier roi);
	static String GetObjectShortDescription(RemoteObjectIdentifier roi);
	static bool IsChannelAddressingObject(RemoteObjectIdentifier objectId);
	static bool IsRecordAddressingObject(RemoteObjectIdentifier objectId);
    static bool IsKeepaliveObject(RemoteObjectIdentifier objectId);

	static juce::Range<float>& GetRemoteObjectRange(RemoteObjectIdentifier roi);

	// ============================================================
	bool isValid() override;

private:
	static bool	ReadObjects(XmlElement* objectsElement, std::vector<RemoteObject>& remoteObjects);
	static bool	WriteObjects(XmlElement* objectsElement, const std::vector<RemoteObject>& remoteObjects);
	static bool	ReplaceObjects(XmlElement* objectsElement, const std::vector<RemoteObject>& remoteObjects);

    int GetNextUniqueId();
	int ValidateUniqueId(int uniqueId);

	// ============================================================
	static std::map<RemoteObjectIdentifier, juce::Range<float>>	m_objectRanges;

};
