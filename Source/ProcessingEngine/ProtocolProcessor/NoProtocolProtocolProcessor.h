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
	enum AnimationMode
	{
		AM_Off = 0,
		AM_Circle,
		AM_Rand
	};

public:
	NoProtocolProtocolProcessor(const NodeId& parentNodeId, bool cacheInit = false);
	virtual ~NoProtocolProtocolProcessor() override;

	bool setStateXml(XmlElement* stateXml) override;

	bool Start() override;
	bool Stop() override;

	bool SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId = -1) override;

protected:
	//==============================================================================
	virtual void InitializeObjectValueCache();
	void InitializeObjectValueCache(const ProjectData& projectData);
	void TriggerSendingObjectValueCache();
	void SetValueToCache(const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const juce::Array<juce::var>& vals);
	void SetInputValuesToCache(ChannelId channel, const juce::String& inputName);
	void SetSceneIndexToCache(std::float_t sceneIndex);
	void SetSpeakerPositionToCache(ChannelId channel, float x, float y, float z, float hor, float vrt, float rot);
	void SetMappingSettingsToCache(ChannelId mapping, const juce::String& mappingName, float realp3[3], float realp2[3], float realp1[3], float realp4[3], float virtp1[3], float virtp3[3], int flip);

	//==============================================================================
	static constexpr int sc_chCnt{ 64 };

private:
	//==============================================================================
	void timerThreadCallback() override;

	int GetCallbackRate() { return m_callbackRate; };
	int m_callbackRate{ 100 };
	int GetCallbackCount() { return m_callbackCount; };
	void BumpCallbackCount() { m_callbackCount++; };
	int m_callbackCount{ 0 };
	int IsHeartBeatCallback() { return (0 == (GetCallbackCount() % 40)); };

	//==============================================================================
	void StepAnimation();
	bool IsAnimationActive() { return AM_Off != m_animationMode; };
	AnimationMode GetAnimationMode() { return m_animationMode; };
	AnimationMode m_animationMode{ AM_Off };
	bool IsAnimatedObject(const RemoteObjectIdentifier& roi);
	float CalculateValueStep(const float& lastValue, const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const int& valueIndex);
	int CalculateValueStep(const int& lastValue, const RemoteObjectIdentifier& roi, const ChannelId& channel, const RecordId& record, const int& valueIndex);
	std::map<ChannelId, float>	m_channelRandomizedFactors;
	std::map<ChannelId, float>	m_channelRandomizedScaleFactors;
	std::map<int, float>	m_valueIdRandomizedFactors;


	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoProtocolProtocolProcessor)
};
