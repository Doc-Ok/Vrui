/***********************************************************************
CommandDispatcher - Class to dispatch text commands read from a file.
Copyright (c) 2020 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Misc/CommandDispatcher.h>

#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <Misc/MessageLogger.h>

namespace Misc {

/**********************************
Methods of class CommandDispatcher:
**********************************/

void CommandDispatcher::listCommandsCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	/* Embedded classes: */
	class Compare
		{
		/* Methods: */
		public:
		bool operator()(const CommandMap::Iterator& it1,const CommandMap::Iterator& it2)
			{
			return it1->getSource()<it2->getSource();
			}
		};
	
	CommandDispatcher* thisPtr=static_cast<CommandDispatcher*>(userData);
	
	/* Put all command callback slots into a vector and sort them alphabetically: */
	std::vector<CommandMap::Iterator> ccs;
	ccs.reserve(thisPtr->commandMap.getNumEntries());
	for(CommandMap::Iterator cmIt=thisPtr->commandMap.begin();!cmIt.isFinished();++cmIt)
		ccs.push_back(cmIt);
	std::sort(ccs.begin(),ccs.end(),Compare());
	
	/* List all defined commands with their optional argument lists and descriptions: */
	bool first=true;
	for(std::vector<CommandMap::Iterator>::iterator ccIt=ccs.begin();ccIt!=ccs.end();++ccIt)
		{
		if(!first)
			Misc::logNote("");
		first=false;
		
		/* Print the command token and its optional argument list: */
		CommandMap::Iterator cmIt=*ccIt;
		CommandCallbackSlot& ccs=cmIt->getDest();
		if(!ccs.arguments.empty())
			Misc::formattedLogNote("%s %s",cmIt->getSource().c_str(),ccs.arguments.c_str());
		else
			Misc::logNote(cmIt->getSource().c_str());
		
		/* Print the optional description: */
		if(!ccs.description.empty())
			Misc::formattedLogNote("    %s",ccs.description.c_str());
		}
	}

CommandDispatcher::CommandDispatcher(void)
	:commandMap(17)
	{
	/* Define a command to list all defined commands: */
	addCommandCallback("listCommands",listCommandsCallback,this,0,"Prints all defined commands and their descriptions");
	}

bool CommandDispatcher::addCommandCallback(const char* command,CommandCallback callback,void* userData,const char* arguments,const char* description)
	{
	/* Check if the command is already claimed: */
	std::string cmd(command);
	if(commandMap.isEntry(cmd))
		return false;
	
	/* Add the callback: */
	CommandCallbackSlot ccs;
	ccs.callback=callback;
	ccs.userData=userData;
	
	/* Copy the optional argument list and command description: */
	if(arguments!=0)
		ccs.arguments=arguments;
	if(description!=0)
		ccs.description=description;
	
	/* Store the command callback: */
	commandMap.setEntry(CommandMap::Entry(cmd,ccs));
	
	return true;
	}

void CommandDispatcher::removeCommandCallback(const char* command)
	{
	/* Remove the callback: */
	commandMap.removeEntry(command);
	}

bool CommandDispatcher::dispatchCommands(int commandFd)
	{
	bool result=false;
	
	/* Read one or more commands from the command pipe: */
	char readBuffer[1024]; // This needs a rather large buffer
	ssize_t readSize=read(commandFd,readBuffer,sizeof(readBuffer));
	
	if(readSize>0)
		{
		/* Find the start of the first command: */
		char* readEnd=readBuffer+readSize;
		char* readPtr=readBuffer;
		while(readPtr!=readEnd&&isspace(*readPtr))
			++readPtr;
		
		/* Parse all commands in the read buffer: */
		while(readPtr!=readEnd)
			{
			char* commandStart=readPtr;
			
			/* Find the end of the command: */
			while(readPtr!=readEnd&&!isspace(*readPtr))
				++readPtr;
			char* commandEnd=readPtr;
			
			/* Find the start of an optional command argument: */
			while(readPtr!=readEnd&&isspace(*readPtr)&&*readPtr!='\n')
				++readPtr;
			char* argumentStart=readPtr;
			
			/* Find the end of the current line: */
			while(readPtr!=readEnd&&*readPtr!='\n')
				++readPtr;
			
			/* Check if there was a command: */
			if(commandEnd!=commandStart)
				{
				/* Find a handler for the command: */
				std::string command(commandStart,commandEnd);
				CommandMap::Iterator cmIt=commandMap.findEntry(command);
				if(!cmIt.isFinished())
					{
					try
						{
						/* Call the callback: */
						cmIt->getDest().callback(argumentStart,readPtr,cmIt->getDest().userData);
						}
					catch(const std::runtime_error& err)
						{
						Misc::formattedLogError("CommandDispatcher: Caught exception %s while handling command %s %s",err.what(),command.c_str(),std::string(argumentStart,readPtr).c_str());
						}
					}
				else
					Misc::formattedLogError("CommandDispatcher: Unrecognized command %s",command.c_str());
				}
			
			/* Find the start of the next command: */
			while(readPtr!=readEnd&&isspace(*readPtr))
				++readPtr;
			}
		}
	else if(readSize==0)
		{
		Misc::formattedLogWarning("CommandDispatcher: Command file %d was closed; not accepting further commands",commandFd);
		result=true;
		}
	else if(errno!=EAGAIN&&errno!=EWOULDBLOCK)
		{
		Misc::formattedLogError("CommandDispatcher: Read error %d (%s) from command file %d; not accepting further commands",commandFd,errno,strerror(errno));
		result=true;
		}
	
	return result;
	}

}
