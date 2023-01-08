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

#include <vector>
#include <cstdint>


// **************************************************************
// class RTTrPMHeader
// **************************************************************
/**
* A class to read a databuffer containing RTTrPM packet modules
* 
*/
class RTTrPMHeader
{
public:
	static constexpr std::uint16_t PacketModuleHeaderVersion = 0x0002;

	typedef std::uint16_t PacketModuleSignature;
	static constexpr PacketModuleSignature BigEndianInt			= 0x4154;
	static constexpr PacketModuleSignature LittleEndianInt		= 0x5441;
	static constexpr PacketModuleSignature BigEndianFloat		= 0x4334;
	static constexpr PacketModuleSignature LittleEndianFloat	= 0x3443;

	typedef std::uint8_t PacketModuleFormat;
	static constexpr PacketModuleFormat Raw			= 0x00;
	static constexpr PacketModuleFormat Protobuf	= 0x01;
	static constexpr PacketModuleFormat Thrift		= 0x02;

public:
	RTTrPMHeader();
	RTTrPMHeader(std::vector<unsigned char>& data, int& readPos);
	~RTTrPMHeader();

	void readData(std::vector<unsigned char>& data, int& readPos);

	PacketModuleSignature GetIntSignature() const;
	PacketModuleSignature GetFloatSignature() const;
	std::uint16_t GetVersion() const;
	std::uint32_t GetPacketID() const;
	PacketModuleFormat GetPacketFormat() const;
	std::uint16_t GetPacketSize() const;
	std::uint32_t GetContext() const;
	std::uint8_t GetNumberOfModules() const;

private:
	PacketModuleSignature m_intSignature{ 0 };
	PacketModuleSignature m_floatSignature{ 0 };
	std::uint16_t m_version{ 0 };
	std::uint32_t m_packetID{ 0 };
	PacketModuleFormat m_packetFormat{ 0 };
	std::uint16_t m_packetSize{ 0 };
	std::uint32_t m_context{ 0 };
	std::uint8_t m_numModules{ 0 };
};
