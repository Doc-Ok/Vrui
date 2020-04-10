/***********************************************************************
Viewer - Class for viewers/observers in VR environments.
Copyright (c) 2004-2019 Oliver Kreylos

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

#ifndef VRUI_VIEWER_INCLUDED
#define VRUI_VIEWER_INCLUDED

#include <Misc/CallbackList.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLLight.h>
#include <Vrui/Geometry.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDeviceAdapter;
class Lightsource;
}

namespace Vrui {

class Viewer
	{
	/* Embedded classes: */
	public:
	enum Eye // Enumerated type for projection eye selection
		{
		MONO,LEFT,RIGHT
		};
	
	struct ConfigChangedCallbackData:public Misc::CallbackData
		{
		/* Embedded classes: */
		public:
		enum ChangeReasons // Enumerated type for bit flags for reasons for configuration change
			{
			HeadDevice=0x1,
			EyePositions=0x2,
			HeadlightState=0x4
			};
		
		/* Elements: */
		public:
		Viewer* viewer; // Pointer to viewer whose configuration changed
		int changeReasons; // Bit mask of configuration change reasons
		
		/* Constructors and destructors: */
		ConfigChangedCallbackData(Viewer* sViewer,int sChangeReasons)
			:viewer(sViewer),changeReasons(sChangeReasons)
			{
			}
		};
	
	/* Elements: */
	private:
	char* viewerName; // Viewer name
	bool headTracked; // Flag if the viewer is head-tracked
	InputDevice* headDevice; // Pointer to input device used for head tracking
	InputDeviceAdapter* headDeviceAdapter; // Pointer to input device adapter responsible for the head tracking device
	int headDeviceIndex; // Index of head tracking device in its input device adapter
	TrackerState headDeviceTransformation; // Fixed head coordinate frame if head tracking is disabled
	Vector deviceViewDirection; // Viewing direction in head device coordinates
	Vector deviceUpDirection; // Up direction in head device coordinates
	Point deviceMonoEyePosition; // Eye position for monoscopic viewing in head device coordinates
	Point deviceLeftEyePosition; // Left eye position in head device coordinates
	Point deviceRightEyePosition; // Right eye position in head device coordinates
	Lightsource* lightsource; // Pointer to the viewer's head light source
	Point headLightDevicePosition; // Position of head light source in head device coordinates
	Vector headLightDeviceDirection; // Direction of head light source in head device coordinates
	Misc::CallbackList configChangedCallbacks; // List of callbacks to be called when the viewer's configuration changed
	
	/* Transient state data: */
	bool enabled; // Flag if the viewer is enabled, i.e., can be used for rendering
	
	/* Private methods: */
	static void setHeadDeviceCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData); // Handles a "setHeadDevice" command on the command pipe
	static void setHeadTransformCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData); // Handles a "setHeadTransform" command on the command pipe
	static void setMonoEyePosCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData); // Handles a "setMonoEyePos" command on the command pipe
	static void setIPDCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData); // Handles a "setIPD" command on the command pipe
	void inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData); // Callback called when an input device changes state
	
	/* Constructors and destructors: */
	public:
	Viewer(void); // Constructs uninitialized viewer
	~Viewer(void);
	
	/* Methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Initializes viewer by reading current configuration file section
	bool isEnabled(void) const // Returns true if the viewer can be used for rendering
		{
		return enabled;
		}
	const InputDevice* getHeadDevice(void) const // Returns pointer to the viewer's head device, or 0 if not headtracked
		{
		if(headTracked)
			return headDevice;
		else
			return 0;
		}
	InputDevice* attachToDevice(InputDevice* newHeadDevice); // Attaches the viewer to a head device if !=0, otherwise creates a static viewer; returns previous head device or 0
	InputDevice* detachFromDevice(const TrackerState& newHeadDeviceTransformation); // Turns viewer into a static viewer; returns previous head device or 0
	void setIPD(Scalar newIPD); // Overrides the viewer's inter-pupillary distance by sliding eyes along their connecting line
	void setEyes(const Vector& newViewDirection,const Point& newMonoEyePosition,const Vector& newEyeOffset); // Sets view direction and eye positions in head device coordinates
	const char* getName(void) const // Returns viewer's name
		{
		return viewerName;
		}
	const Lightsource& getHeadlight(void) const // Returns the viewer's headlight
		{
		return *lightsource;
		}
	void setHeadlightState(bool newHeadlightState); // Enables or disables the viewer's headlight
	void update(void); // Updates viewer state in frame callback
	const TrackerState& getHeadTransformation(void) const // Returns head transformation
		{
		return headTracked?headDevice->getTransformation():headDeviceTransformation;
		}
	TrackerState peekHeadTransformation(void); // Returns an up-to-date head transformation during frame processing
	Point getHeadPosition(void) const // Returns head position in physical coordinates
		{
		return getHeadTransformation().transform(deviceMonoEyePosition);
		}
	const Vector& getDeviceViewDirection(void) const // Returns view direction in head device coordinates
		{
		return deviceViewDirection;
		}
	Vector getViewDirection(void) const // Returns view direction in physical coordinates
		{
		return getHeadTransformation().transform(deviceViewDirection);
		}
	const Vector& getDeviceUpDirection(void) const // Returns up direction in head device coordinates
		{
		return deviceUpDirection;
		}
	Vector getUpDirection(void) const // Returns up direction in physical coordinates
		{
		return getHeadTransformation().transform(deviceUpDirection);
		}
	const Point& getDeviceEyePosition(Eye eye) const // Returns eye position in head device coordinates
		{
		switch(eye)
			{
			case MONO:
				return deviceMonoEyePosition;
			
			case LEFT:
				return deviceLeftEyePosition;
			
			case RIGHT:
				return deviceRightEyePosition;
			}
		
		/* Dummy statement to make the compiler happy: */
		return deviceMonoEyePosition;
		}
	Point getEyePosition(Eye eye) const // Returns eye position in physical coordinates
		{
		return getHeadTransformation().transform(getDeviceEyePosition(eye));
		}
	Misc::CallbackList& getConfigChangedCallbacks(void) // Returns the list of configuration change callbacks
		{
		return configChangedCallbacks;
		}
	};

}

#endif
