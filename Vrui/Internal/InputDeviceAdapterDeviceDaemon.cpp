/***********************************************************************
InputDeviceAdapterDeviceDaemon - Class to convert from Vrui's own
distributed device driver architecture to Vrui's internal device
representation.
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

#include <Vrui/Internal/InputDeviceAdapterDeviceDaemon.h>

#include <stdio.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/FunctionCalls.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/GlyphRenderer.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

// #define MEASURE_LATENCY
// #define SAVE_TRACKERSTATES

#ifdef MEASURE_LATENCY
Realtime::TimePointMonotonic lastUpdate;
#endif

#ifdef SAVE_TRACKERSTATES
#include <Misc/Marshaller.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/GeometryMarshallers.h>
IO::FilePtr realFile;
IO::FilePtr predictedFile;
#endif

namespace Vrui {

/* DEBUGging variables: */
bool deviceDaemonPredictOnUpdate=true;

/***********************************************
Methods of class InputDeviceAdapterDeviceDaemon:
***********************************************/

void InputDeviceAdapterDeviceDaemon::packetNotificationCallback(VRDeviceClient* client)
	{
	#ifdef MEASURE_LATENCY
	Realtime::TimePointMonotonic now;
	VRDeviceState::TimeStamp nowTs=VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
	printf("Packet interval: %f ms, arrival latency: %f ms\n",double(lastUpdate.setAndDiff())*1000.0,double(nowTs-client->getState().getTrackerTimeStamp(0))/1000.0);
	#endif
	
	#ifdef SAVE_TRACKERSTATES
	realFile->write<Misc::UInt32>(client->getState().getTrackerTimeStamp(0));
	Misc::write(client->getState().getTrackerState(0).positionOrientation,*realFile);
	#endif
	
	/* Simply request a new Vrui frame: */
	requestUpdate();
	}

void InputDeviceAdapterDeviceDaemon::errorCallback(const VRDeviceClient::ProtocolError& error)
	{
	/* Show the error message to the user; it's probably quite important: */
	Misc::formattedUserError("Vrui::InputDeviceAdapterDeviceDaemon: %s",error.what());
	}

void InputDeviceAdapterDeviceDaemon::batteryStateUpdatedCallback(unsigned int deviceIndex)
	{
	/* Get the new battery level of the changed device: */
	unsigned int newBatteryState=deviceClient.getBatteryState(deviceIndex).batteryLevel;
	
	/* Map the virtual device index to an input device index: */
	int managedDeviceIndex=batteryStateIndexMap[deviceIndex];
	if(managedDeviceIndex>=0)
		{
		/* Check if the device just went low: */
		if(batteryStates[managedDeviceIndex]>=10U&&newBatteryState<10U)
			{
			/* Show a warning to the user; it's probably time to charge the device: */
			Misc::formattedUserWarning("Vrui::InputDeviceAdapterDeviceDaemon: Input device %s is low on battery",inputDevices[managedDeviceIndex]->getDeviceName());
			}
		
		/* Update the battery state array: */
		batteryStates[managedDeviceIndex]=newBatteryState;
		
		/* Request a new Vrui frame to wake up the main thread: */
		requestUpdate();
		}
	}

