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

#include "MIDIProtocolProcessor.h"

#include "../../ProcessingEngineConfig.h"


// **************************************************************************************
//    class MIDIProtocolProcessor
// **************************************************************************************
/**
 * Derived MIDI remote protocol processing class
 */
MIDIProtocolProcessor::MIDIProtocolProcessor(const NodeId& parentNodeId, bool useMainMessageQueue)
	: ProtocolProcessorBase(parentNodeId)
{
	m_type = ProtocolType::PT_MidiProtocol;

	m_useMainMessageQueue = useMainMessageQueue;
}

/**
 * Destructor
 */
MIDIProtocolProcessor::~MIDIProtocolProcessor()
{
}

/**
 * Reimplemented from MidiInputCallback to handle midi messages.
 * @param source	The data source midi input.
 * @param message	The midi input data.
 */
void MIDIProtocolProcessor::handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message)
{
	if (m_useMainMessageQueue)
	{
		// dispatch message to queue
		postMessage(std::make_unique<CallbackMidiMessage>(message, (source == nullptr ? "UNKNOWN" : source->getName())).release());
	}
	else
	{
		// directly call the midi message processing method. 
		processMidiMessage(message, (source == nullptr ? "UNKNOWN" : source->getName()));
	}
}

/**
 * Reimplemented from MessageListener to handle messages posted to queue.
 * @param msg	The incoming message to handle
 */
void MIDIProtocolProcessor::handleMessage(const Message& msg)
{
	if (auto* callbackMessage = dynamic_cast<const CallbackMidiMessage*> (&msg))
		processMidiMessage(callbackMessage->message, callbackMessage->source);
}

/**
 * Method to verify if a given midi message matches a command assignment.
 * The matching is done against the different assignment types 
 * - 1:1 command
 * - range of commands
 * - range of values
 * - range of values and commands
 * 
 * @param	assiCommandData		The comannd assignment to test the midimessage against
 * @param	midiMessgae			The midimessage to match against the command assignment
 * @return	True if the match is positive, false if no match.
 */
bool MIDIProtocolProcessor::IsMidiMessageMatchingCommandAssignment(const JUCEAppBasics::MidiCommandRangeAssignment& assiCommandData, const juce::MidiMessage& midiMessage)
{
	auto isCmdTriggerAssi = assiCommandData.isCommandTriggerAssignment();
	auto isValNCmdRangeAssi = assiCommandData.isValueRangeAssignment() && assiCommandData.isCommandRangeAssignment();
	auto isCmdRangeAssi = assiCommandData.isCommandRangeAssignment() && !isValNCmdRangeAssi;
	auto isValRangeAssi = assiCommandData.isValueRangeAssignment() && !isValNCmdRangeAssi;

	auto matchesCommand = assiCommandData.isMatchingCommand(midiMessage);
	auto matchesCommandRange = assiCommandData.isMatchingCommandRange(midiMessage);
	auto matchesValueRange = assiCommandData.isMatchingValueRange(midiMessage);

	auto matchesAssignment = false;
	if (isCmdTriggerAssi)
		matchesAssignment = matchesCommand;
	else if (isCmdRangeAssi)
		matchesAssignment = matchesCommandRange;
	else if (isValRangeAssi)
		matchesAssignment = matchesValueRange && matchesCommand;
	else if (isValNCmdRangeAssi)
		matchesAssignment = matchesCommandRange && matchesValueRange;

	return matchesAssignment;
}

int MIDIProtocolProcessor::GetMidiValueFromCommand(const JUCEAppBasics::MidiCommandRangeAssignment& assiCommandData, const juce::MidiMessage& midiMessage)
{
	if ((assiCommandData.isNoteOnCommand() || assiCommandData.isNoteOffCommand()) && midiMessage.isNoteOnOrOff())
		return midiMessage.getNoteNumber();
	else if (assiCommandData.isAftertouchCommand() && midiMessage.isAftertouch())
		return midiMessage.getAfterTouchValue();
	else if (assiCommandData.isChannelPressureCommand() && midiMessage.isChannelPressure())
		return midiMessage.getChannelPressureValue();
	else if (assiCommandData.isControllerCommand() && midiMessage.isController())
		return midiMessage.getControllerValue();
	else if (assiCommandData.isPitchCommand() && midiMessage.isPitchWheel())
		return midiMessage.getPitchWheelValue();
	else if (assiCommandData.isProgramChangeCommand() && midiMessage.isProgramChange())
		return midiMessage.getProgramChangeNumber();
	else if (assiCommandData.isChannelPressureCommand() && midiMessage.isChannelPressure())
		return midiMessage.getChannelPressureValue();

	return -1;
}

