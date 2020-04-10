/***********************************************************************
WAVFile - Class to read from or write to audio files in WAV format using
an IO::File abstraction.
Copyright (c) 2019-2020 Oliver Kreylos

This file is part of the Basic Sound Library (Sound).

The Basic Sound Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Sound Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Sound Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Sound/WAVFile.h>

#include <string.h>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/MessageLogger.h>
#include <IO/SeekableFile.h>

namespace Sound {

/************************
Methods of class WAVFile:
************************/

void WAVFile::writeWAVHeader(size_t numAudioFrames)
	{
	/* Set the file to little endian: */
	file->setEndianness(Misc::LittleEndian);
	
	/* Calculate all chunk sizes: */
	size_t dataChunkSize=numAudioFrames*bytesPerFrame;
	size_t dataHeaderSize=sizeof(char)*4+sizeof(Misc::UInt32);
	size_t fmtChunkSize=2*sizeof(Misc::UInt32)+4*sizeof(Misc::UInt16);
	size_t fmtHeaderSize=sizeof(char)*4+sizeof(Misc::UInt32);
	size_t riffChunkSize=sizeof(char)*4+fmtHeaderSize+fmtChunkSize+dataHeaderSize+dataChunkSize;
	
	/* Write the RIFF chunk: */
	file->write<char>("RIFF",4);
	file->write<Misc::UInt32>(riffChunkSize);
	file->write<char>("WAVE",4);
	
	/* Write the fmt chunk: */
	file->write<char>("fmt ",4);
	file->write<Misc::UInt32>(fmtChunkSize);
	file->write<Misc::UInt16>(1); // PCM
	file->write<Misc::UInt16>(format.samplesPerFrame);
	file->write<Misc::UInt32>(format.framesPerSecond);
	file->write<Misc::UInt32>(format.framesPerSecond*format.samplesPerFrame*format.bytesPerSample);
	file->write<Misc::UInt16>(format.samplesPerFrame*format.bytesPerSample);
	file->write<Misc::UInt16>(format.bitsPerSample);
	
	/* Write the data chunk header: */
	file->write<char>("data",4);
	file->write<Misc::UInt32>(dataChunkSize);
	}