void InputDeviceAdapterDeviceDaemon::createInputDevice(int deviceIndex,const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Check if the device client has a virtual device of the same name as this configuration file section: */
	for(int vdIndex=0;vdIndex<deviceClient.getNumVirtualDevices();++vdIndex)
		{
		const VRDeviceDescriptor& vd=deviceClient.getVirtualDevice(vdIndex);
		if(vd.name==configFileSection.getName())
			{
			/* Ensure that the index mapping tables exist: */
			createIndexMappings();
			
			/* Create an input device from the virtual input device descriptor: */
			int trackType=InputDevice::TRACK_NONE;
			if(vd.trackType&VRDeviceDescriptor::TRACK_POS)
				trackType|=InputDevice::TRACK_POS;
			if(vd.trackType&VRDeviceDescriptor::TRACK_DIR)
				trackType|=InputDevice::TRACK_DIR;
			if(vd.trackType&VRDeviceDescriptor::TRACK_ORIENT)
				trackType|=InputDevice::TRACK_ORIENT;
			
			/* Create new input device as a physical device: */
			std::string deviceName=configFileSection.retrieveString("./name",vd.name);
			InputDevice* newDevice=inputDeviceManager->createInputDevice(deviceName.c_str(),trackType,vd.numButtons,vd.numValuators,true);
			
			/* Set the device's selection ray: */
			Vector rayDirection=configFileSection.retrieveValue<Vector>("./deviceRayDirection",vd.rayDirection);
			Scalar rayStart=configFileSection.retrieveValue<Scalar>("./deviceRayStart",vd.rayStart);
			newDevice->setDeviceRay(rayDirection,rayStart);
			
			/* Initialize the new device's glyph from the current configuration file section: */
			Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(newDevice);
			deviceGlyph.configure(configFileSection,"./deviceGlyphType","./deviceGlyphMaterial");
			
			/* Save the new input device: */
			inputDevices[deviceIndex]=newDevice;
			
			/* Assign the new device's tracker index: */
			trackerIndexMapping[deviceIndex]=vd.trackerIndex;
			
			/* Assign the new device's button indices: */
			if(vd.numButtons>0)
				{
				buttonIndexMapping[deviceIndex]=new int[vd.numButtons];
				for(int i=0;i<vd.numButtons;++i)
					buttonIndexMapping[deviceIndex][i]=vd.buttonIndices[i];
				}
			else
				buttonIndexMapping[deviceIndex]=0;
			
			/* Store the virtual input device's button names: */
			for(int i=0;i<vd.numButtons;++i)
				buttonNames.push_back(vd.buttonNames[i]);
			
			/* Assign the new device's valuator indices: */
			if(vd.numValuators>0)
				{
				valuatorIndexMapping[deviceIndex]=new int[vd.numValuators];
				for(int i=0;i<vd.numValuators;++i)
					valuatorIndexMapping[deviceIndex][i]=vd.valuatorIndices[i];
				}
			else
				valuatorIndexMapping[deviceIndex]=0;
			
			/* Store the virtual input device's valuator names: */
			for(int i=0;i<vd.numValuators;++i)
				valuatorNames.push_back(vd.valuatorNames[i]);
			
			/* Enter the virtual device into the battery state index map: */
			batteryStateIndexMap[vdIndex]=deviceIndex;
			
			/* Check if the virtual device has haptic features: */
			if(vd.numHapticFeatures>0)
				{
				/* Register the device's first haptic feature with the input device manager: */
				inputDeviceManager->addHapticFeature(newDevice,this,vd.hapticFeatureIndices[0]);
				}
			
			/* Skip the usual device creation procedure: */
			return;
			}
		}
	
	/* Call base class method to initialize the input device: */
	InputDeviceAdapterIndexMap::createInputDevice(deviceIndex,configFileSection);
	
	/* Read the list of button names for this device: */
	/* Read the names of all button features: */
	typedef std::vector<std::string> StringList;
	StringList tempButtonNames=configFileSection.retrieveValue<StringList>("./buttonNames",StringList());
	int buttonIndex=0;
	for(StringList::iterator bnIt=tempButtonNames.begin();bnIt!=tempButtonNames.end()&&buttonIndex<inputDevices[deviceIndex]->getNumButtons();++bnIt,++buttonIndex)
		{
		/* Store the button name: */
		buttonNames.push_back(*bnIt);
		}
	for(;buttonIndex<inputDevices[deviceIndex]->getNumButtons();++buttonIndex)
		{
		char buttonName[40];
		snprintf(buttonName,sizeof(buttonName),"Button%d",buttonIndex);
		buttonNames.push_back(buttonName);
		}
	
	/* Read the names of all valuator features: */
	StringList tempValuatorNames=configFileSection.retrieveValue<StringList>("./valuatorNames",StringList());
	int valuatorIndex=0;
	for(StringList::iterator vnIt=tempValuatorNames.begin();vnIt!=tempValuatorNames.end()&&valuatorIndex<inputDevices[deviceIndex]->getNumValuators();++vnIt,++valuatorIndex)
		{
		/* Store the valuator name: */
		valuatorNames.push_back(*vnIt);
		}
	for(;valuatorIndex<inputDevices[deviceIndex]->getNumValuators();++valuatorIndex)
		{
		char valuatorName[40];
		snprintf(valuatorName,sizeof(valuatorName),"Valuator%d",valuatorIndex);
		valuatorNames.push_back(valuatorName);
		}
	}