/**
 * Method to do the actual processing of incoming midi data to internal remote objects.
 * @param midiMessage	The message to process.
 * @param sourceName	The string representation of the midi message source device.
 */
void MIDIProtocolProcessor::processMidiMessage(const juce::MidiMessage& midiMessage, const String& sourceName)
{
	ignoreUnused(sourceName);

	DBG(String(__FUNCTION__) + " MIDI received: " + midiMessage.getDescription());

	RemoteObjectIdentifier newObjectId = ROI_Invalid;
	RemoteObjectMessageData newMsgData;
	newMsgData._addrVal._first = INVALID_ADDRESS_VALUE;
	newMsgData._addrVal._second = INVALID_ADDRESS_VALUE;
	newMsgData._valType = ROVT_NONE;
	newMsgData._valCount = 0;
	newMsgData._payload = 0;
	newMsgData._payloadSize = 0;

	int value = -1;

	// iterate through the available assignments to find a match for the midimessage
	for (auto const& assignmentMapping : m_midiAssiMap)
	{
		auto& assiCommandData = assignmentMapping.second;
		if (IsMidiMessageMatchingCommandAssignment(assiCommandData, midiMessage))
		{
			newObjectId = assignmentMapping.first;
			newMsgData._addrVal._second = static_cast<RecordId>(ProcessingEngineConfig::IsRecordAddressingObject(newObjectId) ? m_mappingAreaId : INVALID_ADDRESS_VALUE);

			// get the command value from midi message
			auto commandValue = JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(midiMessage); 
			auto commandRangeMatch = assiCommandData.isMatchingCommandRange(midiMessage);

			// get the new value from midi message. this depends on the message command type.
			auto midiValue = GetMidiValueFromCommand(assiCommandData, midiMessage);

			// map the incoming value to the correct remote object range. this varies between the different objects.
			switch (newObjectId)
			{
			case ROI_MatrixInput_Select:
			case ROI_RemoteProtocolBridge_SoundObjectSelect:
				{
					// buffer current note number to be able to compare against incoming value
					auto previousSelectedChannel = m_currentSelectedChannel;

					// derive the current note number from message
					if (assiCommandData.isCommandRangeAssignment())
					{
						auto commandValueRange = juce::Range<int>(
							static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getStart())),
							static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getEnd())));
						m_currentSelectedChannel = commandRangeMatch ? 1 + commandValue - commandValueRange.getStart() : INVALID_ADDRESS_VALUE;
					}
					else if (assiCommandData.isValueRangeAssignment())
					{
						jassertfalse; //not supported
					}
					else
						m_currentSelectedChannel = commandValue - assiCommandData.getCommandValue();

					// prepare remote object
					newMsgData._addrVal._first = m_currentSelectedChannel;
					newMsgData._valType = ROVT_INT;
					newMsgData._valCount = 1;
					newMsgData._payloadSize = sizeof(int);

					// if current note has change, send deselect for previous number first
					if (previousSelectedChannel > INVALID_ADDRESS_VALUE)
					{
						value = 0;
						newMsgData._addrVal._first = previousSelectedChannel;
						newMsgData._payload = static_cast<void*>(&value);
						forwardAndDeafProofMessage(newObjectId, newMsgData);
					}

					// send select for newly selected channel
					if (previousSelectedChannel != m_currentSelectedChannel &&
						m_currentSelectedChannel > INVALID_ADDRESS_VALUE)
					{
						value = 1;
						newMsgData._addrVal._first = m_currentSelectedChannel;
						newMsgData._payload = static_cast<void*>(&value);
						forwardAndDeafProofMessage(newObjectId, newMsgData);
					}
					// or mark that currently none is selected (e.g. second press of a note to deselect current)
					else
					{
						m_currentSelectedChannel = -1;
					}

					return; // intentionally, since source select handling is special case (skip channel mute check and final generic object listener callback)
				}
				break;
			case ROI_RemoteProtocolBridge_SoundObjectGroupSelect:
				{
				// buffer current note number to be able to compare against incoming value
				auto previousSelectedChannel = m_currentSelectedChannel;

				// derive the current note number from message
				if (assiCommandData.isCommandRangeAssignment())
				{
					auto commandValueRange = juce::Range<int>(
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getStart())),
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getEnd())));
					m_currentSelectedChannel = commandRangeMatch ? 1 + commandValue - commandValueRange.getStart() : INVALID_ADDRESS_VALUE;
				}
				else if (assiCommandData.isValueRangeAssignment())
				{
					jassertfalse; //not supported
				}
				else
					m_currentSelectedChannel = commandValue - assiCommandData.getCommandValue();

				// prepare remote object
				newMsgData._addrVal._first = m_currentSelectedChannel;
				newMsgData._valType = ROVT_INT;
				newMsgData._valCount = 1;
				newMsgData._payloadSize = sizeof(int);

				// send select for newly selected channel
				if (previousSelectedChannel != m_currentSelectedChannel &&
					m_currentSelectedChannel > INVALID_ADDRESS_VALUE)
				{
					value = 1;
					newMsgData._addrVal._first = m_currentSelectedChannel;
					newMsgData._payload = static_cast<void*>(&value);
					forwardAndDeafProofMessage(newObjectId, newMsgData);
				}
				// or mark that currently none is selected (e.g. second press of a note to deselect current)
				else
				{
					m_currentSelectedChannel = -1;
				}

				return; // intentionally, since source select handling is special case (skip channel mute check and final generic object listener callback)
				}
				break;
			case ROI_Positioning_SourceDelayMode:
				// delaymode can have values 0, 1, 2
				{
				if (assiCommandData.isValueRangeAssignment()) // this is supposed to prevent us from running into zero-division in jmap() call
				{
					m_intValueBuffer[0] = jmap(midiValue,
						assiCommandData.getValueRange().getStart(), assiCommandData.getValueRange().getEnd(),
						static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getStart()), static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getEnd()));

					if (assiCommandData.isCommandRangeAssignment())
					{
						auto commandValueRange = juce::Range<int>(
							static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getStart())),
							static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getEnd())));
						newMsgData._addrVal._first = commandRangeMatch ? 1 + commandValue - commandValueRange.getStart() : INVALID_ADDRESS_VALUE;
					}
					else
						newMsgData._addrVal._first = m_currentSelectedChannel;
				}
				else if (assiCommandData.isCommandRangeAssignment())
				{
					auto assiCommandStartValue = JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getStart());
					m_intValueBuffer[0] = jlimit(static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getStart()), static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getEnd()),
						static_cast<int>(0 + (commandValue - assiCommandStartValue)));

					newMsgData._addrVal._first = m_currentSelectedChannel;
				}
				else
					jassertfalse; //not supported

				newMsgData._valType = ROVT_INT;
				newMsgData._valCount = 1;
				newMsgData._payload = &m_intValueBuffer;
				newMsgData._payloadSize = sizeof(int);
				}
				break;
			case ROI_MatrixInput_Mute:
			case ROI_MatrixOutput_Mute:
				// mute can have values 0, 1
				{
				if (assiCommandData.isValueRangeAssignment())
					jassertfalse; // not supported

				if (assiCommandData.isCommandRangeAssignment())
				{
					auto assiCommandStartValue = JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getStart());
					newMsgData._addrVal._first = commandRangeMatch ? 1 + commandValue - assiCommandStartValue : INVALID_ADDRESS_VALUE;
				}
				else if (assiCommandData.isCommandTriggerAssignment())
					newMsgData._addrVal._first = m_currentSelectedChannel;

				if (assiCommandData.isNoteOffCommand() || assiCommandData.isNoteOnCommand())
				{
					m_intValueBuffer[0] = (1 == GetValueCache().GetIntValue(RemoteObject(newObjectId, newMsgData._addrVal)) ? 0 : 1);
				}
				else
				{
					auto assiCommandValue = JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCommandData.getCommandRange().getStart());
					m_intValueBuffer[0] = jlimit(static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getStart()), static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getEnd()),
						static_cast<int>(0 + (commandValue - assiCommandValue)));
				}

				newMsgData._valType = ROVT_INT;
				newMsgData._valCount = 1;
				newMsgData._payload = &m_intValueBuffer;
				newMsgData._payloadSize = sizeof(int);
				}
				break;
			case ROI_MatrixInput_ReverbSendGain:
			case ROI_MatrixInput_Gain:
			case ROI_MatrixOutput_Gain:
				// gain has values -120 ... 24 o. 10
				{
				if (assiCommandData.isValueRangeAssignment()) // this is supposed to prevent us from running into zero-division in jmap() call
				{
					m_floatValueBuffer[0] = jmap(static_cast<float>(midiValue),
						static_cast<float>(assiCommandData.getValueRange().getStart()), static_cast<float>(assiCommandData.getValueRange().getEnd()),
						ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getStart(), ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getEnd());
				}

				if (assiCommandData.isCommandRangeAssignment())
				{
					auto commandValueRange = juce::Range<int>(
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getStart())),
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getEnd())));
					newMsgData._addrVal._first = commandRangeMatch ? 1 + commandValue - commandValueRange.getStart() : INVALID_ADDRESS_VALUE;
				}
				else
					newMsgData._addrVal._first = m_currentSelectedChannel;

				newMsgData._valType = ROVT_FLOAT;
				newMsgData._valCount = 1;
				newMsgData._payload = &m_floatValueBuffer;
				newMsgData._payloadSize = sizeof(float);
				}
				break;
			case ROI_Scene_Next:
			case ROI_Scene_Previous:
				// scene next/prev has no data and is just a triggering ROI
				{
					newMsgData._valType = ROVT_NONE;
					newMsgData._valCount = 0;
					newMsgData._payload = nullptr;
					newMsgData._payloadSize = 0;
				}
				break;
			case ROI_Positioning_SourceSpread:
			case ROI_CoordinateMapping_SourcePosition_X:
			case ROI_CoordinateMapping_SourcePosition_Y:
			default:
				{
				// all else is between 0 ... 1
				if (assiCommandData.isValueRangeAssignment()) // this is supposed to prevent us from running into zero-division in jmap() call
				{
					m_floatValueBuffer[0] = jmap(static_cast<float>(midiValue),
						static_cast<float>(assiCommandData.getValueRange().getStart()), static_cast<float>(assiCommandData.getValueRange().getEnd()),
						ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getStart(), ProcessingEngineConfig::GetRemoteObjectRange(newObjectId).getEnd());
				}

				if (assiCommandData.isCommandRangeAssignment())
				{
					auto commandValueRange = juce::Range<int>(
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getStart())),
						static_cast<int>(JUCEAppBasics::MidiCommandRangeAssignment::getAsValue(assiCommandData.getCommandRange().getEnd())));
					newMsgData._addrVal._first = commandRangeMatch ? 1 + commandValue - commandValueRange.getStart() : INVALID_ADDRESS_VALUE;
				}
				else
					newMsgData._addrVal._first = m_currentSelectedChannel;

				newMsgData._valType = ROVT_FLOAT;
				newMsgData._valCount = 1;
				newMsgData._payload = &m_floatValueBuffer;
				newMsgData._payloadSize = sizeof(float);
				}
				break;
			}

			// If the received message targets a muted object, return without further processing
			if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
				return;

			// Insert the new object and value to local cache
			GetValueCache().SetValue(RemoteObject(newObjectId, newMsgData._addrVal), newMsgData);

			// finally send the collected data struct to parent node for further handling
			forwardAndDeafProofMessage(newObjectId, newMsgData);

			return;
		}
	}

	// iterate through the available midi to value assignments to find a match for the midimessage
	for (auto const& valueAssignmentMapping : m_midiAssiWithValueMap)
	{
		auto& roi = valueAssignmentMapping.first;
		for (auto const& commandToValueMapping : valueAssignmentMapping.second)
		{
			auto& assiCommandData = commandToValueMapping.first;
			auto& assiValue = commandToValueMapping.second;

			auto midiCommandData = JUCEAppBasics::MidiCommandRangeAssignment(midiMessage);

			if (IsMidiMessageMatchingCommandAssignment(assiCommandData, midiMessage))
			{
				newObjectId = roi;
				newMsgData._addrVal._second = static_cast<RecordId>(ProcessingEngineConfig::IsRecordAddressingObject(newObjectId) ? m_mappingAreaId : INVALID_ADDRESS_VALUE);

				auto isMessageToBeBridged = false;

				// map the incoming value to the correct remote object range. this varies between the different objects.
				switch (newObjectId)
				{
				case ROI_Scene_Recall:
					{
						if (assiCommandData.getCommandRange().isEmpty() && assiCommandData.getValueRange().isEmpty())
						{
							if (assiCommandData.getCommandValue() == midiCommandData.getCommandValue())
							{
								auto majorMinorString = StringArray();
								majorMinorString.addTokens(String(assiValue), ".", "");
								if (majorMinorString.size() == 2)
								{
									m_intValueBuffer[0] = majorMinorString[0].getIntValue();
									m_intValueBuffer[1] = majorMinorString[1].getIntValue();

									newMsgData._valType = ROVT_INT;
									newMsgData._valCount = 2;
									newMsgData._payload = &m_intValueBuffer;
									newMsgData._payloadSize = 2 * sizeof(int);

									isMessageToBeBridged = true;
								}
							}
						}
					}
					break;
				default:
					break;
				}

				if (isMessageToBeBridged)
				{
					// If the received message targets a muted object, return without further processing
					if (IsRemoteObjectMuted(RemoteObject(newObjectId, newMsgData._addrVal)))
						return;

					// Insert the new object and value to local cache
					GetValueCache().SetValue(RemoteObject(newObjectId, newMsgData._addrVal), newMsgData);

					// finally send the collected data struct to parent node for further handling
					forwardAndDeafProofMessage(newObjectId, newMsgData);

					return;
				}
			}
		}
	}
}

