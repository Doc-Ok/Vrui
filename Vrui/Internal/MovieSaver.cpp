/***********************************************************************
MovieSaver - Helper class to save movies, as sequences of frames or
already encoded into a video container format, from VR windows.
Copyright (c) 2010-2018 Oliver Kreylos

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

#include <Vrui/Internal/MovieSaver.h>

#include <Video/Config.h>

#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/CreateNumberedFileName.h>
#include <IO/OpenFile.h>
#include <Sound/SoundDataFormat.h>
#include <Sound/SoundRecorder.h>
#include <Vrui/Internal/ImageSequenceMovieSaver.h>
#if VIDEO_CONFIG_HAVE_THEORA
#include <Vrui/Internal/TheoraMovieSaver.h>
#endif

namespace Vrui {

/****************************************
Methods of class MovieSaver::FrameBuffer:
****************************************/

MovieSaver::FrameBuffer::FrameBuffer(void)
	:buffer(0)
	{
	/* Set an invalid frame size: */
	frameSize[0]=frameSize[1]=0;
	}

MovieSaver::FrameBuffer::FrameBuffer(const MovieSaver::FrameBuffer& source)
	:buffer(source.buffer)
	{
	/* Copy the frame size and reference the source image data: */
	frameSize[0]=source.frameSize[0];
	frameSize[1]=source.frameSize[1];
	ref();
	}

MovieSaver::FrameBuffer& MovieSaver::FrameBuffer::operator=(const MovieSaver::FrameBuffer& source)
	{
	if(buffer!=source.buffer)
		{
		/* Release the current image data: */
		unref();
		
		/* Update the frame size and reference the source image data: */
		frameSize[0]=source.frameSize[0];
		frameSize[1]=source.frameSize[1];
		buffer=source.buffer;
		ref();
		}
	
	return *this;
	}

MovieSaver::FrameBuffer::~FrameBuffer(void)
	{
	/* Unreference and possibly delete the image data: */
	unref();
	}

void MovieSaver::FrameBuffer::setFrameSize(int newWidth,int newHeight)
	{
	if(frameSize[0]!=newWidth||frameSize[1]!=newHeight)
		{
		/* Release the current image data: */
		unref();
		
		/* Update the frame size and allocate new image data: */
		frameSize[0]=newWidth;
		frameSize[1]=newHeight;
		unsigned int* allocBuffer=new unsigned int[(frameSize[1]*frameSize[0]*3+sizeof(unsigned int)-1)/sizeof(unsigned int)];
		allocBuffer[0]=1;
		buffer=reinterpret_cast<unsigned char*>(allocBuffer+1);
		}
	}

void MovieSaver::FrameBuffer::prepareWrite(void)
	{
	if(buffer!=0)
		{
		/* Check if the buffer is shared: */
		if(reinterpret_cast<unsigned int*>(buffer)[-1]!=1)
			{
			/* Release the current image data: */
			unref();
			
			/* Allocate new image data: */
			unsigned int* allocBuffer=new unsigned int[(frameSize[1]*frameSize[0]*3+sizeof(unsigned int)-1)/sizeof(unsigned int)];
			allocBuffer[0]=1;
			buffer=reinterpret_cast<unsigned char*>(allocBuffer+1);
			}
		}
	}

/***************************
Methods of class MovieSaver:
***************************/

void* MovieSaver::frameWritingThreadWrapper(void)
	{
	/* Start the virtual thread method: */
	frameWritingThreadMethod();
	return 0;
	}

int MovieSaver::waitForNextFrame(void)
	{
	/* Check for skipped frames: */
	int numSkippedFrames=0;
	Misc::Time t=Misc::Time::now();
	while(nextFrameTime<t)
		{
		/* We missed a frame: */
		nextFrameTime+=frameInterval;
		++numSkippedFrames;
		}
	
	/* Sleep until the next frame is due: */
	Misc::sleep(nextFrameTime-t);
	nextFrameTime+=frameInterval;
	
	return numSkippedFrames;
	}

void MovieSaver::stopSound(void)
	{
	/* Delete the sound recorder: */
	delete soundRecorder;
	soundRecorder=0;
	}

MovieSaver::MovieSaver(const Misc::ConfigurationFileSection& configFileSection)
	:baseDirectory(IO::openDirectory(configFileSection.retrieveString("./movieBaseDirectory",".").c_str())),
	 frameRate(configFileSection.retrieveValue<double>("./movieFrameRate",30.0)),
	 frameInterval(1.0/frameRate),
	 soundRecorder(0),
	 firstFrame(true)
	{
	/* Check if the user wants to record a commentary track: */
	std::string soundFileName=configFileSection.retrieveString("./movieSoundFileName","");
	if(!soundFileName.empty())
		{
		try
			{
			/* Create a sound data format for recording: */
			Sound::SoundDataFormat soundDataFormat;
			soundDataFormat.bitsPerSample=configFileSection.retrieveValue<int>("./movieSampleResolution",soundDataFormat.bitsPerSample);
			soundDataFormat.samplesPerFrame=configFileSection.retrieveValue<int>("./movieNumChannels",soundDataFormat.samplesPerFrame);
			soundDataFormat.framesPerSecond=configFileSection.retrieveValue<int>("./movieSampleRate",soundDataFormat.framesPerSecond);
			
			/* Create a sound recorder for the given sound file name: */
			std::string audioSourceName=configFileSection.retrieveString("./movieSoundDeviceName","default");
			soundFileName=baseDirectory->getPath(baseDirectory->createNumberedFileName(soundFileName.c_str(),4).c_str());
			soundRecorder=new Sound::SoundRecorder(audioSourceName.c_str(),soundDataFormat,soundFileName.c_str());
			}
		catch(const std::runtime_error& err)
			{
			/* Print a message, but carry on: */
			Misc::formattedConsoleWarning("MovieSaver: Disabling sound recording due to exception %s",err.what());
			}
		}
	}

MovieSaver::~MovieSaver(void)
	{
	/* Delete the sound recorder: */
	delete soundRecorder;
	
	if(!frameWritingThread.isJoined())
		{
		/* Stop the frame writing thread: */
		frameWritingThread.cancel();
		frameWritingThread.join();
		}
	}

MovieSaver* MovieSaver::createMovieSaver(const Misc::ConfigurationFileSection& configFileSection)
	{
	#if VIDEO_CONFIG_HAVE_THEORA
	
	/* Determine the desired movie saver type: */
	if(configFileSection.retrieveValue<bool>("./movieSaveTheora",false))
		{
		/* Return a Theora movie saver: */
		return new TheoraMovieSaver(configFileSection);
		}
	else
		{
		/* Return an image sequence movie saver: */
		return new ImageSequenceMovieSaver(configFileSection);
		}
	
	#else
	
	/* Return an image sequence movie saver: */
	return new ImageSequenceMovieSaver(configFileSection);
	
	#endif
	}

void MovieSaver::postNewFrame(void)
	{
	/* Post the new frame into the triple buffer: */
	frames.postNewValue();
	
	if(firstFrame)
		{
		if(soundRecorder!=0)
			soundRecorder->start();
		
		/* Start the frame timer: */
		nextFrameTime=Misc::Time::now();
		nextFrameTime+=frameInterval;
		
		/* Start the frame writing thread: */
		frameWritingThread.start(this,&MovieSaver::frameWritingThreadWrapper);
		
		firstFrame=false;
		}
	}

}
