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

#include <Threads/EventDispatcher.h>

#include <math.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>

namespace Threads {

namespace {

/****************
Helper functions:
****************/

EventDispatcher* stopDispatcher=0; // Event dispatcher to be stopped when a SIGINT or SIGTERM occur

void stopSignalHandler(int signum)
	{
	/* Stop the dispatcher: */
	if(stopDispatcher!=0&&(signum==SIGINT||signum==SIGTERM))
		stopDispatcher->stop();
	}

}

/*****************************************
Embedded classes of class EventDispatcher:
*****************************************/

struct EventDispatcher::IOEventListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	int fd; // File descriptor belonging to the event
	int typeMask; // Mask of event types (read, write, exception) in which the listener is interested
	IOEventCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	IOEventListener(ListenerKey sKey,int sFd,int sTypeMask,IOEventCallback sCallback,void* sCallbackUserData)
		:key(sKey),fd(sFd),typeMask(sTypeMask),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

struct EventDispatcher::TimerEventListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	Time time; // Time point at which to trigger the event
	Time interval; // Event interval for recurring events; one-shot events need to return true from callback to cancel further events
	TimerEventCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	TimerEventListener(ListenerKey sKey,const Time& sTime,const Time& sInterval,TimerEventCallback sCallback,void* sCallbackUserData)
		:key(sKey),time(sTime),interval(sInterval),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

class EventDispatcher::TimerEventListenerComp
	{
	/* Methods: */
	public:
	static bool lessEqual(const TimerEventListener* v1,const TimerEventListener* v2)
		{
		return v1->time<=v2->time;
		}
	};

struct EventDispatcher::ProcessListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key identifying this event
	ProcessCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	ProcessListener(ListenerKey sKey,ProcessCallback sCallback,void* sCallbackUserData)
		:key(sKey),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

struct EventDispatcher::SignalListener
	{
	/* Elements: */
	public:
	ListenerKey key; // Unique key for identifying this event
	SignalCallback callback; // Function called when an event occurs
	void* callbackUserData; // Opaque pointer to be passed to callback function
	
	/* Constructors and destructors: */
	SignalListener(ListenerKey sKey,SignalCallback sCallback,void* sCallbackUserData)
		:key(sKey),callback(sCallback),callbackUserData(sCallbackUserData)
		{
		}
	};

struct EventDispatcher::PipeMessage // Type for messages sent on an EventDispatcher's self-pipe
	{
	/* Embedded classes: */
	public:
	enum MessageType // Enumerated type for types of messages
		{
		INTERRUPT=0,
		STOP,
		ADD_IO_LISTENER,
		SET_IO_LISTENER_TYPEMASK,
		REMOVE_IO_LISTENER,
		ADD_TIMER_LISTENER,
		REMOVE_TIMER_LISTENER,
		ADD_PROCESS_LISTENER,
		REMOVE_PROCESS_LISTENER,
		ADD_SIGNAL_LISTENER,
		REMOVE_SIGNAL_LISTENER,
		SIGNAL,
		NUM_MESSAGETYPES
		};
	
