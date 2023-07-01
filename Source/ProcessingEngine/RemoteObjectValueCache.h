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

#include "../RemoteProtocolBridgeCommon.h"

/**
 * 
 */
class RemoteObjectValueCache
{
public:
	RemoteObjectValueCache();
	~RemoteObjectValueCache();

	bool Contains(const RemoteObject& ro) const;

	int GetIntValue(const RemoteObject& ro) const;
	float GetFloatValue(const RemoteObject& ro) const;
	std::tuple<float, float> GetDualFloatValues(const RemoteObject& ro) const;
	std::tuple<float, float, float> GetTripleFloatValues(const RemoteObject& ro) const;
	std::string GetStringValue(const RemoteObject& ro) const;

	void SetValue(const RemoteObject& ro, const RemoteObjectMessageData& valueData);
	const RemoteObjectMessageData& GetValue(const RemoteObject& ro);

private:
#ifdef DEBUG
	void DbgPrintCacheContent();
#endif

	std::map<RemoteObject, RemoteObjectMessageData> m_cachedValues;

};
