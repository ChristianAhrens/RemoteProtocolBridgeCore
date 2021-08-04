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

#include "PacketModuleTrackable.h"

#include <memory>


/**
* Constructor of the PacketModuleTrackable class. It reads the number of sub-modules in the packet.
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
PacketModuleTrackable::PacketModuleTrackable(std::vector<unsigned char>& data, int& readPos)
	: PacketModule(data, readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void PacketModuleTrackable::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	if(GetModuleType() == WithTimestamp)
	{
		std::copy(readIter, readIter + 1, (uint8_t *)&m_nameLength);
		readIter += 1;

		m_name = std::string((const char*)(&(*readIter)), m_nameLength);
		readIter += m_nameLength;

		std::copy(readIter, readIter + 4, (uint32_t*)&m_seqNumber);
		readIter += 4;

		std::copy(readIter, readIter + 1, (int *)&m_numberOfSubModules);
		readIter += 1;

		readPos += (1 + m_nameLength + 4 + 1);
	}

	else if(GetModuleType() == WithoutTimestamp)
	{
		std::copy(readIter, readIter + 1, (uint8_t*)&m_nameLength);
		readIter += 1;

		m_name = std::string((const char*)(&(*readIter)), m_nameLength);
		readIter += m_nameLength;

		std::copy(readIter, readIter + 1, (int*)&m_numberOfSubModules);
		readIter += 1;
		
		readPos += (1 + m_nameLength + 1);
	}
}

/**
* Destructor of the class PacketModuleTrackable
*/
PacketModuleTrackable::~PacketModuleTrackable()
{
}

/**
 * Getter for the packet module name
 * @return The name string
 */
std::string PacketModuleTrackable::GetName() const
{
	return m_name;
}

/**
 * Getter for the packet module sequence number
 * @return The sequence number
 */
uint32_t PacketModuleTrackable::GetSeqNumber() const
{
	return m_seqNumber;
}

/**
* Returns the number of sub-modules
* @return	Number of sub-modules
*/
uint8_t PacketModuleTrackable::GetNumberOfSubModules() const
{
	return m_numberOfSubModules;
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool PacketModuleTrackable::isValid() const
{
	return (PacketModule::isValid() && ((GetModuleType() == WithTimestamp) || (GetModuleType() == WithoutTimestamp)));
}