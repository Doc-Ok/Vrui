/***********************************************************************
InputDevice - Class to represent input devices (6-DOF tracker with
associated buttons and valuators) in virtual reality environments.
Copyright (c) 2000-2019 Oliver Kreylos

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

#include <string.h>
#include <Misc/ThrowStdErr.h>

#include <Vrui/InputDevice.h>

namespace Vrui {

/****************************
Methods of class InputDevice:
****************************/

InputDevice::InputDevice(void)
	:deviceName(new char[1]),trackType(TRACK_NONE),
	 numButtons(0),numValuators(0),
	 buttonCallbacks(0),valuatorCallbacks(0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(0),valuatorValues(0),
	 callbacksEnabled(true),deviceRayChanged(false),trackingChanged(false),
	 savedButtonStates(0),savedValuatorValues(0)
	{
	deviceName[0]='\0';
	}

InputDevice::InputDevice(const char* sDeviceName,int sTrackType,int sNumButtons,int sNumValuators)
	:deviceName(new char[strlen(sDeviceName)+1]),trackType(sTrackType),
	 numButtons(sNumButtons),numValuators(sNumValuators),
	 buttonCallbacks(numButtons>0?new Misc::CallbackList[numButtons]:0),
	 valuatorCallbacks(numValuators>0?new Misc::CallbackList[numValuators]:0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(numButtons>0?new bool[numButtons]:0),
	 valuatorValues(numValuators>0?new double[numValuators]:0),
	 callbacksEnabled(true),deviceRayChanged(false),trackingChanged(false),
	 savedButtonStates(numButtons>0?new bool[numButtons]:0),
	 savedValuatorValues(numValuators>0?new double[numValuators]:0)
	{
	/* Copy device name: */
	strcpy(deviceName,sDeviceName);
	
	/* Initialize button and valuator states: */
	for(int i=0;i<numButtons;++i)
		{
		buttonStates[i]=false;
		savedButtonStates[i]=false;
		}
	for(int i=0;i<numValuators;++i)
		{
		valuatorValues[i]=0.0;
		savedValuatorValues[i]=0.0;
		}
	}

InputDevice::InputDevice(const InputDevice& source)
	:deviceName(new char[1]),trackType(TRACK_NONE),
	 numButtons(0),numValuators(0),
	 buttonCallbacks(0),valuatorCallbacks(0),
	 deviceRayDirection(0,1,0),deviceRayStart(0),
	 transformation(TrackerState::identity),linearVelocity(Vector::zero),angularVelocity(Vector::zero),
	 buttonStates(0),valuatorValues(0),
	 callbacksEnabled(true),deviceRayChanged(false),trackingChanged(false),
	 savedButtonStates(0),savedValuatorValues(0)
	{
	deviceName[0]='\0';
	
	/*********************************************************************
	Since we don't actually copy the source data here, throw an exception
	if somebody attempts to copy an already initialized input device.
	That'll teach them.
	*********************************************************************/
	
	if(source.deviceName[0]!='\0'||source.numButtons!=0||source.numValuators!=0)
		Misc::throwStdErr("InputDevice: Attempt to copy initialized input device");
	}

InputDevice::~InputDevice(void)
	{
	delete[] deviceName;
	
	/* Delete state arrays: */
	delete[] buttonCallbacks;
	delete[] valuatorCallbacks;
	delete[] buttonStates;
	delete[] valuatorValues;
	delete[] savedButtonStates;
	delete[] savedValuatorValues;
	}

InputDevice& InputDevice::set(const char* sDeviceName,int sTrackType,int sNumButtons,int sNumValuators)
	{
	delete[] deviceName;
	
	/* Delete old state arrays: */
	delete[] buttonCallbacks;
	delete[] valuatorCallbacks;
	delete[] buttonStates;
	delete[] valuatorValues;
	delete[] savedButtonStates;
	delete[] savedValuatorValues;
	
	/* Set new device layout: */
	deviceName=new char[strlen(sDeviceName)+1];
	strcpy(deviceName,sDeviceName);
	trackType=sTrackType;
	numButtons=sNumButtons;
	numValuators=sNumValuators;
	
	/* Allocate new state arrays: */
	buttonCallbacks=numButtons!=0?new Misc::CallbackList[numButtons]:0;
	valuatorCallbacks=numValuators>0?new Misc::CallbackList[numValuators]:0;
	buttonStates=numButtons!=0?new bool[numButtons]:0;
	valuatorValues=numValuators>0?new double[numValuators]:0;
	savedButtonStates=numButtons!=0?new bool[numButtons]:0;
	savedValuatorValues=numValuators>0?new double[numValuators]:0;
	
	/* Clear all button and valuator states: */
	for(int i=0;i<numButtons;++i)
		{
		buttonStates[i]=false;
		savedButtonStates[i]=false;
		}
	for(int i=0;i<numValuators;++i)
		{
		valuatorValues[i]=0.0;
		savedValuatorValues[i]=0.0;
		}
	
	return *this;
	}

void InputDevice::setTrackType(int newTrackType)
	{
	/* Set the tracking type: */
	trackType=newTrackType;
	}

void InputDevice::setDeviceRay(const Vector& newDeviceRayDirection,Scalar newDeviceRayStart)
	{
	/* Set ray direction and starting parameter: */
	deviceRayDirection=newDeviceRayDirection;
	deviceRayStart=newDeviceRayStart;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all device ray callbacks: */
		CallbackData cbData(this);
		deviceRayCallbacks.call(&cbData);
		}
	else
		deviceRayChanged=true;
	}

void InputDevice::setTransformation(const TrackerState& newTransformation)
	{
	/* Set transformation: */
	transformation=newTransformation;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		trackingChanged=true;
	}

void InputDevice::setLinearVelocity(const Vector& newLinearVelocity)
	{
	/* Set the linear velocity: */
	linearVelocity=newLinearVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		trackingChanged=true;
	}

void InputDevice::setAngularVelocity(const Vector& newAngularVelocity)
	{
	/* Set the angular velocity: */
	angularVelocity=newAngularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		trackingChanged=true;
	}

void InputDevice::setTrackingState(const TrackerState& newTransformation,const Vector& newLinearVelocity,const Vector& newAngularVelocity)
	{
	/* Update tracking state: */
	transformation=newTransformation;
	linearVelocity=newLinearVelocity;
	angularVelocity=newAngularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all tracking callbacks: */
		CallbackData cbData(this);
		trackingCallbacks.call(&cbData);
		}
	else
		trackingChanged=true;
	}

