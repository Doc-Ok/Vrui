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

#ifndef GLMOTIF_VARIABLETRACKER_INCLUDED
#define GLMOTIF_VARIABLETRACKER_INCLUDED

#include <string>
#include <stdexcept>
#include <Misc/SizedTypes.h>

namespace GLMotif {

class VariableTracker
	{
	/* Embedded classes: */
	private:
	enum VariableType // Enumerated type for tracked variable types
		{
		Invalid,
		Boolean,
		UInt8,UInt16,UInt32,UInt64,
		SInt8,SInt16,SInt32,SInt64,
		Float32,Float64,
		String
		};
	
	public:
	class NotTracking:public std::runtime_error // Class for exceptions thrown when a get... call is attempted on an inactive tracker
		{
		/* Constructors and destructors: */
		public:
		NotTracking(void); // Creates a default error object
		};
	
	/* Elements: */
	private:
	VariableType variableType; // Type of tracked variable
	void* variable; // Untyped pointer to tracked variable
	
	/* Protected methods: */
	protected:
	bool getTrackedBool(void) const; // Returns the tracked variable as a boolean value; throws exception if tracking is invalid
	Misc::UInt64 getTrackedUInt(void) const; // Returns the tracked variable as an unsigned integer value; throws exception if tracking is invalid
	Misc::SInt64 getTrackedSInt(void) const; // Returns the tracked variable as a signed integer value; throws exception if tracking is invalid
	Misc::Float64 getTrackedFloat(void) const; // Returns the tracked variable as a floating-point value; throws exception if tracking is invalid
	std::string getTrackedString(int width,int precision) const; // Returns the tracked variable as a string according to the given width and precision; throws exception if tracking is invalid
	void setTrackedBool(bool value); // Sets the tracked variable to the given boolean value; does nothing if tracking is inactive
	void setTrackedUInt(Misc::UInt64 value); // Sets the tracked variable to the given unsigned integer value; does nothing if tracking is inactive
	void setTrackedSInt(Misc::SInt64 value); // Sets the tracked variable to the given signed integer value; does nothing if tracking is inactive
	void setTrackedFloat(Misc::Float64 value); // Sets the tracked variable to the given floating-point value; does nothing if tracking is inactive
	void setTrackedString(const std::string& value); // Sets the tracked variable to the given string value; does nothing if tracking is inactive
	void setTrackedString(const char* value); // Ditto, for C-style string
	
	/* Constructors and destructors: */
	public:
	VariableTracker(void) // Creates an invalid variable tracker
		:variableType(Invalid),variable(0)
		{
		}
	VariableTracker(VariableType sVariableType,void* sVariable) // Creates a tracker for the given variable of the given type
		:variableType(sVariableType),variable(sVariable)
		{
		}
	
	/* Methods: */
	bool isTracking(void) const // Returns true if the tracker is tracking a variable
		{
		return variableType!=Invalid;
		}
	template <class VariableTypeParam>
	void track(VariableTypeParam& newVariable); // Tracks the given variable
	void stopTracking(void); // Stops tracking the current variable
	};

}

#endif
