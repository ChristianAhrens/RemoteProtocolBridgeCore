/* Copyright (c) 2023, Christian Ahrens
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

#include "../../../RemoteProtocolBridgeCommon.h"
#include "../NetworkProtocolProcessorBase.h"

#include <JuceHeader.h>


/**
 * Class OCP1ProtocolProcessor is a derived class for OCA protocol interaction. 
 * This currently is only a dummy for potential future functionality.
 * Feel free to implement something yourself here.
 */
class OCP1ProtocolProcessor : public NetworkProtocolProcessorBase
{
public:
	OCP1ProtocolProcessor(const NodeId& parentNodeId);
	~OCP1ProtocolProcessor();

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	bool Start() override;
	bool Stop() override;

	//==============================================================================
	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

	//==============================================================================
	static String GetRemoteObjectString(RemoteObjectIdentifier id);

};
