/***********************************************************************
MouseCameraTool - Tool class to change a window's view into a 3D
environment by manipulating the positions, orientations, and sizes of a
viewer/screen pair instead of manipulating the navigation transformation.
Copyright (c) 2018-2020 Oliver Kreylos

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

#ifndef VRUI_MOUSECAMERATOOL_INCLUDED
#define VRUI_MOUSECAMERATOOL_INCLUDED

#include <utility>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/UtilityTool.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
namespace Vrui {
class VRScreen;
class Viewer;
class InputDeviceAdapterMouse;
}

namespace Vrui {

class MouseCameraTool;

class MouseCameraToolFactory:public ToolFactory
	{
	friend class MouseCameraTool;
	
	/* Embedded classes: */
	private:
	struct Configuration // Structure containing tool settings
		{
		/* Elements: */
		public:
		int windowIndex; // Index of window whose virtual camera to control
		Scalar rotateFactor; // Distance the device has to be moved to rotate by one radians, as fraction of camera screen's diagonal size
		bool invertDolly; // Flag whether to invert the switch between dollying/scaling
		bool dollyCenter; // Flag whether to dolly around the display center or current device position
		bool scaleCenter; // Flag whether to scale around the display center or current device position
		Vector dollyingDirection; // Direction of dollying line in screen coordinates
		Vector scalingDirection; // Direction of scaling line in screen coordinates
		Scalar dollyFactor; // Distance the device has to be moved along the dollying line to dolly by one display size, as fraction of camera screen's diagonal size
		Scalar scaleFactor; // Distance the device has to be moved along the scaling line to scale by factor of e, as fraction of camera screen's diagonal size
		Scalar wheelDollyFactor; // Physical unit dolly amount for one wheel click
		Scalar wheelScaleFactor; // Scaling factor for one wheel click
		Scalar spinThreshold; // Distance the device has to be moved on the last step of rotation to activate spinning, as fraction of camera screen's diagonal size
		bool showScreenCenter; // Flag whether to draw the center of the screen during navigation
		bool showFrustum; // Flag whether to draw the virtual camera's view frustum in the 3D environment
		
		/* Constructors and destructors: */
		Configuration(void); // Creates default configuration
		
		/* Methods: */
		void read(const Misc::ConfigurationFileSection& cfs); // Overrides configuration from configuration file section
		void write(Misc::ConfigurationFileSection& cfs) const; // Writes configuration to configuration file section
		};
	
	/* Elements: */
	Configuration configuration; // Default configuration for all tools
	
	/* Constructors and destructors: */
	public:
	MouseCameraToolFactory(ToolManager& toolManager);
	virtual ~MouseCameraToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual const char* getButtonFunction(int buttonSlotIndex) const;
	virtual const char* getValuatorFunction(int valuatorSlotIndex) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class MouseCameraTool:public UtilityTool
	{
	friend class MouseCameraToolFactory;
	
	/* Embedded classes: */
	private:
	enum CameraMode // Enumerated type for states the tool can be in
		{
		IDLE,ROTATING,SPINNING,PANNING,DOLLYING,SCALING,DOLLYING_WHEEL,SCALING_WHEEL
		};
	
	/* Elements: */
	static MouseCameraToolFactory* factory; // Pointer to the factory object for this class
	MouseCameraToolFactory::Configuration configuration; // Private configuration of this tool
	InputDeviceAdapterMouse* mouseAdapter; // Mouse input device adapter managing the tool's main input device, or null
	
	/* Initial states of controlled screen and viewer when the tool was created: */
	VRScreen* screen; // Pointer to the screen defining the controlled camera
	InputDevice* screenDevice; // Device to which the controlled screen was attached, or null
	ONTransform screenTransform; // Initial screen transformation to device or physical space
	Scalar screenSize[2]; // Initial size of screen
	Scalar screenDiagonal; // Initial diagonal size of screen to scale interactions
	Point screenCenter; // Screen center in physical space
	ONTransform physScreenTransform; // Initial transformation from screen space to physical space
	
	Viewer* viewer; // Pointer to the viewer defining the controlled camera
	InputDevice* viewerDevice; // Device to which the viewer was attached, or null
	Vector viewerViewDirection; // Viewer's initial viewing direction in viewer space
	Point viewerEyePos; // Position of viewer's monoscopic eye in viewer space
	Vector viewerEyeOffset; // Vector from viewer's monoscopic eye to viewer's right eye in viewer space
	TrackerState physViewerTransform; // Initial transformation from viewer space to physical space
	
	/* Movement state: */
	Point focus; // Camera focus point
	Vector elevationAxis; // Elevation angle rotation axis in physical space
	Vector azimuthAxis; // Azimuth angle rotation axis in physical space
	
	/* Current virtual camera state: */
	Scalar elevation,azimuth; // Rotation angles around vertical and horizontal
	Scalar scale; // Scale factor from original screen size to current screen size
	
	/* Visualization state: */
	bool showFrustum; // Flag whether to draw the virtual camera's frustum in the 3D environment
	
	/* Transient interaction state: */
	bool lockToMainViewer; // Flag whether to temporarily lock the virtual camera to the environment's main viewer
	bool dolly; // Flag whether to dolly instead of scale
	CameraMode cameraMode; // The tool's current camera mode
	std::pair<bool,Point> lastInteractionPos; // Validity and position of interaction position during previous frame
	Scalar currentValue; // Value of the associated valuator
	
	/* Private methods: */
	std::pair<bool,Point> calcInteractionPos(void) const; // Intersects the input device's pointing ray with the controlled screen; boolean is false if intersection is invalid
	void applyCameraState(void); // Applies changed camera state to the controlled screen and viewer
	void startRotating(void); // Sets up rotation
	void startPanning(void); // Sets up panning
	void startDollying(void); // Sets up dollying
	void startScaling(void); // Sets up scaling
	
	/* Constructors and destructors: */
	public:
	MouseCameraTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	
	/* Methods from Tool: */
	virtual void configure(const Misc::ConfigurationFileSection& configFileSection);
	virtual void storeState(Misc::ConfigurationFileSection& configFileSection) const;
	virtual void initialize(void); // Called right after a tool has been created and is fully installed
	virtual void deinitialize(void); // Called right before a tool is destroyed during runtime
	virtual const ToolFactory* getFactory(void) const;
	virtual void buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData);
	virtual void valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

#endif
