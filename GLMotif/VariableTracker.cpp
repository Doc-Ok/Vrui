/***********************************************************************
VariableTracker - Mix-in base class for widgets that track the value of
an application variable.
Copyright (c) 2019 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GLMotif/VariableTracker.h>

#include <stdlib.h>
#include <stdio.h>
#include <Math/Math.h>
#include <Math/Constants.h>

namespace GLMotif {

/*********************************************
Methods of class VariableTracker::NotTracking:
*********************************************/

VariableTracker::NotTracking::NotTracking(void)
	:std::runtime_error("GLMotif::VariableTracker: get...() called on inactive tracker")
	{
	}

/********************************
Methods of class VariableTracker:
********************************/

bool VariableTracker::getTrackedBool(void) const
	{
	switch(variableType)
		{
		case Boolean:
			return *static_cast<const bool*>(variable);
		
		case UInt8:
			return *static_cast<const Misc::UInt8*>(variable)!=Misc::UInt8(0);
		
		case UInt16:
			return *static_cast<const Misc::UInt16*>(variable)!=Misc::UInt16(0);
		
		case UInt32:
			return *static_cast<const Misc::UInt32*>(variable)!=Misc::UInt32(0);
		
		case UInt64:
			return *static_cast<const Misc::UInt64*>(variable)!=Misc::UInt64(0);
		
		case SInt8:
			return *static_cast<const Misc::SInt8*>(variable)!=Misc::SInt8(0);
		
		case SInt16:
			return *static_cast<const Misc::SInt16*>(variable)!=Misc::SInt16(0);
		
		case SInt32:
			return *static_cast<const Misc::SInt32*>(variable)!=Misc::SInt32(0);
		
		case SInt64:
			return *static_cast<const Misc::SInt64*>(variable)!=Misc::SInt64(0);
		
		case Float32:
			return *static_cast<const Misc::Float32*>(variable)!=Misc::Float32(0);
		
		case Float64:
			return *static_cast<const Misc::Float64*>(variable)!=Misc::Float64(0);
		
		case String:
			return !static_cast<const std::string*>(variable)->empty();
		
		default:
			throw NotTracking();
		}
	}

Misc::UInt64 VariableTracker::getTrackedUInt(void) const
	{
	switch(variableType)
		{
		case Boolean:
			return *static_cast<const bool*>(variable)?Misc::UInt64(1):Misc::UInt64(0);
		
		case UInt8:
			return Misc::UInt64(*static_cast<const Misc::UInt8*>(variable));
		
		case UInt16:
			return Misc::UInt64(*static_cast<const Misc::UInt16*>(variable));
		
		case UInt32:
			return Misc::UInt64(*static_cast<const Misc::UInt32*>(variable));
		
		case UInt64:
			return Misc::UInt64(*static_cast<const Misc::UInt64*>(variable));
		
		case SInt8:
			{
			Misc::SInt8 v=*static_cast<const Misc::SInt8*>(variable);
			return v>Misc::SInt8(0)?Misc::UInt64(v):Misc::UInt64(0);
			}
		
		case SInt16:
			{
			Misc::SInt16 v=*static_cast<const Misc::SInt16*>(variable);
			return v>Misc::SInt16(0)?Misc::UInt64(v):Misc::UInt64(0);
			}
		
		case SInt32:
			{
			Misc::SInt32 v=*static_cast<const Misc::SInt32*>(variable);
			return v>Misc::SInt32(0)?Misc::UInt64(v):Misc::UInt64(0);
			}
		
		case SInt64:
			{
			Misc::SInt64 v=*static_cast<const Misc::SInt64*>(variable);
			return v>Misc::SInt64(0)?Misc::UInt64(v):Misc::UInt64(0);
			}
		
		case Float32:
			{
			Misc::Float32 v=*static_cast<const Misc::Float32*>(variable);
			if(v<=Misc::Float32(0))
				return Misc::UInt64(0);
			else if(v>=Misc::Float32(Math::Constants<Misc::UInt64>::max))
				return Math::Constants<Misc::UInt64>::max;
			else
				return Misc::UInt64(Math::floor(v+Misc::Float32(0.5)));
			}
		
		case Float64:
			{
			Misc::Float64 v=*static_cast<const Misc::Float64*>(variable);
			if(v<=Misc::Float64(0))
				return Misc::UInt64(0);
			else if(v>=Misc::Float64(Math::Constants<Misc::UInt64>::max))
				return Math::Constants<Misc::UInt64>::max;
			else
				return Misc::UInt64(Math::floor(v+Misc::Float64(0.5)));
			}
		
		case String:
			return Misc::UInt64(strtoul(static_cast<const std::string*>(variable)->c_str(),0,10));
		
		default:
			throw NotTracking();
		}
	}