/**
 * Sets the xml configuration for the protocol processor object.
 *
 * @param stateXml	The XmlElement containing configuration for this protocol processor instance
 * @return True on success, False on failure
 */
bool MIDIProtocolProcessor::setStateXml(XmlElement* stateXml)
{
	if (!ProtocolProcessorBase::setStateXml(stateXml))
		return false;
	else
	{
		// read the device input index from xml
		auto midiInputIndexXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::INPUTDEVICE));
		if (midiInputIndexXmlElement)
			m_midiInputIdentifier = midiInputIndexXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::DEVICEIDENTIFIER));
		else
			return false;

		// read the device output index from xml
		auto midiOutputIndexXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::OUTPUTDEVICE));
		if (midiOutputIndexXmlElement)
			m_midiOutputIdentifier = midiOutputIndexXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::DEVICEIDENTIFIER));
		else
			return false;

		// read the mapping area id from xml
		auto mappingAreaXmlElement = stateXml->getChildByName(ProcessingEngineConfig::getTagName(ProcessingEngineConfig::TagID::MAPPINGAREA));
		if (mappingAreaXmlElement)
			m_mappingAreaId = static_cast<MappingAreaId>(mappingAreaXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::ID)));
		else
			return false;

		// read all available midi assignment mappings from xml
		for (auto const& roi : m_supportedRemoteObjects)
		{
			auto assiMapXmlElement = stateXml->getChildByName(ProcessingEngineConfig::GetObjectTagName(roi));
			if (assiMapXmlElement)
			{
				if (assiMapXmlElement->getIntAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::MULTIVALUE)) == 1)
				{
					// read multivalue elements
					for (auto assiMapSubXmlElement : assiMapXmlElement->getChildIterator())
					{
						auto value = assiMapSubXmlElement->getStringAttribute(ProcessingEngineConfig::getAttributeName(ProcessingEngineConfig::AttributeID::VALUE));
						auto assiMapSubHexStringTextXmlElement = assiMapSubXmlElement->getFirstChildElement();
						if (assiMapSubHexStringTextXmlElement && assiMapSubHexStringTextXmlElement->isTextElement())
						{
							auto midiAssi = JUCEAppBasics::MidiCommandRangeAssignment();
							auto midiAssiStr = assiMapSubHexStringTextXmlElement->getText();
							if (midiAssiStr.isNotEmpty() && midiAssi.deserializeFromHexString(midiAssiStr))
								m_midiAssiWithValueMap[roi][midiAssi] = value.toStdString();
							else if (value.isEmpty())
								m_midiAssiWithValueMap[roi].erase(midiAssi);
							else
								m_midiAssiWithValueMap[roi].clear();
						}
					}
				}
				else
				{
					// read single value elements
					auto assiMapHexStringTextXmlElement = assiMapXmlElement->getFirstChildElement();
					if (assiMapHexStringTextXmlElement && assiMapHexStringTextXmlElement->isTextElement())
					{
						auto midiAssi = JUCEAppBasics::MidiCommandRangeAssignment();
						auto midiAssiStr = assiMapHexStringTextXmlElement->getText();
						if (midiAssiStr.isNotEmpty() && midiAssi.deserializeFromHexString(midiAssiStr))
							m_midiAssiMap[roi] = midiAssi;
						else
							m_midiAssiMap[roi] = JUCEAppBasics::MidiCommandRangeAssignment();
					}
				}
			}
		}

		return true;
	}
}

