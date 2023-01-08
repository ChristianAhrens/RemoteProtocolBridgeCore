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

#include "PacketModule.h"

#include <vector>
#include <string>

// **************************************************************
// class PacketModuleTrackable 
// **************************************************************
/**
* A class to save the trackable packet module information
*/
class PacketModuleTrackable : public PacketModule
{
public:
	PacketModuleTrackable(std::vector<unsigned char>& data, int & readPos);
	~PacketModuleTrackable() override;

	void readData(std::vector<unsigned char>& data, int& readPos) override;

	std::string GetName() const;
	uint32_t GetSeqNumber() const;
	uint8_t GetNumberOfSubModules() const;

	bool isValid() const override;

private:
	uint8_t		m_nameLength{ 0 };
	std::string	m_name;
	uint32_t	m_seqNumber{ 0 };
	uint8_t		m_numberOfSubModules{ 0 };
};