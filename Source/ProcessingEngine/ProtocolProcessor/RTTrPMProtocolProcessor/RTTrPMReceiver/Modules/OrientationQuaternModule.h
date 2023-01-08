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


// **************************************************************
// class OrientationQuaternionModule
// **************************************************************
/**
* A class to sort the information from RTTrPM - quaternion orientation module
*
*/
class OrientationQuaternionModule : public PacketModule
{
public:
	OrientationQuaternionModule(std::vector<unsigned char>& data, int& readPos);
	~OrientationQuaternionModule() override;

	void readData(std::vector<unsigned char>& data, int& readPos) override;

	uint16_t GetLatency() const;
	double GetQx() const;
	double GetQy() const;
	double GetQz() const;
	double GetQw() const;

	bool isValid() const override;

private:
	uint16_t	m_latency{ 0 };
	double		m_Qx{ 0 };
	double		m_Qy{ 0 };
	double		m_Qz{ 0 };
	double		m_Qw{ 0 };
};
