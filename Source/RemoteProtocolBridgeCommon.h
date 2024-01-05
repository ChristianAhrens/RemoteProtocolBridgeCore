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

#include <JuceHeader.h>

/**
 * Unique, therefor static, counter for id generation
 */
static int s_uniqueIdCounter = 0;

/**
 * Type definitions.
 */
typedef std::uint32_t	NodeId;
typedef std::uint64_t	ProtocolId;
typedef std::int32_t	ChannelId;
typedef std::int8_t		RecordId;

/**
* Generic defines
*/
#define INVALID_ADDRESS_VALUE -1
#define INVALID_IPADDRESS_VALUE String()
#define INVALID_RATE_VALUE -1
#define INVALID_PORT_VALUE -1
#define INVALID_EXTID -1
#define ASYNC_EXTID -2

/**
 * Known Protocol Processor Types
 */
enum ProtocolType
{
	PT_Invalid = 0,			/**< Invalid protocol type value. */
	PT_OCP1Protocol,		/**< OCA protocol type value. */
	PT_OSCProtocol,			/**< OSC protocol type value. */
	PT_MidiProtocol,		/**< MIDI protocol type value. */
	PT_RTTrPMProtocol,		/**< Blacktrax RTTrPMotion protocol type value. */
	PT_YamahaOSCProtocol,	/**< Yamaha OSC protocol type value. */
	PT_ADMOSCProtocol,		/**< ADM OSC protocol type value. */
	PT_RemapOSCProtocol,	/**< Freely remapable OSC protocol type value. */
	PT_NoProtocol,			/**< Protocol processor implementation that acts as dummy, replying to all msgs sent with a reply and has some fixed object values that can be dummy-'polled'. */
	PT_UserMAX				/**< Value to mark enum max; For iteration purpose. */
};

/**
 * Known Protocol Processor Roles
 */
enum ProtocolRole
{
	PR_Invalid = 0,	/**< Invalid protocol role value. */
	PR_A,			/**< role A value. */
	PR_B,			/**< role B value. */
	PR_UserMax		/**< Value to mark enum max; For iteration purpose. */
};					

/**
 * Known ObjectHandling modes
 */
enum ObjectHandlingMode
{
	OHM_Invalid = 0,				/**< Invalid object handling mode value. */
	OHM_Bypass,						/**< Data bypass mode. */
	OHM_Remap_A_X_Y_to_B_XY,		/**< Simple hardcoded data remapping mode (protocol A (x), (y) to protocol B (XY)). */
	OHM_Mux_nA_to_mB,				/**< Data multiplexing mode from n channel typeA protocols to m channel typeB protocols. */
	OHM_Forward_only_valueChanges,	/**< Data filtering mode to only forward value changes. */
	OHM_DS100_DeviceSimulation,		/**< Device simulation mode that answers incoming roi messages without value with appropriate simulated value answer. */
    OHM_Forward_A_to_B_only,        /**< Data filtering mode to only pass on values from Role A to B protocols. */
    OHM_Reverse_B_to_A_only,        /**< Data filtering mode to only pass on values from Role B to A protocols. */
	OHM_Mux_nA_to_mB_withValFilter,	/**< Data multiplexing mode from n channel typeA protocols to m channel typeB protocols, combined with filtering to only forward value changes. */
	OHM_Mirror_dualA_withValFilter,	/**< Data mirroring mode inbetween two typeA protocols and forwarding value changes to typeB protocols. */
	OHM_A1active_withValFilter,		/**< Data filtering mode to only forward value changes and only forward data from first RoleA protocol. */
	OHM_A2active_withValFilter,		/**< Data filtering mode to only forward value changes and only forward data from second RoleA protocol. */
	OHM_UserMAX						/**< Value to mark enum max; For iteration purpose. */
};

typedef std::uint16_t	ObjectHandlingState;								/** Type that describes the different
																			 *  status a ObjectHandling instance can
																			 *  notify registered listners of.
																			 */
