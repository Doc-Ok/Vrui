/***********************************************************************
EventDispatcher - Class to dispatch events from a central listener to
any number of interested clients.
Copyright (c) 2016-2019 Oliver Kreylos

This file is part of the Portable Threading Library (Threads).

The Portable Threading Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Portable Threading Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Threading Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef THREADS_EVENTDISPATCHER_INCLUDED
#define THREADS_EVENTDISPATCHER_INCLUDED

#include <sys/types.h>
#include <sys/time.h>
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <vector>
#include <Misc/PriorityHeap.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/Spinlock.h>

namespace Threads {

class EventDispatcher
	{
	/* Embedded classes: */
	public:
	typedef unsigned int ListenerKey; // Type for keys to uniquely identify registered event listeners
	
	enum IOEventType // Enumerated type for input/output event types
		{
		Read=0x01,Write=0x02,ReadWrite=0x03,Exception=0x04
		};
	
	class Time:public timeval // Class to specify time points or time intervals for timer events; microseconds are assumed in [0, 1000000) even if interval is negative
		{
		/* Constructors and destructors: */
		public:
		Time(void) // Dummy constructor
			{
			}
		Time(long sSec,long sUsec) // Initializes time interval
			{
			tv_sec=sSec;
			tv_usec=sUsec;
			}
		Time(const struct timeval& tv) // Constructor from base class
			{
			tv_sec=tv.tv_sec;
			tv_usec=tv.tv_usec;
			}
		Time(double sSeconds); // Initializes time interval from non-negative number of seconds
		
		/* Methods: */
		static Time now(void); // Returns the current wall-clock time as a time point
		bool operator==(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec==other.tv_sec&&tv_usec==other.tv_usec;
			}
		bool operator!=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec!=other.tv_sec||tv_usec!=other.tv_usec;
			}
		bool operator<=(const Time& other) const // Compares two time points or time intervals
			{
			return tv_sec<other.tv_sec||(tv_sec==other.tv_sec&&tv_usec<=other.tv_usec);
			}
		Time& operator+=(const Time& other) // Adds a time interval to a time point or another time interval
			{
			/* Add time components: */
			tv_sec+=other.tv_sec;
			tv_usec+=other.tv_usec;
			
			/* Handle overflow: */
			if(tv_usec>=1000000)
				{
				++tv_sec;
				tv_usec-=1000000;
				}
			return *this;
			}
		Time& operator-=(const Time& other) // Subtracts a time interval from a time point, or two time intervals, or two time points
			{
			/* Subtract time components: */
			tv_sec-=other.tv_sec;
			tv_usec-=other.tv_usec;
			
			/* Handle underflow: */
			if(tv_usec<0)
				{
				--tv_sec;
				tv_usec+=1000000;
				}
			return *this;
			}
		};
	
	typedef bool (*IOEventCallback)(ListenerKey eventKey,int eventTypeMask,void* userData); // Type for input/output event callback functions with a bit mask of events; if callback returns true, event listener will immediately be removed from dispatcher
	typedef bool (*TimerEventCallback)(ListenerKey eventKey,void* userData); // Type for timer event callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	typedef bool (*ProcessCallback)(ListenerKey processKey,void* userData); // Type for process callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	typedef bool (*SignalCallback)(ListenerKey signalKey,void* signalData,void* userData); // Type for signal event callback functions; if callback returns true, event listener will immediately be removed from dispatcher
	
	/* Helper functions to register class methods as callback functions: */
	template <class ClassParam,bool (ClassParam::*methodParam)(ListenerKey,int)>
	static bool wrapMethod(ListenerKey eventKey,int eventTypeMask,void* userData)
		{
		return (static_cast<ClassParam*>(userData)->*methodParam)(eventKey,eventTypeMask);
		}
	template <class ClassParam,bool (ClassParam::*methodParam)(ListenerKey)>
	static bool wrapMethod(ListenerKey eventKey,void* userData)
		{
		return (static_cast<ClassParam*>(userData)->*methodParam)(eventKey);
		}
	template <class ClassParam,bool (ClassParam::*methodParam)(ListenerKey,void*)>
	static bool wrapMethod(ListenerKey eventKey,void* signalData,void* userData)
		{
		return (static_cast<ClassParam*>(userData)->*methodParam)(eventKey,signalData);
		}
	
	private:
	struct IOEventListener; // Structure representing listeners that have registered interest in some input/output event(s)
	struct TimerEventListener; // Structure representing listeners that have registered interest in timer events
	class TimerEventListenerComp; // Helper class to compare timer event listener structures by next event time
	typedef Misc::PriorityHeap<TimerEventListener*,TimerEventListenerComp> TimerEventListenerHeap; // Type for heap of timer event listeners, ordered by next event time
	struct ProcessListener; // Structure representing listeners that are called after any event has been handled
	struct SignalListener; // Structure representing listeners that react to user-defined signals
	typedef Misc::HashTable<ListenerKey,SignalListener> SignalListenerMap; // Hash table mapping listener keys to signal listeners
	struct PipeMessage;
	
	/* Elements: */
	private:
	Spinlock pipeMutex; // Mutex protecting the self-pipe used to change the dispatcher's internal state or raise signals
	int pipeFds[2]; // A uni-directional unnamed pipe to trigger events internal to the dispatcher
	size_t numMessages; // Number of messages in the self-pipe read buffer
	PipeMessage* messages; // A buffer to read pipe messages from the self-pipe
	size_t messageReadSize; // Number of bytes read during previous call to readPipeMessages
	ListenerKey nextKey; // Next key to be assigned to an event listener
	std::vector<IOEventListener> ioEventListeners; // List of currently registered input/output event listeners
	TimerEventListenerHeap timerEventListeners; // Heap of currently registered timer event listeners, sorted by next event time
	std::vector<ProcessListener> processListeners; // List of currently registered process event listeners
	SignalListenerMap signalListeners; // Map of currently registered signal event listeners
	fd_set readFds,writeFds,exceptionFds; // Three sets of file descriptors waiting for reads, writes, and exceptions, respectively
	int numReadFds,numWriteFds,numExceptionFds; // Number of file descriptors in the three descriptor sets
	int maxFd; // Largest file descriptor set in any of the three descriptor sets
	bool hadBadFd; // Flag if the last invocation of dispatchNextEvent() tripped on a bad file descriptor
	Time dispatchTime; // Time point of current iteration of dispatchNextEvent() method
	
	/* Private methods: */
	ListenerKey getNextKey(void); // Returns a new listener key
	size_t readPipeMessages(void); // Reads messages from the self-pipe; returns number of complete messages read
	void writePipeMessage(const PipeMessage& pm,const char* methodName); // Writes a message to the self-pipe
	void updateFdSets(int fd,int oldEventMask,int newEventMask); // Updates the three descriptor sets based on the given file descriptor changing its interest mask
	
	/* Constructors and destructors: */
	public:
	EventDispatcher(void); // Creates an event dispatcher
	private:
	EventDispatcher(const EventDispatcher& source); // Prohibit copy constructor
	EventDispatcher& operator=(const EventDispatcher& source); // Prohibit assignment operator
	public:
	~EventDispatcher(void);
	
	/* Methods: */
	bool dispatchNextEvent(void); // Waits for the next event and dispatches it; returns false if the stop() method was called
	void dispatchEvents(void); // Waits for and dispatches events until stopped
	void interrupt(void); // Forces an invocation of dispatchNextEvent() to return with a true value
	void stop(void); // Forces an invocation of dispatchNextEvent() to return with a false value, or an invocation of dispatchEvents() to return
	void stopOnSignals(void); // Installs a signal handler that stops the event dispatcher when a SIGINT or SIGTERM occur
	const Time& getCurrentTime(void) const // Returns the time point of the current invocation of the dispatchNextEvent method, to be used to schedule timer events; can only be called from inside an event callback
		{
		return dispatchTime;
		}
	ListenerKey addIOEventListener(int eventFd,int eventTypeMask,IOEventCallback eventCallback,void* eventCallbackUserData); // Adds a new input/output event listener for the given file descriptor and event type mask; returns unique event listener key
	void setIOEventListenerEventTypeMask(ListenerKey listenerKey,int newEventTypeMask); // Changes the event type mask of the input/output event listener with the given listener key
	void setIOEventListenerEventTypeMaskFromCallback(ListenerKey listenerKey,int newEventTypeMask); // Ditto, but can only be called from inside an input/output event callback
	void removeIOEventListener(ListenerKey listenerKey); // Removes the input/output event listener with the given listener key
	ListenerKey addTimerEventListener(const Time& eventTime,const Time& eventInterval,TimerEventCallback eventCallback,void* eventCallbackUserData); // Adds a new timer event listener for the given event time and repeat interval; returns unique event listener key
	void removeTimerEventListener(ListenerKey listenerKey); // Removes the timer event listener with the given listener key
	ListenerKey addProcessListener(ProcessCallback eventCallback,void* eventCallbackUserData); // Adds a new process listener; returns unique event listener key
	void removeProcessListener(ListenerKey listenerKey); // Removes the process listener with the given listener key
	ListenerKey addSignalListener(SignalCallback eventCallback,void* eventCallbackUserData); // Adds a new signal listener; returns unique event listener key
	void removeSignalListener(ListenerKey listenerKey); // Removes the signal listener with the given listener key
	void signal(ListenerKey listenerKey,void* signalData); // Raises a signal with the given listener key and opaque data pointer
	};

}

#endif
