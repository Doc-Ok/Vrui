/***********************************************************************
VruiSoundTest - Small application to illustrate principles of spatial
audio programming using Vrui's OpenAL interface.
Copyright (c) 2010-2020 Oliver Kreylos

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

#include <string>
#include <stdexcept>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLModels.h>
#include <GL/GLGeometryWrappers.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/WAVFile.h>
#include <AL/Config.h>
#include <AL/ALGeometryWrappers.h>
#include <AL/ALObject.h>
#include <AL/ALContextData.h>
#include <Vrui/Application.h>
#include <Vrui/Vrui.h>

class VruiSoundTest:public Vrui::Application,public GLObject,public ALObject
	{
	/* Embedded classes: */
	private:
	struct GLDataItem:public GLObject::DataItem // Structure to hold application state associated with a particular OpenGL context
		{
		/* Elements: */
		public:
		GLuint displayListId; // Display list to render a sphere
		
		/* Constructors and destructors: */
		public:
		GLDataItem(void)
			:displayListId(glGenLists(1))
			{
			}
		virtual ~GLDataItem(void)
			{
			glDeleteLists(displayListId,1);
			}
		};
	
	struct ALDataItem:public ALObject::DataItem // Structure to hold application state associated with a particular OpenAL context
		{
		/* Elements: */
		public:
		#if ALSUPPORT_CONFIG_HAVE_OPENAL
		ALuint source; // Sound sources
		ALuint buffer; // Sample buffer for the sound source
		#endif
		bool soundPaused; // Flag whether the sound source is currently paused
		
		/* Constructors and destructors: */
		public:
		ALDataItem(void)
			:soundPaused(false)
			{
			#if ALSUPPORT_CONFIG_HAVE_OPENAL
			/* Generate buffers and sources: */
			alGenSources(1,&source);
			alGenBuffers(1,&buffer);
			#endif
			}
		virtual ~ALDataItem(void)
			{
			#if ALSUPPORT_CONFIG_HAVE_OPENAL
			/* Destroy buffers and sources: */
			alDeleteSources(1,&source);
			alDeleteBuffers(1,&buffer);
			#endif
			}
		};
	
	/* Elements: */
	std::string wavFileName; // Name of an audio file in WAV format to play back
	Vrui::Point sourcePosition; // Position of the sound source in navigational space
	bool pauseSound; // Flag to request pausing sound playback
	
	/* Constructors and destructors: */
	public:
	VruiSoundTest(int& argc,char**& argv);
	virtual ~VruiSoundTest(void);
	
	/* Methods from class Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void sound(ALContextData& contextData) const;
	virtual void resetNavigation(void);
	virtual void eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* Methods from class ALObject: */
	virtual void initContext(ALContextData& contextData) const;
	};

/******************************
Methods of class VruiSoundTest:
******************************/

VruiSoundTest::VruiSoundTest(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 pauseSound(false)
	{
	/* Check if the user wants to play back a WAV file: */
	if(argc>1)
		wavFileName=argv[1];
	
	/* Request sound processing: */
	Vrui::requestSound();
	
	/* Initialize the sound source position: */
	sourcePosition=Vrui::Point(0,10,0);
	
	/* Create an event tool class to pause/unpause sound playback: */
	addEventTool("Pause Audio",0,0);
	}

VruiSoundTest::~VruiSoundTest(void)
	{
	}

void VruiSoundTest::frame(void)
	{
	}

void VruiSoundTest::display(GLContextData& contextData) const
	{
	/* Get the data item: */
	GLDataItem* dataItem=contextData.retrieveDataItem<GLDataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_LIGHTING_BIT);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
	
	/* Draw the sphere indicating the sound source's position: */
	glPushMatrix();
	glTranslate(sourcePosition-Vrui::Point::origin);
	glColor3f(0.0f,1.0f,0.0f);
	glCallList(dataItem->displayListId);
	glPopMatrix();
	
	/* Reset OpenGL state: */
	glPopAttrib();
	}

