/***********************************************************************
InputDeviceAdapterPlayback - Class to read input device states from a
pre-recorded file for playback and/or movie generation.
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

#include <Vrui/Internal/InputDeviceAdapterPlayback.h>

#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <Misc/PrintfTemplateTests.h>
#include <Misc/Time.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/Endianness.h>
#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/StringMarshaller.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Math/Constants.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/GeometryValueCoders.h>
#include <Sound/SoundPlayer.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputDeviceFeature.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/TextEventDispatcher.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Internal/MouseCursorFaker.h>
#include <Vrui/VRWindow.h>
#include <Vrui/Internal/Vrui.h>
#include <Vrui/Internal/Config.h>
#ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT
#include <Vrui/Internal/KinectPlayback.h>
#endif

namespace Vrui {

/*******************************************
Methods of class InputDeviceAdapterPlayback:
*******************************************/

void InputDeviceAdapterPlayback::readDeviceStates(void)
	{
	/* Update all input devices: */
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		/* Get a handle on the device: */
		InputDevice* device=inputDevices[deviceIndex];
		
		/* Data file version 5 and later contain per-device valid flags: */
		bool deviceValid=fileVersion>=5?inputDeviceDataFile->read<unsigned char>()!=0U:true;
		
		if(deviceValid)
			{
			/* Update tracker state: */
			if(device->getTrackType()!=InputDevice::TRACK_NONE)
				{
				/* Data file version 3 and later contain per-time step device ray data: */
				if(fileVersion>=3)
					{
					/* Read device ray data: */
					Vector deviceRayDir;
					inputDeviceDataFile->read(deviceRayDir.getComponents(),3);
					Scalar deviceRayStart=inputDeviceDataFile->read<Scalar>();
					device->setDeviceRay(deviceRayDir,deviceRayStart);
					}
				
				/* Read 6-DOF tracker state: */
				TrackerState::Vector translation;
				inputDeviceDataFile->read(translation.getComponents(),3);
				Scalar quat[4];
				inputDeviceDataFile->read(quat,4);
				TrackerState::Rotation rotation(quat);
				if(applyPreTransform)
					{
					/* Apply the pre-transformation to the 6-DOF tracker state: */
					translation=preTransform.getTranslation()+preTransform.getRotation().transform(translation*preTransform.getScaling());
					rotation.leftMultiply(preTransform.getRotation());
					}
				
				/* Data file version 3 and later contain linear and angular velocities: */
				if(fileVersion>=3)
					{
					/* Read velocity data: */
					Vector linearVelocity,angularVelocity;
					inputDeviceDataFile->read(linearVelocity.getComponents(),3);
					inputDeviceDataFile->read(angularVelocity.getComponents(),3);
					
					/* Set full device tracking state: */
					device->setTrackingState(TrackerState(translation,rotation),linearVelocity,angularVelocity);
					}
				else
					{
					/* Update the device's transformation: */
					device->setTransformation(TrackerState(translation,rotation));
					}
				}
			
			/* Update button states: */
			if(fileVersion>=3)
				{
				/* Extract button data from 8-bit bit masks: */
				unsigned char buttonBits=0x00U;
				int numBits=0;
				for(int i=0;i<device->getNumButtons();++i)
					{
					if(numBits==0)
						{
						buttonBits=inputDeviceDataFile->read<unsigned char>();
						numBits=8;
						}
					device->setButtonState(i,(buttonBits&0x80U)!=0x00U);
					buttonBits<<=1;
					--numBits;
					}
				}
			else
				{
				/* Read button data as sequence of 32-bit integers (oh my!): */
				for(int i=0;i<device->getNumButtons();++i)
					{
					int buttonState=inputDeviceDataFile->read<int>();
					device->setButtonState(i,buttonState);
					}
				}
			
			/* Update valuator states: */
			for(int i=0;i<device->getNumValuators();++i)
				{
				double valuatorState=inputDeviceDataFile->read<double>();
				device->setValuator(i,valuatorState);
				}
			}
		
		/* Check if the device's valid flag changed: */
		if(validFlags[deviceIndex]!=deviceValid)
			{
			/* Enable or disable the device in the input graph manager: */
			inputDeviceManager->getInputGraphManager()->setEnabled(device,deviceValid);
			
			/* Update the device's valid flag: */
			validFlags[deviceIndex]=deviceValid;
			}
		}
	
	/* Data file version 4 and later contain text event data: */
	if(fileVersion>=4)
		{
		/* Read and enqueue all text and text control events: */
		inputDeviceManager->getTextEventDispatcher()->readEventQueues(*inputDeviceDataFile);
		}
	}