Misc::SInt64 VariableTracker::getTrackedSInt(void) const
	{
	switch(variableType)
		{
		case Boolean:
			return *static_cast<const bool*>(variable)?Misc::SInt64(1):Misc::SInt64(0);
		
		case UInt8:
			return Misc::UInt64(*static_cast<const Misc::UInt8*>(variable));
		
		case UInt16:
			return Misc::UInt64(*static_cast<const Misc::UInt16*>(variable));
		
		case UInt32:
			return Misc::UInt64(*static_cast<const Misc::UInt32*>(variable));
		
		case UInt64:
			{
			Misc::UInt64 v=*static_cast<const Misc::UInt64*>(variable);
			return v<Misc::UInt64(Math::Constants<Misc::SInt64>::max)?Misc::SInt64(v):Math::Constants<Misc::SInt64>::max;
			}
		
		case SInt8:
			return Misc::SInt64(*static_cast<const Misc::SInt8*>(variable));
		
		case SInt16:
			return Misc::SInt64(*static_cast<const Misc::SInt16*>(variable));
		
		case SInt32:
			return Misc::SInt64(*static_cast<const Misc::SInt32*>(variable));
		
		case SInt64:
			return Misc::SInt64(*static_cast<const Misc::SInt64*>(variable));
		
		case Float32:
			{
			Misc::Float32 v=*static_cast<const Misc::Float32*>(variable);
			if(v<=Misc::Float32(Math::Constants<Misc::SInt64>::min))
				return Math::Constants<Misc::SInt64>::min;
			else if(v>=Misc::Float32(Math::Constants<Misc::SInt64>::max))
				return Math::Constants<Misc::SInt64>::max;
			else
				return Misc::SInt64(Math::floor(v+Misc::Float32(0.5)));
			}
		
		case Float64:
			{
			Misc::Float64 v=*static_cast<const Misc::Float32*>(variable);
			if(v<=Misc::Float64(Math::Constants<Misc::SInt64>::min))
				return Math::Constants<Misc::SInt64>::min;
			else if(v>=Misc::Float64(Math::Constants<Misc::SInt64>::max))
				return Math::Constants<Misc::SInt64>::max;
			else
				return Misc::SInt64(Math::floor(v+Misc::Float64(0.5)));
			}
		
		case String:
			return Misc::SInt64(strtol(static_cast<const std::string*>(variable)->c_str(),0,10));
		
		default:
			throw NotTracking();
		}
	}

Misc::Float64 VariableTracker::getTrackedFloat(void) const
	{
	switch(variableType)
		{
		case Boolean:
			return *static_cast<const bool*>(variable)?Misc::Float64(1):Misc::Float64(0);
		
		case UInt8:
			return Misc::Float64(*static_cast<const Misc::UInt8*>(variable));
		
		case UInt16:
			return Misc::Float64(*static_cast<const Misc::UInt16*>(variable));
		
		case UInt32:
			return Misc::Float64(*static_cast<const Misc::UInt32*>(variable));
		
		case UInt64:
			return Misc::Float64(*static_cast<const Misc::UInt64*>(variable));
		
		case SInt8:
			return Misc::Float64(*static_cast<const Misc::SInt8*>(variable));
		
		case SInt16:
			return Misc::Float64(*static_cast<const Misc::SInt16*>(variable));
		
		case SInt32:
			return Misc::Float64(*static_cast<const Misc::SInt32*>(variable));
		
		case SInt64:
			return Misc::Float64(*static_cast<const Misc::SInt64*>(variable));
		
		case Float32:
			return Misc::Float64(*static_cast<const Misc::Float32*>(variable));
		
		case Float64:
			return *static_cast<const Misc::Float64*>(variable);
		
		case String:
			return Misc::Float64(strtod(static_cast<const std::string*>(variable)->c_str(),0));
		
		default:
			throw NotTracking();
		}
	}

