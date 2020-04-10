/***********************************************************************
Viewer - Class for viewers/observers in VR environments.
Copyright (c) 2004-2020 Oliver Kreylos

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

#include <Vrui/Viewer.h>

#include <string.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/CommandDispatcher.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLLightTemplates.h>
#include <GL/GLLight.h>
#include <GL/GLValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapter.h>

namespace Vrui {

/***********************
Methods of class Viewer:
***********************/

void Viewer::setHeadDeviceCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	Viewer* thisPtr=static_cast<Viewer*>(userData);
	
	/* Parse the new head device name: */
	std::string newHeadDeviceName(argumentsBegin,argumentsEnd);
	
	/* Attach the viewer to the new device, or disable head tracking if device name is empty: */
	if(newHeadDeviceName.empty())
		thisPtr->attachToDevice(0);
	else
		{
		Vrui::InputDevice* newHeadDevice=findInputDevice(newHeadDeviceName.c_str());
		if(newHeadDevice==0)
			Misc::throwStdErr("Viewer::setHeadDeviceCallback: Head device \"%s\" not found",newHeadDeviceName.c_str());
		thisPtr->attachToDevice(newHeadDevice);
		}
	}

void Viewer::setHeadTransformCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	Viewer* thisPtr=static_cast<Viewer*>(userData);
	
	if(thisPtr->headTracked)
		throw std::runtime_error("Viewer::setHeadTransformCallback: Viewer is head-tracked");
	
	/* Parse the new head transformation: */
	TrackerState newHeadTransform=Misc::ValueCoder<TrackerState>::decode(argumentsBegin,argumentsEnd);
	
	/* Override the head transformation: */
	thisPtr->headDeviceTransformation=newHeadTransform;
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(thisPtr,ConfigChangedCallbackData::HeadDevice);
	thisPtr->configChangedCallbacks.call(&cbData);
	}
	}

void Viewer::setMonoEyePosCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	Viewer* thisPtr=static_cast<Viewer*>(userData);
	
	/* Parse the new mono eye position: */
	Point newMonoEyePos=Misc::ValueCoder<Point>::decode(argumentsBegin,argumentsEnd);
	
	/* Override the eye positions: */
	Vector offset=newMonoEyePos-thisPtr->deviceMonoEyePosition;
	thisPtr->deviceMonoEyePosition=newMonoEyePos;
	thisPtr->deviceLeftEyePosition+=offset;
	thisPtr->deviceRightEyePosition+=offset;
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(thisPtr,ConfigChangedCallbackData::EyePositions);
	thisPtr->configChangedCallbacks.call(&cbData);
	}
	}

void Viewer::setIPDCallback(const char* argumentsBegin,const char* argumentsEnd,void* userData)
	{
	Viewer* thisPtr=static_cast<Viewer*>(userData);
	
	/* Parse the new inter-pupillary distance: */
	Scalar newIPD=Misc::ValueCoder<Scalar>::decode(argumentsBegin,argumentsEnd);
	
	/* Set the new IPD: */
	thisPtr->setIPD(newIPD);
	}

void Viewer::inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Set viewer state if this is our head tracking device: */
	if(headTracked&&cbData->inputDevice==headDevice)
		enabled=cbData->newEnabled;
	}

