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

#ifndef SOUND_WAVFILE_INCLUDED
#define SOUND_WAVFILE_INCLUDED

#include <stddef.h>
#include <IO/File.h>
#include <Sound/SoundDataFormat.h>

namespace Sound {

class WAVFile
	{
	/* Elements: */
	private:
	IO::FilePtr file; // The underlying file object, which must be seekable if the file is opened for writing and the number of audio frames is not known a-priori
	SoundDataFormat format; // Sound data format extracted from a WAV file opened for reading, or configured for a WAV file opened for writing
	size_t bytesPerFrame; // Number of bytes per audio frame
	size_t numPresetAudioFrames; // Number of audio frames preset in a non-seekable write-only file
	size_t numAudioFrames; // Number of audio frames currently in the file, for reading and writing
	
	/* Private methods: */
	void writeWAVHeader(size_t numAudioFrames); // Writes a temporary or final WAV file header to the file
	
	/* Constructors and destructors: */
	public:
	WAVFile(IO::FilePtr sFile); // Creates a read-only WAV file representation for the given underlying file object, which must be opened for reading
	WAVFile(IO::FilePtr sFile,const SoundDataFormat& sFormat); // Creates a write-only WAV file representation for the given underlying file object, which must be seekable and opened for writing
	WAVFile(IO::FilePtr sFile,const SoundDataFormat& sFormat,size_t sNumPresetAudioFrames); // Creates a write-only WAV file representation for the given underlying file object and the known number of audio frames
	~WAVFile(void); // Finishes writing a WAV file opened for writing and closes the file
	
	/* Methods: */
	const SoundDataFormat& getFormat(void) const // Returns the WAV file's sound data format
		{
		return format;
		}
	size_t getNumAudioFrames(void) const // Returns the number of audio frames currently in the file, both for read-only and write-only WAV files
		{
		return numAudioFrames;
		}
	void setBlockSize(size_t numFrames); // If audio data is read or written in fixed-size blocks, tries adjusting underlying file's buffers so that reads or writes bypass the file's buffers
	void readAudioFrames(void* frames,size_t numFrames); // Reads a block of audio frames from the file into the given buffer
	void readMonoAudioFrames(void* frames,size_t numFrames); // Reads a block of audio frames from the file and downmixes it to mono into the given buffer
	void writeAudioFrames(const void* frames,size_t numFrames); // Writes a block of audio frames from the given buffer to the file
	};

}

#endif