	/* Elements: */
	int messageType; // Type of message
	union
		{
		struct
			{
			ListenerKey key;
			int fd;
			int typeMask;
			IOEventCallback callback;
			void* callbackUserData;
			} addIOListener; // Message to add a new input/output event listener to the event dispatcher
		struct
			{
			ListenerKey key;
			int newTypeMask;
			} setIOListenerEventTypeMask; // Message to change the event type mask of an input/output event listener
		ListenerKey removeIOListener; // Message to remove an input/output event listener from the event dispatcher
		struct
			{
			ListenerKey key;
			struct timeval time;
			struct timeval interval;
			TimerEventCallback callback;
			void* callbackUserData;
			} addTimerListener; // Message to add a new timer event listener to the event dispatcher
		ListenerKey removeTimerListener; // Message to remove a timer event listener from the event dispatcher
		struct
			{
			ListenerKey key;
			ProcessCallback callback;
			void* callbackUserData;
			} addProcessListener; // Message to add a new process listener to the event dispatcher
		ListenerKey removeProcessListener; // Message to remove a process listener from the event dispatcher
		struct
			{
			ListenerKey key;
			SignalCallback callback;
			void* callbackUserData;
			} addSignalListener; // Message to add a new signal listener to the event dispatcher
		ListenerKey removeSignalListener; // Message to remove a signal listener from the event dispatcher
		struct
			{
			ListenerKey key;
			void* signalData;
			} signal; // Message to raise a signal
		};
	};

/**************************************
Methods of class EventDispatcher::Time:
**************************************/

EventDispatcher::Time::Time(double sSeconds)
	{
	/* Take integer and fractional parts of given time, ensuring that tv_usec>=0: */
	tv_sec=long(floor(sSeconds));
	tv_usec=long(floor((sSeconds-double(tv_sec))*1.0e6+0.5));
	
	/* Check for rounding: */
	if(tv_usec>=1000000)
		{
		++tv_sec;
		tv_usec=0;
		}
	}

EventDispatcher::Time EventDispatcher::Time::now(void)
	{
	Time result;
	gettimeofday(&result,0);
	return result;
	}

/********************************
Methods of class EventDispatcher:
********************************/

EventDispatcher::ListenerKey EventDispatcher::getNextKey(void)
	{
	/* Lock the self-pipe (mutex does double duty for key increment): */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Increment the next key: */
	do
		{
		++nextKey;
		}
	while(nextKey==0);
	
	/* Return the new key: */
	return nextKey;
	}

size_t EventDispatcher::readPipeMessages(void)
	{
	/* Check if there was a partial message during the previous call: */
	char* messageReadPtr=reinterpret_cast<char*>(messages);
	size_t readSize=numMessages*sizeof(PipeMessage);
	size_t partialSize=messageReadSize%sizeof(PipeMessage);
	if(partialSize!=0)
		{
		// DEBUGGING
		Misc::logNote("Threads::EventDispatcher::readPipeMessages: Partial read during last call");
		
		/* Move the partial data to the front of the buffer: */
		memcpy(messageReadPtr,messageReadPtr+messageReadSize-partialSize,partialSize);
		messageReadPtr+=partialSize;
		readSize-=partialSize;
		}
	
	/* Read up to the given number of messages: */
	ssize_t readResult=read(pipeFds[0],messageReadPtr,readSize);
	
	/* Check for errors: */
	if(readResult<=0)
		{
		if(readResult<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))
			Misc::logWarning("Threads::EventDispatcher::readPipeMessages: No data to read");
		else
			Misc::throwStdErr("Threads::EventDispatcher::readPipeMessages: Fatal error %d (%s) while reading commands",errno,strerror(errno));
		}
	
	/* Calculate the number of complete messages read: */
	messageReadSize=partialSize+size_t(readResult);
	return messageReadSize/sizeof(PipeMessage);
	}

void EventDispatcher::writePipeMessage(const EventDispatcher::PipeMessage& pm,const char* methodName)
	{
	/* Lock the self-pipe: */
	Threads::Spinlock::Lock pipeLock(pipeMutex);
	
	/* Make sure to write the complete message: */
	const unsigned char* writePtr=reinterpret_cast<const unsigned char*>(&pm);
	size_t writeSize=sizeof(PipeMessage);
	while(writeSize>0U)
		{
		ssize_t writeResult=write(pipeFds[1],writePtr,writeSize);
		if(writeResult>0)
			{
			writePtr+=writeResult;
			writeSize-=size_t(writeResult);
			if(writeSize>0U)
				Misc::formattedLogWarning("Threads::EventDispatcher::%s: Incomplete write",methodName);
			}
		else if(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR)
			{
			Misc::formattedLogWarning("Threads::EventDispatcher::%s: Error %d (%s) while writing command",methodName,errno,strerror(errno));
			}
		else
			Misc::throwStdErr("Threads::EventDispatcher::%s: Fatal error %d (%s) while writing command",methodName,errno,strerror(errno));
		}
	}

