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

#include "RTTrPMHeader.h"


/**
* Default constructor of the class RTTrPMHeader.
*/
RTTrPMHeader::RTTrPMHeader()
{
}

/**
* Constructor of the class RTTrPMHeader.
*
* @param	data	Input byte data as vector reference.
* @param	readPos	Reference variable which helps to know from which bytes the next modul read should beginn
*/
RTTrPMHeader::RTTrPMHeader(std::vector<unsigned char>& data, int& readPos)
{
	readData(data, readPos);
}

/**
 * Helper method to parse the input data vector starting at given read position.
 * @param data	The input data in a byte vector.
 * @param readPos	The position in the given vector where reading shall be started.
 */
void RTTrPMHeader::readData(std::vector<unsigned char>& data, int& readPos)
{
	auto readIter = data.begin() + readPos;

	std::copy(readIter, readIter + 2, (unsigned char*)&m_intSignature);
	readIter += 2;
	std::copy(readIter, readIter + 2, (unsigned char*)&m_floatSignature);
	readIter += 2;

	readPos += 4;

	if((m_intSignature == BigEndianInt) && (m_floatSignature == BigEndianFloat))
	{
		std::copy(readIter, readIter + 2, (std::uint16_t*)&m_version);
		readIter += 2;
		std::copy(readIter, readIter + 4, (std::uint32_t*)&m_packetID);	   
		readIter += 4;
		std::copy(readIter, readIter + 1, (std::uint8_t*)&m_packetFormat);   
		readIter += 1;
		std::copy(readIter, readIter + 2, (std::uint16_t*)&m_packetSize);  
		readIter += 2;
		std::copy(readIter, readIter + 4, (std::uint32_t*)&m_context);	   
		readIter += 4;
		std::copy(readIter, readIter + 1, (std::uint8_t*)&m_numModules);	   
		readIter += 1;

		readPos += 14;
	}
}

/**
* Destructor of the class RTTrPMHeader
*/
RTTrPMHeader::~RTTrPMHeader()
{
}

/**
* Method that returns the int signature bytes read from input data
*
* @return	Integer signature of header
*/
RTTrPMHeader::PacketModuleSignature RTTrPMHeader::GetIntSignature() const
{
	return m_intSignature;
}

/**
* Method that returns the float signature bytes read from input data
*
* @return	Float signature of header
*/
RTTrPMHeader::PacketModuleSignature RTTrPMHeader::GetFloatSignature() const
{
	return m_floatSignature;
}

/**
* Method that returns the header version (0x0002)
*
* @return	Header version
*/
std::uint16_t RTTrPMHeader::GetVersion() const
{
	return m_version;
}

/**
* Method that returns the id of the received packet
*
* @return	Id of the packet
*/
std::uint32_t RTTrPMHeader::GetPacketID() const
{
	return m_packetID;
}

/**
* Method that returns the format of the packet
*
* @return	Format of the packet
*/
RTTrPMHeader::PacketModuleFormat RTTrPMHeader::GetPacketFormat() const
{
	return m_packetFormat;
}

/**
* Method that returns the size of the packet
*
* @return	Size of the packet
*/
std::uint16_t RTTrPMHeader::GetPacketSize() const
{
	return m_packetSize;
}

/**
* Method that returns the header context.
* The context is user definable
*
* @return	User context
*/
std::uint32_t RTTrPMHeader::GetContext() const
{
	return m_context;
}

/**
* Method that returns the number of trackable modules
*
* @return	Number of trackable modules
*/
std::uint8_t RTTrPMHeader::GetNumberOfModules() const
{ 
	return m_numModules; 
}
