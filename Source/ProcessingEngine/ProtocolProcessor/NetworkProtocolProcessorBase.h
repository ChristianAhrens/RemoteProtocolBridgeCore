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

#pragma once

#include "ProtocolProcessorBase.h"

#include <string>

/**
 * Class NetworkProtocolProcessorBase is an abstract interfacing base class for protocol interaction.
 * It provides a gerenic interface to start, stop, initialize and interact with the protocol it
 * implements in a derived object. Its parent node object provides a handler method to processed
 * received protocol message data.
 */
class NetworkProtocolProcessorBase : public ProtocolProcessorBase
{
public:
	NetworkProtocolProcessorBase(const NodeId& parentNodeId);
	virtual ~NetworkProtocolProcessorBase();

	//==============================================================================
	std::unique_ptr<XmlElement> createStateXml() override { return nullptr; };
	virtual bool setStateXml(XmlElement* stateXml) override;

protected:
	const std::string& GetIpAddress();
	virtual void SetIpAddress(const std::string& ipAddress);
	std::int32_t GetClientPort();
	void SetClientPort(std::int32_t clientPort);
	std::int32_t GetHostPort();
	virtual void SetHostPort(std::int32_t hostPort);

private:
	std::string		m_ipAddress;	/**< IP Address where messages will be sent to / received from. */
	std::int32_t	m_clientPort;	/**< TCP/UDP port where messages will be received from. */
	std::int32_t	m_hostPort;		/**< TCP/UDP port where messages will be sent to. */

};
