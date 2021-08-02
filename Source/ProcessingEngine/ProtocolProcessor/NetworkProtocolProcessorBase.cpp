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

#include "NetworkProtocolProcessorBase.h"

#include "../ProcessingEngineNode.h"

// **************************************************************************************
//    class NetworkProtocolProcessorBase
// **************************************************************************************

/**
 * Constructor of abstract class NetworkProtocolProcessorBase.
 */
NetworkProtocolProcessorBase::NetworkProtocolProcessorBase(const NodeId& parentNodeId)
	: ProtocolProcessorBase(parentNodeId)
{
}

/**
 * Destructor
 */
NetworkProtocolProcessorBase::~NetworkProtocolProcessorBase()
{

}

/**
 * Sets the configuration data for the protocol processor object.
 *
 * @param stateXml	The configuration data to parse and set active
 * @return True on success, false on failure
 */
bool NetworkProtocolProcessorBase::setStateXml(XmlElement* stateXml)
{
	if (!ProtocolProcessorBase::setStateXml(stateXml))
		return false;

	auto retVal = true;

	auto ipAdressXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::IPADDRESS));
	if (ipAdressXmlElement)
		SetIpAddress(ipAdressXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ADRESS)).toStdString());
	else
		retVal = false;

	auto clientPortXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::CLIENTPORT));
	if (clientPortXmlElement)
		SetClientPort(clientPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT)));
	else
		retVal = false;

	auto hostPortXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::HOSTPORT));
	if (hostPortXmlElement)
		SetHostPort(hostPortXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::PORT)));
	else
		retVal = false;

	return retVal;
}

/**
 * Getter for the internal ip address string member
 * @return	The internal ip address string member.
 */
const std::string& NetworkProtocolProcessorBase::GetIpAddress()
{
	return m_ipAddress;
}

/**
 * Setter for the internal ip address string member
 * @param	ipAddress	The value to set for internal ip address string member.
 */
void NetworkProtocolProcessorBase::SetIpAddress(const std::string& ipAddress)
{
	jassert(!ipAddress.empty());
	m_ipAddress = ipAddress;
}

/**
 * Getter for the internal client port member
 * @return	The internal client port member.
 */
std::int32_t NetworkProtocolProcessorBase::GetClientPort()
{
	return m_clientPort;
}

/**
 * Setter for the internal client port member
 * @param	ipAddress	The value to set for internal client port member.
 */
void NetworkProtocolProcessorBase::SetClientPort(std::int32_t clientPort)
{
	jassert(clientPort != INVALID_ADDRESS_VALUE);
	m_clientPort = clientPort;
}

/**
 * Getter for the internal host port member
 * @return	The internal host port member.
 */
std::int32_t NetworkProtocolProcessorBase::GetHostPort()
{
	return m_hostPort;
}
/**
 * Setter for the internal ip address string member
 * @param	ipAddress	The value to set for internal ip address string member.
 */
void NetworkProtocolProcessorBase::SetHostPort(std::int32_t hostPort)
{
	jassert(hostPort != INVALID_ADDRESS_VALUE);
	m_hostPort = hostPort;
}