void EventDispatcher::updateFdSets(int fd,int oldEventMask,int newEventMask)
	{
	/* Check if the read set needs to be updated: */
	if((oldEventMask^newEventMask)&Read)
		{
		/* Add or remove the file descriptor to/from the read set: */
		if(newEventMask&Read)
			{
			FD_SET(fd,&readFds);
			++numReadFds;
			}
		else
			{
			FD_CLR(fd,&readFds);
			--numReadFds;
			}
		}
	
	/* Check if the write set needs to be updated: */
	if((oldEventMask^newEventMask)&Write)
		{
		/* Add or remove the file descriptor to/from the write set: */
		if(newEventMask&Write)
			{
			FD_SET(fd,&writeFds);
			++numWriteFds;
			}
		else
			{
			FD_CLR(fd,&writeFds);
			--numWriteFds;
			}
		}
	
	/* Check if the exception set needs to be updated: */
	if((oldEventMask^newEventMask)&Exception)
		{
		/* Add or remove the file descriptor to/from the exception set: */
		if(newEventMask&Exception)
			{
			FD_SET(fd,&exceptionFds);
			++numExceptionFds;
			}
		else
			{
			FD_CLR(fd,&exceptionFds);
			--numExceptionFds;
			}
		}
	
	/* Check if the maximum file descriptor needs to be updated: */
	if(newEventMask!=0x0)
		{
		if(maxFd<fd)
			maxFd=fd;
		}
	else
		{
		if(maxFd==fd)
			{
			/* Find the new largest file descriptor: */
			maxFd=pipeFds[0];
			for(std::vector<IOEventListener>::iterator it=ioEventListeners.begin();it!=ioEventListeners.end();++it)
				if(it->typeMask!=0&&maxFd<it->fd)
					maxFd=it->fd;
			}
		}
	}

EventDispatcher::EventDispatcher(void)
	:numMessages(4096/sizeof(PipeMessage)),messages(new PipeMessage[numMessages]),messageReadSize(0),
	 nextKey(0),
	 signalListeners(17),
	 numReadFds(0),numWriteFds(0),numExceptionFds(0),
	 hadBadFd(false)
	{
	/* Create the self-pipe: */
	pipeFds[1]=pipeFds[0]=-1;
	if(pipe2(pipeFds,O_NONBLOCK)<0)
		Misc::throwStdErr("Misc::EventDispatcher: Cannot open event pipe due to error %d (%s)",errno,strerror(errno));
	
	/* Initialize the three file descriptor sets: */
	FD_ZERO(&readFds);
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptionFds);
	
	/* Add the read end of the self-pipe to the read descriptor set: */
	FD_SET(pipeFds[0],&readFds);
	numReadFds=1;
	maxFd=pipeFds[0];
	}

EventDispatcher::~EventDispatcher(void)
	{
	/* Close the self-pipe: */
	close(pipeFds[0]);
	close(pipeFds[1]);
	delete[] messages;
	
	/* Delete all timer event listeners: */
	for(TimerEventListenerHeap::Iterator telIt=timerEventListeners.begin();telIt!=timerEventListeners.end();++telIt)
		delete *telIt;
	}