/**
 * Helper method to activate a MIDI input based on a given input index.
 * This also tries deactivating previously activate inputs.
 * @param	midiInputIdentifier		The identifier of the input device to activate.
 * @return	True if new device was activated and old deactivated or only old deactivated if requested by -1 index. False if this did not succeed.
 */
bool MIDIProtocolProcessor::activateMidiInput(const String& midiInputIdentifier)
{
	if (midiInputIdentifier.isNotEmpty())
	{
		// verfiy availability of device corresponding to new device identifier
		auto midiDevicesInfos = juce::MidiInput::getAvailableDevices();
		bool newMidiDeviceFound = false;
		for (auto const& midiDeviceInfo : midiDevicesInfos)
		{
			if (midiDeviceInfo.identifier == midiInputIdentifier)
				newMidiDeviceFound = true;
		}
		if (!newMidiDeviceFound)
			return false;
	}

	// and deactivate old and activate new device for callback
	if (m_midiInput && midiInputIdentifier.isEmpty())
	{
		DBG(String(__FUNCTION__) + " Deactivating MIDI input " + m_midiInput->getName() + +" (" + m_midiInput->getIdentifier() + ")");

		m_midiInput->stop();
		m_midiInput.reset();

		return true;
	}
	else if (m_midiInput && m_midiInput->getIdentifier() != midiInputIdentifier)
	{
		m_midiInput->stop();
		m_midiInput.reset();

		m_midiInput = juce::MidiInput::openDevice(midiInputIdentifier, this);
		m_midiInput->start();

		DBG(String(__FUNCTION__) + " Activated MIDI input " + m_midiInput->getName() + +" (" + m_midiInput->getIdentifier() + ")");

		return (m_midiInput != nullptr);
	}
	else if (!m_midiInput && midiInputIdentifier.isNotEmpty())
	{
		m_midiInput = juce::MidiInput::openDevice(midiInputIdentifier, this);
		m_midiInput->start();

		DBG(String(__FUNCTION__) + " Activated MIDI input " + m_midiInput->getName() + +" (" + m_midiInput->getIdentifier() + ")");

		return (m_midiInput != nullptr);
	}
	else
		return true; // nothing to do if neither valid new device identifier nor existing old device
}

