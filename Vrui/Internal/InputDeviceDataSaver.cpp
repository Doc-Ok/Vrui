/***********************************************************************
InputDeviceDataSaver - Class to save input device data to a file for
later playback.
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

#include <Vrui/Internal/InputDeviceDataSaver.h>

#include <iostream>
#include <Misc/StringMarshaller.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <IO/OpenFile.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/SoundRecorder.h>
#include <Vrui/Vrui.h>
#include <Vrui/Geometry.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/TextEventDispatcher.h>
#ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
#include <Vrui/Internal/KinectRecorder.h>
#endif

namespace Vrui {

/*************************************
Methods of class InputDeviceDataSaver:
*************************************/

void InputDeviceDataSaver::inputDeviceStateChangeCallback(InputGraphManager::InputDeviceStateChangeCallbackData* cbData)
	{
	/* Update the valid flag of the given device: */
	for(int i=0;i<numInputDevices;++i)
		if(inputDevices[i]==cbData->inputDevice)
			{
			validFlags[i]=cbData->newEnabled;
			break;
			}
	}

InputDeviceDataSaver::InputDeviceDataSaver(const Misc::ConfigurationFileSection& configFileSection,InputDeviceManager& inputDeviceManager,TextEventDispatcher* sTextEventDispatcher,unsigned int randomSeed)
	:numInputDevices(inputDeviceManager.getNumInputDevices()),
	 inputDevices(new InputDevice*[numInputDevices]),validFlags(new bool[numInputDevices]),
	 textEventDispatcher(sTextEventDispatcher),
	 soundRecorder(0)
	 #ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
	 kinectRecorder(0)
	 #endif
	{
	/* Open the common base directory: */
	IO::DirectoryPtr baseDirectory=IO::openDirectory(configFileSection.retrieveString("./baseDirectory",".").c_str());
	
	/* Open the input device data file relative to the base directory: */
	inputDeviceDataFile=baseDirectory->openFile(baseDirectory->createNumberedFileName(configFileSection.retrieveString("./inputDeviceDataFileName").c_str(),4).c_str(),IO::File::WriteOnly);
	
	/* Write a file identification header: */
	inputDeviceDataFile->setEndianness(Misc::LittleEndian);
	static const char* fileHeader="Vrui Input Device Data File v5.0\n";
	inputDeviceDataFile->write<char>(fileHeader,34);
	
	/* Save the random number seed: */
	inputDeviceDataFile->write<unsigned int>(randomSeed);
	
	/* Save number of input devices: */
	inputDeviceDataFile->write<int>(numInputDevices);
	
	/* Save layout and feature names of all input devices in the input device manager: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Get pointer to the input device: */
		inputDevices[i]=inputDeviceManager.getInputDevice(i);
		
		/* Save input device's name and layout: */
		Misc::writeCString(inputDevices[i]->getDeviceName(),*inputDeviceDataFile);
		inputDeviceDataFile->write<int>(inputDevices[i]->getTrackType());
		inputDeviceDataFile->write<int>(inputDevices[i]->getNumButtons());
		inputDeviceDataFile->write<int>(inputDevices[i]->getNumValuators());
		
		/* Save input device's feature names: */
		for(int j=0;j<inputDevices[i]->getNumFeatures();++j)
			{
			std::string featureName=inputDeviceManager.getFeatureName(InputDeviceFeature(inputDevices[i],j));
			Misc::writeCppString(featureName,*inputDeviceDataFile);
			}
		
		/* Initialize device as valid: */
		validFlags[i]=true;
		}
	
	/* Register a callback with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().add(this,&InputDeviceDataSaver::inputDeviceStateChangeCallback);
	
	/* Check if the user wants to record a commentary track: */
	std::string soundFileName=configFileSection.retrieveString("./soundFileName","");
	if(!soundFileName.empty())
		{
		try
			{
			/* Create a sound data format for recording: */
			Sound::SoundDataFormat soundFormat;
			soundFormat.bitsPerSample=configFileSection.retrieveValue<int>("./sampleResolution",soundFormat.bitsPerSample);
			soundFormat.samplesPerFrame=configFileSection.retrieveValue<int>("./numChannels",soundFormat.samplesPerFrame);
			soundFormat.framesPerSecond=configFileSection.retrieveValue<int>("./sampleRate",soundFormat.framesPerSecond);
			
			/* Create a sound recorder for the given sound file name: */
			std::string soundDeviceName=configFileSection.retrieveValue<std::string>("./soundDeviceName","default");
			soundFileName=baseDirectory->getPath(baseDirectory->createNumberedFileName(soundFileName.c_str(),4).c_str());
			soundRecorder=new Sound::SoundRecorder(soundDeviceName.c_str(),soundFormat,soundFileName.c_str());
			}
		catch(const std::runtime_error& err)
			{
			/* Print a message, but carry on: */
			Misc::formattedConsoleWarning("InputDeviceDataSaver: Disabling sound recording due to exception %s",err.what());
			}
		}
	
	#ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
	/* Check if the user wants to record 3D video: */
	std::string kinectRecorderSectionName=configFileSection.retrieveString("./kinectRecorder","");
	if(!kinectRecorderSectionName.empty())
		{
		/* Go to the Kinect recorder's section: */
		Misc::ConfigurationFileSection kinectRecorderSection=configFileSection.getSection(kinectRecorderSectionName.c_str());
		kinectRecorder=new KinectRecorder(kinectRecorderSection);
		}
	#endif
	}