static constexpr ObjectHandlingState OHS_Invalid			= 0x00000000;	/**< . */
static constexpr ObjectHandlingState OHS_Protocol_Up		= 0x00000001;	/**< . */
static constexpr ObjectHandlingState OHS_Protocol_Down		= 0x00000002;	/**< . */
static constexpr ObjectHandlingState OHS_Protocol_Master	= 0x00000010;	/**< . */
static constexpr ObjectHandlingState OHS_Protocol_Slave		= 0x00000020;	/**< . */

/**
 * Remote Object Identification
 */
enum RemoteObjectIdentifier
{
	ROI_HeartbeatPing = 0,			/**< Hearbeat request (OSC-exclusive) without data content. */
	ROI_HeartbeatPong,				/**< Hearbeat answer (OSC-exclusive) without data content. */
	ROI_Invalid,					/**< Invalid remote object id. This is not the first
									   * value to allow iteration over enum starting 
									   * here (e.g. to not show the user the internal-only ping/pong). */
	ROI_Settings_DeviceName,
	ROI_Status_StatusText,
	ROI_Status_AudioNetworkSampleStatus,
	ROI_Error_GnrlErr,
	ROI_Error_ErrorText,
	ROI_MatrixInput_Select,
	ROI_MatrixInput_Mute,
	ROI_MatrixInput_Gain,
	ROI_MatrixInput_Delay,
	ROI_MatrixInput_DelayEnable,
	ROI_MatrixInput_EqEnable,
	ROI_MatrixInput_Polarity,
	ROI_MatrixInput_ChannelName,
	ROI_MatrixInput_LevelMeterPreMute,
	ROI_MatrixInput_LevelMeterPostMute,
	ROI_MatrixInput_ReverbSendGain,				/**< reverbsendgain remote object id. */
	ROI_MatrixNode_Enable,
	ROI_MatrixNode_Gain,
	ROI_MatrixNode_DelayEnable,
	ROI_MatrixNode_Delay,
	ROI_MatrixOutput_Mute,
	ROI_MatrixOutput_Gain,
	ROI_MatrixOutput_Delay,
	ROI_MatrixOutput_DelayEnable,
	ROI_MatrixOutput_EqEnable,
	ROI_MatrixOutput_Polarity,
	ROI_MatrixOutput_ChannelName,
	ROI_MatrixOutput_LevelMeterPreMute,
	ROI_MatrixOutput_LevelMeterPostMute,
	ROI_Positioning_SourceSpread,				/**< spread remote object id. */
	ROI_Positioning_SourceDelayMode,			/**< delaymode remote object id. */
	ROI_Positioning_SourcePosition_XY,
	ROI_Positioning_SourcePosition_X,
	ROI_Positioning_SourcePosition_Y,
	ROI_Positioning_SourcePosition,
	ROI_CoordinateMapping_SourcePosition_XY,	/**< combined xy position remote object id. */
	ROI_CoordinateMapping_SourcePosition_X,		/**< x position remote object id. */
	ROI_CoordinateMapping_SourcePosition_Y,		/**< y position remote object id. */
	ROI_CoordinateMapping_SourcePosition,		/**< combined xyz position remote object id. */
	ROI_MatrixSettings_ReverbRoomId,
	ROI_MatrixSettings_ReverbPredelayFactor,
	ROI_MatrixSettings_ReverbRearLevel,
	ROI_FunctionGroup_Name,
	ROI_FunctionGroup_Delay,
	ROI_FunctionGroup_SpreadFactor,
	ROI_ReverbInput_Gain,
	ROI_ReverbInputProcessing_Mute,
	ROI_ReverbInputProcessing_Gain,
	ROI_ReverbInputProcessing_EqEnable,
	ROI_ReverbInputProcessing_LevelMeter,
	ROI_Scene_SceneIndex,
	ROI_Scene_SceneName,
	ROI_Scene_SceneComment,
	ROI_Scene_Previous,
	ROI_Scene_Next,
	ROI_Scene_Recall,
	ROI_CoordinateMappingSettings_P1real,
	ROI_CoordinateMappingSettings_P2real,
	ROI_CoordinateMappingSettings_P3real,
	ROI_CoordinateMappingSettings_P4real,
	ROI_CoordinateMappingSettings_P1virtual,
	ROI_CoordinateMappingSettings_P3virtual,
	ROI_CoordinateMappingSettings_Flip,
	ROI_CoordinateMappingSettings_Name,
	ROI_Positioning_SpeakerPosition,			// 6-float loudspeaker position (x, y, z, hor, vert, rot)
	ROI_SoundObjectRouting_Mute,
	ROI_SoundObjectRouting_Gain,
	ROI_BridgingMAX,							/**< Value to mark max enum iteration scope. ROIs greater than this can/will not be bridged.*/
	ROI_Device_Clear,
	ROI_RemoteProtocolBridge_SoundObjectSelect,
	ROI_RemoteProtocolBridge_UIElementIndexSelect,
	ROI_RemoteProtocolBridge_GetAllKnownValues,
	ROI_RemoteProtocolBridge_SoundObjectGroupSelect,
	ROI_RemoteProtocolBridge_MatrixInputGroupSelect,
	ROI_RemoteProtocolBridge_MatrixOutputGroupSelect,
	ROI_InvalidMAX
};