/**
 * Helper method to activate a MIDI output based on a given input index.
 * This also tries deactivating previously activate outputs.
 * @param	midiOutputIdentifier	The identifier of the output device to activate.
 * @return	True if new device was activated and old deactivated or only old deactivated if requested by -1 index. False if this did not succeed.
 */
bool MIDIProtocolProcessor::activateMidiOutput(const String& midiOutputIdentifier)
{
	if (midiOutputIdentifier.isNotEmpty())
	{
		// verfiy availability of device corresponding to new device identifier
		auto midiDevicesInfos = juce::MidiOutput::getAvailableDevices();
		bool newMidiDeviceFound = false;
		for (auto const& midiDeviceInfo : midiDevicesInfos)
		{
			if (midiDeviceInfo.identifier == midiOutputIdentifier)
				newMidiDeviceFound = true;
		}
		if (!newMidiDeviceFound)
			return false;
	}

	// and deactivate old and activate new device for callback
	if (midiOutputIdentifier.isEmpty() && m_midiOutput)
	{
		DBG(String(__FUNCTION__) + " Deactivating MIDI output " + m_midiOutput->getName() + +" (" + m_midiOutput->getIdentifier() + ")");

		m_midiOutput.reset();

		return true;
	}
	else if (m_midiOutput && m_midiOutput->getIdentifier() != midiOutputIdentifier)
	{
		m_midiOutput.reset();

		m_midiOutput = juce::MidiOutput::openDevice(midiOutputIdentifier);

		DBG(String(__FUNCTION__) + " Activated MIDI output " + m_midiOutput->getName() + +" (" + m_midiOutput->getIdentifier() + ")");

		return (m_midiOutput != nullptr);
	}
	else if (!m_midiOutput && midiOutputIdentifier.isNotEmpty())
	{
		m_midiOutput = juce::MidiOutput::openDevice(midiOutputIdentifier);

		DBG(String(__FUNCTION__) + " Activated MIDI output " + m_midiOutput->getName() + +" (" + m_midiOutput->getIdentifier() + ")");

		return (m_midiOutput != nullptr);
	}
	else
		return true; // nothing to do if neither valid new device identifier nor existing old device
}

