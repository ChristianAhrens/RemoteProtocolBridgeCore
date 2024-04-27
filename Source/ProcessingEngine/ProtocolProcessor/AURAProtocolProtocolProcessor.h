/* Copyright (c) 2024, Christian Ahrens
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

#include "NoProtocolProtocolProcessor.h"

/**
 * Class AURAProtocolProtocolProcessor is a derived class for dummy protocol interaction.
 */
class AURAProtocolProtocolProcessor : public NoProtocolProtocolProcessor
{
public:
	AURAProtocolProtocolProcessor(const NodeId& parentNodeId);
	virtual ~AURAProtocolProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool Start() override;
	bool Stop() override;

	void SetListenerPosition(const juce::Vector3D<float>& pos);
	void SetAURAArea(const juce::Rectangle<float>& area);

protected:
	//==============================================================================
	void InitializeObjectValueCache() override;

private:
	//==============================================================================
	juce::Vector3D<float>	m_listenerPosition{ 5.0f, 5.0f, 0.0f };
	juce::Rectangle<float>	m_auraArea{ 10.0f, 10.0f };
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AURAProtocolProtocolProcessor)
};
