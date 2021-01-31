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
	void SetRemoteObjectsActive(XmlElement* activeObjsXmlElement) override;

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
		ROI_CoordinateMapping_SourcePosition_X, 
		ROI_CoordinateMapping_SourcePosition_Y, 
		ROI_Positioning_SourceSpread, 
		ROI_Positioning_SourceDelayMode, 
		ROI_MatrixInput_ReverbSendGain };

	std::map<RemoteObjectIdentifier, JUCEAppBasics::MidiCommandRangeAssignment>	m_midiAssiMap;
	String																		m_midiInputIdentifier;
	String																		m_midiOutputIdentifier;

	std::map<RemoteObjectIdentifier, std::map<RemoteObjectAddressing, double>>	m_addressedObjectOutputDeafStampMap;
	const double																m_outputDeafTimeMs{ 300 };

	float m_floatValueBuffer[3] = { 0.0f, 0.0f, 0.0f };
	int m_intValueBuffer[2] = { 0, 0 };
	String m_stringValueBuffer;
};
