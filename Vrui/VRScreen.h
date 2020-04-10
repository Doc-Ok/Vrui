/***********************************************************************
VRScreen - Class for display screens (fixed and head-mounted) in VR
environments.
Copyright (c) 2004-2018 Oliver Kreylos

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

#ifndef VRUI_VRSCREEN_INCLUDED
#define VRUI_VRSCREEN_INCLUDED

#include <Misc/CallbackList.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <Vrui/Geometry.h>
#include <Vrui/InputGraphManager.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class VRScreen
	{
	/* Embedded classes: */
	public:
	typedef Geometry::ProjectiveTransformation<Scalar,2> PTransform2; // Type for 2D homography transformations
	
	struct CallbackData:public Misc::CallbackData // Generic callback data for screen events
		{
		/* Elements: */
		public:
		VRScreen* screen; // The screen that caused the callback
		
		/* Constructors and destructors: */
		CallbackData(VRScreen* sScreen)
			:screen(sScreen)
			{
			}
		};
	
	struct SizeChangedCallbackData:public CallbackData // Callback data when a screen changes size
		{
		/* Elements: */
		public:
		Scalar newScreenSize[2]; // New screen size; screen object's size is yet unchanged
		
		/* Constructors and destructors: */
		SizeChangedCallbackData(VRScreen* sScreen,Scalar newWidth,Scalar newHeight)
			:CallbackData(sScreen)
			{
			newScreenSize[0]=newWidth;
			newScreenSize[1]=newHeight;
			}
		};
	
	/* Elements: */
	private:
	char* screenName; // Name for the screen
	bool deviceMounted; // Flag if this screen is attached to an input device
	InputDevice* device; // Pointer to the input device this screen is attached to
	Scalar screenSize[2]; // Screen width and height in physical units
	ONTransform transform; // Transformation from screen to physical or device coordinates
	ONTransform inverseTransform; // Transformation from physical or device to screen coordinates
	bool offAxis; // Flag whether the screen is projected off-axis, i.e., has a non-identity homography
	PTransform2 screenHomography; // The screen's screen space homography
	PTransform inverseClipHomography; // The inverse of the screen's clip space homography
	bool intersect; // Flag whether to use this screen for interaction queries
	Misc::CallbackList sizeChangedCallbacks; // List of callbacks to be called when the screen's size changes
	
	/* Transient state data: */
	bool enabled; // Flag if the screen is enabled, i.e., can be used for rendering
	
	/* Private methods: */
	void inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData); // Callback called when an input device changes state
	
	/* Constructors and destructors: */
	public:
	VRScreen(void); // Creates uninitialized screen
	~VRScreen(void);
	
	/* Methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Initializes screen by reading current section of configuration file
	bool isEnabled(void) const // Returns true if the screen can be used for rendering
		{
		return enabled;
		}
	InputDevice* attachToDevice(InputDevice* newDevice); // Attaches the screen to an input device if !=0, otherwise, creates fixed screen; returns previous device or 0
	void setSize(Scalar newWidth,Scalar newHeight); // Adjusts the screen's size in physical units; maintains the current center position
	void setTransform(const ONTransform& newTransform); // Sets the transformation from screen to physical or device coordinates
	const char* getName(void) const // Returns screen's name
		{
		return screenName;
		}
	const Scalar* getScreenSize(void) const // Returns size of screen in physical units
		{
		return screenSize;
		}
	Scalar getWidth(void) const // Returns width of screen in physical units
		{
		return screenSize[0];
		}
	Scalar getHeight(void) const // Returns height of screen in physical units
		{
		return screenSize[1];
		}
	Scalar* getViewport(Scalar resultViewport[4]) const // Copies screen's viewport into provided array and returns pointer to array
		{
		resultViewport[0]=Scalar(0);
		resultViewport[1]=screenSize[0];
		resultViewport[2]=Scalar(0);
		resultViewport[3]=screenSize[1];
		return resultViewport;
		}
	const ONTransform& getTransform(void) const // Returns screen transformation from physical or device coordinates
		{
		return transform;
		}
	ONTransform getScreenTransformation(void) const; // Returns screen transformation from physical coordinates
	bool isOffAxis(void) const // Returns whether the screen is projected off-axis
		{
		return offAxis;
		}
	const PTransform2& getScreenHomography(void) const // Returns the screen's screen-space homography transformation
		{
		return screenHomography;
		}
	const PTransform& getInverseClipHomography(void) const // Returns the screen's inverse clip-space homography transformation
		{
		return inverseClipHomography;
		}
	bool isIntersect(void) const // Returns true if this screen is to be used in intersection queries
		{
		return intersect;
		}
	void setScreenTransform(void) const; // Sets up OpenGL matrices to render directly onto the screen
	void resetScreenTransform(void) const; // Resets OpenGL matrices back to state before calling setScreenTransform()
	Misc::CallbackList& getSizeChangedCallbacks(void) // Returns the list of callbacks called when the screen's size changes
		{
		return sizeChangedCallbacks;
		}
	};

}

#endif
