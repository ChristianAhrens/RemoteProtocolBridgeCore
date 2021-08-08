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

#include "TimerThreadBase.h"

// **************************************************************************************
//    class TimerThreadBase
// **************************************************************************************
/**
 * Constructor
 */
TimerThreadBase::TimerThreadBase()
	: Thread("TimerThreadBase_Thread")
{
}

/**
 * Destructor
 */
TimerThreadBase::~TimerThreadBase()
{
}

/**
 * Helper method to start the internal timer thread.
 * This checks, if the thread is already running and if yes, stops it first.
 * @param callbackInterval  The interval at which the new thread shall call the given callbackfunction.
 */
void TimerThreadBase::startTimerThread(int callbackInterval, int initialCallbackOffset)
{
    if (isThreadRunning())
        stopTimerThread();

    m_callbackInterval = callbackInterval;
    m_initialCallbackOffset = initialCallbackOffset;

    startThread();
}

/**
 * Helper method to stop the internal timer thread.
 * This simply calls the stoptread function of thread with 10ms timeout.
 */
void TimerThreadBase::stopTimerThread()
{
    stopThread(2 * m_callbackInterval);
}

/**
 * Helper method to get the internal timer thread running state.
 * @return  True if the juce::Thread is running, false if not.
 */
bool TimerThreadBase::isTimerThreadRunning()
{
    return isThreadRunning();
}

/**
 * Main thread loop reimplementation from JUCE thread.
 * This handles calling the pure virtual callback function calling in the given time intervals
 */
void TimerThreadBase::run()
{
    Thread::sleep(m_initialCallbackOffset);

    while (!threadShouldExit())
    {
        // execute the timed callback and measure the time it takes
        auto callbackStartTime = Time::getMillisecondCounterHiRes() * 1000.0f;
        timerThreadCallback();
        auto callbackStopTime = Time::getMillisecondCounterHiRes() * 1000.0f;
        auto callbackDuration = callbackStopTime - callbackStartTime;

        // calculate the remaining time that needs to be spent until next callback
        auto remainingInterval = (m_callbackInterval * 1000) - callbackDuration;
        if (remainingInterval < 0)
        {
            // we are in an overload situation, the callback takes more time than the configured interval allows
            jassertfalse;
            remainingInterval = 0;
        }
        auto remainingIntervalMs = static_cast<int>(0.001f * remainingInterval);
        if (remainingIntervalMs >= m_callbackInterval)
        {
            // we are in an undefined state, must not happen!
            jassertfalse;
            remainingIntervalMs = 0;
        }

        auto threadExitCheckInterval = 25;
        auto remainingThreadExitCheckTimeFragments = remainingIntervalMs / threadExitCheckInterval;
        auto finalTimeFragmentMs = remainingIntervalMs % threadExitCheckInterval;

        while (remainingThreadExitCheckTimeFragments > 0)
        {
            // To avoid overly long blocking, even though the thread is 
            // already scheduled for termination, 
            // check here if the thread shall be terminated 
            if (threadShouldExit())
                break;

            // Spend another time fragment before continuing to next loop iteration and recheck for thread termination.
            Thread::sleep(threadExitCheckInterval);

            remainingThreadExitCheckTimeFragments--;
        }

        // Spend the remaining time fragment.
        Thread::sleep(finalTimeFragmentMs);
    }
}