Viewer::Viewer(void)
	:viewerName(0),
	 headTracked(false),headDevice(0),headDeviceAdapter(0),headDeviceIndex(-1),
	 headDeviceTransformation(TrackerState::identity),
	 deviceViewDirection(0,1,0),deviceUpDirection(0,0,1),
	 deviceMonoEyePosition(Point::origin),
	 deviceLeftEyePosition(Point::origin),
	 deviceRightEyePosition(Point::origin),
	 lightsource(0),
	 headLightDevicePosition(Point::origin),
	 headLightDeviceDirection(0,1,0),
	 enabled(true)
	{
	/* Create the viewer's light source: */
	lightsource=getLightsourceManager()->createLightsource(true);
	
	/* Disable the light source by default: */
	lightsource->disable();
	
	/* Register callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().add(this,&Viewer::inputDeviceStateChangeCallback);
	}

Viewer::~Viewer(void)
	{
	delete[] viewerName;
	if(lightsource!=0)
		getLightsourceManager()->destroyLightsource(lightsource);
	
	/* Unregister callbacks with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().remove(this,&Viewer::inputDeviceStateChangeCallback);
	}

void Viewer::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Read the viewer's name: */
	std::string name=configFileSection.retrieveString("./name",configFileSection.getName());
	viewerName=new char[name.size()+1];
	strcpy(viewerName,name.c_str());
	
	/* Determine whether the viewer is head-tracked: */
	headTracked=configFileSection.retrieveValue<bool>("./headTracked",headTracked);
	if(headTracked)
		{
		/* Retrieve head tracking device pointer: */
		headDevice=findInputDevice(configFileSection.retrieveString("./headDevice").c_str());
		if(headDevice==0)
			Misc::throwStdErr("Viewer: Head device \"%s\" not found",configFileSection.retrieveString("./headDevice").c_str());
		attachToDevice(headDevice);
		}
	else
		{
		/* Retrieve fixed head position/orientation: */
		headDeviceTransformation=configFileSection.retrieveValue<TrackerState>("./headDeviceTransformation");
		}
	
	/* Get view direction and eye positions in head device coordinates: */
	deviceViewDirection=configFileSection.retrieveValue<Vector>("./viewDirection",deviceViewDirection);
	deviceUpDirection=configFileSection.retrieveValue<Vector>("./upDirection",deviceUpDirection);
	deviceMonoEyePosition=configFileSection.retrieveValue<Point>("./monoEyePosition",deviceMonoEyePosition);
	deviceLeftEyePosition=configFileSection.retrieveValue<Point>("./leftEyePosition",deviceLeftEyePosition);
	deviceRightEyePosition=configFileSection.retrieveValue<Point>("./rightEyePosition",deviceRightEyePosition);
	
	/* Get head light enable flag: */
	if(configFileSection.retrieveValue<bool>("./headLightEnabled",true))
		lightsource->enable();
	
	/* Get head light position and direction in head device coordinates: */
	headLightDevicePosition=configFileSection.retrieveValue<Point>("./headLightPosition",deviceMonoEyePosition);
	headLightDeviceDirection=configFileSection.retrieveValue<Vector>("./headLightDirection",deviceViewDirection);
	
	/* Retrieve head light settings: */
	GLLight::Color headLightColor=configFileSection.retrieveValue<GLLight::Color>("./headLightColor",GLLight::Color(1.0f,1.0f,1.0f));
	lightsource->getLight().diffuse=headLightColor;
	lightsource->getLight().specular=headLightColor;
	lightsource->getLight().spotCutoff=configFileSection.retrieveValue<GLfloat>("./headLightSpotCutoff",180.0f);
	lightsource->getLight().spotExponent=configFileSection.retrieveValue<GLfloat>("./headLightSpotExponent",0.0f);
	
	/* Initialize head light source if head tracking is disabled: */
	if(!headTracked)
		{
		Point hlp=headDeviceTransformation.transform(headLightDevicePosition);
		lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
		Vector hld=headDeviceTransformation.transform(headLightDeviceDirection);
		hld.normalize();
		lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
		}
	
	/* Register pipe command callbacks: */
	getCommandDispatcher().addCommandCallback(("Viewer("+name+").setHeadDevice").c_str(),&Viewer::setHeadDeviceCallback,this,"<head device name>","Attaches the viewer to the tracked input device of the given name");
	getCommandDispatcher().addCommandCallback(("Viewer("+name+").setHeadTransform").c_str(),&Viewer::setHeadTransformCallback,this,"<head transformation string>","Sets the viewer's fixed head transformation in physical space");
	getCommandDispatcher().addCommandCallback(("Viewer("+name+").setMonoEyePos").c_str(),&Viewer::setMonoEyePosCallback,this,"(<eye X>, <eye Y>, <eye Z>)","Sets the position of the viewer's monoscopic eye in head space");
	getCommandDispatcher().addCommandCallback(("Viewer("+name+").setIPD").c_str(),&Viewer::setIPDCallback,this,"<IPD>","Sets viewer's inter-pupillary distance in physical coordinate units");
	}

