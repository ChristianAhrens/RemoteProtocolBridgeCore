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

#pragma once

#include <JuceHeader.h>

/**
 * Class TimerThreadBase is a class for running a thread to call a given callback function at defined time intervals.
 * The callback function reimplementation has to ensure itself that it meets all threadsafety requirements.
 */
class TimerThreadBase :    private Thread
{
public:
	TimerThreadBase();
	~TimerThreadBase();

	void startTimerThread(int callbackInterval, int initialCallbackOffset = 0);
	void stopTimerThread();
	bool isTimerThreadRunning();

	//==============================================================================
	void run() override;
	
protected:
	//==============================================================================
	virtual void timerThreadCallback() = 0;

private:
	int	m_callbackInterval{ 100 };		/**< The interval at which the callback function shall be called from running thread. */
	int	m_initialCallbackOffset{ 0 };	/**< The initial delay in ms before first calling the callback function. Use e.g. if something that requires some time needs to be set up first before the callback thread becomes active. */

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimerThreadBase)
};