/**
 * Remote Object Identification
 */
enum RemoteObjectValueType
{
	ROVT_NONE,		/**< Invalid type. */
	ROVT_INT,		/**< Integer type. Shall be equivalent to 'int' with size 'sizeof(int)'. */
	ROVT_FLOAT,		/**< Floating point type. Shall be equivalent to 'float' with size 'sizeof(float)'. */
	ROVT_STRING		/**< String type. */
};

/**
 * Dataset for Remote object addressing.
 */
struct RemoteObjectAddressing
{
	ChannelId	_first;	/**< First address definition value. Equivalent to channels in d&b OCA world or SourceId for OSC positioning messages. */
	RecordId	_second;	/**< Second address definition value. Equivalent to records in d&b OCA world or MappingId for OSC positioning messages. */

	/**
	 * Constructor to initialize with invalid values
	 */
	RemoteObjectAddressing()
	{
		_first = INVALID_ADDRESS_VALUE;
		_second = INVALID_ADDRESS_VALUE;
	};
	/**
	 * Copy Constructor
	 */
	RemoteObjectAddressing(const RemoteObjectAddressing& rhs)
	{
		*this = rhs;
	};
	/**
	 * Constructor to initialize with parameter values
	 *
	 * @param a	The value to set for internal 'first' - SourceId (Channel)
	 * @param b	The value to set for internal 'second' - MappingId (Record)
	 */
	RemoteObjectAddressing(ChannelId a, RecordId b)
	{
		_first = a;
		_second = b;
	};
	/**
	 * Helper method to create a string representation of the remote object
	 * @return	The string representation as created
	 */
	juce::String toString() const
	{
		return juce::String(_first) + "," + juce::String(_second);
	}
	/**
	 * Helper method to create a string representation of a given list of remote object addressings
	 * @param	remoteObjectAddressings		The list of addressings that shall be dumped into a string representation.
	 * @return	The string representation as created
	 */
	static juce::String toString(const std::vector<RemoteObjectAddressing>& remoteObjectAddressings)
	{
		auto objectListString = juce::String();

		for (auto const& objectAddressing : remoteObjectAddressings)
			objectListString << objectAddressing.toString() << ";";

		return objectListString;
	}
	/**
	 * Helper method to initialize this object instance's members
	 * from its string representation of the remote object
	 * @param	commaseparatedStringRepresentation	The string representation of a roiaddressing, comma separated
	 * @return	False if the input string could not be processed to valid member data
	 */
	bool fromString(const juce::String& commaseparatedStringRepresentation)
	{
		juce::StringArray sa;
		auto tokens = sa.addTokens(commaseparatedStringRepresentation, ",", "");
		if (tokens != 2 || sa.size() != 2)
			return false;

		_first = ChannelId(sa[0].getIntValue());
		_second = RecordId(sa[1].getIntValue());

		return true;
	}
	/**
	 * Static helper method to create an object instance
	 * from its string representation of the remote object
	 * @param	commaseparatedStringRepresentation	The string representation of a roiaddressing, comma separated
	 * @return	The remote object as created. Invalid remote object if the string could not be processed correctly
	 */
	static RemoteObjectAddressing createFromString(const juce::String& commaseparatedStringRepresentation)
	{
		juce::StringArray sa;
		sa.addTokens(commaseparatedStringRepresentation.trimCharactersAtEnd(","), ",", "");
		if (sa.size() != 2)
			return RemoteObjectAddressing();

		return RemoteObjectAddressing(ChannelId(sa[0].getIntValue()), RecordId(sa[1].getIntValue()));
	}
	/**
	 * Static helper method to create an list of object instances
	 * from their concatenated string representations
	 * @param	objectListStringRepresentation	The string representation of a list of remote objects, semicolon separated
	 * @return	The list of remote objects as created. Invalid remote objects might be contained if the string could not be processed correctly
	 */
	static std::vector<RemoteObjectAddressing> createFromListString(const juce::String& objectListStringRepresentation)
	{
		auto remoteObjects = std::vector<RemoteObjectAddressing>();

		juce::StringArray sa;
		sa.addTokens(objectListStringRepresentation.trimCharactersAtEnd(";"), ";", "");

		for (auto const& commaseparatedStringRepresentation : sa)
			remoteObjects.push_back(RemoteObjectAddressing::createFromString(commaseparatedStringRepresentation));

		return remoteObjects;
	}
	/**
	 * Equality comparison operator overload
	 */
	bool operator==(const RemoteObjectAddressing& rhs) const
	{
		return (_first == rhs._first) && (_second == rhs._second);
	}
	/**
	 * Unequality comparison operator overload
	 */
	bool operator!=(const RemoteObjectAddressing& rhs) const
	{
		return !(*this == rhs);
	}
	/**
	 * Lesser than comparison operator overload
	 */
	bool operator<(const RemoteObjectAddressing& rhs) const
	{
		return (_first < rhs._first) || ((_first == rhs._first) && (_second < rhs._second));
	}
	/**
	 * Greater than comparison operator overload
	 */
	bool operator>(const RemoteObjectAddressing& rhs) const
	{
		// not less and not equal is greater
		return (!(*this < rhs) && (*this != rhs));
	}
	/**
	 * Assignment operator
	 */
	RemoteObjectAddressing& operator=(const RemoteObjectAddressing& rhs)
	{
		if (this != &rhs)
		{
			_first = rhs._first;
			_second = rhs._second;
		}

		return *this;
	}