InputDeviceAdapterDeviceDaemon::InputDeviceAdapterDeviceDaemon(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapterIndexMap(sInputDeviceManager),
	 deviceClient(configFileSection),
	 predictMotion(configFileSection.retrieveValue<bool>("./predictMotion",false)),
	 motionPredictionDelta(configFileSection.retrieveValue<double>("./motionPredictionDelta",0.0)),
	 validFlags(0),batteryStateIndexMap(0),batteryStates(0)
	{
	#ifdef SAVE_TRACKERSTATES
	realFile=IO::openFile("RealTrackerData.dat",IO::File::WriteOnly);
	realFile->setEndianness(Misc::LittleEndian);
	predictedFile=IO::openFile("PredictedTrackerData.dat",IO::File::WriteOnly);
	predictedFile->setEndianness(Misc::LittleEndian);
	#endif
	
	/* Initialize the battery state index map: */
	batteryStateIndexMap=new int[deviceClient.getNumVirtualDevices()];
	for(int i=0;i<deviceClient.getNumVirtualDevices();++i)
		batteryStateIndexMap[i]=-1;
	
	/* Initialize input device adapter: */
	InputDeviceAdapterIndexMap::initializeAdapter(deviceClient.getState().getNumTrackers(),deviceClient.getState().getNumButtons(),deviceClient.getState().getNumValuators(),configFileSection);
	
	/* Initialize the valid flag array: */
	validFlags=new bool[numInputDevices];
	for(int i=0;i<numInputDevices;++i)
		validFlags[i]=true;
	
	/* Initialize the battery state array: */
	batteryStates=new unsigned int[numInputDevices];
	for(int i=0;i<numInputDevices;++i)
		batteryStates[i]=100U;
	
	/* Start VR devices: */
	deviceClient.activate();
	
	/* Register a callback to receive battery status updates: */
	deviceClient.setBatteryStateUpdatedCallback(Misc::createFunctionCall(this,&InputDeviceAdapterDeviceDaemon::batteryStateUpdatedCallback));
	
	/* Start streaming; waits for first packet to arrive: */
	deviceClient.startStream(Misc::createFunctionCall(packetNotificationCallback),Misc::createFunctionCall(this,&InputDeviceAdapterDeviceDaemon::errorCallback));
	}

InputDeviceAdapterDeviceDaemon::~InputDeviceAdapterDeviceDaemon(void)
	{
	/* Stop VR devices: */
	deviceClient.stopStream();
	deviceClient.deactivate();
	
	#ifdef SAVE_TRACKERSTATES
	realFile=0;
	predictedFile=0;
	#endif
	
	/* Clean up: */
	delete[] validFlags;
	delete[] batteryStateIndexMap;
	delete[] batteryStates;
	}

std::string InputDeviceAdapterDeviceDaemon::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==feature.getDevice())
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		Misc::throwStdErr("InputDeviceAdapterDeviceDaemon::getFeatureName: Unknown device %s",feature.getDevice()->getDeviceName());
	
	/* Check whether the feature is a button or a valuator: */
	std::string result;
	if(feature.isButton())
		{
		/* Return the button feature's name: */
		result=buttonNames[buttonIndexBase+feature.getIndex()];
		}
	if(feature.isValuator())
		{
		/* Return the valuator feature's name: */
		result=valuatorNames[valuatorIndexBase+feature.getIndex()];
		}
	
	return result;
	}

int InputDeviceAdapterDeviceDaemon::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Find the input device owning the given feature: */
	bool deviceFound=false;
	int buttonIndexBase=0;
	int valuatorIndexBase=0;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==device)
			{
			deviceFound=true;
			break;
			}
		
		/* Go to the next device: */
		buttonIndexBase+=inputDevices[deviceIndex]->getNumButtons();
		valuatorIndexBase+=inputDevices[deviceIndex]->getNumValuators();
		}
	if(!deviceFound)
		Misc::throwStdErr("InputDeviceAdapterDeviceDaemon::getFeatureIndex: Unknown device %s",device->getDeviceName());
	
	/* Check if the feature names a button or a valuator: */
	for(int buttonIndex=0;buttonIndex<device->getNumButtons();++buttonIndex)
		if(buttonNames[buttonIndexBase+buttonIndex]==featureName)
			return device->getButtonFeatureIndex(buttonIndex);
	for(int valuatorIndex=0;valuatorIndex<device->getNumValuators();++valuatorIndex)
		if(valuatorNames[valuatorIndexBase+valuatorIndex]==featureName)
			return device->getValuatorFeatureIndex(valuatorIndex);
	
	return -1;
	}

