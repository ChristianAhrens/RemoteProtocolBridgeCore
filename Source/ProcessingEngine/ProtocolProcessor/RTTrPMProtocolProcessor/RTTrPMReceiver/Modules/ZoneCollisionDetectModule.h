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

#pragma once

#include "PacketModule.h"

#include <vector>
#include <string>
#include <memory>


// **************************************************************
// class ZoneObjectSubModule
// **************************************************************
/**
* A class to sort the information from RTTrPM - ZoneObject sub-module
*
*/
class ZoneObjectSubModule : public PacketModule
{
public:
	ZoneObjectSubModule(std::vector<unsigned char>& data, int& readPos);
	~ZoneObjectSubModule() override;

	void readData(std::vector<unsigned char>& data, int& readPos) override;

	uint8_t GetSize() const;
	std::string GetName() const;

	bool isValid() const override;

private:
	uint8_t		m_size{ 0 };
	uint8_t		m_nameLength{ 0 };
	std::string m_name;
};


// **************************************************************
// class ZoneCollisionDetectionModule
// **************************************************************
/**
* A class to sort the information from RTTrPM - Zone collision detection module
*
*/
class ZoneCollisionDetectionModule : public PacketModule
{
public:
	ZoneCollisionDetectionModule(std::vector<unsigned char>& data, int& readPos);
	~ZoneCollisionDetectionModule();

	void readData(std::vector<unsigned char>& data, int& readPos) override;

	const std::vector<std::unique_ptr<ZoneObjectSubModule>>& GetZoneSubModules() const;

	bool isValid() const override;

private:
	uint8_t		m_numberOfZoneSubModules{ 0 };
	std::vector<std::unique_ptr<ZoneObjectSubModule>>	m_zoneObjectSubModules;
};