InputDeviceAdapterPlayback::InputDeviceAdapterPlayback(InputDeviceManager* sInputDeviceManager,const Misc::ConfigurationFileSection& configFileSection)
	:InputDeviceAdapter(sInputDeviceManager),
	 applyPreTransform(false),
	 mouseCursorFaker(0),
	 synchronizePlayback(configFileSection.retrieveValue<bool>("./synchronizePlayback",false)),
	 quitWhenDone(configFileSection.retrieveValue<bool>("./quitWhenDone",false)),
	 soundPlayer(0),
	 #ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT
	 kinectPlayer(0),
	 #endif
	 saveMovie(configFileSection.retrieveValue<bool>("./saveMovie",false)),
	 movieWindowIndex(0),movieWindow(0),movieFrameTimeInterval(1.0/30.0),
	 movieFrameStart(0),movieFrameOffset(0),
	 timeStamp(0.0),timeStampOffset(0.0),
	 nextTimeStamp(0.0),
	 validFlags(0),
	 nextMovieFrameTime(0.0),nextMovieFrameCounter(0),
	 done(false)
	{
	/* Open the common base directory: */
	IO::DirectoryPtr baseDirectory=IO::openDirectory(configFileSection.retrieveString("./baseDirectory",".").c_str());
	
	/* Open the input device data file: */
	inputDeviceDataFile=baseDirectory->openFile(configFileSection.retrieveString("./inputDeviceDataFileName").c_str());
	
	/* Read file header: */
	inputDeviceDataFile->setEndianness(Misc::LittleEndian);
	static const char* fileHeader="Vrui Input Device Data File v5.0\n";
	char header[34];
	inputDeviceDataFile->read<char>(header,34);
	header[33]='\0';
	
	if(strncmp(header,fileHeader,29)!=0)
		{
		/* Pre-versioning file version: */
		fileVersion=1;
		
		/* Old file format doesn't have the header text; open it again to start over: */
		inputDeviceDataFile=baseDirectory->openFile(configFileSection.retrieveString("./inputDeviceDataFileName").c_str());
		}
	else if(strcmp(header+29,"2.0\n")==0)
		{
		/* File version without ray direction and velocities: */
		fileVersion=2;
		}
	else if(strcmp(header+29,"3.0\n")==0)
		{
		/* File version with ray direction and velocities: */
		fileVersion=3;
		}
	else if(strcmp(header+29,"4.0\n")==0)
		{
		/* File version with text and text control events: */
		fileVersion=4;
		}
	else if(strcmp(header+29,"5.0\n")==0)
		{
		/* File version with valid flags: */
		fileVersion=5;
		}
	else
		{
		header[32]='\0';
		Misc::throwStdErr("Vrui::InputDeviceAdapterPlayback: Unsupported input device data file version %s",header+29);
		}
	
	/* Read random seed value: */
	unsigned int randomSeed=inputDeviceDataFile->read<unsigned int>();
	setRandomSeed(randomSeed);
	
	/* Read number of saved input devices: */
	numInputDevices=inputDeviceDataFile->read<int>();
	inputDevices=new InputDevice*[numInputDevices];
	deviceFeatureBaseIndices=new int[numInputDevices];
	validFlags=new bool[numInputDevices];
	
	/* Initialize devices: */
	for(int i=0;i<numInputDevices;++i)
		{
		/* Read device's name and layout from file: */
		std::string name;
		if(fileVersion>=2)
			name=Misc::readCppString(*inputDeviceDataFile);
		else
			{
			/* Read a fixed-size string: */
			char nameBuffer[40];
			inputDeviceDataFile->read(nameBuffer,sizeof(nameBuffer));
			name=nameBuffer;
			}
		int trackType=inputDeviceDataFile->read<int>();
		int numButtons=inputDeviceDataFile->read<int>();
		int numValuators=inputDeviceDataFile->read<int>();
		
		/* Create new input device: */
		InputDevice* newDevice=inputDeviceManager->createInputDevice(name.c_str(),trackType,numButtons,numValuators,true);
		
		if(fileVersion<3)
			{
			/* Read the device ray direction: */
			Vector deviceRayDir;
			inputDeviceDataFile->read(deviceRayDir.getComponents(),3);
			newDevice->setDeviceRay(deviceRayDir,Scalar(0));
			}
		
		/* Initialize the new device's glyph from the current configuration file section: */
		Glyph& deviceGlyph=inputDeviceManager->getInputGraphManager()->getInputDeviceGlyph(newDevice);
		char deviceGlyphTypeTag[32];
		snprintf(deviceGlyphTypeTag,sizeof(deviceGlyphTypeTag),"./device%dGlyphType",i);
		char deviceGlyphMaterialTag[32];
		snprintf(deviceGlyphMaterialTag,sizeof(deviceGlyphMaterialTag),"./device%dGlyphMaterial",i);
		deviceGlyph.configure(configFileSection,deviceGlyphTypeTag,deviceGlyphMaterialTag);
		
		/* Store the input device: */
		inputDevices[i]=newDevice;
		
		/* Read or create the device's feature names: */
		deviceFeatureBaseIndices[i]=int(deviceFeatureNames.size());
		if(fileVersion>=2)
			{
			/* Read feature names from file: */
			for(int j=0;j<newDevice->getNumFeatures();++j)
				deviceFeatureNames.push_back(Misc::readCppString(*inputDeviceDataFile));
			}
		else
			{
			/* Create default feature names: */
			for(int j=0;j<newDevice->getNumFeatures();++j)
				deviceFeatureNames.push_back(getDefaultFeatureName(InputDeviceFeature(newDevice,j)));
			}
		
		/* Initialize the device as valid: */
		validFlags[i]=true;
		}
	
	/* Check if the user wants to pre-transform stored device data: */
	if(configFileSection.hasTag("./preTransform"))
		{
		/* Read the pre-transformation: */
		applyPreTransform=true;
		preTransform=configFileSection.retrieveValue<OGTransform>("./preTransform");
		}
	
	/* Check if the user wants to use a fake mouse cursor: */
	int fakeMouseCursorDevice=configFileSection.retrieveValue<int>("./fakeMouseCursorDevice",-1);
	if(fakeMouseCursorDevice>=0)
		{
		/* Read the cursor file name and nominal size: */
		std::string mouseCursorImageFileName=VRUI_INTERNAL_CONFIG_SHAREDIR;
		mouseCursorImageFileName.append("/Textures/Cursor.Xcur");
		mouseCursorImageFileName=configFileSection.retrieveString("./mouseCursorImageFileName",mouseCursorImageFileName.c_str());
		unsigned int mouseCursorNominalSize=configFileSection.retrieveValue<unsigned int>("./mouseCursorNominalSize",24);
		
		/* Create the mouse cursor faker: */
		mouseCursorFaker=new MouseCursorFaker(inputDevices[fakeMouseCursorDevice],mouseCursorImageFileName.c_str(),mouseCursorNominalSize);
		mouseCursorFaker->setCursorSize(configFileSection.retrieveValue<Size>("./mouseCursorSize",mouseCursorFaker->getCursorSize()));
		mouseCursorFaker->setCursorHotspot(configFileSection.retrieveValue<Vector>("./mouseCursorHotspot",mouseCursorFaker->getCursorHotspot()));
		}
	
	/* Read the initial application time stamp: */
	try
		{
		timeStamp=inputDeviceDataFile->read<double>();
		synchronize(timeStamp);
		}
	catch(const IO::File::ReadError&)
		{
		done=true;
		nextTimeStamp=Math::Constants<double>::max;
		
		if(quitWhenDone)
			{
			/* Request exiting the program: */
			shutdown();
			}
		}
	
	/* Check if the user wants to play back a commentary sound track: */
	std::string soundFileName=configFileSection.retrieveString("./soundFileName","");
	if(!soundFileName.empty())
		{
		try
			{
			/* Create a sound player for the given sound file name: */
			soundPlayer=new Sound::SoundPlayer(baseDirectory->getPath(soundFileName.c_str()).c_str());
			}
		catch(const std::runtime_error& err)
			{
			/* Print a message, but carry on: */
			Misc::formattedConsoleWarning("InputDeviceAdapterPlayback: Disabling sound recording due to exception %s",err.what());
			}
		}
	
	#ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT
	/* Check if the user wants to play back 3D video: */
	std::string kinectPlayerSectionName=configFileSection.retrieveString("./kinectPlayer","");
	if(!kinectPlayerSectionName.empty())
		{
		/* Go to the Kinect player's section: */
		Misc::ConfigurationFileSection kinectPlayerSection=configFileSection.getSection(kinectPlayerSectionName.c_str());
		kinectPlayer=new KinectPlayback(nextTimeStamp,kinectPlayerSection);
		}
	#endif
	
	/* Check if the user wants to save a movie: */
	if(saveMovie)
		{
		/* Read the movie image file name template: */
		movieFileNameTemplate=baseDirectory->getPath(configFileSection.retrieveString("./movieFileNameTemplate").c_str());
		
		/* Check if the name template has the correct format: */
		if(!Misc::isValidTemplate(movieFileNameTemplate,'d',1024))
			Misc::throwStdErr("InputDeviceAdapterPlayback::InputDeviceAdapterPlayback: movie file name template \"%s\" does not have exactly one %%d conversion",movieFileNameTemplate.c_str());
		
		/* Get the index of the window from which to save the frames: */
		movieWindowIndex=configFileSection.retrieveValue<int>("./movieWindowIndex",movieWindowIndex);
		
		/* Get the intended frame rate for the movie: */
		movieFrameTimeInterval=1.0/configFileSection.retrieveValue<double>("./movieFrameRate",1.0/movieFrameTimeInterval);
		
		/* Get the number of movie frames to skip: */
		movieFrameStart=configFileSection.retrieveValue<int>("./movieSkipFrames",movieFrameStart);
		
		/* Get the index of the first movie frame: */
		movieFrameOffset=configFileSection.retrieveValue<int>("./movieFirstFrameIndex",movieFrameOffset);
		}
	}