void InputDeviceAdapterDeviceDaemon::updateInputDevices(void)
	{
	/* Update all managed input devices: */
	deviceClient.lockState();
	const VRDeviceState& state=deviceClient.getState();
	
	#ifdef MEASURE_LATENCY
	
	Realtime::TimePointMonotonic now;
	VRDeviceState::TimeStamp ts=VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
	
	double staleness=0.0;
	for(int i=0;i<state.getNumTrackers();++i)
		staleness+=double(ts-state.getTrackerTimeStamp(i));
	printf("Tracking data staleness: %f ms\n",staleness*0.001/double(state.getNumTrackers()));
	
	#endif
	
	/* Check if motion prediction is enabled: */
	if((predictMotion||inputDeviceManager->isPredictionEnabled())&&deviceDaemonPredictOnUpdate)
		{
		/* Calculate the prediction time point: */
		VRDeviceState::TimeStamp predictionTs;
		if(inputDeviceManager->isPredictionEnabled())
			{
			/* Get the prediction time point from the input device manager: */
			const Realtime::TimePointMonotonic& pt=inputDeviceManager->getPredictionTime();
			predictionTs=VRDeviceState::TimeStamp(pt.tv_sec*1000000+(pt.tv_nsec+500)/1000);
			}
		else
			{
			/* Get the current time for input device motion prediction and offset by the motion prediction delta: */
			Realtime::TimePointMonotonic now;
			now+=motionPredictionDelta;
			predictionTs=VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
			}
		
		for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
			{
			/* Get pointer to the input device: */
			InputDevice* device=inputDevices[deviceIndex];
			
			/* Don't update tracker-related state for devices that are not tracked: */
			int trackerIndex=trackerIndexMapping[deviceIndex];
			if(trackerIndex>=0)
				{
				/* Get the tracker's valid flag: */
				bool valid=state.getTrackerValid(trackerIndex);
				
				/* Only process tracking and feature data if the tracking data is valid: */
				if(valid)
					{
					/* Get device's tracker state from VR device client: */
					const VRDeviceState::TrackerState& ts=state.getTrackerState(trackerIndex);
					
					/* Motion-predict the device's tracker state from its sampling time to the current time: */
					typedef VRDeviceState::TrackerState::PositionOrientation PO;
					
					float predictionDelta=float(predictionTs-state.getTrackerTimeStamp(trackerIndex))*1.0e-6f;
			
					PO::Rotation predictRot=PO::Rotation::rotateScaledAxis(ts.angularVelocity*predictionDelta)*ts.positionOrientation.getRotation();
					predictRot.renormalize();
					PO::Vector predictTrans=ts.linearVelocity*predictionDelta+ts.positionOrientation.getTranslation();
					
					#ifdef SAVE_TRACKERSTATES
					predictedFile->write<Misc::UInt32>(nowTs+Misc::UInt32(predictionDelta*1.0e6f+0.5f));
					Misc::write(PO(predictTrans,predictRot),*predictedFile);
					#endif
					
					/* Set device's tracking state: */
					device->setTrackingState(TrackerState(predictTrans,predictRot),Vector(ts.linearVelocity),Vector(ts.angularVelocity));
					}
				
				/* Check if the tracker's validity changed: */
				if(validFlags[trackerIndex]!=valid)
					{
					/* Enable or disable the input device: */
					inputDeviceManager->getInputGraphManager()->setEnabled(device,valid);
					
					/* Update the validity flag: */
					validFlags[trackerIndex]=valid;
					}
				}
			
			/* Update button states: */
			for(int i=0;i<device->getNumButtons();++i)
				device->setButtonState(i,state.getButtonState(buttonIndexMapping[deviceIndex][i]));
			
			/* Update valuator states: */
			for(int i=0;i<device->getNumValuators();++i)
				device->setValuator(i,state.getValuatorState(valuatorIndexMapping[deviceIndex][i]));
			}
		}
	else
		{
		for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
			{
			/* Get pointer to the input device: */
			InputDevice* device=inputDevices[deviceIndex];
			
			/* Don't update tracker-related state for devices that are not tracked: */
			int trackerIndex=trackerIndexMapping[deviceIndex];
			if(trackerIndex>=0)
				{
				/* Get the tracker's valid flag: */
				bool valid=state.getTrackerValid(trackerIndex);
				
				/* Only process tracking data if the tracking data is valid: */
				if(valid)
					{
					/* Get device's tracker state from VR device client: */
					const VRDeviceState::TrackerState& ts=state.getTrackerState(trackerIndex);
					
					/* Set device's tracking state: */
					device->setTrackingState(ts.positionOrientation,Vector(ts.linearVelocity),Vector(ts.angularVelocity));
					}
				
				/* Check if the tracker's validity changed: */
				if(validFlags[trackerIndex]!=valid)
					{
					/* Enable or disable the input device: */
					inputDeviceManager->getInputGraphManager()->setEnabled(device,valid);
					
					/* Update the validity flag: */
					validFlags[trackerIndex]=valid;
					}
				}
			
			/* Update button states: */
			for(int i=0;i<device->getNumButtons();++i)
				device->setButtonState(i,state.getButtonState(buttonIndexMapping[deviceIndex][i]));
			
			/* Update valuator states: */
			for(int i=0;i<device->getNumValuators();++i)
				device->setValuator(i,state.getValuatorState(valuatorIndexMapping[deviceIndex][i]));
			}
		}
		
	deviceClient.unlockState();
	}