bool EventDispatcher::dispatchNextEvent(void)
	{
	/* Update the dispatch time point: */
	dispatchTime=Time::now();
	
	/* Handle elapsed timer events and find the time interval to the next unelapsed event: */
	Time interval;
	while(!timerEventListeners.isEmpty())
		{
		/* Calculate the interval to the next timer event and dispatch elapsed events on-the-fly: */
		TimerEventListener* tel=timerEventListeners.getSmallest();
		interval=tel->time;
		interval-=dispatchTime;
		
		/* Bail out if the event is still in the future: */
		if(interval.tv_sec>=0)
			break;
		
		/* Call the event callback: */
		if(tel->callback(tel->key,tel->callbackUserData))
			{
			/* Remove the event listener from the heap: */
			delete tel;
			timerEventListeners.removeSmallest();
			}
		else
			{
			/* Move the event time to the next iteration that is still in the future and count the number of missed events: */
			tel->time+=tel->interval;
			// unsigned int numMissedEvents=0; // Need to figure out how to communicate this to timer event handlers in a meaningful way
			while(!(dispatchTime<=tel->time))
				{
				// ++numMissedEvents;
				tel->time+=tel->interval;
				}
			
			/* Re-schedule the event at the next time: */
			timerEventListeners.reinsertSmallest();
			}
		}
	
	/* Create lists of watched file descriptors: */
	fd_set rds,wds,eds;
	int numRfds,numWfds,numEfds,numFds;
	if(hadBadFd)
		{
		/* Listen only on the self-pipe to recover from EBADF errors: */
		FD_ZERO(&rds);
		FD_SET(pipeFds[0],&rds);
		numRfds=1;
		numWfds=0;
		numEfds=0;
		numFds=pipeFds[0]+1;
		
		hadBadFd=false;
		}
	else
		{
		/* Copy all used file descriptor sets: */
		if(numReadFds>0)
			rds=readFds;
		if(numWriteFds>0)
			wds=writeFds;
		if(numExceptionFds>0)
			eds=exceptionFds;
		numRfds=numReadFds;
		numWfds=numWriteFds;
		numEfds=numExceptionFds;
		numFds=maxFd+1;
		}
	
	int numSetFds=0;
	if(timerEventListeners.isEmpty())
		{
		/* Wait for the next event on any watched file descriptor: */
		numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,0);
		}
	else
		{
		/* Wait for the next event on any watched file descriptor or until the next timer event elapses: */
		numSetFds=select(numFds,numRfds>0?&rds:0,numWfds>0?&wds:0,numEfds>0?&eds:0,&interval);
		}
	
	/* Update the dispatch time point: */
	dispatchTime=Time::now();
	
	/* Handle all received events: */
	if(numSetFds>0)
		{
		/* Check for a message on the self-pipe: */
		if(FD_ISSET(pipeFds[0],&rds))
			{
			/* Read and handle pipe messages: */
			size_t numMessages=readPipeMessages();
			PipeMessage* pmPtr=messages;
			for(size_t i=0;i<numMessages;++i,++pmPtr)
				{
				switch(pmPtr->messageType)
					{
					case PipeMessage::INTERRUPT: // Interrupt wait
						
						/* Do nothing */
						
						break;
					
					case PipeMessage::STOP: // Stop dispatching events
						return false;
						break;
					
					case PipeMessage::ADD_IO_LISTENER: // Add input/output event listener
						
						/* Add the new input/output event listener to the list: */
						ioEventListeners.push_back(IOEventListener(pmPtr->addIOListener.key,pmPtr->addIOListener.fd,pmPtr->addIOListener.typeMask,pmPtr->addIOListener.callback,pmPtr->addIOListener.callbackUserData));
						
						/* Update the file descriptor sets: */
						updateFdSets(pmPtr->addIOListener.fd,0x0,pmPtr->addIOListener.typeMask);
						
						break;
					
					case PipeMessage::SET_IO_LISTENER_TYPEMASK: // Change the event type mask of an input/output event listener
						
						/* Find the input/output event listener with the given key: */
						for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
							if(elIt->key==pmPtr->setIOListenerEventTypeMask.key)
								{
								/* Update the input/output event listener: */
								int typeMask=elIt->typeMask;
								elIt->typeMask=pmPtr->setIOListenerEventTypeMask.newTypeMask;
								
								/* Update the file descriptor sets: */
								updateFdSets(elIt->fd,typeMask,elIt->typeMask);
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::REMOVE_IO_LISTENER: // Remove input/output event listener
						
						/* Find the input/output event listener with the given key: */
						for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
							if(elIt->key==pmPtr->removeIOListener)
								{
								/* Remove the input/output event listener from the list: */
								int fd=elIt->fd;
								int typeMask=elIt->typeMask;
								*elIt=ioEventListeners.back();
								ioEventListeners.pop_back();
								
								/* Update the file descriptor sets: */
								updateFdSets(fd,typeMask,0x0);
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::ADD_TIMER_LISTENER: // Add timer event listener
						
						/* Add the new timer event listener to the heap: */
						timerEventListeners.insert(new TimerEventListener(pmPtr->addTimerListener.key,pmPtr->addTimerListener.time,pmPtr->addTimerListener.interval,pmPtr->addTimerListener.callback,pmPtr->addTimerListener.callbackUserData));
						
						break;
					
					case PipeMessage::REMOVE_TIMER_LISTENER: // Remove timer event listener
						
						/* Find the timer event listener with the given key: */
						for(TimerEventListenerHeap::Iterator elIt=timerEventListeners.begin();elIt!=timerEventListeners.end();++elIt)
							if((*elIt)->key==pmPtr->removeTimerListener)
								{
								/* Remove the timer event listener from the heap: */
								delete *elIt;
								timerEventListeners.remove(elIt);
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::ADD_PROCESS_LISTENER:
						
						/* Add the new process listener to the list: */
						processListeners.push_back(ProcessListener(pmPtr->addProcessListener.key,pmPtr->addProcessListener.callback,pmPtr->addProcessListener.callbackUserData));
						
						break;
					
					case PipeMessage::REMOVE_PROCESS_LISTENER:
						
						/* Find the process listener with the given key: */
						for(std::vector<ProcessListener>::iterator plIt=processListeners.begin();plIt!=processListeners.end();++plIt)
							if(plIt->key==pmPtr->removeProcessListener)
								{
								/* Remove the process listener from the list: */
								*plIt=processListeners.back();
								processListeners.pop_back();
								
								/* Stop looking: */
								break;
								}
						
						break;
					
					case PipeMessage::ADD_SIGNAL_LISTENER:
						
						/* Add the new signal listener to the map: */
						signalListeners.setEntry(SignalListenerMap::Entry(pmPtr->addSignalListener.key,SignalListener(pmPtr->addSignalListener.key,pmPtr->addSignalListener.callback,pmPtr->addSignalListener.callbackUserData)));
						
						break;
					
					case PipeMessage::REMOVE_SIGNAL_LISTENER:
						
						/* Remove the signal listener with the given key from the map: */
						signalListeners.removeEntry(pmPtr->removeSignalListener);
						
						break;
					
					case PipeMessage::SIGNAL:
						{
						/* Find the signal listener with the given key in the map: */
						SignalListener& sl=signalListeners.getEntry(pmPtr->signal.key).getDest();
						
						/* Call the callback: */
						sl.callback(sl.key,pmPtr->signal.signalData,sl.callbackUserData);
						
						break;
						}
						
					default:
						/* Do nothing: */
						
						// DEBUGGING
						Misc::formattedLogWarning("Threads::EventDispatcher::dispatchNextEvent: Unknown pipe message %d",pmPtr->messageType);
					}
				}
			
			--numSetFds;
			}
		
		/* Handle all input/output events: */
		for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();numSetFds>0&&elIt!=ioEventListeners.end();++elIt)
			{
			/* Determine all event types on the listener's file descriptor: */
			int eventTypeMask=0x0;
			if(numRfds>0&&FD_ISSET(elIt->fd,&rds))
				{
				/* Signal a read event: */
				eventTypeMask|=Read;
				--numSetFds;
				}
			if(numWfds>0&&FD_ISSET(elIt->fd,&wds))
				{
				/* Signal a write event: */
				eventTypeMask|=Write;
				--numSetFds;
				}
			if(numEfds>0&&FD_ISSET(elIt->fd,&eds))
				{
				/* Signal an exception event: */
				eventTypeMask|=Exception;
				--numSetFds;
				}
			
			/* Limit to events in which the listener is interested: */
			int interestEventTypeMask=eventTypeMask&elIt->typeMask;
			
			/* Check for spurious events: */
			if(interestEventTypeMask!=eventTypeMask)
				Misc::logWarning("Threads::EventDispatcher::dispatchNextEvent: Spurious event");
			
			/* Call the listener's event callback and check whether the listener wants to be removed: */
			if(interestEventTypeMask!=0x0&&elIt->callback(elIt->key,interestEventTypeMask,elIt->callbackUserData))
				{
				/* Update the file descriptor sets: */
				updateFdSets(elIt->fd,elIt->typeMask,0x0);
				
				/* Remove the event listener from the list: */
				*elIt=ioEventListeners.back();
				ioEventListeners.pop_back();
				--elIt;
				}
			}
		}
	else if(numSetFds<0&&errno!=EINTR)
		{
		if(errno==EBADF)
			{
			// DEBUGGING
			Misc::logWarning("Threads::EventDispatcher::dispatchNextEvent: Bad file descriptor in select");
			
			/* Set error flag; only wait on self-pipe on next iteration to hopefully receive a "remove listener" message for the bad descriptor: */
			hadBadFd=true;
			}
		else
			{
			int error=errno;
			Misc::throwStdErr("Threads::EventDispatcher::dispatchNextEvent: Error %d (%s) during select",error,strerror(error));
			}
		}
	
	/* Call all process listeners: */
	for(std::vector<ProcessListener>::iterator plIt=processListeners.begin();plIt!=processListeners.end();++plIt)
		{
		/* Call the listener and check if it wants to be removed: */
		if(plIt->callback(plIt->key,plIt->callbackUserData))
			{
			/* Remove the event listener from the list: */
			*plIt=processListeners.back();
			processListeners.pop_back();
			--plIt;
			}
		}
	
	return true;
	}

void EventDispatcher::dispatchEvents(void)
	{
	/* Dispatch events until the stop() method is called: */
	while(dispatchNextEvent())
		;
	}

void EventDispatcher::interrupt(void)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::INTERRUPT;
	writePipeMessage(pm,"interrupt");
	}

