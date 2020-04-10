/***********************************************************************
ShowEarthModel - Simple Vrui application to render a model of Earth,
with the ability to additionally display earthquake location data and
other geology-related stuff.
Copyright (c) 2005-2020 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SHOWEARTHMODEL_INCLUDED
#define SHOWEARTHMODEL_INCLUDED

#include <vector>
#include <Geometry/Geoid.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/Slider.h>
#include <Vrui/GeodeticCoordinateTransform.h>
#include <Vrui/ToolManager.h>
#include <Vrui/SurfaceNavigationTool.h>
#include <Vrui/Application.h>
#if USE_COLLABORATION
#include <Collaboration2/Plugins/KoinoniaClient.h>
#endif

#include "EarthquakeSet.h"

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
class GLPolylineTube;
namespace GLMotif {
class PopupMenu;
class PopupWindow;
class TextField;
}
namespace SceneGraph {
class GroupNode;
}
class PointSet;
class SeismicPath;

class ShowEarthModel:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	typedef Geometry::Geoid<Vrui::Scalar> Geoid; // Class to reference ellipsoids
	
	class RotatedGeodeticCoordinateTransform:public Vrui::GeodeticCoordinateTransform
		{
		/* Elements: */
		private:
		Vrui::Scalar rotationAngle; // Current rotation angle of the Earth model
		Vrui::Scalar raSin,raCos; // Sine and cosine of rotation angle
		
		/* Constructors and destructors: */
		public:
		RotatedGeodeticCoordinateTransform(void);
		
		/* Methods from GeodeticCoordinateTransform: */
		virtual const char* getUnitName(int componentIndex) const;
		virtual const char* getUnitAbbreviation(int componentIndex) const;
		virtual Vrui::Point transform(const Vrui::Point& navigationPoint) const;
		virtual Vrui::Point inverseTransform(const Vrui::Point& userPoint) const;
		
		/* New methods: */
		void setRotationAngle(Vrui::Scalar newRotationAngle);
		};
	
	struct Settings // Structure holding rendering settings
		{
		/* Elements: */
		public:
		static const int maxNumObjectFlags=32; // Maximum number of object enable flags
		float rotationAngle; // Current Earth rotation angle
		bool showSurface; // Flag if the Earth surface is rendered
		bool surfaceTransparent; // Flag if the Earth surface is rendered transparently
		float surfaceAlpha; // Opacity of Earth surface
		bool showGrid; // Flag if the long/lat grid is rendered
		float gridAlpha; // Opacity of long/lat grid
		bool showEarthquakeSets[maxNumObjectFlags]; // Flags to render individual earthquake sets
		bool showPointSets[maxNumObjectFlags]; // Flags to render individual additional point sets
		bool showSceneGraphs[maxNumObjectFlags]; // Flags to render individual scene graphs
		bool showSeismicPaths; // Flag if the seismic paths are rendered
		bool showOuterCore; // Flag if the outer core is rendered
		bool outerCoreTransparent; // Flag if the outer core is rendered transparently
		float outerCoreAlpha; // Opacity of outer core
		bool showInnerCore; // Flag if the inner core is rendered
		bool innerCoreTransparent; // Flag if the inner core is rendered transparently
		float innerCoreAlpha; // Opacity of inner core
		float earthquakePointSize; // Point size to render earthquake hypocenters
		double playSpeed; // Animation playback speed in real-world seconds per visualization second
		double currentTime; // Current animation time in seconds since the epoch in UTC
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		bool hasVertexBufferObjectExtension; // Flag if buffer objects are supported by the local GL
		GLuint surfaceVertexBufferObjectId; // Vertex buffer object ID for Earth surface
		GLuint surfaceIndexBufferObjectId; // Index buffer object ID for Earth surface
		GLuint surfaceTextureObjectId; // Texture object ID for Earth surface texture
		GLuint displayListIdBase; // Base ID of set of display lists for Earth model components
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	Geoid geoid; // The reference ellipsoid used to convert map coordinates to Cartesian coordinates
	std::vector<EarthquakeSet*> earthquakeSets; // Vector of earthquake sets to render
	EarthquakeSet::TimeRange earthquakeTimeRange; // Range to earthquake event times
	std::vector<PointSet*> pointSets; // Vector of additional point sets to render
	std::vector<SeismicPath*> seismicPaths; // Vector of seismic paths to render
	std::vector<GLPolylineTube*> sensorPaths; // Vector of sensor paths to render
	std::vector<SceneGraph::GroupNode*> sceneGraphs; // Vector of scene graphs to render
	Settings settings; // Rendering settings
	#if USE_COLLABORATION
	KoinoniaClient* koinonia; // Koinonia plug-in protocol
	KoinoniaProtocol::ObjectID settingsId; // Koinonia ID to share render settings
	#endif
	bool scaleToEnvironment; // Flag if the Earth model should be scaled to fit the environment
	bool rotateEarth; // Flag if the Earth model should be rotated
	double lastFrameTime; // Application time when last frame was rendered (to determine Earth angle updates)
	float rotationSpeed; // Earth rotation speed in degree/second
	RotatedGeodeticCoordinateTransform* userTransform; // Coordinate transformation from user space to navigation space
	Images::BaseImage surfaceImage; // Texture image for the Earth surface
	GLMaterial surfaceMaterial; // OpenGL material properties for the Earth surface
	GLMaterial outerCoreMaterial; // OpenGL material properties for the outer core
	GLMaterial innerCoreMaterial; // OpenGL material properties for the inner core
	GLMaterial sensorPathMaterial; // OpenGL material properties for sensor paths
	bool fog; // Flag whether depth cueing via fog is enabled
	float bpDist; // Current backplane distance for clipping and fog attenuation
	bool play; // Flag if automatic playback is enabled
	bool lockToSphere; // Flag whether the navigation transformation is locked to a fixed-radius sphere
	Vrui::Scalar sphereRadius; // Radius of the fixed sphere to which to lock the navigation transformation
	Vrui::NavTransform sphereTransform; // Transformation pre-applied to navigation transformation to lock it to a sphere
	GLMotif::PopupMenu* mainMenu; // The program's main menu
	GLMotif::PopupWindow* renderDialog; // The rendering settings dialog
	GLMotif::PopupWindow* animationDialog; // The animation dialog
	GLMotif::TextField* currentTimeValue; // Text field showing the current animation time
	GLMotif::Slider* currentTimeSlider; // Slider to adjust the current animation time
	GLMotif::TextField* playSpeedValue; // Text field showing the animation speed
	GLMotif::Slider* playSpeedSlider; // Slider to adjust the animation speed
	GLMotif::ToggleButton* playToggle; // Toggle button for automatic playback
	
	/* Private methods: */
	void settingsChangedCallback(Misc::CallbackData* cbData);
	#if USE_COLLABORATION
	static void settingsUpdatedCallback(KoinoniaClient* client,KoinoniaProtocol::ObjectID id,void* object,void* userData);
	#endif
	GLMotif::PopupMenu* createRenderTogglesMenu(void); // Creates the "Rendering Modes" submenu
	void rotateEarthValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void resetRotationCallback(Misc::CallbackData* cbData);
	void lockToSphereCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void showRenderDialogCallback(Misc::CallbackData* cbData);
	void showAnimationDialogCallback(Misc::CallbackData* cbData);
	GLMotif::PopupMenu* createMainMenu(void); // Creates the program's main menu
	void useFogCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void backplaneDistCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createRenderDialog(void); // Creates the rendering settings dialog
	void updateCurrentTime(void); // Updates the current time text field
	void currentTimeCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	void playSpeedCallback(GLMotif::Slider::ValueChangedCallbackData* cbData);
	GLMotif::PopupWindow* createAnimationDialog(void); // Create the animation dialog
	GLPolylineTube* readSensorPathFile(const char* sensorPathFileName,double scaleFactor);
	
	/* Constructors and destructors: */
	public:
	ShowEarthModel(int& argc,char**& argv);
	virtual ~ShowEarthModel(void);
	
	/* Methods: */
	virtual void initContext(GLContextData& contextData) const;
	virtual void toolCreationCallback(Vrui::ToolManager::ToolCreationCallbackData* cbData);
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	void alignSurfaceFrame(Vrui::SurfaceNavigationTool::AlignmentData& alignmentData);
	void setEventTime(double newEventTime);
	};

#endif