TrackerState InputDeviceAdapterDeviceDaemon::peekTrackerState(int deviceIndex)
	{
	if(trackerIndexMapping[deviceIndex]>=0)
		{
		/* Check if motion prediction is enabled: */
		if(predictMotion||inputDeviceManager->isPredictionEnabled())
			{
			/* Calculate the prediction time point: */
			VRDeviceState::TimeStamp predictionTs;
			if(inputDeviceManager->isPredictionEnabled())
				{
				/* Get the prediction time point from the input device manager: */
				const Realtime::TimePointMonotonic& pt=inputDeviceManager->getPredictionTime();
				predictionTs=VRDeviceState::TimeStamp(pt.tv_sec*1000000+(pt.tv_nsec+500)/1000);
				}
			else
				{
				/* Get the current time for input device motion prediction and offset by the motion prediction delta: */
				Realtime::TimePointMonotonic now;
				now+=motionPredictionDelta;
				predictionTs=VRDeviceState::TimeStamp(now.tv_sec*1000000+(now.tv_nsec+500)/1000);
				}
			
			/* Get device's tracker state from VR device client: */
			deviceClient.lockState();
			const VRDeviceState& state=deviceClient.getState();
			const VRDeviceState::TrackerState& ts=state.getTrackerState(trackerIndexMapping[deviceIndex]);
			
			/* Motion-predict the device's tracker state from its sampling time to the current time: */
			typedef VRDeviceState::TrackerState::PositionOrientation PO;
			float predictionDelta=float(predictionTs-state.getTrackerTimeStamp(trackerIndexMapping[deviceIndex]))*1.0e-6f;
			PO::Rotation predictRot=PO::Rotation::rotateScaledAxis(ts.angularVelocity*predictionDelta)*ts.positionOrientation.getRotation();
			predictRot.renormalize();
			PO::Vector predictTrans=ts.linearVelocity*predictionDelta+ts.positionOrientation.getTranslation();
			TrackerState result=TrackerState(predictTrans,predictRot);
			
			deviceClient.unlockState();
			
			return result;
			}
		else
			{
			/* Get device's tracker state from VR device client: */
			deviceClient.lockState();
			const VRDeviceState& state=deviceClient.getState();
			TrackerState result=state.getTrackerState(trackerIndexMapping[deviceIndex]).positionOrientation;
			deviceClient.unlockState();
			
			return result;
			}
		}
	else
		{
		/* Fall back to base class, which will throw an exception: */
		return InputDeviceAdapter::peekTrackerState(deviceIndex);
		}
	}

void InputDeviceAdapterDeviceDaemon::hapticTick(unsigned int hapticFeatureIndex,unsigned int duration,unsigned int frequency,unsigned int amplitude)
	{
	/* Forward the request to the VR device client: */
	deviceClient.hapticTick(hapticFeatureIndex,duration,frequency,amplitude);
	}

int InputDeviceAdapterDeviceDaemon::findTrackerIndex(const InputDevice* device) const
	{
	/* Search through the list of input devices: */
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		if(inputDevices[deviceIndex]==device)
			return trackerIndexMapping[deviceIndex];
	
	/* Didn't find the device: */
	return -1;
	}

const HMDConfiguration* InputDeviceAdapterDeviceDaemon::findHmdConfiguration(const InputDevice* device) const
	{
	const HMDConfiguration* result=0;
	
	/* Find the tracker index associated with the given device: */
	int trackerIndex=-1;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		if(inputDevices[deviceIndex]==device)
			trackerIndex=trackerIndexMapping[deviceIndex];
	
	/* Check if a tracker was found: */
	if(trackerIndex>=0)
		{
		/* Search through the list of HMD configurations managed by the device client: */
		unsigned int numHmdConfigurations=deviceClient.getNumHmdConfigurations();
		deviceClient.lockHmdConfigurations();
		for(unsigned int i=0;i<numHmdConfigurations;++i)
			if(deviceClient.getHmdConfiguration(i).getTrackerIndex()==trackerIndex)
				{
				/* Return a pointer to the matching HMD configuration: */
				result=&deviceClient.getHmdConfiguration(i);
				}
		deviceClient.unlockHmdConfigurations();
		}
	
	return result;
	}

}
