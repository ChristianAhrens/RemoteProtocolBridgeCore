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

#ifdef DEBUG
#include "ProcessingEngineConfig.h"
#endif


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

	DBG(String(__FUNCTION__) + " no ROVT_INT value available for requested RO");
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

	DBG(String(__FUNCTION__) + " no ROVT_FLOAT value available for requested RO");
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

	DBG(String(__FUNCTION__) + " no ROVT_STRING value available for requested RO");
	return "";
}

/**
 * Sets the data value for a given remote object in the cache map
 * @param	ro			The remote object to set the value for
 * @param	valueData	The value to set
 */
void RemoteObjectValueCache::SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData)
{
	m_cachedValues[ro].payloadCopy(valueData);

#ifdef DEBUG
	DbgPrintCacheContent();
#endif
}

#ifdef DEBUG
void RemoteObjectValueCache::DbgPrintCacheContent()
{
	for (auto const& val : m_cachedValues)
	{
		auto valString = String();
		switch (val.second._valType)
		{
		case ROVT_INT:
			{
				auto p = static_cast<int*>(val.second._payload);
				for (auto i = 0; i < val.second._valCount; i++)
					valString = String(*(p + i)) + ";";
			}
			break;
		case ROVT_FLOAT:
			{
			auto p = static_cast<float*>(val.second._payload);
			for (auto i = 0; i < val.second._valCount; i++)
				valString = String(*(p + i)) + ";";
			}
			break;
		default:
			break;
		}
		DBG(ProcessingEngineConfig::GetObjectShortDescription(val.first._Id)
			+ " (" + String(val.second._addrVal._first) + ":" + String(val.second._addrVal._second) + ") "
			+ valString);
	}
}
#endif