/**
 * Private method to forward an incoming and already processed (midi to internal struct) message
 * and at the same time submit it to internal hashing with timestamp to ensure deafness regarding
 * sending next midi output for this object. (avoids race conditions on motorfaders of hw console surfaces...)
 *
 * @param roi		The message object id that corresponds to the received message
 * @param msgData	The actual message data that was received
 */
void MIDIProtocolProcessor::forwardAndDeafProofMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData)
{
	m_addressedObjectOutputDeafStampMap[roi][msgData._addrVal] = juce::Time::getMillisecondCounterHiRes();

	if (m_messageListener)
		m_messageListener->OnProtocolMessageReceived(this, roi, msgData);
}

/**
 * Overloaded method to start the protocol processing object.
 * Usually called after configuration has been set.
 */
bool MIDIProtocolProcessor::Start()
{
	bool retVal = true;

	if (m_midiInputIdentifier.isNotEmpty() && !activateMidiInput(m_midiInputIdentifier))
		retVal = false;
	if (m_midiOutputIdentifier.isNotEmpty() && !activateMidiOutput(m_midiOutputIdentifier))
		retVal = false;

	return retVal;
}

/**
 * Overloaded method to stop to protocol processing object.
 */
bool MIDIProtocolProcessor::Stop()
{
	bool retVal = true;

	m_midiInputIdentifier.clear();
	if (!activateMidiInput(String()))
		retVal = false;
	m_midiOutputIdentifier.clear();
	if (!activateMidiOutput(String()))
		retVal = false;

	return retVal;
}

