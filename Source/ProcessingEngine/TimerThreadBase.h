/*
  ==============================================================================

    TimerThreadBase.h
    Created: 23 Dec 2020 9:56:34am
    Author:  Christian Ahrens

  ==============================================================================
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