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

#include <Vrui/Tools/MouseCameraTool.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/VRScreen.h>
#include <Vrui/Viewer.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>

namespace Vrui {

/******************************************************
Methods of class MouseCameraToolFactory::Configuration:
******************************************************/

MouseCameraToolFactory::Configuration::Configuration(void)
	:windowIndex(0),
	 rotateFactor(8),
	 invertDolly(false),
	 dollyCenter(true),scaleCenter(true),
	 dollyingDirection(Vector(0,-1,0)),
	 scalingDirection(Vector(0,-1,0)),
	 dollyFactor(1),
	 scaleFactor(4),
	 wheelDollyFactor(Scalar(0.5)),
	 wheelScaleFactor(Scalar(0.5)),
	 spinThreshold(getUiSize()/getDisplaySize()),
	 showScreenCenter(true),showFrustum(true)
	{
	}

void MouseCameraToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	/* Retrieve the tool configuration parameters: */
	windowIndex=cfs.retrieveValue<int>("./windowIndex",windowIndex);
	rotateFactor=cfs.retrieveValue<Scalar>("./rotateFactor",rotateFactor);
	invertDolly=cfs.retrieveValue<bool>("./invertDolly",invertDolly);
	dollyCenter=cfs.retrieveValue<bool>("./dollyCenter",dollyCenter);
	scaleCenter=cfs.retrieveValue<bool>("./scaleCenter",scaleCenter);
	dollyingDirection=cfs.retrieveValue<Vector>("./dollyingDirection",dollyingDirection);
	scalingDirection=cfs.retrieveValue<Vector>("./scalingDirection",scalingDirection);
	dollyFactor=cfs.retrieveValue<Scalar>("./dollyFactor",dollyFactor);
	scaleFactor=cfs.retrieveValue<Scalar>("./scaleFactor",scaleFactor);
	wheelDollyFactor=cfs.retrieveValue<Scalar>("./wheelDollyFactor",wheelDollyFactor);
	wheelScaleFactor=cfs.retrieveValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	spinThreshold=cfs.retrieveValue<Scalar>("./spinThreshold",spinThreshold);
	showScreenCenter=cfs.retrieveValue<bool>("./showScreenCenter",showScreenCenter);
	showFrustum=cfs.retrieveValue<bool>("./showFrustum",showFrustum);
	}

void MouseCameraToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	/* Store the tool configuration parameters: */
	cfs.storeValue<int>("./windowIndex",windowIndex);
	cfs.storeValue<Scalar>("./rotateFactor",rotateFactor);
	cfs.storeValue<bool>("./invertDolly",invertDolly);
	cfs.storeValue<bool>("./dollyCenter",dollyCenter);
	cfs.storeValue<bool>("./scaleCenter",scaleCenter);
	cfs.storeValue<Vector>("./dollyingDirection",dollyingDirection);
	cfs.storeValue<Vector>("./scalingDirection",scalingDirection);
	cfs.storeValue<Scalar>("./dollyFactor",dollyFactor);
	cfs.storeValue<Scalar>("./scaleFactor",scaleFactor);
	cfs.storeValue<Scalar>("./wheelDollyFactor",wheelDollyFactor);
	cfs.storeValue<Scalar>("./wheelScaleFactor",wheelScaleFactor);
	cfs.storeValue<Scalar>("./spinThreshold",spinThreshold);
	cfs.storeValue<bool>("./showScreenCenter",showScreenCenter);
	cfs.storeValue<bool>("./showFrustum",showFrustum);
	}

/***************************************
Methods of class MouseCameraToolFactory:
***************************************/

MouseCameraToolFactory::MouseCameraToolFactory(ToolManager& toolManager)
	:ToolFactory("MouseCameraTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(6);
	layout.setNumValuators(1);
	
	/* Insert class into class hierarchy: */
	ToolFactory* toolFactory=toolManager.loadClass("UtilityTool");
	toolFactory->addChildClass(this);
	addParentClass(toolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	MouseCameraTool::factory=this;
	}

MouseCameraToolFactory::~MouseCameraToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MouseCameraTool::factory=0;
	}

const char* MouseCameraToolFactory::getName(void) const
	{
	return "Mouse Camera Control";
	}

