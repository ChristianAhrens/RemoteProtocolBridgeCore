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


// **************************************************************
// class CentroidAccelAndVeloModule
// **************************************************************
/**
* A class to sort the information from RTTrPM - centroid with acceleration and velocity module
*
*/
class CentroidAccelAndVeloModule : public PacketModule
{
public:
	CentroidAccelAndVeloModule(std::vector<unsigned char>& data, int& readPos);
	~CentroidAccelAndVeloModule() override;

	void readData(std::vector<unsigned char>& data, int& readPos) override;

	double GetXCoordinate() const;
	double GetYCoordinate() const;
	double GetZCoordinate() const;
	float GetXAcceleration() const;
	float GetYAcceleration() const;
	float GetZAcceleration() const;
	float GetXVelocity() const;
	float GetYVelocity() const;
	float GetZVelocity() const;

	bool isValid() const override;

private:
	double		m_coordinateX{ 0 };
	double		m_coordinateY{ 0 };
	double		m_coordinateZ{ 0 };
	float		m_accelerationX{ 0 };
	float		m_accelerationY{ 0 };
	float		m_accelerationZ{ 0 };
	float		m_velocityX{ 0 };
	float		m_velocityY{ 0 };
	float		m_velocityZ{ 0 };
};