/***********************************************************************
MeasurePoints - Vrui application to measure sets of 3D positions using a
tracked VR input device.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef MEASUREPOINTS_INCLUDED
#define MEASUREPOINTS_INCLUDED

#include <string>
#include <vector>
#include <Threads/Mutex.h>
#include <Threads/TripleBuffer.h>
#include <Geometry/Point.h>
#include <Geometry/AffineCombiner.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/LinearUnit.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLNumberRenderer.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextField.h>
#include <GLMotif/ListBox.h>
#include <GLMotif/HSVColorSelector.h>
#include <GLMotif/FileSelectionHelper.h>
#include <Vrui/Geometry.h>
#include <Vrui/Application.h>
#include <Vrui/ObjectSnapperTool.h>
#include <Vrui/Internal/VRDeviceState.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace GLMotif {
class PopupMenu;
class PopupWindow;
class ScrolledListBox;
}
namespace Vrui {
class VRDeviceClient;
class VRDeviceDescriptor;
}

class MeasurePoints:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef std::vector<const Vrui::VRDeviceDescriptor*> DeviceList; // Type for lists of virtual input devices
	typedef float Scalar; // Scalar type for VRDeviceDaemon's affine space
	typedef Geometry::Point<Scalar,3> Point; // Point type
	typedef Geometry::OrthonormalTransformation<Scalar,3> ONTransform; // List for tracker transformations
	typedef Geometry::OrthogonalTransformation<Scalar,3> OGTransform; // List for point set alignment transformations
	typedef std::vector<Point> PointList; // Type for lists of point measurements
	typedef GLColor<GLfloat,3> Color; // Type for colors
	
	class ProbeTipCalibrator; // Class to calculate the probe tip position of a tracked input device
	
	struct PointSet // Structure for sets of measured points
		{
		/* Elements: */
		public:
		std::string label; // Label for the point set
		Color color; // Color to draw the point set
		OGTransform transform; // Transformation to be applied to the point set
		PointList points; // List of points in the set
		bool draw; // Flag whether to draw this point set
		};
	
	typedef std::vector<PointSet> PointSetList; // Type for lists of point sets
	
	/* Elements: */
	static const Color pointSetColors[]; // Default colors for point sets
	Vrui::VRDeviceClient* deviceClient; // Connection to the VRDeviceDaemon
	DeviceList buttonDevices; // Map from button indices to virtual input devices advertised by the VRDeviceDaemon
	Geometry::LinearUnit trackingUnit; // Measurement unit of VRDeviceDaemon's coordinate space
	Threads::TripleBuffer<Vrui::VRDeviceState::TrackerState::PositionOrientation> trackerFrame; // Triple buffer of tracking state for the active tracker
	Point probeTip; // Position of measurement probe tip in tracked device's local coordinate system
	int triggerButtonIndex; // Index of the button to trigger measurements in VRDeviceDaemon's namespace
	bool triggerButtonIndexChanged; // Flag whether the trigger button index changed recently
	bool triggerButtonState; // Current state of trigger button
	unsigned int numSamples; // Number of samples to be averaged for each 3D position measurement
	unsigned int numSamplesLeft; // Number of measurement samples still outstanding
	Point::AffineCombiner sampleAccumulator; // Accumulator for 3D position measurement samples
	mutable Threads::Mutex pointSetsMutex; // Mutex protecting the point sets
	PointSetList pointSets; // List of separate point sets
	size_t activePointSet; // Index of currently active point set
	ProbeTipCalibrator* calibrator; // Temporary calibration object to calculate the position of a tracked input device's probe tip
	GLMotif::FileSelectionHelper saveHelper; // Helper object to load and save point sets
	GLMotif::Popup* pressButtonPrompt; // Popup window to prompt user to press an input device button
	GLMotif::PopupMenu* mainMenu; // The main menu
	GLMotif::PopupWindow* calibrationDialog; // Dialog window to calibrate a measurement probe tip
	GLMotif::PopupWindow* pointSetsDialog; // Dialog window to manage point sets
	GLMotif::ScrolledListBox* pointSetList;
	GLMotif::TextField* pointSetLabel;
	GLMotif::ToggleButton* pointSetDraw;
	GLMotif::HSVColorSelector* pointSetColor;
	GLNumberRenderer numberRenderer; // Helper object to label measured points with indices
	
	/* Private methods: */
	void trackingCallback(Vrui::VRDeviceClient* client); // Called when new tracking data arrives
	void setActivePointSet(size_t newActivePointSet); // Sets the active point set
	void startNewPointSet(void); // Starts a new point set
	void loadPointSets(IO::Directory& directory,const char* fileName); // Loads point sets from a file of the given name inside the given directory; appends loaded sets to current point set list
	void savePointSets(IO::Directory& directory,const char* fileName) const; // Saves all point sets to a file of the given name inside the given directory
	void objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest); // Callback called when an object snapper tool issues a snap request
	GLMotif::Popup* createPressButtonPrompt(void);
	void selectDeviceCallback(Misc::CallbackData* cbData); // Callback when the user wants to select a new measurement input device
	void calibrateCallback(Misc::CallbackData* cbData); // Callback called when the user wants to show the calibration dialog
	void loadPointSetsCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback called when the user wants to load point sets
	void savePointSetsCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData); // Callback called when the user wants to save the current point sets
	void showPointSetsDialogCallback(Misc::CallbackData* cbData); // Callback called when the user wants to show the point sets dialog
	GLMotif::PopupMenu* createMainMenu(void); // Creates the main menu
	void calibrationAddSampleCallback(Misc::CallbackData* cbData);
	void calibrationOkCallback(Misc::CallbackData* cbData);
	void calibrationCancelCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createCalibrationDialog(void); // Creates the probe tip calibration dialog
	void pointSetListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData);
	void addPointSetCallback(Misc::CallbackData* cbData); // Callback when the user wants to add a point set
	void deletePointSetCallback(Misc::CallbackData* cbData); // Callback when the user wants to remove a point set
	void pointSetLabelValueChangedCallback(GLMotif::TextField::ValueChangedCallbackData* cbData);
	void pointSetDrawValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void pointSetColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData);
	void pointSetResetTransformCallback(Misc::CallbackData* cbData);
	GLMotif::PopupWindow* createPointSetsDialog(void); // Creates the point set dialog
	
	/* Constructors and destructors: */
	public:
	MeasurePoints(int& argc,char**& argv);
	virtual ~MeasurePoints(void);
	
	/* Methods from class Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	};

#endif
