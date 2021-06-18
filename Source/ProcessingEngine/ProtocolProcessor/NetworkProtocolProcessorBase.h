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

#include "ProtocolProcessorBase.h"


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
	void SetIpAddress(const std::string& ipAddress);
	std::int32_t GetClientPort();
	void SetClientPort(std::int32_t clientPort);
	std::int32_t GetHostPort();
	void SetHostPort(std::int32_t hostPort);

private:
	std::string		m_ipAddress;	/**< IP Address where messages will be sent to / received from. */
	std::int32_t	m_clientPort;	/**< TCP/UDP port where messages will be received from. */
	std::int32_t	m_hostPort;		/**< TCP/UDP port where messages will be sent to. */

};