InputDevice* Viewer::attachToDevice(InputDevice* newHeadDevice)
	{
	/* Return the previous head device: */
	InputDevice* result=headTracked?headDevice:0;
	
	/* Set the new head device and update the head tracked flag: */
	headTracked=newHeadDevice!=0;
	headDevice=newHeadDevice;
	
	/* Update the viewer's state: */
	if(headTracked)
		{
		/* Get the new head device's adapter and device index: */
		headDeviceAdapter=getInputDeviceManager()->findInputDeviceAdapter(headDevice);
		headDeviceIndex=headDeviceAdapter->findInputDevice(headDevice);
		
		/* Check if the head device is currently enabled: */
		enabled=getInputGraphManager()->isEnabled(headDevice);
		}
	else
		{
		/* If the viewer was previously head tracked, initialize its static transformation with the previous head device's current transformation: */
		if(result!=0)
			headDeviceTransformation=result->getTransformation();
		
		/* Reset device adapter and index: */
		headDeviceAdapter=0;
		headDeviceIndex=-1;
		
		/* Enable the viewer: */
		enabled=true;
		}
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(this,ConfigChangedCallbackData::HeadDevice);
	configChangedCallbacks.call(&cbData);
	}
	
	return result;
	}

InputDevice* Viewer::detachFromDevice(const TrackerState& newHeadDeviceTransformation)
	{
	/* Return the previous head device: */
	InputDevice* result=headTracked?headDevice:0;
	
	/* Disable head tracking and set the static head device transformation: */
	headTracked=false;
	headDeviceTransformation=newHeadDeviceTransformation;
	headDeviceAdapter=0;
	headDeviceIndex=-1;
	
	/* Update head light source state: */
	Point hlp=headDeviceTransformation.transform(headLightDevicePosition);
	lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
	Vector hld=headDeviceTransformation.transform(headLightDeviceDirection);
	hld.normalize();
	lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
	
	/* Enable the viewer: */
	enabled=true;
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(this,ConfigChangedCallbackData::HeadDevice);
	configChangedCallbacks.call(&cbData);
	}
	
	return result;
	}

void Viewer::setIPD(Scalar newIPD)
	{
	/* Calculate the new eye displacement vector: */
	Point eyeMid=Geometry::mid(deviceLeftEyePosition,deviceRightEyePosition);
	Vector eyeDist=deviceRightEyePosition-deviceLeftEyePosition;
	eyeDist*=newIPD/(Scalar(2)*eyeDist.mag());
	
	/* Set the left and right eye positions: */
	deviceLeftEyePosition=eyeMid-eyeDist;
	deviceRightEyePosition=eyeMid+eyeDist;
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(this,ConfigChangedCallbackData::EyePositions);
	configChangedCallbacks.call(&cbData);
	}
	}

void Viewer::setEyes(const Vector& newViewDirection,const Point& newMonoEyePosition,const Vector& newEyeOffset)
	{
	/* Set the view direction: */
	deviceViewDirection=newViewDirection;
	
	/* Set the mono eye position: */
	deviceMonoEyePosition=newMonoEyePosition;
	
	/* Set the left and right eye positions: */
	deviceLeftEyePosition=deviceMonoEyePosition-newEyeOffset;
	deviceRightEyePosition=deviceMonoEyePosition+newEyeOffset;
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(this,ConfigChangedCallbackData::EyePositions);
	configChangedCallbacks.call(&cbData);
	}
	}

void Viewer::setHeadlightState(bool newHeadlightState)
	{
	if(newHeadlightState)
		lightsource->enable();
	else
		lightsource->disable();
	
	/* Call the configuration change callbacks: */
	{
	ConfigChangedCallbackData cbData(this,ConfigChangedCallbackData::HeadlightState);
	configChangedCallbacks.call(&cbData);
	}
	}

void Viewer::update(void)
	{
	/* Update head light source state if head tracking is enabled: */
	if(headTracked)
		{
		/* Update head light source state: */
		const TrackerState& headTransform=headTracked?headDevice->getTransformation():headDeviceTransformation;
		Point hlp=headTransform.transform(headLightDevicePosition);
		lightsource->getLight().position=GLLight::Position(GLfloat(hlp[0]),GLfloat(hlp[1]),GLfloat(hlp[2]),1.0f);
		Vector hld=headTransform.transform(headLightDeviceDirection);
		hld.normalize();
		lightsource->getLight().spotDirection=GLLight::SpotDirection(GLfloat(hld[0]),GLfloat(hld[1]),GLfloat(hld[2]));
		}
	}

TrackerState Viewer::peekHeadTransformation(void)
	{
	if(headTracked)
		{
		/* Return up-to-date tracking data from the input device adapter: */
		return headDeviceAdapter->peekTrackerState(headDeviceIndex);
		}
	else
		{
		/* Return fixed head transformation: */
		return headDeviceTransformation;
		}
	}

}