WAVFile::WAVFile(IO::FilePtr sFile)
	:file(sFile)
	{
	/* Check if the file is opened for reading: */
	if(file->getReadBufferSize()==0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File is not opened for reading");
	
	/* Set the file to little endian: */
	file->setEndianness(Misc::LittleEndian);
	
	/* Read the RIFF chunk: */
	char riffTag[4];
	file->read<char>(riffTag,4);
	if(memcmp(riffTag,"RIFF",4)!=0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File is not a RIFF file");
	file->skip<Misc::UInt32>(1); // Skip RIFF chunk size
	char waveTag[4];
	file->read<char>(waveTag,4);
	if(memcmp(waveTag,"WAVE",4)!=0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File is not a WAVE file");
	
	/* Read the format chunk: */
	size_t fmtHeaderSize=2*sizeof(Misc::UInt32)+4*sizeof(Misc::UInt16);
	char fmtTag[4];
	file->read<char>(fmtTag,4);
	if(memcmp(fmtTag,"fmt ",4)!=0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File does not have a fmt chunk");
	size_t fmtChunkSize=file->read<Misc::UInt32>();
	if(fmtChunkSize<fmtHeaderSize)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File has truncated fmt chunk");
	if(file->read<Misc::UInt16>()!=1U) // Can only do linear PCM samples for now
		throw std::runtime_error("Sound::WAVFile::WAVFile: File does not contain linear PCM samples");
	format.samplesPerFrame=int(file->read<Misc::UInt16>());
	format.framesPerSecond=int(file->read<Misc::UInt32>());
	size_t bytesPerSecond=file->read<Misc::UInt32>();
	bytesPerFrame=file->read<Misc::UInt16>();
	format.bitsPerSample=int(file->read<Misc::UInt16>());
	
	/* Skip any unused data in the format chunk: */
	fmtChunkSize=(fmtChunkSize+1)&~0x1; // Pad to the next two-byte boundary
	if(fmtChunkSize>fmtHeaderSize)
		file->skip<char>(fmtChunkSize-fmtHeaderSize);
	
	/* Check if the WAV file's sound data format is compatible and fill in missing data: */
	if(format.bitsPerSample<8||format.bitsPerSample>32||(format.bitsPerSample&0x7)!=0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File has unsupported number of bits per sample");
	if(format.bitsPerSample==24)
		format.bytesPerSample=4; // 24 bit sound data padded into 32 bit words
	else
		format.bytesPerSample=format.bitsPerSample/8;
	format.signedSamples=format.bitsPerSample>8;
	format.sampleEndianness=SoundDataFormat::LittleEndian;
	if(format.samplesPerFrame<1)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File has unsupported number of samples per frame");
	if(bytesPerFrame!=size_t(format.samplesPerFrame)*size_t(format.bytesPerSample)
	   ||bytesPerSecond!=size_t(format.framesPerSecond)*size_t(format.samplesPerFrame)*size_t(format.bytesPerSample))
		throw std::runtime_error("Sound::WAVFile::WAVFile: File has inconsistent fmt chunk");
	
	/* Ignore any additional chunks until the data chunk: */
	size_t dataChunkSize=0;
	while(!file->eof())
		{
		/* Read the chunk's header: */
		char tag[4];
		file->read<char>(tag,4);
		size_t chunkSize=file->read<Misc::UInt32>();
		
		/* Stop if it's a data chunk: */
		if(memcmp(tag,"data",4)==0)
			{
			dataChunkSize=chunkSize;
			break;
			}
		
		/* Skip the chunk: */
		chunkSize=(chunkSize+1)&~0x1; // Pad to two-byte boundary
		file->skip<char>(chunkSize);
		}
	if(dataChunkSize==0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File does not contain a data chunk");
	
	/* Calculate the number of audio frames in the data chunk: */
	numPresetAudioFrames=dataChunkSize/(format.samplesPerFrame*format.bytesPerSample);
	numAudioFrames=numPresetAudioFrames;
	}

WAVFile::WAVFile(IO::FilePtr sFile,const SoundDataFormat& sFormat)
	:file(sFile),
	 format(sFormat),
	 bytesPerFrame(size_t(format.samplesPerFrame)*size_t(format.bytesPerSample)),
	 numPresetAudioFrames(0),numAudioFrames(0)
	{
	/* Check if the file is opened for writing: */
	if(file->getWriteBufferSize()==0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File is not opened for writing");
	
	/* Check if the sound data format is compatible with WAV files: */
	// ...
	
	/* Write a placeholder WAV header: */
	writeWAVHeader(numPresetAudioFrames);
	file->flush();
	}

WAVFile::WAVFile(IO::FilePtr sFile,const SoundDataFormat& sFormat,size_t sNumPresetAudioFrames)
	:file(sFile),
	 format(sFormat),
	 bytesPerFrame(size_t(format.samplesPerFrame)*size_t(format.bytesPerSample)),
	 numPresetAudioFrames(sNumPresetAudioFrames),numAudioFrames(0)
	{
	/* Check if the file is opened for writing: */
	if(file->getWriteBufferSize()==0)
		throw std::runtime_error("Sound::WAVFile::WAVFile: File is not opened for writing");
	
	/* Check if the sound data format is compatible with WAV files: */
	// ...
	
	/* Write a placeholder WAV header: */
	writeWAVHeader(numPresetAudioFrames);
	file->flush();
	}

WAVFile::~WAVFile(void)
	{
	/* Check if the file is opened for writing: */
	if(file->getWriteBufferSize()!=0)
		{
		/* Pad the data chunk if its current size is odd: */
		if((numAudioFrames*bytesPerFrame)&0x1U)
			file->write<char>(0);
		
		/* Check if the actual number of audio frames is different from the number written into the WAV header: */
		if(numAudioFrames!=numPresetAudioFrames)
			{
			/* Check if the WAV file can be rewound: */
			IO::SeekableFilePtr seekableFile(file);
			if(seekableFile!=0)
				{
				seekableFile->setWritePosAbs(0);
				writeWAVHeader(numAudioFrames);
				}
			else
				{
				/* Warn the user that an invalid WAV file was written: */
				Misc::userWarning("Sound::WAVFile::~WAVFile: Invalid WAV file was written; number of audio frames does not match WAV header");
				}
			}
		}
	}

void WAVFile::setBlockSize(size_t numFrames)
	{
	size_t blockSize=numFrames*bytesPerFrame;
	
	/* Adjust the file's buffer sizes: */
	if(file->getReadBufferSize()!=0)
		file->resizeReadBuffer(blockSize);
	if(file->getWriteBufferSize()!=0)
		file->resizeWriteBuffer(blockSize);
	}

void WAVFile::readAudioFrames(void* frames,size_t numFrames)
	{
	/* Read frames based on file's sample data type: */
	switch(format.bytesPerSample)
		{
		case 1:
			file->read(static_cast<Misc::UInt8*>(frames),numFrames*format.samplesPerFrame);
			break;
		
		case 2:
			file->read(static_cast<Misc::SInt16*>(frames),numFrames*format.samplesPerFrame);
			break;
		
		case 4:
			file->read(static_cast<Misc::SInt32*>(frames),numFrames*format.samplesPerFrame);
			break;
		}
	}

namespace {

/****************
Helper functions:
****************/

template <class SampleScalarParam,class AccumScalarParam>
inline
void
downmix(
	IO::File& file,
	int numChannels,
	SampleScalarParam* frames,
	size_t numFrames)
	{
	SampleScalarParam* fEnd=frames+numFrames;
	
	/* Check for the common cases of mono or stereo WAV files: */
	if(numChannels==1)
		{
		/* Read the mono WAV file straight through: */
		file.read(frames,numFrames);
		}
	else if(numChannels==2)
		{
		/* Downmix a stereo WAV file into mono: */
		SampleScalarParam frame[2];
		for(;frames!=fEnd;++frames)
			{
			file.read(frame,2);
			*frames=SampleScalarParam((AccumScalarParam(frame[0])+AccumScalarParam(frame[1])+AccumScalarParam(1))>>1);
			}
		}
	else
		{
		/* Downmix a multi-channel WAV file into mono: */
		SampleScalarParam frame[256];
		AccumScalarParam round(numChannels/2);
		for(;frames!=fEnd;++frames)
			{
			file.read(frame,numChannels);
			AccumScalarParam accum=round;
			for(int c=0;c<numChannels;++c)
				accum+=AccumScalarParam(frame[c]);
			*frames=SampleScalarParam(accum/AccumScalarParam(numChannels));
			}
		}
	}

}

void WAVFile::readMonoAudioFrames(void* frames,size_t numFrames)
	{
	/* Read frames based on file's sample data type and downmix to mono depending on the file's number of channels: */
	switch(format.bytesPerSample)
		{
		case 1:
			
			/* Read and downmix the 8-bit unsigned int WAV file: */
			downmix<Misc::UInt8,Misc::UInt16>(*file,format.samplesPerFrame,static_cast<Misc::UInt8*>(frames),numFrames);
			
			break;
		
		case 2:
			
			/* Read and downmix the 16-bit signed int WAV file: */
			downmix<Misc::SInt16,Misc::SInt32>(*file,format.samplesPerFrame,static_cast<Misc::SInt16*>(frames),numFrames);
			
			break;
		
		case 4:
			
			/* Read and downmix the 32-bit signed int WAV file: */
			downmix<Misc::SInt32,Misc::SInt64>(*file,format.samplesPerFrame,static_cast<Misc::SInt32*>(frames),numFrames);
			
			break;
		}
	}

void WAVFile::writeAudioFrames(const void* frames,size_t numFrames)
	{
	/* Write frames based on file's sample data type: */
	switch(format.bytesPerSample)
		{
		case 1:
			file->write(static_cast<const Misc::UInt8*>(frames),numFrames*format.samplesPerFrame);
			break;
		
		case 2:
			file->write(static_cast<const Misc::SInt16*>(frames),numFrames*format.samplesPerFrame);
			break;
		
		case 4:
			file->write(static_cast<const Misc::SInt32*>(frames),numFrames*format.samplesPerFrame);
			break;
		}
	
	/* Count the total amount of audio frames written: */
	numAudioFrames+=numFrames;
	}

}