InputDeviceAdapterPlayback::~InputDeviceAdapterPlayback(void)
	{
	delete mouseCursorFaker;
	delete soundPlayer;
	#ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT
	delete kinectPlayer;
	#endif
	delete[] deviceFeatureBaseIndices;
	delete[] validFlags;
	}

std::string InputDeviceAdapterPlayback::getFeatureName(const InputDeviceFeature& feature) const
	{
	/* Find the input device owning the given feature: */
	int featureBaseIndex=-1;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==feature.getDevice())
			{
			featureBaseIndex=deviceFeatureBaseIndices[deviceIndex];
			break;
			}
		}
	if(featureBaseIndex<0)
		Misc::throwStdErr("InputDeviceAdapterPlayback::getFeatureName: Unknown device %s",feature.getDevice()->getDeviceName());
	
	/* Return the feature name: */
	return deviceFeatureNames[featureBaseIndex+feature.getFeatureIndex()];
	}

int InputDeviceAdapterPlayback::getFeatureIndex(InputDevice* device,const char* featureName) const
	{
	/* Find the input device owning the given feature: */
	int featureBaseIndex=-1;
	for(int deviceIndex=0;deviceIndex<numInputDevices;++deviceIndex)
		{
		if(inputDevices[deviceIndex]==device)
			{
			featureBaseIndex=deviceFeatureBaseIndices[deviceIndex];
			break;
			}
		}
	if(featureBaseIndex<0)
		Misc::throwStdErr("InputDeviceAdapterPlayback::getFeatureIndex: Unknown device %s",device->getDeviceName());
	
	/* Compare the given feature name against the device's feature names: */
	for(int i=0;i<device->getNumFeatures();++i)
		if(deviceFeatureNames[featureBaseIndex+i]==featureName)
			return i;
	
	return -1;
	}

