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

#include "../ObjectDataHandling_Abstract.h"
#include "../../../RemoteProtocolBridgeCommon.h"
#include "../../ProcessingEngineConfig.h"

#include <JuceHeader.h>

// Fwd. declarations
class ProcessingEngineNode;

/**
 * Class Reverse_B_to_A_only is a class for filtering received value data from RoleA protocols to not be forwarded
 */
class Reverse_B_to_A_only : public ObjectDataHandling_Abstract
{
public:
	Reverse_B_to_A_only(ProcessingEngineNode* parentNode);
	~Reverse_B_to_A_only();

	bool OnReceivedMessageFromProtocol(const ProtocolId PId, const RemoteObjectIdentifier Id, const RemoteObjectMessageData& msgData, const RemoteObjectMessageMetaInfo& msgMeta) override;

protected:

};