std::string VariableTracker::getTrackedString(int width,int precision) const
	{
	switch(variableType)
		{
		case Boolean:
			return *static_cast<const bool*>(variable)?"T":"";
		
		case UInt8:
			{
			Misc::UInt64 v=Misc::UInt64(*static_cast<const Misc::UInt8*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*lu",width,precision,v);
			return std::string(buffer);
			}
		
		case UInt16:
			{
			Misc::UInt64 v=Misc::UInt64(*static_cast<const Misc::UInt16*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*lu",width,precision,v);
			return std::string(buffer);
			}
		
		case UInt32:
			{
			Misc::UInt64 v=Misc::UInt64(*static_cast<const Misc::UInt32*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*lu",width,precision,v);
			return std::string(buffer);
			}
		
		case UInt64:
			{
			Misc::UInt64 v=*static_cast<const Misc::UInt16*>(variable);
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*lu",width,precision,v);
			return std::string(buffer);
			}
		
		case SInt8:
			{
			Misc::SInt64 v=Misc::SInt64(*static_cast<const Misc::SInt8*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*ld",width,precision,v);
			return std::string(buffer);
			}
		
		case SInt16:
			{
			Misc::SInt64 v=Misc::SInt64(*static_cast<const Misc::SInt16*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*ld",width,precision,v);
			return std::string(buffer);
			}
		
		case SInt32:
			{
			Misc::SInt64 v=Misc::SInt64(*static_cast<const Misc::SInt32*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*ld",width,precision,v);
			return std::string(buffer);
			}
		
		case SInt64:
			{
			Misc::SInt64 v=*static_cast<const Misc::SInt64*>(variable);
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*ld",width,precision,v);
			return std::string(buffer);
			}
		
		case Float32:
			{
			Misc::Float64 v=Misc::Float64(*static_cast<const Misc::Float32*>(variable));
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*g",width,precision,v);
			return std::string(buffer);
			}
		
		case Float64:
			{
			Misc::Float64 v=*static_cast<const Misc::Float32*>(variable);
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%*.*g",width,precision,v);
			return std::string(buffer);
			}
		
		case String:
			return *static_cast<const std::string*>(variable);
		
		default:
			throw NotTracking();
		}
	}

void VariableTracker::setTrackedBool(bool value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=value;
			break;
		
		case UInt8:
			*static_cast<Misc::UInt8*>(variable)=value?Misc::UInt8(1):Misc::UInt8(0);
			break;
		
		case UInt16:
			*static_cast<Misc::UInt16*>(variable)=value?Misc::UInt16(1):Misc::UInt16(0);
			break;
		
		case UInt32:
			*static_cast<Misc::UInt32*>(variable)=value?Misc::UInt32(1):Misc::UInt32(0);
			break;
		
		case UInt64:
			*static_cast<Misc::UInt64*>(variable)=value?Misc::UInt64(1):Misc::UInt64(0);
			break;
		
		case SInt8:
			*static_cast<Misc::SInt8*>(variable)=value?Misc::SInt8(1):Misc::SInt8(0);
			break;
		
		case SInt16:
			*static_cast<Misc::SInt16*>(variable)=value?Misc::SInt16(1):Misc::SInt16(0);
			break;
		
		case SInt32:
			*static_cast<Misc::SInt32*>(variable)=value?Misc::SInt32(1):Misc::SInt32(0);
			break;
		
		case SInt64:
			*static_cast<Misc::SInt64*>(variable)=value?Misc::SInt64(1):Misc::SInt64(0);
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=value?Misc::Float32(1):Misc::Float32(0);
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=value?Misc::Float64(1):Misc::Float64(0);
			break;
		
		case String:
			*static_cast<std::string*>(variable)=value?"T":"";
			break;
		
		default:
			/* Ignore silently */
			;
		}
	}

void VariableTracker::setTrackedUInt(Misc::UInt64 value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=value!=Misc::UInt64(0);
			break;
		
		case UInt8:
			if(value>=Misc::UInt64(Math::Constants<Misc::UInt8>::max))
				*static_cast<Misc::UInt8*>(variable)=Math::Constants<Misc::UInt8>::max;
			else
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(value);
			break;
		
		case UInt16:
			if(value>=Misc::UInt64(Math::Constants<Misc::UInt16>::max))
				*static_cast<Misc::UInt16*>(variable)=Math::Constants<Misc::UInt16>::max;
			else
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(value);
			break;
		
		case UInt32:
			if(value>=Misc::UInt64(Math::Constants<Misc::UInt32>::max))
				*static_cast<Misc::UInt32*>(variable)=Math::Constants<Misc::UInt32>::max;
			else
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(value);
			break;
		
		case UInt64:
			*static_cast<Misc::UInt64*>(variable)=value;
			break;
		
		case SInt8:
			if(value>=Misc::UInt64(Math::Constants<Misc::SInt8>::max))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::max;
			else
				*static_cast<Misc::SInt8*>(variable)=Misc::SInt8(value);
			break;
		
		case SInt16:
			if(value>=Misc::UInt64(Math::Constants<Misc::SInt16>::max))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::max;
			else
				*static_cast<Misc::SInt16*>(variable)=Misc::SInt16(value);
			break;
		
		case SInt32:
			if(value>=Misc::UInt64(Math::Constants<Misc::SInt32>::max))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::max;
			else
				*static_cast<Misc::SInt32*>(variable)=Misc::SInt32(value);
			break;
		
		case SInt64:
			if(value>=Misc::UInt64(Math::Constants<Misc::SInt64>::max))
				*static_cast<Misc::SInt64*>(variable)=Math::Constants<Misc::SInt64>::max;
			else
				*static_cast<Misc::SInt64*>(variable)=Misc::SInt64(value);
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=Misc::Float32(value);
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=Misc::Float64(value);
			break;
		
		case String:
			{
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%lu",value);
			*static_cast<std::string*>(variable)=buffer;
			break;
			}
		
		default:
			/* Ignore silently */
			;
		}
	}

void VariableTracker::setTrackedSInt(Misc::SInt64 value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=value!=Misc::SInt64(0);
			break;
		
		case UInt8:
			if(value<=Misc::SInt64(0))
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(0);
			else if(value>=Misc::SInt64(Math::Constants<Misc::UInt8>::max))
				*static_cast<Misc::UInt8*>(variable)=Math::Constants<Misc::UInt8>::max;
			else
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(value);
			break;
		
		case UInt16:
			if(value<=Misc::SInt64(0))
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(0);
			else if(value>=Misc::SInt64(Math::Constants<Misc::UInt16>::max))
				*static_cast<Misc::UInt16*>(variable)=Math::Constants<Misc::UInt16>::max;
			else
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(value);
			break;
		
		case UInt32:
			if(value<=Misc::SInt64(0))
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(0);
			else if(value>=Misc::SInt64(Math::Constants<Misc::UInt32>::max))
				*static_cast<Misc::UInt32*>(variable)=Math::Constants<Misc::UInt32>::max;
			else
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(value);
			break;
		
		case UInt64:
			if(value<=Misc::SInt64(0))
				*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(0);
			else
				*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(value);
			break;
		
		case SInt8:
			if(value<=Misc::SInt64(Math::Constants<Misc::SInt8>::min))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::min;
			else if(value>=Misc::SInt64(Math::Constants<Misc::SInt8>::max))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::max;
			else
				*static_cast<Misc::SInt8*>(variable)=Misc::SInt8(value);
			break;
		
		case SInt16:
			if(value<=Misc::SInt64(Math::Constants<Misc::SInt16>::min))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::min;
			else if(value>=Misc::SInt64(Math::Constants<Misc::SInt16>::max))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::max;
			else
				*static_cast<Misc::SInt16*>(variable)=Misc::SInt16(value);
			break;
		
		case SInt32:
			if(value<=Misc::SInt64(Math::Constants<Misc::SInt32>::min))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::min;
			else if(value>=Misc::SInt64(Math::Constants<Misc::SInt32>::max))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::max;
			else
				*static_cast<Misc::SInt32*>(variable)=Misc::SInt32(value);
			break;
		
		case SInt64:
			*static_cast<Misc::SInt64*>(variable)=value;
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=Misc::Float32(value);
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=Misc::Float64(value);
			break;
		
		case String:
			{
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%ld",value);
			*static_cast<std::string*>(variable)=buffer;
			break;
			}
		
		default:
			/* Ignore silently */
			;
		}
	}

void VariableTracker::setTrackedFloat(Misc::Float64 value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=value!=Misc::Float64(0);
			break;
		
		case UInt8:
			if(value<=Misc::Float64(0))
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(0);
			else if(value>=Misc::Float64(Math::Constants<Misc::UInt8>::max))
				*static_cast<Misc::UInt8*>(variable)=Math::Constants<Misc::UInt8>::max;
			else
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case UInt16:
			if(value<=Misc::Float64(0))
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(0);
			else if(value>=Misc::Float64(Math::Constants<Misc::UInt16>::max))
				*static_cast<Misc::UInt16*>(variable)=Math::Constants<Misc::UInt16>::max;
			else
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case UInt32:
			if(value<=Misc::Float64(0))
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(0);
			else if(value>=Misc::Float64(Math::Constants<Misc::UInt32>::max))
				*static_cast<Misc::UInt32*>(variable)=Math::Constants<Misc::UInt32>::max;
			else
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case UInt64:
			if(value<=Misc::Float64(0))
				*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(0);
			else if(value>=Misc::Float64(Math::Constants<Misc::UInt64>::max))
				*static_cast<Misc::UInt64*>(variable)=Math::Constants<Misc::UInt64>::max;
			else
				*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case SInt8:
			if(value<=Misc::Float64(Math::Constants<Misc::SInt8>::min))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::min;
			else if(value>=Misc::Float64(Math::Constants<Misc::SInt8>::max))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::max;
			else
				*static_cast<Misc::SInt8*>(variable)=Misc::SInt8(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case SInt16:
			if(value<=Misc::Float64(Math::Constants<Misc::SInt16>::min))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::min;
			else if(value>=Misc::Float64(Math::Constants<Misc::SInt16>::max))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::max;
			else
				*static_cast<Misc::SInt16*>(variable)=Misc::SInt16(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case SInt32:
			if(value<=Misc::Float64(Math::Constants<Misc::SInt32>::min))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::min;
			else if(value>=Misc::Float64(Math::Constants<Misc::SInt32>::max))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::max;
			else
				*static_cast<Misc::SInt32*>(variable)=Misc::SInt32(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case SInt64:
			if(value<=Misc::Float64(Math::Constants<Misc::SInt64>::min))
				*static_cast<Misc::SInt64*>(variable)=Math::Constants<Misc::SInt64>::min;
			else if(value>=Misc::Float64(Math::Constants<Misc::SInt64>::max))
				*static_cast<Misc::SInt64*>(variable)=Math::Constants<Misc::SInt64>::max;
			else
				*static_cast<Misc::SInt64*>(variable)=Misc::SInt64(Math::floor(value+Misc::Float64(0.5)));
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=Misc::Float32(value);
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=value;
			break;
		
		case String:
			{
			char buffer[64];
			snprintf(buffer,sizeof(buffer),"%g",value);
			*static_cast<std::string*>(variable)=buffer;
			break;
			}
		
		default:
			/* Ignore silently */
			;
		}
	}

void VariableTracker::setTrackedString(const std::string& value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=!value.empty();
			break;
		
		case UInt8:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value.c_str(),0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt8>::max))
				*static_cast<Misc::UInt8*>(variable)=Math::Constants<Misc::UInt8>::max;
			else
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(v);
			break;
			}
		
		case UInt16:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value.c_str(),0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt16>::max))
				*static_cast<Misc::UInt16*>(variable)=Math::Constants<Misc::UInt16>::max;
			else
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(v);
			break;
			}
		
		case UInt32:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value.c_str(),0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt32>::max))
				*static_cast<Misc::UInt32*>(variable)=Math::Constants<Misc::UInt32>::max;
			else
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(v);
			break;
			}
		
		case UInt64:
			*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(strtoul(value.c_str(),0,10));
			break;
		
		case SInt8:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value.c_str(),0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt8>::min))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt8>::max))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::max;
			else
				*static_cast<Misc::SInt8*>(variable)=Misc::SInt8(v);
			break;
			}
		
		case SInt16:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value.c_str(),0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt16>::min))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt16>::max))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::max;
			else
				*static_cast<Misc::SInt16*>(variable)=Misc::SInt16(v);
			break;
			}
		
		case SInt32:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value.c_str(),0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt32>::min))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt32>::max))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::max;
			else
				*static_cast<Misc::SInt32*>(variable)=Misc::SInt32(v);
			break;
			}
		
		case SInt64:
			*static_cast<Misc::SInt64*>(variable)=Misc::SInt64(strtol(value.c_str(),0,10));
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=Misc::Float32(strtod(value.c_str(),0));
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=Misc::Float64(strtod(value.c_str(),0));
			break;
		
		case String:
			*static_cast<std::string*>(variable)=value;
			break;
		
		default:
			/* Ignore silently */
			;
		}
	}

