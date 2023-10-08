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

#include "../../RemoteProtocolBridgeCommon.h"
#include "ProtocolProcessorBase.h"

#include <JuceHeader.h>

/** 
 * fwd. decls.
 */
struct ProjectData;

/**
 * Class NoProtocolProtocolProcessor is a derived class for dummy protocol interaction.
 */
class NoProtocolProtocolProcessor : public ProtocolProcessorBase
{
public:
	NoProtocolProtocolProcessor(const NodeId& parentNodeId);
	virtual ~NoProtocolProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool Start() override;
	bool Stop() override;

	bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId = -1) override;

private:
	//==============================================================================
	void timerThreadCallback() override;

	//==============================================================================
	void InitializeObjectValueCache();
	void InitializeObjectValueCache(const ProjectData& projectData);
	void TriggerSendingObjectValueCache();
	void SetSceneIndexToCache(std::float_t sceneIndex);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoProtocolProtocolProcessor)
};