	JUCE_LEAK_DETECTOR(RemoteObjectAddressing)
};

/**
 * Dataset defining a remote object including adressing (info regarding channel/record)
 */
struct RemoteObject
{
	RemoteObjectIdentifier	_Id;		/**< The remote object id for the object. */
	RemoteObjectAddressing	_Addr;	/**< The remote object addressings (channel/record) for the object. */

	/**
	 * Constructor to initialize with invalid values
	 */
	RemoteObject()
		: _Id(ROI_Invalid),
		  _Addr(RemoteObjectAddressing(static_cast<ChannelId>(INVALID_ADDRESS_VALUE), static_cast<RecordId>(INVALID_ADDRESS_VALUE)))
	{
	};
	/**
	 * Copy Constructor
	 */
	RemoteObject(const RemoteObject& rhs)
	{
		*this = rhs;
	};
	/**
	 * Constructor to initialize with parameter values
	 *
	 * @param Id	Remote object id value to initialize with.
	 * @param Addr	Remote object addressing value to initialize with.
	 */
	RemoteObject(RemoteObjectIdentifier	Id, RemoteObjectAddressing Addr)
		: _Id(Id),
		  _Addr(Addr)
	{
	};
	/**
	 * Equality comparison operator overload
	 */
	bool operator==(const RemoteObject& rhs) const
	{
		return (_Id == rhs._Id && _Addr == rhs._Addr);
	};
	/**
	 * Unequality comparison operator overload
	 */
	bool operator!=(const RemoteObject& rhs) const
	{
		return !(*this == rhs);
	}
	/**
	 * Lesser than comparison operator overload
	 */
	bool operator<(const RemoteObject& rhs) const
	{
		return (!(*this > rhs) && (*this != rhs));
	}
	/**
	 * Greater than comparison operator overload
	 */
	bool operator>(const RemoteObject& rhs) const
	{
		return (_Id > rhs._Id) || ((_Id == rhs._Id) && (_Addr > rhs._Addr));
	}
	/**
	 * Assignment operator
	 */
	RemoteObject& operator=(const RemoteObject& rhs)
	{
		if (this != &rhs)
		{
			_Id = rhs._Id;
			_Addr = rhs._Addr;
		}

		return *this;
	}