void InputDevice::copyTrackingState(const InputDevice* source)
	{
	/* Copy device ray state: */
	deviceRayDirection=source->deviceRayDirection;
	deviceRayStart=source->deviceRayStart;
	
	/* Copy tracking state: */
	transformation=source->transformation;
	linearVelocity=source->linearVelocity;
	angularVelocity=source->angularVelocity;
	
	/* Call callbacks: */
	if(callbacksEnabled)
		{
		/* Call all device ray and tracking callbacks: */
		CallbackData cbData(this);
		deviceRayCallbacks.call(&cbData);
		trackingCallbacks.call(&cbData);
		}
	else
		{
		deviceRayChanged=true;
		trackingChanged=true;
		}
	}

void InputDevice::clearButtonStates(void)
	{
	for(int i=0;i<numButtons;++i)
		{
		if(buttonStates[i])
			{
			buttonStates[i]=false;
			if(callbacksEnabled)
				{
				ButtonCallbackData cbData(this,i,false);
				buttonCallbacks[i].call(&cbData);
				}
			}
		}
	}

void InputDevice::setButtonState(int index,bool newButtonState)
	{
	ButtonCallbackData cbData(this,index,newButtonState);
	if(buttonStates[index]!=newButtonState)
		{
		buttonStates[index]=newButtonState;
		if(callbacksEnabled)
			buttonCallbacks[index].call(&cbData);
		}
	}