void EventDispatcher::stop(void)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::STOP;
	writePipeMessage(pm,"stop");
	}

void EventDispatcher::stopOnSignals(void)
	{
	/* Check if there is already a signal-stopped event dispatcher: */
	if(stopDispatcher!=0)
		throw std::runtime_error("Threads::EventDispatcher::stopOnSignals: Already registered another dispatcher");
	
	/* Register this dispatcher: */
	stopDispatcher=this;
	
	/* Intercept SIGINT and SIGTERM: */
	struct sigaction sigIntAction;
	memset(&sigIntAction,0,sizeof(struct sigaction));
	sigIntAction.sa_handler=stopSignalHandler;
	if(sigaction(SIGINT,&sigIntAction,0)<0)
		throw std::runtime_error("Threads::EventDispatcher::stopOnSignals: Unable to intercept SIGINT");
	struct sigaction sigTermAction;
	memset(&sigTermAction,0,sizeof(struct sigaction));
	sigTermAction.sa_handler=stopSignalHandler;
	if(sigaction(SIGTERM,&sigTermAction,0)<0)
		throw std::runtime_error("Threads::EventDispatcher::stopOnSignals: Unable to intercept SIGTERM");
	}

EventDispatcher::ListenerKey EventDispatcher::addIOEventListener(int eventFd,int eventTypeMask,EventDispatcher::IOEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_IO_LISTENER;
	pm.addIOListener.key=getNextKey();
	pm.addIOListener.fd=eventFd;
	pm.addIOListener.typeMask=eventTypeMask;
	pm.addIOListener.callback=eventCallback;
	pm.addIOListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,"addIOEventListener");
	
	return pm.addIOListener.key;
	}

