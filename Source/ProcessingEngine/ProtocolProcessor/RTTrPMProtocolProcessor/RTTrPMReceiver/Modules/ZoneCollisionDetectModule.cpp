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

#include "ZoneCollisionDetectModule.h"


/**
* Constructor of the class ZoneCollisionDetectionModule
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
ZoneCollisionDetectionModule::ZoneCollisionDetectionModule(std::vector<unsigned char>& data, int& readPos)
	: PacketModule(data, readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void ZoneCollisionDetectionModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 1, (unsigned char*)&m_numberOfZoneSubModules);
	readIter += 1;

	readPos += 1;

	for (int i = 1; i < m_numberOfZoneSubModules; ++i) // we start at 1 because the read num of submods includes this base module
	{
		m_zoneObjectSubModules.push_back(std::make_unique<ZoneObjectSubModule>(data, readPos));
	}
}

/**
* Destructor of the ZoneCollisionDetectionModule class
*/
ZoneCollisionDetectionModule::~ZoneCollisionDetectionModule()
{
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool ZoneCollisionDetectionModule::isValid() const
{
	return (PacketModule::isValid() && (GetModuleType() == ZoneCollisionDetection));
}

/**
* Returns the total number of Zones Sub-Modules including this packet.
* @return The total number of zon sub-modules
*/
const std::vector<std::unique_ptr<ZoneObjectSubModule>>& ZoneCollisionDetectionModule::GetZoneSubModules() const
{
	return m_zoneObjectSubModules;
}


/**
* Constructor of the class ZoneObjectSubModule
* @param	data	: keeps the caught data information.
*/
ZoneObjectSubModule::ZoneObjectSubModule(std::vector<unsigned char>& data, int& readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void ZoneObjectSubModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 1, (unsigned char*)&m_size);
	readIter += 1;

	std::copy(readIter, readIter + 1, (unsigned char*)&m_nameLength);
	readIter += 1;

	m_name = std::string((const char*)(&(*readIter)), m_nameLength);
	readIter += m_nameLength;

	readPos += (1 + 1 + m_nameLength);
}

/**
* Destructor of the ZoneObjectSubModule class
*/
ZoneObjectSubModule::~ZoneObjectSubModule()
{
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool ZoneObjectSubModule::isValid() const
{
	return (m_nameLength == m_name.length());
}

/**
* Returns the size of the Sub-Module including the name and size.
* @return The size of the sub-module
*/
uint8_t ZoneObjectSubModule::GetSize() const
{
	return m_size;
}

/**
* Returns the name of the zone itself.
* @return The name of the zone.
*/
std::string ZoneObjectSubModule::GetName() const
{
	return m_name;
}