const char* MouseCameraToolFactory::getButtonFunction(int buttonSlotIndex) const
	{
	switch(buttonSlotIndex)
		{
		case 0:
			return "Rotate";
		
		case 1:
			return "Pan";
		
		case 2:
			return "Zoom/Dolly Switch";
		
		case 3:
			return "Main Viewer View";
		
		case 4:
			return "Show Frustum";
		
		case 5:
			return "Reset Camera";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

const char* MouseCameraToolFactory::getValuatorFunction(int valuatorSlotIndex) const
	{
	switch(valuatorSlotIndex)
		{
		case 0:
			return "Quick Zoom/Dolly";
		}
	
	/* Never reached; just to make compiler happy: */
	return 0;
	}

Tool* MouseCameraToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MouseCameraTool(this,inputAssignment);
	}

void MouseCameraToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMouseCameraToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("UtilityTool");
	}

extern "C" ToolFactory* createMouseCameraToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MouseCameraToolFactory* mouseCameraToolFactory=new MouseCameraToolFactory(*toolManager);
	
	/* Return factory object: */
	return mouseCameraToolFactory;
	}

extern "C" void destroyMouseCameraToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/****************************************
Static elements of class MouseCameraTool:
****************************************/

MouseCameraToolFactory* MouseCameraTool::factory=0;

/********************************
Methods of class MouseCameraTool:
********************************/

std::pair<bool,Point> MouseCameraTool::calcInteractionPos(void) const
	{
	/* Transform the device's pointing ray into the controlled screen's space: */
	Ray ray=getButtonDeviceRay(0);
	ray.inverseTransform(screen->getTransform());
	
	/* Intersect the transformed ray with the screen plane: */
	if(ray.getOrigin()[2]<=Scalar(0)||ray.getDirection()[2]>=Scalar(0))
		return std::pair<bool,Point>(false,Point::origin);
	else
		{
		Scalar lambda=(Scalar(0)-ray.getOrigin()[2])/ray.getDirection()[2];
		Point p=ray(lambda);
		p[0]/=scale;
		p[1]/=scale;
		p[2]=Scalar(0);
		return std::pair<bool,Point>(true,p);
		}
	}

void MouseCameraTool::applyCameraState(void)
	{
	/* Assemble the camera transformation: */
	ONTransform cameraTransform=ONTransform::translateFromOriginTo(focus);
	cameraTransform*=ONTransform::rotate(Rotation::rotateAxis(azimuthAxis,azimuth));
	cameraTransform*=ONTransform::rotate(Rotation::rotateAxis(elevationAxis,elevation));
	cameraTransform*=ONTransform::translateToOriginFrom(screenCenter);
	
	/* Position and scale the screen: */
	screen->setSize(screenSize[0]*scale,screenSize[1]*scale);
	ONTransform screenT=cameraTransform;
	screenT*=physScreenTransform;
	screenT*=ONTransform::translate(Vector(Math::div2(screenSize[0]*(Scalar(1)-scale)),Math::div2(screenSize[1]*(Scalar(1)-scale)),0));
	screenT.renormalize();
	screen->setTransform(screenT);
	
	/* Position and scale the viewer: */
	ONTransform viewerT=cameraTransform;
	viewerT*=ONTransform::translate((screenCenter-physViewerTransform.getOrigin())*(Scalar(1)-scale));
	viewerT*=physViewerTransform;
	viewerT.renormalize();
	viewer->detachFromDevice(viewerT); // This looks a tad weird, but is perfectly cromulent
	viewer->setEyes(viewerViewDirection,Point::origin+(viewerEyePos-Point::origin)*scale,viewerEyeOffset*scale);
	
	if(mouseAdapter!=0)
		{
		/* Update the mouse input device based on the new viewer/screen combination: */
		mouseAdapter->invalidateMousePosition();
		}
	}

void MouseCameraTool::startRotating(void)
	{
	/* Set initial interaction position: */
	lastInteractionPos=calcInteractionPos();
	
	/* Go to rotating mode: */
	cameraMode=ROTATING;
	}

void MouseCameraTool::startPanning(void)
	{
	/* Set initial interaction position: */
	lastInteractionPos=calcInteractionPos();
	
	/* Go to panning mode: */
	cameraMode=PANNING;
	}

void MouseCameraTool::startDollying(void)
	{
	/* Set initial interaction position: */
	lastInteractionPos=calcInteractionPos();
	
	/* Go to dollying mode: */
	cameraMode=DOLLYING;
	}

void MouseCameraTool::startScaling(void)
	{
	/* Set initial interaction position: */
	lastInteractionPos=calcInteractionPos();
	
	/* Go to scaling mode: */
	cameraMode=SCALING;
	}

MouseCameraTool::MouseCameraTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment)
	:UtilityTool(factory,inputAssignment),
	 configuration(MouseCameraTool::factory->configuration),
	 mouseAdapter(0),
	 screen(0),screenDevice(0),
	 viewer(0),viewerDevice(0),
	 showFrustum(false),
	 lockToMainViewer(false),
	 dolly(configuration.invertDolly),cameraMode(IDLE)
	{
	}

void MouseCameraTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void MouseCameraTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

void MouseCameraTool::initialize(void)
	{
	/* Check if the main input device is a mouse: */
	mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(getInputDeviceManager()->findInputDeviceAdapter(getButtonDevice(0)));
	
	/* Access the window whose camera is to be controlled: */
	VRWindow* window=getWindow(configuration.windowIndex);
	if(window==0)
		Misc::throwStdErr("MouseCameraTool: Invalid window index %d",configuration.windowIndex);
	
	/* Check that the window only uses a single screen and a single viewer: */
	if(window->getVRScreen(0)!=window->getVRScreen(1))
		Misc::throwStdErr("MouseCameraTool: Window %d has two different screens attached",configuration.windowIndex);
	if(window->getViewer(0)!=window->getViewer(1))
		Misc::throwStdErr("MouseCameraTool: Window %d has two different viewers attached",configuration.windowIndex);
	
	/* Get the camera screen, detach it from a potential tracking device, and save its initial state: */
	screen=window->getVRScreen(0);
	physScreenTransform=screen->getScreenTransformation();
	screenDevice=screen->attachToDevice(0);
	screenTransform=screen->getTransform();
	for(int i=0;i<2;++i)
		screenSize[i]=screen->getScreenSize()[i];
	screenDiagonal=Math::sqrt(Math::sqr(screenSize[0])+Math::sqr(screenSize[1]));
	screenCenter=physScreenTransform.transform(Point(Math::div2(screenSize[0]),Math::div2(screenSize[1]),0));
	
	/* Get the camera viewer, detach it from a potential tracking device, and save its initial state: */
	viewer=window->getViewer(0);
	physViewerTransform=viewer->getHeadTransformation();
	viewerDevice=viewer->detachFromDevice(physViewerTransform);
	viewerViewDirection=viewer->getDeviceViewDirection();
	viewerEyePos=viewer->getDeviceEyePosition(Viewer::MONO);
	viewerEyeOffset=(viewer->getDeviceEyePosition(Viewer::RIGHT)-viewer->getDeviceEyePosition(Viewer::LEFT))*Scalar(0.5);
	
	/* Calculate the elevation and azimuth rotation axes: */
	elevationAxis=getForwardDirection()^getUpDirection();
	elevationAxis.normalize();
	azimuthAxis=getUpDirection();
	azimuthAxis.normalize();
	
	/* Initialize the virtual camera state: */
	focus=screenCenter;
	azimuth=elevation=Scalar(0);
	scale=Scalar(1);
	
	showFrustum=configuration.showFrustum;
	
	lockToMainViewer=false;
	dolly=false;
	cameraMode=IDLE;
	}

void MouseCameraTool::deinitialize(void)
	{
	/* Re-attach the camera screen to its original tracking device: */
	screen->attachToDevice(screenDevice);
	
	/* Restore the camera screen's transformation and dimensions: */
	screen->setSize(screenSize[0],screenSize[1]);
	screen->setTransform(screenTransform);
	
	/* Re-attach the viewer to its original tracking device and restore its transformation: */
	if(viewerDevice!=0)
		viewer->attachToDevice(viewerDevice);
	else
		viewer->detachFromDevice(physViewerTransform);
	
	/* Restore the viewer's dimensions: */
	viewer->setEyes(viewerViewDirection,viewerEyePos,viewerEyeOffset);
	}

const ToolFactory* MouseCameraTool::getFactory(void) const
	{
	return factory;
	}

void MouseCameraTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Process based on which button was pressed: */
	switch(buttonSlotIndex)
		{
		case 0: // Rotate button
			if(cbData->newButtonState)
				{
				switch(cameraMode)
					{
					case IDLE:
					case SPINNING:
						startRotating();
						break;
					
					case PANNING:
						if(dolly)
							startDollying();
						else
							startScaling();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						;
					}
				}
			else
				{
				switch(cameraMode)
					{
					case ROTATING:
						/* Check for spinning here... */
						
						cameraMode=IDLE;
						break;
					
					case DOLLYING:
					case SCALING:
						startPanning();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						;
					}
				}
			break;
		
		case 1: // Pan button
			if(cbData->newButtonState)
				{
				switch(cameraMode)
					{
					case IDLE:
					case SPINNING:
						startPanning();
						break;
					
					case ROTATING:
						if(dolly)
							startDollying();
						else
							startScaling();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						;
					}
				}
			else
				{
				switch(cameraMode)
					{
					case PANNING:
						cameraMode=IDLE;
						break;
					
					case DOLLYING:
					case SCALING:
						startRotating();
						break;
					
					default:
						/* This shouldn't happen; just ignore the event */
						;
					}
				}
			break;
		
		case 2: // Zoom/dolly switch
			/* Set the dolly flag: */
			dolly=cbData->newButtonState;
			if(configuration.invertDolly)
				dolly=!dolly;
			
			/* Act depending on the new value of the dolly flag: */
			if(dolly)
				{
				/* Act depending on this tool's current state: */
				switch(cameraMode)
					{
					case SCALING:
						startDollying();
						break;
					
					case SCALING_WHEEL:
						startDollying();
						cameraMode=DOLLYING_WHEEL;
						break;
					
					default:
						/* Nothing to do */
						;
					}
				}
			else
				{
				/* Act depending on this tool's current state: */
				switch(cameraMode)
					{
					case DOLLYING:
						startScaling();
						break;
					
					case DOLLYING_WHEEL:
						startScaling();
						cameraMode=SCALING_WHEEL;
						break;
					
					default:
						/* Nothing to do */
						;
					}
				}
			break;
		
		case 3: // Attach to main viewer
			break;
		
		case 4: // Show frustum
			if(cbData->newButtonState)
				showFrustum=!showFrustum;
			break;
		
		case 5: // Reset camera
			if(cbData->newButtonState)
				{
				/* Reset the camera transform to identity: */
				focus=screenCenter;
				azimuth=elevation=Scalar(0);
				scale=Scalar(1);
				applyCameraState();
				}
			break;
		}
	}

void MouseCameraTool::valuatorCallback(int valuatorSlotIndex,InputDevice::ValuatorCallbackData* cbData)
	{
	currentValue=Scalar(cbData->newValuatorValue);
	if(currentValue!=Scalar(0))
		{
		/* Act depending on this tool's current state: */
		switch(cameraMode)
			{
			case IDLE:
			case SPINNING:
				if(dolly)
					{
					/* Start normal dollying: */
					startDollying();
					
					/* Change to wheel dollying mode: */
					cameraMode=DOLLYING_WHEEL;
					}
				else
					{
					/* Start normal scaling: */
					startScaling();
					
					/* Change to wheel scaling mode: */
					cameraMode=SCALING_WHEEL;
					}
				break;
			
			default:
				/* This can definitely happen; just ignore the event */
				break;
			}
		}
	else
		{
		/* Act depending on this tool's current state: */
		switch(cameraMode)
			{
			case DOLLYING_WHEEL:
			case SCALING_WHEEL:
				/* Go to idle mode: */
				cameraMode=IDLE;
				break;
			
			default:
				/* This can definitely happen; just ignore the event */
				break;
			}
		}
	}

void MouseCameraTool::frame(void)
	{
	/* Bail out if the tool is idle: */
	if(cameraMode==IDLE)
		return;
	
	/* Check if the tool is spinning: */
	if(cameraMode==SPINNING)
		{
		/* Bail out: */
		return;
		}
	
	#if 0
	/* Check if the camera is locked to the environment's main viewer: */
	if(lockToMainViewer)
		{
		/* Erect a coordinate frame around the main viewer's head position and viewing direction in physical space: */
		Point headPos=getMainViewer()->getHeadPosition();
		Vector headY=getMainViewer()->getViewDirection();
		Vector headX=headY^getUpDirection();
		Rotation headRot=Rotation::fromBaseVectors(headX,headY);
		OGTransform headTransform(headPos-Point::origin,headRot,cameraTransform.getScaling());
		}
	#endif
	
	if(cameraMode==DOLLYING_WHEEL)
		{
		/* Calculate an incremental translation vector: */
		Vector trans=viewer->getHeadPosition()-focus;
		trans*=configuration.wheelDollyFactor*currentValue;
		
		/* Update the screen center: */
		focus+=trans;
		applyCameraState();
		}
	else if(cameraMode==SCALING_WHEEL)
		{
		/* Calculate an incremental scale factor: */
		scale*=Math::pow(configuration.wheelScaleFactor,-currentValue);
		applyCameraState();
		}
	else
		{
		/* Calculate the new interaction position: */
		std::pair<bool,Point> interactionPos=calcInteractionPos();
		
		/* Only act if both the old and new positions are valid: */
		if(lastInteractionPos.first&&interactionPos.first)
			{
			Vector offset=lastInteractionPos.second-interactionPos.second;
			
			switch(cameraMode)
				{
				case ROTATING:
					{
					/* Calculate an incremental rotation around the screen's center: */
					azimuth+=offset[0]*configuration.rotateFactor/screenDiagonal;
					if(azimuth<Math::rad(Scalar(-180)))
						azimuth+=Math::rad(Scalar(360));
					else if(azimuth>=Math::rad(Scalar(180)))
						azimuth-=Math::rad(Scalar(360));
					elevation-=offset[1]*configuration.rotateFactor/screenDiagonal;
					elevation=Math::clamp(elevation,Math::rad(Scalar(-90)),Math::rad(Scalar(90)));
					applyCameraState();
					break;
					}
				
				case PANNING:
					{
					/* Calculate an incremental translation vector: */
					Vector trans=screen->getTransform().transform(offset*scale);
					focus+=trans;
					applyCameraState();
					break;
					}
				
				case DOLLYING:
					{
					/* Calculate an incremental translation vector: */
					Scalar dollyDist=(offset*configuration.dollyingDirection)*configuration.dollyFactor/screenDiagonal;
					Vector trans=viewer->getHeadPosition()-focus;
					trans*=dollyDist;
					
					/* Update the screen center: */
					focus+=trans;
					applyCameraState();
					break;
					}
				
				case SCALING:
					{
					/* Calculate an incremental scale factor: */
					scale*=Math::exp((offset*configuration.scalingDirection)*configuration.scaleFactor/screenDiagonal);
					applyCameraState();
					break;
					}
				
				default:
					/* Do nothing: */
					;
				}
			}
		
		lastInteractionPos=interactionPos;
		}
	}