void VariableTracker::setTrackedString(const char* value)
	{
	switch(variableType)
		{
		case Boolean:
			*static_cast<bool*>(variable)=value[0]!='\0';
			break;
		
		case UInt8:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value,0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt8>::max))
				*static_cast<Misc::UInt8*>(variable)=Math::Constants<Misc::UInt8>::max;
			else
				*static_cast<Misc::UInt8*>(variable)=Misc::UInt8(v);
			break;
			}
		
		case UInt16:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value,0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt16>::max))
				*static_cast<Misc::UInt16*>(variable)=Math::Constants<Misc::UInt16>::max;
			else
				*static_cast<Misc::UInt16*>(variable)=Misc::UInt16(v);
			break;
			}
		
		case UInt32:
			{
			Misc::UInt64 v=Misc::UInt64(strtoul(value,0,10));
			if(v>=Misc::UInt64(Math::Constants<Misc::UInt32>::max))
				*static_cast<Misc::UInt32*>(variable)=Math::Constants<Misc::UInt32>::max;
			else
				*static_cast<Misc::UInt32*>(variable)=Misc::UInt32(v);
			break;
			}
		
		case UInt64:
			*static_cast<Misc::UInt64*>(variable)=Misc::UInt64(strtoul(value,0,10));
			break;
		
		case SInt8:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value,0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt8>::min))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt8>::max))
				*static_cast<Misc::SInt8*>(variable)=Math::Constants<Misc::SInt8>::max;
			else
				*static_cast<Misc::SInt8*>(variable)=Misc::SInt8(v);
			break;
			}
		
		case SInt16:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value,0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt16>::min))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt16>::max))
				*static_cast<Misc::SInt16*>(variable)=Math::Constants<Misc::SInt16>::max;
			else
				*static_cast<Misc::SInt16*>(variable)=Misc::SInt16(v);
			break;
			}
		
		case SInt32:
			{
			Misc::SInt64 v=Misc::SInt64(strtol(value,0,10));
			if(v<=Misc::SInt64(Math::Constants<Misc::SInt32>::min))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::min;
			else if(v>=Misc::SInt64(Math::Constants<Misc::SInt32>::max))
				*static_cast<Misc::SInt32*>(variable)=Math::Constants<Misc::SInt32>::max;
			else
				*static_cast<Misc::SInt32*>(variable)=Misc::SInt32(v);
			break;
			}
		
		case SInt64:
			*static_cast<Misc::SInt64*>(variable)=Misc::SInt64(strtol(value,0,10));
			break;
		
		case Float32:
			*static_cast<Misc::Float32*>(variable)=Misc::Float32(strtod(value,0));
			break;
		
		case Float64:
			*static_cast<Misc::Float64*>(variable)=Misc::Float64(strtod(value,0));
			break;
		
		case String:
			*static_cast<std::string*>(variable)=value;
			break;
		
		default:
			/* Ignore silently */
			;
		}
	}

template <>
void VariableTracker::track(bool& newVariable)
	{
	variableType=Boolean;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::UInt8& newVariable)
	{
	variableType=UInt8;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::UInt16& newVariable)
	{
	variableType=UInt16;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::UInt32& newVariable)
	{
	variableType=UInt32;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::UInt64& newVariable)
	{
	variableType=UInt64;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::SInt8& newVariable)
	{
	variableType=SInt8;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::SInt16& newVariable)
	{
	variableType=SInt16;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::SInt32& newVariable)
	{
	variableType=SInt32;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::SInt64& newVariable)
	{
	variableType=SInt64;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::Float32& newVariable)
	{
	variableType=Float32;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(Misc::Float64& newVariable)
	{
	variableType=Float64;
	variable=&newVariable;
	}

template <>
void VariableTracker::track(std::string& newVariable)
	{
	variableType=String;
	variable=&newVariable;
	}

void VariableTracker::stopTracking(void)
	{
	variableType=Invalid;
	variable=0;
	}

}
