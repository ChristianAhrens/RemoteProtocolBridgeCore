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

#include "PacketModule.h"


/**
* Constructor of the PacketModule class
*/
PacketModule::PacketModule()
{
}

/**
* Constructor of the PacketModule class.
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
PacketModule::PacketModule(std::vector<unsigned char>& data, int& readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void PacketModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 1, (uint8_t*)&m_moduleType);
	readIter += 1;
	std::copy(readIter, readIter + 2, (uint16_t*)&m_moduleSize);
	readIter += 2;

	readPos += 3;
}

/**
* Destructor of the PacketModule class
*/
PacketModule::~PacketModule()
{
}

/**
 * Helper method to check validity of the packet module based on size greater zero and correct type.
 */
bool PacketModule::isValid() const
{
	return ((m_moduleSize > 0) && (m_moduleType != Invalid));
}

/**
* Returns the type of the module
* @return	The module type
*/
PacketModule::PacketModuleType PacketModule::GetModuleType() const
{
	return m_moduleType;
}

/**
* Returns the size of the module
* @return The module size in bytes
*/
uint16_t PacketModule::GetModuleSize() const
{
	return m_moduleSize;
}