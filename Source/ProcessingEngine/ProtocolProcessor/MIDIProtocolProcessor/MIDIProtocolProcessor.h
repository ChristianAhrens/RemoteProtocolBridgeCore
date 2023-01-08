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

#include "../../../RemoteProtocolBridgeCommon.h"
#include "../ProtocolProcessorBase.h"

#include <MidiCommandRangeAssignment.h>

#include <JuceHeader.h>


/**
 * Class MIDIProtocolProcessor is a derived class for MIDI protocol interaction.
 */
class MIDIProtocolProcessor :	public ProtocolProcessorBase,
								private MessageListener,
								private MidiInputCallback
{
public:
	MIDIProtocolProcessor(const NodeId& parentNodeId, bool useMainMessageQueue = false);
	~MIDIProtocolProcessor();

	//==============================================================================
	void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

	//==============================================================================
	void handleMessage(const Message& msg) override;

	//==============================================================================
	bool setStateXml(XmlElement* stateXml) override;

	//==============================================================================
	bool Start() override;
	bool Stop() override;

	//==============================================================================
	bool SendRemoteObjectMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData) override;

private:
	// This is used to dispach an incoming midi message to the message thread
	class CallbackMidiMessage : public Message
	{
	public:
		/**
		* Constructor with default initialization of message and source.
		* @param m	The midi message to handle.
		* @param s	The source the message was received from.
		*/
		CallbackMidiMessage(const juce::MidiMessage& m, const juce::String& s) : message(m), source(s) {}

		juce::MidiMessage message;
		juce::String source;
	};

	bool IsMidiMessageMatchingCommandAssignment(const JUCEAppBasics::MidiCommandRangeAssignment& assiCommandData, const juce::MidiMessage& midiMessage);
	int GetMidiValueFromCommand(const JUCEAppBasics::MidiCommandRangeAssignment& assiCommandData, const juce::MidiMessage& midiMessage);

	void processMidiMessage(const juce::MidiMessage& midiMessage, const String& sourceName);
	bool activateMidiInput(const String& midiInputIdentifier);
	bool activateMidiOutput(const String& midiOutputIdentifier);
	void forwardAndDeafProofMessage(RemoteObjectIdentifier id, const RemoteObjectMessageData& msgData);

	MappingAreaId								m_mappingAreaId{ MAI_Invalid };	/**< The DS100 mapping area to be used when converting incoming coords into relative messages. If this is MAI_Invalid, absolute messages will be generated. */

	bool										m_useMainMessageQueue{ false };

	std::unique_ptr<MidiInput>					m_midiInput;
	std::unique_ptr<MidiOutput>					m_midiOutput;

	int											m_currentSelectedChannel{ INVALID_ADDRESS_VALUE };

	const std::vector<RemoteObjectIdentifier>	m_supportedRemoteObjects{ 
		ROI_MatrixInput_Select,
		ROI_RemoteProtocolBridge_SoundObjectSelect,
		ROI_RemoteProtocolBridge_SoundObjectGroupSelect,
		ROI_CoordinateMapping_SourcePosition_X, 
		ROI_CoordinateMapping_SourcePosition_Y, 
		ROI_Positioning_SourceSpread, 
		ROI_Positioning_SourceDelayMode, 
		ROI_MatrixInput_ReverbSendGain, 
		ROI_MatrixInput_Gain, 
		ROI_MatrixInput_Mute, 
		ROI_MatrixOutput_Gain, 
		ROI_MatrixOutput_Mute,
		ROI_Scene_Next,
		ROI_Scene_Previous,
		ROI_Scene_Recall};

	std::map<RemoteObjectIdentifier, JUCEAppBasics::MidiCommandRangeAssignment>							m_midiAssiMap;
	std::map<RemoteObjectIdentifier, std::map<JUCEAppBasics::MidiCommandRangeAssignment, std::string>>	m_midiAssiWithValueMap;
	String																								m_midiInputIdentifier;
	String																								m_midiOutputIdentifier;

	std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, double>>							m_addressedObjectOutputDeafStampMap;
	const double																						m_outputDeafTimeMs{ 300 };

	float																								m_floatValueBuffer[3] = { 0.0f, 0.0f, 0.0f };
	int																									m_intValueBuffer[2] = { 0, 0 };
	String																								m_stringValueBuffer;
};