void VruiSoundTest::sound(ALContextData& contextData) const
	{
	typedef ALContextData::Point ALPoint;
	typedef ALContextData::Transform ALTransform;
	
	/* Get the data item: */
	ALDataItem* dataItem=contextData.retrieveDataItem<ALDataItem>(this);
	
	/* Check if the sound source needs to be paused or unpaused: */
	if(dataItem->soundPaused!=pauseSound)
		{
		/* Pause or unpause the audio source: */
		if(pauseSound)
			alSourcePause(dataItem->source);
		else
			alSourcePlay(dataItem->source);
		
		dataItem->soundPaused=pauseSound;
		}
	
	/* Get the current modelview matrix from the OpenAL context: */
	const ALTransform& transform=contextData.getMatrix();
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	/* Set the source position transformed to physical coordinates: */
	alSourcePosition(dataItem->source,ALPoint(sourcePosition),transform);
	#endif
	}

void VruiSoundTest::resetNavigation(void)
	{
	/* Initialize the navigation transformation: */
	Vrui::setNavigationTransformation(Vrui::Point::origin,Vrui::Scalar(10),Vrui::Vector(0,0,1));
	}

void VruiSoundTest::eventCallback(EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	pauseSound=cbData->newButtonState;
	}

void VruiSoundTest::initContext(GLContextData& contextData) const
	{
	/* Create a new context data item: */
	GLDataItem* dataItem=new GLDataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create a display list to render a sphere: */
	glNewList(dataItem->displayListId,GL_COMPILE);
	
	/* Draw the sphere: */
	glDrawSphereIcosahedron(1.0f,5);
	
	glEndList();
	}

void VruiSoundTest::initContext(ALContextData& contextData) const
	{
	/* Create a new context data item: */
	ALDataItem* dataItem=new ALDataItem;
	contextData.addDataItem(this,dataItem);
	
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	
	/* Check if the user wants to play back a WAV file: */
	if(!wavFileName.empty())
		{
		/* Open the WAV file and access its sound data format: */
		Sound::WAVFile wav(IO::openFile(wavFileName.c_str()));
		const Sound::SoundDataFormat& format=wav.getFormat();
		
		/* Create a mono sound buffer matching the file's sample format: */
		switch(format.bytesPerSample)
			{
			case 1:
				{
				/* Upload 8-bit unsigned integer samples: */
				ALubyte* pcmData=new ALubyte[wav.getNumAudioFrames()];
				
				/* Read and downmix the WAV file into the audio buffer: */
				wav.readMonoAudioFrames(pcmData,wav.getNumAudioFrames());
				
				/* Upload the audio buffer: */
				alBufferData(dataItem->buffer,AL_FORMAT_MONO8,pcmData,wav.getNumAudioFrames()*sizeof(ALubyte),format.framesPerSecond);
				
				break;
				}
			
			case 2:
				{
				/* Upload 16-bit signed integer samples: */
				ALshort* pcmData=new ALshort[wav.getNumAudioFrames()];
				
				/* Read and downmix the WAV file into the audio buffer: */
				wav.readMonoAudioFrames(pcmData,wav.getNumAudioFrames());
				
				/* Upload the audio buffer: */
				alBufferData(dataItem->buffer,AL_FORMAT_MONO16,pcmData,wav.getNumAudioFrames()*sizeof(ALshort),format.framesPerSecond);
				
				break;
				}
			
			default:
				throw std::runtime_error("VruiSoundTest: Unsupported sample size in input WAV file");
			}
		}
	else
		{
		/* Upload a one-second sound sample into the sound buffer: */
		const int pcmFreq=44100;
		ALubyte* pcmData=new ALubyte[pcmFreq];
		
		/* Create a 400 Hz sine wave: */
		for(int i=0;i<pcmFreq;++i)
			{
			double angle=400.0*2.0*Math::Constants<double>::pi*double(i)/double(pcmFreq);
			pcmData[i]=ALubyte(Math::sin(angle)*127.5+128.0);
			}
		alBufferData(dataItem->buffer,AL_FORMAT_MONO8,pcmData,pcmFreq,pcmFreq);
		
		delete[] pcmData;
		}
	
	/* Create a looping source object: */
	alSourceBuffer(dataItem->source,dataItem->buffer);
	alSourceLooping(dataItem->source,AL_TRUE);
	alSourcePitch(dataItem->source,1.0f);
	alSourceGain(dataItem->source,1.0f);
	
	/* Set the source attenuation factors in physical coordinates: */
	alSourceReferenceDistance(dataItem->source,contextData.getReferenceDistance());
	alSourceRolloffFactor(dataItem->source,contextData.getRolloffFactor());
	
	/* Start playing the source object: */
	alSourcePlay(dataItem->source);
	
	#endif
	}

VRUI_APPLICATION_RUN(VruiSoundTest)
