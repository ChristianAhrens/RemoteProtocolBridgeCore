/* Copyright (c) 2020-2021, Christian Ahrens
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
// class PacketModule 
// **************************************************************
/**
* A class to save the basic packet module information
*/
class PacketModule
{
public:
	typedef std::uint8_t	PacketModuleType;
	static constexpr PacketModuleType Invalid								= 0x00;
	static constexpr PacketModuleType WithTimestamp							= 0x51;
	static constexpr PacketModuleType WithoutTimestamp						= 0x01;
	static constexpr PacketModuleType CentroidPosition						= 0x02;
	static constexpr PacketModuleType TrackedPointPosition					= 0x06;
	static constexpr PacketModuleType OrientationQuaternion					= 0x03;
	static constexpr PacketModuleType OrientationEuler						= 0x04;
	static constexpr PacketModuleType CentroidAccelerationAndVelocity		= 0x20;
	static constexpr PacketModuleType TrackedPointAccelerationandVelocity	= 0x21;
	static constexpr PacketModuleType ZoneCollisionDetection				= 0x22;

public:
	PacketModule();
	PacketModule(std::vector<unsigned char>& data, int & readPos);
	virtual ~PacketModule();

	virtual void readData(std::vector<unsigned char>& data, int& readPos);

	PacketModuleType GetModuleType() const;
	uint16_t GetModuleSize() const;

	virtual bool isValid() const;

private:
	PacketModuleType	m_moduleType{ Invalid };	//	Type of the packet module
	std::uint16_t			m_moduleSize{ 0 };	//	Size of the module
};