void MouseCameraTool::display(GLContextData& contextData) const
	{
	/* Determine whether something needs to be drawn: */
	int windowIndex=getDisplayState(contextData).windowIndex;
	bool drawScreenCenter=configuration.showScreenCenter&&cameraMode!=IDLE&&windowIndex==configuration.windowIndex;
	bool drawFrustum=showFrustum&&windowIndex!=configuration.windowIndex;
	if(drawScreenCenter||drawFrustum)
		{
		/* Save and set up OpenGL state: */
		glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Go to screen space: */
		glPushMatrix();
		const ONTransform& screenT=screen->getTransform();
		glMultMatrix(screenT);
		Scalar sw=screen->getWidth();
		Scalar sh=screen->getHeight();
		
		if(drawScreenCenter)
			{
			glDepthFunc(GL_LEQUAL);
			
			/* Draw the screen center crosshairs: */
			glLineWidth(3.0f);
			glColor(getBackgroundColor());
			glBegin(GL_LINES);
			glVertex(Point(0,Math::div2(sh),0));
			glVertex(Point(sw,Math::div2(sh),0));
			glVertex(Point(Math::div2(sw),0,0));
			glVertex(Point(Math::div2(sw),sh,0));
			glEnd();
			glLineWidth(1.0f);
			glColor(getForegroundColor());
			glBegin(GL_LINES);
			glVertex(Point(0,Math::div2(sh),0));
			glVertex(Point(sw,Math::div2(sh),0));
			glVertex(Point(Math::div2(sw),0,0));
			glVertex(Point(Math::div2(sw),sh,0));
			glEnd();
			}
		
		if(drawFrustum)
			{
			Point eye=screenT.inverseTransform(viewer->getHeadPosition());
			Point c[4];
			c[0]=Point(0,0,0);
			c[1]=Point(sw,0,0);
			c[2]=Point(sw,sh,0);
			c[3]=Point(0,sh,0);
			Scalar fp=getFrontplaneDist()/eye[2];
			Scalar bp=getBackplaneDist()/eye[2];
			
			glLineWidth(1.0f);
			glColor(getForegroundColor());
			
			/* Draw the front plane: */
			glBegin(GL_LINE_LOOP);
			for(int i=0;i<4;++i)
				glVertex(Geometry::affineCombination(eye,c[i],fp));
			glEnd();
			
			/* Draw the screen: */
			glBegin(GL_LINE_LOOP);
			for(int i=0;i<4;++i)
				glVertex(c[i]);
			glEnd();
			
			/* Draw the back plane: */
			glBegin(GL_LINE_LOOP);
			for(int i=0;i<4;++i)
				glVertex(Geometry::affineCombination(eye,c[i],bp));
			glEnd();
			
			/* Draw the frustum: */
			glBegin(GL_LINES);
			for(int i=0;i<4;++i)
				{
				glVertex(eye);
				glVertex(Geometry::affineCombination(eye,c[i],bp));
				}
			glVertex(eye);
			glVertex(Geometry::affineCombination(eye,Point(Math::div2(sw),Math::div2(sh),0),bp));
			glEnd();
			}
		
		/* Return to physical space: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

}