	JUCE_LEAK_DETECTOR(RemoteObject)
};

/**
 * Dataset for a generic (non-protocol-specific) remote object message
 */
struct RemoteObjectMessageData
{
	RemoteObjectAddressing	_addrVal;		/**< Address definition value. Equivalent to channels/records in d&b OCA world or SourceId/MappingId for OSC positioning messages. */

	RemoteObjectValueType	_valType;		/**< Datatype used for data values of the remote object. */
	std::uint16_t			_valCount;		/**< Value count used by the remote object. */

	void*					_payload;		/**< Pointer to the actual payload data. */
	std::uint32_t			_payloadSize;	/**< Size of the payload data. */
	bool					_payloadOwned;	/**< Indicator if the payload is owned by this object. */

	/**
	 * Constructor to initialize with invalid values
	 */
	RemoteObjectMessageData()
		: _addrVal(RemoteObjectAddressing()),
		  _valType(ROVT_NONE),
		  _valCount(0),
		  _payload(nullptr),
		  _payloadSize(0),
		  _payloadOwned(false)
	{
	};
	/**
	 * Copy Constructor
	 */
	RemoteObjectMessageData(const RemoteObjectMessageData& rhs)
	{
		*this = rhs;
	};
	/**
	 * Constructor to initialize with parameter values
	 */
	RemoteObjectMessageData(RemoteObjectAddressing addrVal, RemoteObjectValueType valType, std::uint16_t valCount, void* payload, std::uint32_t payloadSize)
		: _addrVal(addrVal),
		  _valType(valType),
		  _valCount(valCount),
		  _payload(payload),
		  _payloadSize(payloadSize),
		  _payloadOwned(false)
	{
	};
	/**
	 * Destructor
	 */
	~RemoteObjectMessageData()
	{
		// do not let the unique_ptr delete the payload, since it is configured to not be internally owned.
		if (_payloadOwned)
		{
			switch (_valType)
			{
			case ROVT_INT:
				delete[] static_cast<int*>(_payload);
				break;
			case ROVT_FLOAT:
				delete[] static_cast<float*>(_payload);
				break;
			case ROVT_STRING:
				delete[] static_cast<char*>(_payload);
				break;
			default:
				break;
			}
			_payload = nullptr;
			_payloadSize = 0;
		}
	};
	/**
	 * Equality comparison operator overload
	 */
	bool operator==(const RemoteObjectMessageData& rhs) const
	{
		return (_addrVal == rhs._addrVal && _valType == rhs._valType && _valCount == rhs._valCount && _payloadSize == rhs._payloadSize && _payload == rhs._payload);
	};
	/**
	 * Unequality comparison operator overload
	 */
	bool operator!=(const RemoteObjectMessageData& rhs) const
	{
		return !(*this == rhs);
	}
	/**
	 * Lesser than comparison operator overload
	 */
	bool operator<(const RemoteObjectMessageData& rhs) const
	{
		return (!(*this > rhs) && (*this != rhs));
	}
	/**
	 * Greater than comparison operator overload
	 */
	bool operator>(const RemoteObjectMessageData& rhs) const
	{
		return (_payloadSize > rhs._payloadSize);
	}
	/**
	 * Assignment operator
	 */
	RemoteObjectMessageData& operator=(const RemoteObjectMessageData& rhs)
	{
		if (this != &rhs)
		{
			_addrVal = rhs._addrVal;
			_valType = rhs._valType;
			_valCount = rhs._valCount;
			_payloadSize = rhs._payloadSize;
			_payload = rhs._payload;
			_payloadOwned = false;
		}

		return *this;
	}
	/**
	 * Method to assign a ROMD object with all members to this object, including copying data behind payload pointer.
	 */
	RemoteObjectMessageData& payloadCopy(const RemoteObjectMessageData& rhs)
	{
		// if the allocated memory does not fit our new needs, resize it appropriately
		if (_payloadSize != rhs._payloadSize)
		{
			switch (_valType)
			{
			case ROVT_INT:
				delete[] static_cast<int*>(_payload);
				break;
			case ROVT_FLOAT:
				delete[] static_cast<float*>(_payload);
				break;
			case ROVT_STRING:
				delete[] static_cast<char*>(_payload);
				break;
			case ROVT_NONE:
			default:
				jassert(_payload == nullptr && _payloadSize == 0);
				break;
			}

			_payloadSize = rhs._payloadSize;
			if (_payloadSize > 0)
				_payload = new unsigned char[_payloadSize];
			else
				_payload = nullptr;
		}

		// now copy the new data
		if (_payloadSize > 0 && _payload != nullptr)
			std::memcpy(_payload, rhs._payload, _payloadSize);
            
		_addrVal = rhs._addrVal;
		_valType = rhs._valType;
		_valCount = rhs._valCount;
		_payloadOwned = true;

		return *this;
	}
	/**
	 * Method to check if the internal data is empty.
	 */
	bool isDataEmpty() const
	{
		return (_payloadSize == 0 && _valCount == 0 && _payload == nullptr);
	}

