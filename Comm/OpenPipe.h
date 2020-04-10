/***********************************************************************
OpenPipe - Convenience functions to open pipes of several types using
the File abstraction.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef COMM_OPENPIPE_INCLUDED
#define COMM_OPENPIPE_INCLUDED

#include <Comm/Opener.h>

namespace Comm {

Comm::NetPipePtr openTCPPipe(const char* hostName,int portId) // Opens a TCP pipe to the given host / port
	{
	return Comm::Opener::getOpener()->openTCPPipe(hostName,portId);
	}

Comm::NetPipePtr openTLSPipe(const char* hostName,int portId) // Opens a TLS-secured TCP pipe to the given host / port
	{
	return Comm::Opener::getOpener()->openTLSPipe(hostName,portId);
	}

}

#endif