/**
 * Method to trigger sending of a message
 *
 * @param roi		The id of the object to send a message for
 * @param msgData	The message payload and metadata
 * @param externalId	An optional external id for identification of replies, etc. 
 *						(unused in this protocolprocessor impl)
 */
bool MIDIProtocolProcessor::SendRemoteObjectMessage(const RemoteObjectIdentifier roi, const RemoteObjectMessageData& msgData, const int externalId)
{
	ignoreUnused(externalId);

	// keep our MIDI specific value cache in sync with what is going on in the rest of the world first
	GetValueCache().SetValue(RemoteObject(roi, msgData._addrVal), msgData);

	// without valid output, we cannot send anything
	if (!m_midiOutput)
		return false;

	// if we do not have a valid mapping for the remote object to be sent, we cannot send anything either
	if (m_midiAssiMap.count(roi) <= 0)
		return false;

	// Verify that we have not received any input for the given addressing in the last deaf time period.
	// This avoids race conditions with external MIDI hw, e.g. hardware motorfaders, that otherwise might end up in a freezing state.
	if ((Time::getMillisecondCounterHiRes() - m_addressedObjectOutputDeafStampMap[roi][msgData._addrVal]) < m_outputDeafTimeMs)
		return false;

	auto channel = static_cast<int>(msgData._addrVal._first);
	auto record = static_cast<int>(msgData._addrVal._second);

	// if the remote object to be sent uses mappings/records, but the protocol processor is configured to handle a differend recordId, we do not want to send midi for the object
	if (ProcessingEngineConfig::IsRecordAddressingObject(roi) && (m_mappingAreaId != record))
		return false;

	// depending on remote object message data, derive the contained data values
	auto intValues = std::vector<int>();
	if (msgData._valType == ROVT_INT && (msgData._payloadSize == msgData._valCount * sizeof(int)))
	{
		intValues.reserve(msgData._valCount);
		for (int i = 0; i < msgData._valCount; i++)
			intValues.push_back(static_cast<int*>(msgData._payload)[i]);
	}
	auto floatValues = std::vector<float>();
	if (msgData._valType == ROVT_FLOAT && (msgData._payloadSize == msgData._valCount * sizeof(float)))
	{
		floatValues.reserve(msgData._valCount);
		for (int i = 0; i < msgData._valCount; i++)
			floatValues.push_back(static_cast<float*>(msgData._payload)[i]);
	}
	auto stringValue = String();
	if (msgData._valType == ROVT_STRING && (msgData._payloadSize == msgData._valCount * sizeof(char)))
	{
		for (int i = 0; i < msgData._valCount; i++)
			stringValue += static_cast<char*>(msgData._payload)[i];
	}

	auto midiAssi = m_midiAssiMap.at(roi);

	auto assiCmdValue		= midiAssi.getCommandValue();
	auto assiCmdRange		= midiAssi.getCommandRange();
	auto assiValRange		= midiAssi.getValueRange();
	auto assiMidiChannel	= midiAssi.getCommandChannel();

	auto newMidiValue = 0;
	if (floatValues.size() > 0)
		newMidiValue = static_cast<int>(jmap(floatValues.at(0), 
			ProcessingEngineConfig::GetRemoteObjectRange(roi).getStart(), ProcessingEngineConfig::GetRemoteObjectRange(roi).getEnd(),
			static_cast<float>(assiValRange.getStart()), static_cast<float>(assiValRange.getEnd())));
	else if (intValues.size() > 0)
		newMidiValue = jmap(intValues.at(0), 
			static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(roi).getStart()), static_cast<int>(ProcessingEngineConfig::GetRemoteObjectRange(roi).getEnd()),
			assiValRange.getStart(), assiValRange.getEnd());

	auto newMidiMessage = juce::MidiMessage();

	switch (midiAssi.getCommandType())
	{
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_NoteOn:
		{
		auto noteNumber = channel;
		if (midiAssi.isCommandRangeAssignment())
			noteNumber += JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCmdRange.getStart());
		else
			noteNumber += midiAssi.getCommandValue();
		
		newMidiMessage = juce::MidiMessage::noteOn(assiMidiChannel, noteNumber, static_cast<juce::uint8>(127));
		}
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_NoteOff:
		{
		auto noteNumber = channel;
		if (midiAssi.isCommandRangeAssignment())
			noteNumber += JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCmdRange.getStart());
		else
			noteNumber += midiAssi.getCommandValue();

		newMidiMessage = juce::MidiMessage::noteOff(assiMidiChannel, noteNumber);
		}
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_Pitch:
		newMidiMessage = juce::MidiMessage::pitchWheel(assiMidiChannel, newMidiValue);
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_ProgramChange:
		newMidiMessage = juce::MidiMessage::programChange(assiMidiChannel, newMidiValue);
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_Aftertouch:
		{
		auto noteNumber = channel;
		if (midiAssi.isCommandRangeAssignment())
			noteNumber += JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCmdRange.getStart());
		else
			noteNumber += midiAssi.getCommandValue();

		newMidiMessage = juce::MidiMessage::aftertouchChange(assiMidiChannel, noteNumber, 127);
		}
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_Controller:
		{
		auto controllerNumber = channel - 1;
		if (midiAssi.isCommandRangeAssignment())
			controllerNumber += JUCEAppBasics::MidiCommandRangeAssignment::getCommandValue(assiCmdRange.getStart());
		else
			controllerNumber += assiCmdValue;

		newMidiMessage = juce::MidiMessage::controllerEvent(assiMidiChannel, controllerNumber, newMidiValue);
		}
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_ChannelPressure:
		newMidiMessage = juce::MidiMessage::channelPressureChange(assiMidiChannel, newMidiValue);
		break;
	case JUCEAppBasics::MidiCommandRangeAssignment::CT_Invalid:
	default:
		return false;
	}

	DBG(String(__FUNCTION__) + " sending MIDI: " + newMidiMessage.getDescription());

	m_midiOutput->sendMessageNow(newMidiMessage);

	return true;
}
