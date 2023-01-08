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

#include "OrientationQuaternModule.h"


/**
* Constructor of the class OrientationQuaternionModule
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
OrientationQuaternionModule::OrientationQuaternionModule(std::vector<unsigned char>& data, int& readPos)
	: PacketModule(data, readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void OrientationQuaternionModule::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 2, (unsigned char*)&m_latency);
	readIter += 2;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_Qx);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_Qy);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_Qz);
	readIter += 8;

	std::copy(readIter, readIter + 8, (unsigned char*)&m_Qw);
	readIter += 8;

	readPos += (2 + (4 * 8));
}

/**
* Destructor of the OrientationQuaternionModule class
*/
OrientationQuaternionModule::~OrientationQuaternionModule()
{
}

/**
 * Reimplemented helper method to check validity of the packet module based on size greater zero and correct type.
 * @return True if valid, false if not
 */
bool OrientationQuaternionModule::isValid() const
{
	return (PacketModule::isValid() && (GetModuleType() == OrientationQuaternion));
}

/**
* Returns the latency.
* Approximate time in milliseconds since last measurement, if equal to 0xFFFF, overflow
* @return The latency value
*/
uint16_t OrientationQuaternionModule::GetLatency() const
{
	return m_latency;
}

/**
* Returns the X component of tracked object using quaternions
* @return The x component value
*/
double OrientationQuaternionModule::GetQx() const
{
	return m_Qx;
}

/**
* Returns the Y component of tracked object using quaternions
* @return The y component value
*/
double OrientationQuaternionModule::GetQy() const
{
	return m_Qy;
}

/**
* Returns the Z component of tracked object using quaternions
* @return The z component value
*/
double OrientationQuaternionModule::GetQz() const
{
	return m_Qz;
}

/**
* Returns the Complex component of tracked object using quaternions
* @return The complex component value
*/
double OrientationQuaternionModule::GetQw() const
{
	return m_Qw;
}