void EventDispatcher::setIOEventListenerEventTypeMask(EventDispatcher::ListenerKey listenerKey,int newEventTypeMask)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::SET_IO_LISTENER_TYPEMASK;
	pm.setIOListenerEventTypeMask.key=listenerKey;
	pm.setIOListenerEventTypeMask.newTypeMask=newEventTypeMask;
	writePipeMessage(pm,"setIOEventListenerEventTypeMask");
	}

void EventDispatcher::setIOEventListenerEventTypeMaskFromCallback(EventDispatcher::ListenerKey listenerKey,int newEventTypeMask)
	{
	/* Find the input/output event listener with the given key: */
	for(std::vector<IOEventListener>::iterator elIt=ioEventListeners.begin();elIt!=ioEventListeners.end();++elIt)
		if(elIt->key==listenerKey)
			{
			/* Update the file descriptor sets: */
			updateFdSets(elIt->fd,elIt->typeMask,newEventTypeMask);
			
			/* Update the input/output event listener: */
			elIt->typeMask=newEventTypeMask;
			
			/* Stop looking: */
			break;
			}
	}

void EventDispatcher::removeIOEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_IO_LISTENER;
	pm.removeIOListener=listenerKey;
	writePipeMessage(pm,"removeIOEventListener");
	}

EventDispatcher::ListenerKey EventDispatcher::addTimerEventListener(const EventDispatcher::Time& eventTime,const EventDispatcher::Time& eventInterval,EventDispatcher::TimerEventCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_TIMER_LISTENER;
	pm.addTimerListener.key=getNextKey();
	pm.addTimerListener.time=eventTime;
	pm.addTimerListener.interval=eventInterval;
	pm.addTimerListener.callback=eventCallback;
	pm.addTimerListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,"addTimerEventListener");
	
	return pm.addTimerListener.key;
	}

