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
 * Class Remap_A_X_Y_to_B_XY_Handling is a class for hardcoded remapping fo message data
 * of separate received x and y position data from protocol a to a combined xy data message
 * forwarded to protocol b. Combined xy data received from protocol b on the other hand is
 * split in two messages and sent out over protocol a. Other data is simply bypassed.
 */
class Remap_A_X_Y_to_B_XY_Handling : public ObjectDataHandling_Abstract
{
	// helper type to be used in hashmap for three position related floats
    struct xyzVals
	{
		float x;	//< x pos component. */
		float y;	//< y pos component. */
		float z;	//< z pos component. */
	};

public:
	Remap_A_X_Y_to_B_XY_Handling(ProcessingEngineNode* parentNode);
	~Remap_A_X_Y_to_B_XY_Handling();

	bool OnReceivedMessageFromProtocol(ProtocolId PId, RemoteObjectIdentifier Id, RemoteObjectMessageData& msgData) override;

protected:
	HashMap<int32, xyzVals> m_currentPosValue;	/**< Hash to hold current x y values for all currently used objects (identified by merge of obj. addressing to a single uint32 used as key). */

};
