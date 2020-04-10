/***********************************************************************
TheoraMovieSaver - Helper class to save movies as Theora video streams
packed into an Ogg container.
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

#include <Vrui/Internal/TheoraMovieSaver.h>

#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/File.h>
#include <Video/FrameBuffer.h>
#include <Video/ImageExtractorRGB8.h>
#include <Video/OggPage.h>
#include <Video/TheoraInfo.h>
#include <Video/TheoraComment.h>

namespace Vrui {

/*********************************
Methods of class TheoraMovieSaver:
*********************************/

void TheoraMovieSaver::frameWritingThreadMethod(void)
	{
	/* Save frames until shut down: */
	unsigned int frameIndex=0;
	while(!done)
		{
		/* Add the most recent frame to the captured frame queue: */
		{
		Threads::MutexCond::Lock captureLock(captureCond);
		frames.lockNewValue();
		capturedFrames.push_back(frames.getLockedValue());
		captureCond.signal();
		}
		
		/* Wait for the next frame: */
		int numSkippedFrames=waitForNextFrame();
		if(numSkippedFrames>0)
			{
			std::cerr<<"TheoraMovieSaver: Skipped frames "<<frameIndex<<" to "<<frameIndex+numSkippedFrames-1<<std::endl;
			frameIndex+=numSkippedFrames;
			}
		}
	}

void* TheoraMovieSaver::frameSavingThreadMethod(void)
	{
	/* Wait for the first frame: */
	FrameBuffer frame;
	{
	Threads::MutexCond::Lock captureLock(captureCond);
	while(!done&&capturedFrames.empty())
		captureCond.wait(captureLock);
	if(capturedFrames.empty()) // Bail out if there will be no more frames
		return 0;
	frame=capturedFrames.front();
	capturedFrames.pop_front();
	}
	
	/* Create the Theora info structure: */
	Video::TheoraInfo theoraInfo;
	unsigned int imageSize[2];
	for(int i=0;i<2;++i)
		imageSize[i]=(unsigned int)frame.getFrameSize()[i];
	theoraInfo.setImageSize(imageSize);
	theoraInfo.colorspace=TH_CS_UNSPECIFIED;
	theoraInfo.pixel_fmt=TH_PF_420;
	theoraInfo.target_bitrate=theoraBitrate;
	theoraInfo.quality=theoraQuality;
	theoraInfo.setGopSize(theoraGopSize);
	theoraInfo.fps_numerator=theoraFrameRate;
	theoraInfo.fps_denominator=1;
	theoraInfo.aspect_numerator=1;
	theoraInfo.aspect_denominator=1;
	theoraEncoder.init(theoraInfo);
	if(!theoraEncoder.isValid())
		{
		std::cerr<<"TheoraMovieSaver: Could not initialize Theora encoder"<<std::endl;
		return 0;
		}
	
	/* Create the image extractor: */
	imageExtractor=new Video::ImageExtractorRGB8(imageSize);
	
	/* Create the Theora frame buffer: */
	theoraFrame.init420(theoraInfo);
	
	/*************************************************
	Write the Theora stream headers to the Ogg stream:
	*************************************************/
	
	/* Set up a comment structure: */
	Video::TheoraComment comments;
	comments.setVendorString("Virtual Reality User Interface (Vrui) MovieSaver");
	
	/* Write the first stream header packet to the movie file: */
	Video::TheoraPacket packet;
	if(theoraEncoder.emitHeader(comments,packet))
		{
		/* Write the packet to the movie file: */
		oggStream.packetIn(packet);
		Video::OggPage page;
		while(oggStream.flush(page))
			page.write(*movieFile);
		}
	
	/* Write all remaining stream header packets to the movie file: */
	while(theoraEncoder.emitHeader(comments,packet))
		{
		oggStream.packetIn(packet);
		Video::OggPage page;
		while(oggStream.pageOut(page))
			page.write(*movieFile);
		}
	
	/* Flush the Ogg stream: */
	Video::OggPage page;
	while(oggStream.flush(page))
		page.write(*movieFile);
	
	/* Encode and save frames until shut down: */
	while(true)
		{
		/* Wait for the next frame: */
		FrameBuffer frame;
		{
		Threads::MutexCond::Lock captureLock(captureCond);
		while(!done&&capturedFrames.empty())
			captureCond.wait(captureLock);
		if(capturedFrames.empty()) // Bail out if there will be no more frames
			break;
		frame=capturedFrames.front();
		capturedFrames.pop_front();
		
		/* Print a progress report if movie saver is already shut down: */
		if(done)
			{
			std::cout<<"\rTheoraMovieSaver: "<<capturedFrames.size()+1<<" movie frames left to encode ";
			if(capturedFrames.empty())
				std::cout<<std::endl;
			else
				std::cout<<std::flush;
			}
		}
		
		/* Check if the frame is still the same size: */
		if(imageSize[0]!=(unsigned int)frame.getFrameSize()[0]||imageSize[1]!=(unsigned int)frame.getFrameSize()[1])
			{
			/* Theora cannot handle changing frame sizes; bail out with an error: */
			std::cerr<<"TheoraMovieSaver: Terminating due to changed frame size"<<std::endl;
			return 0;
			}
		
		/* Convert the new raw RGB frame to Y'CbCr 4:2:0: */
		Video::FrameBuffer tempFrame;
		tempFrame.start=frame.getBuffer();
		imageExtractor->extractYpCbCr420(&tempFrame,theoraFrame.planes[0].data,theoraFrame.planes[0].stride,theoraFrame.planes[1].data,theoraFrame.planes[1].stride,theoraFrame.planes[2].data,theoraFrame.planes[2].stride);
		
		/* Feed the last converted Y'CbCr 4:2:0 frame to the Theora encoder: */
		theoraEncoder.encodeFrame(theoraFrame);
		
		/* Write all encoded Theora packets to the movie file: */
		Video::TheoraPacket packet;
		while(theoraEncoder.emitPacket(packet))
			{
			/* Add the packet to the Ogg stream: */
			oggStream.packetIn(packet);
			
			/* Write any generated pages to the movie file: */
			Video::OggPage page;
			while(oggStream.pageOut(page))
				page.write(*movieFile);
			}
		}
	
	return 0;
	}