void EventDispatcher::removeTimerEventListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_TIMER_LISTENER;
	pm.removeTimerListener=listenerKey;
	writePipeMessage(pm,"removeTimerEventListener");
	}

EventDispatcher::ListenerKey EventDispatcher::addProcessListener(EventDispatcher::ProcessCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_PROCESS_LISTENER;
	pm.addProcessListener.key=getNextKey();
	pm.addProcessListener.callback=eventCallback;
	pm.addProcessListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,"addProcessListener");
	
	return pm.addProcessListener.key;
	}

void EventDispatcher::removeProcessListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_PROCESS_LISTENER;
	pm.removeProcessListener=listenerKey;
	writePipeMessage(pm,"removeProcessListener");
	}

EventDispatcher::ListenerKey EventDispatcher::addSignalListener(EventDispatcher::SignalCallback eventCallback,void* eventCallbackUserData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::ADD_SIGNAL_LISTENER;
	pm.addSignalListener.key=getNextKey();
	pm.addSignalListener.callback=eventCallback;
	pm.addSignalListener.callbackUserData=eventCallbackUserData;
	writePipeMessage(pm,"addSignalListener");
	
	return pm.addSignalListener.key;
	}

void EventDispatcher::removeSignalListener(EventDispatcher::ListenerKey listenerKey)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::REMOVE_SIGNAL_LISTENER;
	pm.removeSignalListener=listenerKey;
	writePipeMessage(pm,"removeSignalListener");
	}

void EventDispatcher::signal(EventDispatcher::ListenerKey listenerKey,void* signalData)
	{
	/* Write a pipe message to the self pipe: */
	PipeMessage pm;
	memset(&pm,0,sizeof(PipeMessage));
	pm.messageType=PipeMessage::SIGNAL;
	pm.signal.key=listenerKey;
	pm.signal.signalData=signalData;
	writePipeMessage(pm,"signal");
	}

}