void InputDevice::setSingleButtonPressed(int index)
	{
	for(int i=0;i<numButtons;++i)
		{
		if(i!=index)
			{
			if(buttonStates[i])
				{
				buttonStates[i]=false;
				if(callbacksEnabled)
					{
					ButtonCallbackData cbData(this,i,false);
					buttonCallbacks[i].call(&cbData);
					}
				}
			}
		}
	ButtonCallbackData cbData(this,index,true);
	if(!buttonStates[index])
		{
		buttonStates[index]=true;
		if(callbacksEnabled)
			buttonCallbacks[index].call(&cbData);
		}
	}

void InputDevice::setValuator(int index,double value)
	{
	ValuatorCallbackData cbData(this,index,valuatorValues[index],value);
	if(valuatorValues[index]!=value)
		{
		valuatorValues[index]=value;
		if(callbacksEnabled)
			valuatorCallbacks[index].call(&cbData);
		}
	}

void InputDevice::disableCallbacks(void)
	{
	callbacksEnabled=false;
	
	/* Reset the device ray and tracking change tracker: */
	deviceRayChanged=false;
	trackingChanged=false;
	
	/* Save all button states and valuator values to call the appropriate callbacks once callbacks are enabled again: */
	for(int i=0;i<numButtons;++i)
		savedButtonStates[i]=buttonStates[i];
	for(int i=0;i<numValuators;++i)
		savedValuatorValues[i]=valuatorValues[i];
	}

void InputDevice::triggerFeatureCallback(int featureIndex)
	{
	/* Check if the given feature is a button or a valuator: */
	if(featureIndex>=numButtons)
		{
		/* Check a valuator: */
		int valuatorIndex=featureIndex-numButtons;
		if(savedValuatorValues[valuatorIndex]!=valuatorValues[valuatorIndex])
			{
			/* Call the callback: */
			ValuatorCallbackData cbData(this,valuatorIndex,savedValuatorValues[valuatorIndex],valuatorValues[valuatorIndex]);
			valuatorCallbacks[valuatorIndex].call(&cbData);
			
			/* Update the saved valuator value so the callback won't be called again: */
			savedValuatorValues[valuatorIndex]=valuatorValues[valuatorIndex];
			}
		}
	else
		{
		/* Check a button: */
		int buttonIndex=featureIndex;
		if(savedButtonStates[buttonIndex]!=buttonStates[buttonIndex])
			{
			/* Call the callback: */
			ButtonCallbackData cbData(this,buttonIndex,buttonStates[buttonIndex]);
			buttonCallbacks[buttonIndex].call(&cbData);
			
			/* Update the saved button state so the callback won't be called again: */
			savedButtonStates[buttonIndex]=buttonStates[buttonIndex];
			}
		}
	}

void InputDevice::enableCallbacks(void)
	{
	callbacksEnabled=true;
	
	/* Call callbacks for everything that has changed, to update the user program's state: */
	if(deviceRayChanged)
		{
		CallbackData deviceRayCbData(this);
		deviceRayCallbacks.call(&deviceRayCbData);
		}
	if(trackingChanged)
		{
		CallbackData trackingCbData(this);
		trackingCallbacks.call(&trackingCbData);
		}
	for(int i=0;i<numButtons;++i)
		if(savedButtonStates[i]!=buttonStates[i])
			{
			ButtonCallbackData cbData(this,i,buttonStates[i]);
			buttonCallbacks[i].call(&cbData);
			}
	for(int i=0;i<numValuators;++i)
		if(savedValuatorValues[i]!=valuatorValues[i])
			{
			ValuatorCallbackData cbData(this,i,savedValuatorValues[i],valuatorValues[i]);
			valuatorCallbacks[i].call(&cbData);
			}
	}

}