	JUCE_LEAK_DETECTOR(RemoteObjectMessageData)
};

/**
 * Dataset holding meta info on a remote object message
 */
struct RemoteObjectMessageMetaInfo
{
	enum MessageCategory
	{
		MC_None,
		MC_UnsolicitedMessage,
		MC_SetMessageAcknowledgement
	};

	MessageCategory	_Category;		/**< The message data category of the object. */
	int				_ExternalId;	/**< An external identifier for the object. 
									 * In case of category MC_SetMessageAcknowledgement 
									 * this is the protocol id of the protocol that triggered thet SET.
									 * -2 if no protocol but other means triggered the SET. */

	/**
	 * Constructor to initialize with invalid values
	 */
	RemoteObjectMessageMetaInfo()
		:	_Category(MC_None),
			_ExternalId(-1)
	{
	};
	/**
	 * Copy Constructor
	 */
	RemoteObjectMessageMetaInfo(const RemoteObjectMessageMetaInfo& rhs)
	{
		*this = rhs;
	};
	/**
	 * Constructor to initialize with parameter values
	 *
	 * @param category		Message data category value to initialize with.
	 * @param externalId	Message data external id value to initialize with.
	 */
	RemoteObjectMessageMetaInfo(MessageCategory category, int externalId)
		:	_Category(category),
			_ExternalId(externalId)
	{
	};
	/**
	 * Equality comparison operator overload
	 */
	bool operator==(const RemoteObjectMessageMetaInfo& rhs) const
	{
		return (_Category == rhs._Category && _ExternalId == rhs._ExternalId);
	};
	/**
	 * Unequality comparison operator overload
	 */
	bool operator!=(const RemoteObjectMessageMetaInfo& rhs) const
	{
		return !(*this == rhs);
	}
	/**
	 * Lesser than comparison operator overload
	 */
	bool operator<(const RemoteObjectMessageMetaInfo& rhs) const
	{
		return (!(*this > rhs) && (*this != rhs));
	}
	/**
	 * Greater than comparison operator overload
	 */
	bool operator>(const RemoteObjectMessageMetaInfo& rhs) const
	{
		return (_Category > rhs._Category) || ((_Category == rhs._Category) && (_ExternalId > rhs._ExternalId));
	}
	/**
	 * Assignment operator
	 */
	RemoteObjectMessageMetaInfo& operator=(const RemoteObjectMessageMetaInfo& rhs)
	{
		if (this != &rhs)
		{
			_Category = rhs._Category;
			_ExternalId = rhs._ExternalId;
		}

		return *this;
	}

