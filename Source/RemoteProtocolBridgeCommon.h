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
static int uniqueIdCounter = 0;

/**
 * Type definitions.
 */
typedef std::uint32_t	NodeId;
typedef std::uint64_t	ProtocolId;
typedef std::int32_t	SourceId;
typedef std::int8_t		MappingId;

/**
* Generic defines
*/
#define INVALID_ADDRESS_VALUE -1
#define INVALID_IPADDRESS_VALUE String()
#define INVALID_RATE_VALUE -1
#define INVALID_PORT_VALUE -1

/**
 * Known Protocol Processor Types
 */
enum ProtocolType
{
	PT_Invalid = 0,			/**< Invalid protocol type value. */
	PT_OCAProtocol,			/**< OCA protocol type value. */
	PT_OSCProtocol,			/**< OSC protocol type value. */
	PT_MidiProtocol,		/**< MIDI protocol type value. */
	PT_RTTrPMProtocol,		/**< Blacktrax RTTrPMotion protocol type value. */
	PT_YamahaOSCProtocol,	/**< Yamaha OSC protocol type value. */
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
	OHM_UserMAX						/**< Value to mark enum max; For iteration purpose. */
};

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
	ROI_Error_GnrlErr,
	ROI_Error_ErrorText,
	ROI_Status_StatusText,
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
	ROI_Positioning_SourcePosition,
	ROI_Positioning_SourcePosition_XY,
	ROI_Positioning_SourcePosition_X,
	ROI_Positioning_SourcePosition_Y,
	ROI_CoordinateMapping_SourcePosition,		/**< combined xyz position remote object id. */
	ROI_CoordinateMapping_SourcePosition_XY,	/**< combined xy position remote object id. */
	ROI_CoordinateMapping_SourcePosition_X,		/**< x position remote object id. */
	ROI_CoordinateMapping_SourcePosition_Y,		/**< y position remote object id. */
	ROI_MatrixSettings_ReverbRoomId,
	ROI_MatrixSettings_ReverbPredelayFactor,
	ROI_MatrixSettings_RevebRearLevel,
	ROI_MatrixInput_ReverbSendGain,				/**< reverbsendgain remote object id. */
	ROI_ReverbInput_Gain,
	ROI_ReverbInputProcessing_Mute,
	ROI_ReverbInputProcessing_Gain,
	ROI_ReverbInputProcessing_LevelMeter,
	ROI_ReverbInputProcessing_EqEnable,
	ROI_Scene_SceneIndex,
	ROI_Scene_SceneName,
	ROI_Scene_SceneComment,
	ROI_BridgingMAX,								/**< Value to mark max enum iteration scope. ROIs greater than this can/will not be bridged.*/
	ROI_Device_Clear,
	ROI_Scene_Previous,
	ROI_Scene_Next,
	ROI_Scene_Recall,
	ROI_RemoteProtocolBridge_SoundObjectSelect,
	ROI_RemoteProtocolBridge_UIElementIndexSelect,
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
	SourceId	first;	/**< First address definition value. Equivalent to channels in d&b OCA world or SourceId for OSC positioning messages. */
	MappingId	second;	/**< Second address definition value. Equivalent to records in d&b OCA world or MappingId for OSC positioning messages. */

	/**
	 * Constructor to initialize with invalid values
	 */
	RemoteObjectAddressing()
	{
		first = INVALID_ADDRESS_VALUE;
		second = INVALID_ADDRESS_VALUE;
	};
	/**
	 * Constructor to initialize with parameter values
	 *
	 * @param a	The value to set for internal 'first' - SourceId (Channel)
	 * @param b	The value to set for internal 'second' - MappingId (Record)
	 */
	RemoteObjectAddressing(SourceId a, MappingId b)
	{
		first = a;
		second = b;
	};
	/**
	 * Equality comparison operator overload
	 */
	bool operator==(const RemoteObjectAddressing& o) const
	{
		return (first == o.first) && (second == o.second);
	}
	/**
	 * Unequality comparison operator overload
	 */
	bool operator!=(const RemoteObjectAddressing& o) const
	{
		return !(*this == o);
	}
	/**
	 * Lesser than comparison operator overload
	 */
	bool operator<(const RemoteObjectAddressing& o) const
	{
		return (!(*this > o) && (*this != o));
	}
	/**
	 * Greater than comparison operator overload
	 */
	bool operator>(const RemoteObjectAddressing& o) const
	{
		return (first > o.first) || ((first == o.first) && (second > o.second));
	}
};

/**
 * Dataset defining a remote object including adressing (info regarding channel/record)
 */
struct RemoteObject
{
	RemoteObjectIdentifier	Id;		/**< The remote object id for the object. */
	RemoteObjectAddressing	Addr;	/**< The remote object addressings (channel/record) for the object. */

	bool operator==(const RemoteObject& rhs) const
	{
		return (Id == rhs.Id && Addr == rhs.Addr);
	};
};

/**
 * Dataset for a generic (non-protocol-specific) remote object message
 */
struct RemoteObjectMessageData
{
	RemoteObjectAddressing	addrVal;		/**< Address definition value. Equivalent to channels/records in d&b OCA world or SourceId/MappingId for OSC positioning messages. */

	RemoteObjectValueType	valType;		/**< Datatype used for data values of the remote object. */
	std::uint16_t			valCount;		/**< Value count used by the remote object. */

	void*					payload;		/**< Pointer to the actual payload data. */
	std::uint64_t			payloadSize;	/**< Size of the payload data. */
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
