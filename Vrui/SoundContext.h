/***********************************************************************
SoundContext - Class for OpenAL contexts that are used to map a listener
to an OpenAL sound device.
Copyright (c) 2008-2020 Oliver Kreylos

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

#ifndef VRUI_SOUNDCONTEXT_INCLUDED
#define VRUI_SOUNDCONTEXT_INCLUDED

#include <AL/Config.h>

#include <string>
#if ALSUPPORT_CONFIG_HAVE_OPENAL
#ifdef __APPLE__
#include <OpenAL/alc.h>
#else
#include <AL/alc.h>
#endif
#endif

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
class ALContextData;
namespace Vrui {
class Listener;
class VruiState;
}

namespace Vrui {

class SoundContext
	{
	/* Embedded classes: */
	public:
	enum DistanceAttenuationModel // Enumerated type for distance attenuation models
		{
		CONSTANT,INVERSE,INVERSE_CLAMPED,LINEAR,LINEAR_CLAMPED,EXPONENTIAL,EXPONENTIAL_CLAMPED
		};
	
	/* Elements: */
	private:
	VruiState* vruiState; // Pointer to the Vrui state object this sound context belongs to
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	ALCdevice* alDevice; // Pointer to OpenAL sound device
	ALCcontext* alContext; // Pointer to OpenAL sound context
	#endif
	ALContextData* contextData; // An OpenAL context data structure for this sound context
	Listener* listener; // Pointer to listener listening to this sound context
	float speedOfSound; // Speed of sound in physical coordinate units/s
	float dopplerFactor; // Exaggeration factor for Doppler effect
	DistanceAttenuationModel distanceAttenuationModel; // Distance attenuation model
	float referenceDistance; // Reference distance for distance attenuation in physical coordinate units
	float rolloffFactor; // Roll-off factor for distance attenuation
	std::string recordingDeviceName; // Name of a recording device to be used with this sound context
	
	/* Constructors and destructors: */
	public:
	SoundContext(const Misc::ConfigurationFileSection& configFileSection,VruiState* sVruiState); // Initializes sound context based on settings from given configuration file section
	~SoundContext(void);
	
	/* Methods: */
	const Listener* getListener(void) const // Returns the listener listening to this sound context
		{
		return listener;
		}
	float getReferenceDistance(void) const // Returns the reference distance
		{
		return referenceDistance;
		}
	float getRolloffFactor(void) const // Returns the roll-off factor
		{
		return rolloffFactor;
		}
	const std::string& getRecordingDeviceName(void) const // Returns the name of the recording device associated with this sound context
		{
		return recordingDeviceName;
		}
	ALContextData& getContextData(void) // Returns the sound context's context data
		{
		return *contextData;
		}
	void makeCurrent(void); // Makes sound context current
	void draw(void); // Updates the sound context
	};

}

#endif
