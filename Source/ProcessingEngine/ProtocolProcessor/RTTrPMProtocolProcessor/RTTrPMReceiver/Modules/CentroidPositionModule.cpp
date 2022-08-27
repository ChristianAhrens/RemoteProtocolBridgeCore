/* Copyright (c) 2020-2022, Christian Ahrens
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

#include "CentroidPositionModule.h"


/**
* Constructor of the class CentroidPositionModule
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
CentroidPositionModule::CentroidPositionModule(std::vector<unsigned char>& data, int& readPos)
	: PacketModule(data, readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void CentroidPositionModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 2, (unsigned char*)&m_latency);
	readIter += 2;

	std::copy(readIter, readIter + 8, (unsigned char *)&m_coordinateX);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char *)&m_coordinateY);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char *)&m_coordinateZ);
	readIter += 8;

	readPos += (2 + (3 * 8));
}

/**
* Destructor of the CentroidPositionModule class
*/
CentroidPositionModule::~CentroidPositionModule()
{
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool CentroidPositionModule::isValid() const
{
	return (PacketModule::isValid() && (GetModuleType() == CentroidPosition));
}

/**
* Returns the latency.
* Approximate time in milliseconds since last measurement, if equal to 0xFFFF, overflow
* @return The latency value
*/
uint16_t CentroidPositionModule::GetLatency() const
{
	return m_latency;
}

/**
* Returns the X coordinate
* @return The x coordinate value
*/
double CentroidPositionModule::GetX() const
{ 
	return m_coordinateX; 
}

/**
* Returns the Y coordinate
* @return The y coordinate value
*/
double CentroidPositionModule::GetY() const
{
	return m_coordinateY; 
}

/**
* Returns the Z coordinate
* @return The z coordinate value
*/
double CentroidPositionModule::GetZ() const
{ 
	return m_coordinateZ; 
}