	JUCE_LEAK_DETECTOR(RemoteObjectMessageMetaInfo)
};

/**
 * Common size values used in UI
 */
enum UISizes
{
	UIS_MainComponentWidth		= 500,	/** The main component windows' overall width. */
	UIS_Margin_s				= 5,	/** A small margin. */
	UIS_Margin_m				= 10,	/** A medium margin. */
	UIS_Margin_l				= 25,	/** A large margin. */
	UIS_Margin_xl				= 30,	/** An extra large margin. */
	UIS_ElmSize					= 20,	/** The usual element size (1D). */
	UIS_OpenConfigWidth			= 120,	/** Width of global config button. */
	UIS_ButtonWidth				= 70,	/** The usual button width. */
	UIS_AttachedLabelWidth		= 110,	/** The width of the text label when attached to other elms. */
	UIS_WideAttachedLabelWidth	= 140,	/** The width of a wide text label when attached to other elms. */
	UIS_NodeModeDropWidthOffset	= 120,	/** The offset used for node mode drop in x dim. */
	UIS_PortEditWidth			= 90,	/** The width used for port edits. */
	UIS_ProtocolDropWidth		= 80,	/** The width used for protocol type drop. */
	UIS_ConfigButtonWidth		= 80,	/** Protocols' open-config button width. */
	UIS_ProtocolLabelWidth		= 100,	/** The width used for protocol label. */
	UIS_OSCConfigWidth			= 420,	/** The width of the osc specific config window (component). */
	UIS_BasicConfigWidth		= 400,	/** The width of the basic config window (component). */
	UIS_GlobalConfigWidth		= 300,	/** The width of the global config window (component). */
};

/**
 * Common color values used in UI
 */
enum UIColors
{
	UIC_WindowColor		= 0xFF1B1B1B,		// 27 27 27	- Window background
	UIC_DarkLineColor	= 0xFF313131,		// 49 49 49 - Dark lines between table rows
	UIC_DarkColor		= 0xFF434343,		// 67 67 67	- Dark
	UIC_MidColor		= 0xFF535353,		// 83 83 83	- Mid
	UIC_ButtonColor		= 0xFF7D7D7D,		// 125 125 125 - Button off
	UIC_LightColor		= 0xFFC9C9C9,		// 201 201 201	- Light
	UIC_TextColor		= 0xFFEEEEEE,		// 238 238 238 - Text
	UIC_DarkTextColor	= 0xFFB4B4B4,		// 180 180 180 - Dark text
	UIC_HighlightColor	= 0xFF738C9B,		// 115 140 155 - Highlighted text
	UIC_FaderGreenColor = 0xFF8CB45A,		// 140 180 90 - Green sliders
	UIC_ButtonBlueColor = 0xFF1B78A3,		// 28 122 166 - Button Blue
};

/**
 * Common timing values used in Engine
 */
enum EngineTimings
{
	ET_DefaultPollingRate	= 100,	/** OSC polling interval in ms. */
	ET_LoggingFlushRate		= 300	/** Flush interval for accumulated messages to be printed. */
};

/**
 * Separate enum for mapping area identification.
 * Allows distinguishing between valid and invalid values.
 */
enum MappingAreaId
{
	MAI_Invalid = -1,	/**> identification for an invalid mapping area id - used in protocolprocessors 
						 *   to signal if absolute value handling shall be used 
						 *   instead of relative to a mapping area. */
	MAI_First = 1,		/**> first of four valid mapping areas. */
	MAI_Second,			/**> second of four valid mapping areas. */
	MAI_Third,			/**> third of four valid mapping areas. */
	MAI_Fourth,			/**> last of four valid mapping areas. */
};
