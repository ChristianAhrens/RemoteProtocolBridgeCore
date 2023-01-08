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

#include "OrientationEulerModule.h"


/**
* Constructor of the class OrientationEulerModule
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
OrientationEulerModule::OrientationEulerModule(std::vector<unsigned char>& data, int& readPos)
	: PacketModule(data, readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void OrientationEulerModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 2, (unsigned char*)&m_latency);
	readIter += 2;

	std::copy(readIter, readIter + 2, (unsigned char*)&m_order);
	readIter += 2;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_R1);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_R2);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_R3);
	readIter += 8;

	readPos += (2 + 2 + (3 * 8));
}

/**
* Destructor of the OrientationEulerModule class
*/
OrientationEulerModule::~OrientationEulerModule()
{
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool OrientationEulerModule::isValid() const
{
	return (PacketModule::isValid() && (GetModuleType() == OrientationEuler));
}

/**
* Returns the latency.
* Approximate time in milliseconds since last measurement, if equal to 0xFFFF, overflow
* @return The latency value
*/
uint16_t OrientationEulerModule::GetLatency() const
{
	return m_latency;
}

/**
* Returns the Euler Angle Order
* @return The Euler Angle Order value
*/
OrientationEulerModule::EulerOrder OrientationEulerModule::GetOrder() const
{
	return m_order;
}

/**
* Returns the rotation in Rad along first axis
* @return The rotation in Rad along first axis
*/
double OrientationEulerModule::GetR1() const
{
	return m_R1;
}

/**
* Returns the rotation in Rad along second axis
* @return The rotation in Rad along second axis
*/
double OrientationEulerModule::GetR2() const
{
	return m_R2;
}

/**
* Returns the rotation in Rad along third axis
* @return The rotation in Rad along third axis
*/
double OrientationEulerModule::GetR3() const
{
	return m_R3;
}
