/***********************************************************************
Pipe - Wrapper class for UNIX unnamed pipes for inter-process
communication between a parent and child process, or for FIFO self-
communication.
Copyright (c) 2016-2019 Oliver Kreylos

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

#ifndef MISC_PIPE_INCLUDED
#define MISC_PIPE_INCLUDED

#include <errno.h>
#include <unistd.h>
#include <stdexcept>

namespace Misc {

class Pipe
	{
	/* Elements: */
	private:
	int pipeFds[2]; // File descriptors for the read and write ends of the pipe, respectively
	bool haveEof; // Flag if an end-of-file notification was received during a previous read from the pipe
	
	/* Private methods: */
	static void throwReadError(int errorCode); // Throws an exception when an error occurs while reading from the pipe
	static void throwWriteError(int errorCode); // Throws an exception when an error occurs while writing to the pipe
	
	/* Constructors and destructors: */
	public:
	Pipe(bool nonBlocking =false); // Creates an unnamed pipe in blocking or non-blocking mode; throws exception on error
	private:
	Pipe(const Pipe& source); // Prohibit copy constructor
	Pipe& operator=(const Pipe& source); // Prohibit assignment operator
	public:
	~Pipe(void); // Closes the pipe
	
	/* Methods: */
	
	/* Read interface: */
	int getReadFd(void) const // Returns the file descriptor for the read end of the pipe
		{
		return pipeFds[0];
		}
	bool eof(void) const // Returns true if no more data can be read from the pipe
		{
		return haveEof;
		}
	size_t read(void* buffer,size_t count) // Reads from the read end of the pipe; returns number of bytes read; throws exception on error
		{
		ssize_t readResult=::read(pipeFds[0],buffer,count);
		
		/* Check for exceptional conditions: */
		if(readResult<=0)
			{
			if(readResult==0)
				haveEof=true;
			else if(errno!=EAGAIN&&errno!=EWOULDBLOCK)
				throwReadError(errno);
			readResult=0;
			}
		
		return size_t(readResult);
		}
	template <class DataParam>
	void read(DataParam& data) // Reads from the pipe into the given data value; throws an exception if read wasn't complete or caused error
		{
		if(read(&data,sizeof(DataParam))!=sizeof(DataParam))
			throw std::runtime_error("Misc::Pipe::read: Truncated read");
		}
	void closeRead(void); // Closes the read end of the pipe
	
	/* Write interface: */
	int getWriteFd(void) const // Returns the file descriptor for the write end of the pipe
		{
		return pipeFds[1];
		}
	size_t write(const void* buffer,size_t count) // Writes to the write end of the pipe; returns number of bytes written; throws exception on error
		{
		ssize_t writeResult=::write(pipeFds[1],buffer,count);
		if(writeResult<0)
			throwWriteError(errno);
		
		return size_t(writeResult);
		}
	template <class DataParam>
	void write(const DataParam& data) // Writes from the given data value into the pipe; throws an exception if write wasn't complete or caused error
		{
		if(write(&data,sizeof(DataParam))!=sizeof(DataParam))
			throw std::runtime_error("Misc::Pipe::write: Truncated write");
		}
	void closeWrite(void); // Closes the write end of the pipe
	};

}

#endif
