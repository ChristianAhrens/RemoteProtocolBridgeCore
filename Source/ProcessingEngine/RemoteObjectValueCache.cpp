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
 * Clears the internal value cache map. No questions asked.
 */
void RemoteObjectValueCache::Clear()
{
	m_cachedValues.clear();
}

/**
 * Checks if the given remote object is already present in the cache.
 * @param	ro	The remote object to test for existance in cache
 * @return	True if the given object is available in the cache
 */
bool RemoteObjectValueCache::Contains(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
		return true;

	DBG(String(__FUNCTION__) + " no value available for requested RO");
	return false;
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
		if (m_cachedValues.at(ro)._valType == ROVT_INT && m_cachedValues.at(ro)._valCount == 1 && m_cachedValues.at(ro)._payloadSize == sizeof(int))
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
		if (m_cachedValues.at(ro)._valType == ROVT_FLOAT && m_cachedValues.at(ro)._valCount == 1 && m_cachedValues.at(ro)._payloadSize == sizeof(float))
			return *static_cast<float*>(m_cachedValues.at(ro)._payload);
		else
			jassertfalse;
	}

	DBG(String(__FUNCTION__) + " no ROVT_FLOAT value available for requested RO");
	return 0.0f;
}

/**
 * Gets the two float values cached for a given remote object if available.
 * @param	ro	The remote object to get the cached float value for
 * @return	The cached dual float values if available, 0 if not available
 */
std::tuple<float, float> RemoteObjectValueCache::GetDualFloatValues(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
	{
		auto plptr = static_cast<float*>(m_cachedValues.at(ro)._payload);
		if (m_cachedValues.at(ro)._valType == ROVT_FLOAT && m_cachedValues.at(ro)._valCount == 2 && m_cachedValues.at(ro)._payloadSize == 2 * sizeof(float))
			return { plptr[0], plptr[1] };
		else
			jassertfalse;
	}

	DBG(String(__FUNCTION__) + " no two ROVT_FLOAT values available for requested RO");
	return { 0.0f, 0.0f };
}

/**
 * Gets the three float values cached for a given remote object if available.
 * @param	ro	The remote object to get the cached float value for
 * @return	The cached float values if available, 0 if not available
 */
std::tuple<float, float, float> RemoteObjectValueCache::GetTripleFloatValues(const RemoteObject& ro) const
{
	if (m_cachedValues.count(ro) != 0)
	{
		auto plptr = static_cast<float*>(m_cachedValues.at(ro)._payload);
		if (m_cachedValues.at(ro)._valType == ROVT_FLOAT && m_cachedValues.at(ro)._valCount == 3 && m_cachedValues.at(ro)._payloadSize == 3 * sizeof(float))
			return { plptr[0], plptr[1], plptr[2] };
		else
			jassertfalse;
	}

	DBG(String(__FUNCTION__) + " no three ROVT_FLOAT values available for requested RO");
	return { 0.0f, 0.0f, 0.0f };
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

//#ifdef DEBUG
//	DbgPrintCacheContent();
//#endif

#ifdef DEBUG
	auto val = m_cachedValues[ro];
	auto valString = juce::String();

	switch (val._valType)
	{
	case ROVT_FLOAT:
	{
		jassert(val._valCount * sizeof(float) == val._payloadSize);
		auto p = static_cast<float*>(val._payload);
		for (auto i = 0; i < val._valCount; i++)
			valString += String(*(p + i)) + ";";
	}
	break;
	default:
		break;
	}

	DBG(juce::String(__FUNCTION__) << " " << ro._Id << " (" << static_cast<int>(ro._Addr._first) << "," << static_cast<int>(ro._Addr._second) << ") " << valString);
#endif
}

/**
 * Gets the data value for a given remote object in the cache map
 * @param	ro			The remote object to set the value for
 * @return	The value to get
 */
const RemoteObjectMessageData& RemoteObjectValueCache::GetValue(const RemoteObject& ro)
{
	if (!Contains(ro))
		m_cachedValues[ro] = RemoteObjectMessageData(ro._Addr, ROVT_NONE, 0, nullptr, 0);

	#ifdef DEBUG
		auto val = m_cachedValues[ro];
		auto valString = juce::String();

		switch (val._valType)
		{
		case ROVT_FLOAT:
		{
			jassert(val._valCount * sizeof(float) == val._payloadSize);
			auto p = static_cast<float*>(val._payload);
			for (auto i = 0; i < val._valCount; i++)
				valString += String(*(p + i)) + ";";
		}
		break;
		default:
			break;
		}

		DBG(juce::String(__FUNCTION__) << " " << ro._Id << " (" << static_cast<int>(ro._Addr._first) << "," << static_cast<int>(ro._Addr._second) << ") " << valString);
	#endif

	return m_cachedValues[ro];
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
					valString += String(*(p + i)) + ";";
			}
			break;
		case ROVT_FLOAT:
			{
			auto p = static_cast<float*>(val.second._payload);
			for (auto i = 0; i < val.second._valCount; i++)
				valString += String(*(p + i)) + ";";
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
