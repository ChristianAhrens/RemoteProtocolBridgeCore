/*
  ==============================================================================

    PacketModule.h
    Created: 23 Oct 2020 10:07:56am
    Author:  Christian Ahrens

  ==============================================================================
*/

#pragma once

#include <vector>


// **************************************************************
// class PacketModule 
// **************************************************************
/**
* A class to save the basic packet module information
*/
class PacketModule
{
public:
	typedef uint8_t	PacketModuleType;
	static constexpr PacketModuleType Invalid								= 0x00;
	static constexpr PacketModuleType WithTimestamp							= 0x51;
	static constexpr PacketModuleType WithoutTimestamp						= 0x01;
	static constexpr PacketModuleType CentroidPosition						= 0x02;
	static constexpr PacketModuleType TrackedPointPosition					= 0x06;
	static constexpr PacketModuleType OrientationQuaternion					= 0x03;
	static constexpr PacketModuleType OrientationEuler						= 0x04;
	static constexpr PacketModuleType CentroidAccelerationAndVelocity		= 0x20;
	static constexpr PacketModuleType TrackedPointAccelerationandVelocity	= 0x21;
	static constexpr PacketModuleType ZoneCollisionDetection				= 0x22;

public:
	PacketModule();
	PacketModule(std::vector<unsigned char>& data, int & readPos);
	virtual ~PacketModule();

	virtual void readData(std::vector<unsigned char>& data, int& readPos);

	PacketModuleType GetModuleType() const;
	uint16_t GetModuleSize() const;

	virtual bool isValid() const;

private:
	PacketModuleType	m_moduleType{ Invalid };	//	Type of the packet module
	uint16_t			m_moduleSize{ 0 };	//	Size of the module
};