InputDeviceDataSaver::~InputDeviceDataSaver(void)
	{
	/* Log the total recording time as a convenience: */
	Misc::formattedLogNote("Vrui::InputDeviceDataSaver: Total recording time: %fs",getApplicationTime());
	
	/* Shut down recording: */
	delete[] inputDevices;
	delete[] validFlags;
	delete soundRecorder;
	#ifdef VRUI_INPUTDEVICEDATASAVER_USE_KINECT
	delete kinectRecorder;
	#endif
	
	/* Unregister the callback with the input graph manager: */
	getInputGraphManager()->getInputDeviceStateChangeCallbacks().remove(this,&InputDeviceDataSaver::inputDeviceStateChangeCallback);
	}

void InputDeviceDataSaver::prepareMainLoop(void)
	{
	try
		{
		/* Start recording sound now, if requested: */
		if(soundRecorder!=0)
			soundRecorder->start();
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedConsoleWarning("InputDeviceDataSaver: Disabling sound recording due to exception %s",err.what());
		delete soundRecorder;
		soundRecorder=0;
		}
	}

void InputDeviceDataSaver::saveCurrentState(double currentTimeStamp)
	{
	/* Write current time stamp: */
	inputDeviceDataFile->write(currentTimeStamp);
	
	/* Write state of all input devices: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Check if the input device is valid: */
		if(validFlags[i])
			{
			/* Write valid flag: */
			inputDeviceDataFile->write<unsigned char>(1);
			
			/* Write input device's tracker state: */
			if(inputDevices[i]->getTrackType()!=InputDevice::TRACK_NONE)
				{
				inputDeviceDataFile->write(inputDevices[i]->getDeviceRayDirection().getComponents(),3);
				inputDeviceDataFile->write(inputDevices[i]->getDeviceRayStart());
				const TrackerState& t=inputDevices[i]->getTransformation();
				inputDeviceDataFile->write(t.getTranslation().getComponents(),3);
				inputDeviceDataFile->write(t.getRotation().getQuaternion(),4);
				inputDeviceDataFile->write(inputDevices[i]->getLinearVelocity().getComponents(),3);
				inputDeviceDataFile->write(inputDevices[i]->getAngularVelocity().getComponents(),3);
				}
			
			/* Write input device's button states: */
			unsigned char buttonBits=0x00U;
			int numBits=0;
			for(int j=0;j<inputDevices[i]->getNumButtons();++j)
				{
				buttonBits<<=1;
				if(inputDevices[i]->getButtonState(j))
					buttonBits|=0x01U;
				if(++numBits==8)
					{
					inputDeviceDataFile->write(buttonBits);
					buttonBits=0x00U;
					numBits=0;
					}
				}
			if(numBits!=0)
				{
				buttonBits<<=8-numBits;
				inputDeviceDataFile->write(buttonBits);
				}
			
			/* Write input device's valuator states: */
			for(int j=0;j<inputDevices[i]->getNumValuators();++j)
				{
				double valuatorState=inputDevices[i]->getValuator(j);
				inputDeviceDataFile->write(valuatorState);
				}
			}
		else
			{
			/* Write valid flag: */
			inputDeviceDataFile->write<unsigned char>(0);
			}
		
		}
	
	/* Write all enqueued text and text control events: */
	textEventDispatcher->writeEventQueues(*inputDeviceDataFile);
	}

}
