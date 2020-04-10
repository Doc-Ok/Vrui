/***********************************************************************
ALSAPCMDevice - Simple wrapper class around PCM devices as represented
by the Advanced Linux Sound Architecture (ALSA) library.
Copyright (c) 2009-2019 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SOUND_LINUX_ALSAPCMDEVICE_INCLUDED
#define SOUND_LINUX_ALSAPCMDEVICE_INCLUDED

#include <alsa/asoundlib.h>
#include <stdexcept>
#include <Threads/EventDispatcher.h>

/* Forward declarations: */
struct pollfd;
namespace Sound {
class SoundDataFormat;
}

namespace Sound {

class ALSAPCMDevice
	{
	/* Embedded classes: */
	public:
	class XrunError:public std::runtime_error // Base exception class to report overrun or underrun errors
		{
		/* Constructors and destructors: */
		public:
		XrunError(const char* error)
			:std::runtime_error(error)
			{
			}
		};
	
	class OverrunError:public XrunError // Exception class for overrun errors
		{
		/* Constructors and destructors: */
		public:
		OverrunError(const char* error)
			:XrunError(error)
			{
			}
		};
	
	class UnderrunError:public XrunError // Exception class for underrun errors
		{
		/* Constructors and destructors: */
		public:
		UnderrunError(const char* error)
			:XrunError(error)
			{
			}
		};
	
	typedef void (*PCMEventCallback)(ALSAPCMDevice& device,void* userData); // Type for PCM event callback functions
	
	/* Elements: */
	private:
	snd_pcm_t* pcmDevice; // Handle to the ALSA PCM device
	bool recording; // Flag whether the PCM device is recording
	snd_pcm_hw_params_t* pcmHwParams; // Hardware parameter context for the PCM device; used to accumulate settings until prepare() is called
	PCMEventCallback pcmEventCallback; // Function to be called when a PCM event occurs (ability to write for playback devices, ability to read for capture devices)
	void* pcmEventCallbackUserData; // Extra user data passed to PCM event callback function
	int numPCMEventFds; // Number of file descriptors that need to be watched for PCM events
	struct pollfd* pcmEventPolls; // Array of poll structures to translate select events into poll events for ALSA API
	Threads::EventDispatcher::ListenerKey* pcmEventListenerKeys; // Array of listener keys for the set of watched file descriptors
	
	/* Private methods: */
	void throwException(const char* methodName,int error); // Throws an exception from the given method for the given error code
	static bool pcmEventForwarder(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData); // Callback wrapper to parse event descriptors
	
	/* Constructors and destructors: */
	public:
	ALSAPCMDevice(const char* pcmDeviceName,bool sRecording,bool nonBlocking =false); // Opens the named PCM device for recording or playback and optionally in non-blocking mode
	~ALSAPCMDevice(void); // Closes the PCM device
	
	/* Methods: */
	snd_async_handler_t* registerAsyncHandler(snd_async_callback_t callback,void* privateData); // Registers an asynchronous callback with the PCM device
	void setSoundDataFormat(const SoundDataFormat& format); // Sets the PCM device's sample format
	void setBufferSize(size_t numBufferFrames,size_t numPeriodFrames); // Sets the device's buffer and period sizes
	size_t getBufferSize(void) const; // Returns the actual buffer size selected by the device
	size_t getPeriodSize(void) const; // Returns the actual period size selected by the device
	void prepare(void); // Applies cached hardware parameters to PCM device and prepares it for recording / playback
	void setStartThreshold(size_t numStartFrames); // Sets automatic PCM start threshold for playback and capture devices
	void link(ALSAPCMDevice& other); // Links this PCM with another such that status changes and frame clocks are synchronized; throws exception if devices cannot be linked due to being on different hardware
	void unlink(void); // Unlinks this PCM from any other PCMs to which it was linked
	void addPCMEventListener(Threads::EventDispatcher& dispatcher,PCMEventCallback eventCallback,void* eventCallbackUserData); // Adds a PCM event listener for this audio device to the given event dispatcher
	void removePCMEventListener(Threads::EventDispatcher& dispatcher); // Removes a previously-added PCM event listener from the given event dispatcher
	void start(void); // Starts recording or playback on the PCM device
	size_t getAvailableFrames(void) // Returns the number of audio frames that can be read from a recording device or written to a playback device without blocking
		{
		snd_pcm_sframes_t result=snd_pcm_avail(pcmDevice);
		if(result<0)
			throwException("getAvailableFrames",int(result));
		return size_t(result);
		}
	size_t getAvailableFramesCached(void) // Ditto, but does not round-trip to hardware. Can be used after wake-up from poll or select
		{
		snd_pcm_sframes_t result=snd_pcm_avail_update(pcmDevice);
		if(result<0)
			throwException("getAvailableFramesCached",int(result));
		return size_t(result);
		}
	bool wait(int timeout) // Waits for the PCM device to get ready for I/O; timeout is in milliseconds; negative values wait forever; returns true if device is ready
		{
		int result=snd_pcm_wait(pcmDevice,timeout);
		if(result<0)
			throwException("wait",result);
		
		/* Return true if PCM device is ready: */
		return result==1;
		}
	size_t read(void* buffer,size_t numFrames) // Reads from PCM device into buffer; returns number of frames read
		{
		snd_pcm_sframes_t result=snd_pcm_readi(pcmDevice,buffer,numFrames);
		if(result<0)
			throwException("read",int(result));
		return size_t(result);
		}
	size_t write(const void* buffer,size_t numFrames) // Writes from buffer to PCM device; returns number of frames written
		{
		snd_pcm_sframes_t result=snd_pcm_writei(pcmDevice,buffer,numFrames);
		if(result<0)
		if(result<0)
			throwException("write",int(result));
		return size_t(result);
		}
	void drop(void); // Stops recording/playback and discards pending frames
	void drain(void); // Stops recording/playback but delays until all pending frames have been processed
	};

}

#endif
