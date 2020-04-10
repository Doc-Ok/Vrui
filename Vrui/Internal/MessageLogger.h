/***********************************************************************
MessageLogger - Class derived from Misc::MessageLogger to log and
present messages inside a Vrui application.
Copyright (c) 2015-2019 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_MESSAGELOGGER_INCLUDED
#define VRUI_MESSAGELOGGER_INCLUDED

#include <string>
#include <vector>
#include <Misc/MessageLogger.h>
#include <Threads/Mutex.h>

namespace Vrui {

class MessageLogger:public Misc::MessageLogger
	{
	/* Embedded classes: */
	private:
	struct PendingMessage // Structure to hold messages until they can be delivered during Vrui's frame method
		{
		/* Elements: */
		public:
		int messageLevel; // Severity level of the message
		std::string message; // The message string
		
		/* Constructors and destructors: */
		PendingMessage(int sMessageLevel,const char* sMessage)
			:messageLevel(sMessageLevel),message(sMessage)
			{
			}
		};
	
	/* Elements: */
	private:
	bool userToConsole; // Flag whether to route user messages to the console
	Threads::Mutex pendingMessagesMutex; // Mutex serializing access to the pending message list
	std::vector<PendingMessage> pendingMessages; // List of messages awaiting presentation to the user
	bool frameCallbackRegistered; // Flag if the message logger's frame callback has already been registered
	
	/* Protected methods from Misc::MessageLogger: */
	protected:
	virtual void logMessageInternal(Target target,int messageLevel,const char* message);
	
	/* Private methods: */
	void showMessageDialog(int messageLevel,const char* messageString); // Displays a message as a GLMotif dialog
	static bool frameCallback(void* userData); // Callback called from Vrui's frame method when the message logger has synchronous work to do
	
	/* Constructors and destructors: */
	public:
	MessageLogger(void);
	virtual ~MessageLogger(void);
	
	/* New methods: */
	void setUserToConsole(bool newUserToConsole); // If true, user messages are re-routed to the console
	};

}

#endif
