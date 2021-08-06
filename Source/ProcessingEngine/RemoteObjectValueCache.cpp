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

#include "RemoteObjectValueCache.h"


// **************************************************************************************
//    class RemoteObjectValueCache
// **************************************************************************************

/**
 * Constructor of class RemoteObjectValueCache.
 */
RemoteObjectValueCache::RemoteObjectValueCache()
{
}

/**
 * Destructor
 */
RemoteObjectValueCache::~RemoteObjectValueCache()
{
}

/**
 * Gets the int value cached for a given remote object if available.
 * @param	ro	The remote object to get the cached int value for
 * @return	The cached int value if available, 0 if not available
 */
int RemoteObjectValueCache::GetIntValue(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
	{
		if (m_cachedValues.at(ro)._valType == ROVT_INT)
			return *static_cast<int*>(m_cachedValues.at(ro)._payload);
		else
			jassertfalse;
	}

	return 0;
}

/**
 * Gets the float value cached for a given remote object if available.
 * @param	ro	The remote object to get the cached float value for
 * @return	The cached float value if available, 0 if not available
 */
float RemoteObjectValueCache::GetFloatValue(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
	{
		if (m_cachedValues.at(ro)._valType == ROVT_FLOAT)
			return *static_cast<float*>(m_cachedValues.at(ro)._payload);
		else
			jassertfalse;
	}

	return 0.0f;
}

/**
 * Gets the string data cached for a given remote object if available.
 * @param	ro	The remote object to get the cached string data for
 * @return	The cached string data if available, empty if not available
 */
std::string RemoteObjectValueCache::GetStringValue(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
	{
		if (m_cachedValues.at(ro)._valType == ROVT_STRING)
			return std::string(static_cast<char*>(m_cachedValues.at(ro)._payload), m_cachedValues.at(ro)._payloadSize);
		else
			jassertfalse;
	}

	return "";
}

/**
 * Sets the data value for a given remote object in the cache map
 * @param	ro			The remote object to set the value for
 * @param	valueData	The value to set
 */
void RemoteObjectValueCache::SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData)
{
	m_cachedValues[ro] = valueData;
}