void InputDeviceAdapterPlayback::prepareMainLoop(void)
	{
	if(synchronizePlayback)
		{
		/* Calculate the offset between the saved timestamps and the system's wall clock time: */
		Misc::Time rt=Misc::Time::now();
		double realTime=double(rt.tv_sec)+double(rt.tv_nsec)/1000000000.0;
		timeStampOffset=nextTimeStamp-realTime;
		}
	
	/* Start the sound player, if there is one: */
	if(soundPlayer!=0)
		soundPlayer->start();
	
	if(saveMovie)
		{
		/* Get a pointer to the window from which to save movie frames: */
		if(movieWindowIndex>=0&&movieWindowIndex<getNumWindows())
			movieWindow=getWindow(movieWindowIndex);
		else
			Misc::formattedConsoleWarning("InputDeviceAdapterPlayback: Not saving movie due to invalid movie window index %d",movieWindowIndex);
		
		/* Calculate the first time at which to save a frame: */
		nextMovieFrameTime=nextTimeStamp+movieFrameTimeInterval*0.5;
		}
	}

void InputDeviceAdapterPlayback::updateInputDevices(void)
	{
	/* Do nothing if at end of file: */
	if(done)
		return;
	
	timeStamp=nextTimeStamp;
	
	if(synchronizePlayback)
		{
		/* Check if there is positive drift between the system's offset wall clock time and the next time stamp: */
		Misc::Time rt=Misc::Time::now();
		double realTime=double(rt.tv_sec)+double(rt.tv_nsec)/1000000000.0;
		double delta=nextTimeStamp-(realTime+timeStampOffset);
		if(delta>0.0)
			{
			/* Block to correct the drift: */
			vruiDelay(delta);
			}
		}
	
	/* Read new device states: */
	readDeviceStates();
	
	/* Read time stamp of next data frame: */
	try
		{
		nextTimeStamp=inputDeviceDataFile->read<double>();
		
		/* Request a synchronized update for the next frame: */
		synchronize(nextTimeStamp,false);
		requestUpdate();
		}
	catch(const IO::File::ReadError&)
		{
		done=true;
		nextTimeStamp=Math::Constants<double>::max;
		
		if(quitWhenDone)
			{
			/* Request exiting the program: */
			shutdown();
			}
		}
	
	#ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT
	if(kinectPlayer!=0)
		{
		/* Update the 3D video player and prepare it for the next frame: */
		kinectPlayer->frame(timeStamp,nextTimeStamp);
		}
	#endif
	
	if(saveMovie&&movieWindow!=0)
		{
		/* Copy the last saved screenshot if multiple movie frames needed to be taken during the last Vrui frame: */
		while(nextMovieFrameTime<timeStamp&&nextMovieFrameCounter>movieFrameStart)
			{
			/* Copy the last saved screenshot: */
			pid_t childPid=fork();
			if(childPid==0)
				{
				/* Create the old and new file names: */
				char oldImageFileName[1024];
				snprintf(oldImageFileName,sizeof(oldImageFileName),movieFileNameTemplate.c_str(),nextMovieFrameCounter-movieFrameStart+movieFrameOffset-1);
				char imageFileName[1024];
				snprintf(imageFileName,sizeof(imageFileName),movieFileNameTemplate.c_str(),nextMovieFrameCounter-movieFrameStart+movieFrameOffset);
				
				/* Execute the cp system command: */
				char* cpArgv[10];
				int cpArgc=0;
				cpArgv[cpArgc++]=const_cast<char*>("/bin/cp");
				cpArgv[cpArgc++]=oldImageFileName;
				cpArgv[cpArgc++]=imageFileName;
				cpArgv[cpArgc++]=0;
				execvp(cpArgv[0],cpArgv);
				}
			else
				{
				/* Wait for the copy process to finish: */
				waitpid(childPid,0,0);
				}
			
			/* Advance the frame counters: */
			nextMovieFrameTime+=movieFrameTimeInterval;
			++nextMovieFrameCounter;
			}
		
		if(nextTimeStamp>nextMovieFrameTime)
			{
			if(nextMovieFrameCounter>=movieFrameStart)
				{
				/* Request a screenshot from the movie window: */
				char imageFileName[1024];
				snprintf(imageFileName,sizeof(imageFileName),movieFileNameTemplate.c_str(),nextMovieFrameCounter-movieFrameStart+movieFrameOffset);
				movieWindow->requestScreenshot(imageFileName);
				}
			
			/* Advance the movie frame counters: */
			nextMovieFrameTime+=movieFrameTimeInterval;
			++nextMovieFrameCounter;
			}
		}
	}

#ifdef VRUI_INPUTDEVICEADAPTERPLAYBACK_USE_KINECT

void InputDeviceAdapterPlayback::glRenderAction(GLContextData& contextData) const
	{
	if(kinectPlayer!=0)
		{
		/* Let the 3D video player render its current frames: */
		kinectPlayer->glRenderAction(contextData);
		}
	}

#endif

}
