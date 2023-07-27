/* Copyright (c) 2020-2023, Christian Ahrens
 *
 * This file is part of RemoteProtocolBridgeCore <https://github.com/ChristianAhrens/RemoteProtocolBridgeCore>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
	m_clientPort = 0;
	m_hostPort = 0;
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
	if (juce::IPAddress(ipAddress).toString() == juce::String(ipAddress))
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