TheoraMovieSaver::TheoraMovieSaver(const Misc::ConfigurationFileSection& configFileSection)
	:MovieSaver(configFileSection),
	 movieFile(baseDirectory->openFile(configFileSection.retrieveString("./movieFileName").c_str(),IO::File::WriteOnly)),
	 oggStream(1),
	 theoraBitrate(0),theoraQuality(32),theoraGopSize(32),
	 done(false),
	 imageExtractor(0)
	{
	movieFile->setEndianness(Misc::LittleEndian);
	
	/* Read the encoder parameters: */
	theoraBitrate=configFileSection.retrieveValue<int>("./movieBitrate",theoraBitrate);
	if(theoraBitrate<0)
		theoraBitrate=0;
	theoraQuality=configFileSection.retrieveValue<int>("./movieQuality",theoraQuality);
	if(theoraQuality<0)
		theoraQuality=0;
	if(theoraQuality>63)
		theoraQuality=63;
	theoraGopSize=configFileSection.retrieveValue<int>("./movieGopSize",theoraGopSize);
	if(theoraGopSize<1)
		theoraGopSize=1;
	
	/* Set the Theora frame rate and adjust the initially configured frame rate: */
	theoraFrameRate=int(frameRate+0.5);
	frameRate=theoraFrameRate;
	frameInterval=Misc::Time(1.0/frameRate);
	
	/* Start the movie file writing thread: */
	frameSavingThread.start(this,&TheoraMovieSaver::frameSavingThreadMethod);
	}

TheoraMovieSaver::~TheoraMovieSaver(void)
	{
	/* Stop sound recording at this moment: */
	stopSound();
	
	/* Signal the frame capturing and saving threads to shut down: */
	done=true;
	captureCond.signal();
	
	/* Wait until the frame saving thread has saved all frames and terminates: */
	frameSavingThread.join();
	
	/* Flush the Ogg stream: */
	Video::OggPage page;
	while(oggStream.flush(page))
		page.write(*movieFile);
	
	/* Delete the image extractor: */
	delete imageExtractor;
	}

}
