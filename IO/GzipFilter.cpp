/***********************************************************************
GzipFilter - Class for read/write access to gzip-compressed files using
a IO::File abstraction.
Copyright (c) 2011-2019 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/GzipFilter.h>

#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>

namespace IO {

/***************************
Methods of class GzipFilter:
***************************/

size_t GzipFilter::readData(File::Byte* buffer,size_t bufferSize)
	{
	/* Check for end-of-file: */
	if(readEof)
		return 0;
	
	/* Decompress data into the given buffer: */
	stream.next_out=buffer;
	stream.avail_out=bufferSize;
	
	/* Try until at least some output is produced: */
	do
		{
		/* Check if the decompressor needs more input: */
		if(stream.avail_in==0)
			{
			/* Read the next glob of compressed data: */
			void* compressedBuffer;
			size_t compressedSize=gzippedFile->readInBuffer(compressedBuffer);
			
			/* Pass the compressed data to the decompressor: */
			stream.next_in=static_cast<Bytef*>(compressedBuffer);
			stream.avail_in=compressedSize;
			}
		
		/* Decompress from the gzipped file's buffer: */
		int result=inflate(&stream,Z_NO_FLUSH);
		if(result==Z_STREAM_END)
			{
			/* Set the eof flag and clean out the decompressor: */
			readEof=true;
			if(inflateEnd(&stream)!=Z_OK)
				{
				if(stream.msg!=0)
					{
					char buffer[512];
					throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" after decompression",stream.msg));
					}
				else
					throw Error("IO::GzipFilter: Data corruption detected after decompression");
				}
			break;
			}
		else if(result!=Z_OK)
			{
			if(stream.msg!=0)
				{
				char buffer[512];
				throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" while decompressing",stream.msg));
				}
			else
				throw Error("IO::GzipFilter: Internal zlib error while decompressing");
			}
		}
	while(stream.avail_out==bufferSize);
	
	return bufferSize-stream.avail_out;
	}

void GzipFilter::writeData(const File::Byte* buffer,size_t bufferSize)
	{
	/* Set the buffer's content as the compressor's input: */
	stream.next_in=const_cast<Bytef*>(buffer);
	stream.avail_in=bufferSize;
	
	/* We must completely clear out the write buffer: */
	while(stream.avail_in>0)
		{
		/* Set up the compressor's outpuf buffer: */
		void* outputBuffer;
		size_t outputSize=gzippedFile->writeInBufferPrepare(outputBuffer);
		stream.next_out=static_cast<Bytef*>(outputBuffer);
		stream.avail_out=outputSize;
		
		/* Compress into the gzipped file's buffer: */
		if(deflate(&stream,Z_NO_FLUSH)!=Z_OK)
			{
			if(stream.msg!=0)
				{
				char buffer[512];
				throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" while compressing",stream.msg));
				}
			else
				throw Error("IO::GzipFilter: Internal zlib error while compressing");
			}
		gzippedFile->writeInBufferFinish(outputSize-stream.avail_out);
		}
	}

size_t GzipFilter::writeDataUpTo(const File::Byte* buffer,size_t bufferSize)
	{
	/* Calculate the gzipped file's write buffer fill ratio: */
	size_t compressedBufferSpace=gzippedFile->getWriteBufferSpace();
	
	/* Write to the gzipped file first if its write buffer is at least half full: */
	bool writeFirst=compressedBufferSpace*2>=gzippedFile->getWriteBufferSize();
	if(writeFirst)
		gzippedFile->writeSomeData();
	
	/* Set the buffer's content as the compressor's input: */
	stream.next_in=const_cast<Bytef*>(buffer);
	stream.avail_in=bufferSize;
	
	/* Set the gzipped file's write buffer as the compressor's output: */
	void* outputBuffer;
	size_t outputSize=gzippedFile->writeInBufferPrepare(outputBuffer); // This will not trigger a buffer flush, as the buffer cannot be full at this point
	stream.next_out=static_cast<Bytef*>(outputBuffer);
	stream.avail_out=outputSize;
	
	/* Compress one chunk of data from the input buffer into the gzipped file's write buffer: */
	if(deflate(&stream,Z_NO_FLUSH)!=Z_OK)
		{
		if(stream.msg!=0)
			{
			char buffer[512];
			throw Error(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" while compressing",stream.msg));
			}
		else
			throw Error("IO::GzipFilter: Internal zlib error while compressing");
		}
	gzippedFile->writeInBufferFinish(outputSize-stream.avail_out);
	
	/* Write second if the gzipped file's write buffer was less than half full: */
	if(!writeFirst)
		gzippedFile->writeSomeData();
	
	/* Return the number of bytes that were read by the compressor: */
	return bufferSize-stream.avail_in;
	}

void GzipFilter::init(void)
	{
	/* Adopt the compressed file's write mode: */
	bool canRead=gzippedFile->getReadBufferSize()!=0;
	bool canWrite=gzippedFile->getWriteBufferSize()!=0;
	if(canRead&&canWrite)
		throw Error("IO::GzipFilter: Cannot read and write from/to gzipped file simultaneously");
	else if(canRead)
		{
		/* Install an output buffer for uncompressed data: */
		resizeReadBuffer(gzippedFile->getReadBufferSize()*2);
		
		/* Initialize the zlib stream object: */
		stream.next_in=Z_NULL;
		stream.avail_in=0;
		stream.zalloc=Z_NULL;
		stream.zfree=Z_NULL;
		stream.opaque=0;
		if(inflateInit2(&stream,15+32)!=Z_OK) // Detect zlib and gzip headers
			{
			if(stream.msg!=0)
				{
				char buffer[512];
				throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" during initialization",stream.msg));
				}
			else
				throw OpenError("IO::GzipFilter: Internal zlib error during initialization");
			}
		
		/* Read the compressed data header to determine if the file really is zlib- or gzip-compressed: */
		while(stream.avail_in==0)
			{
			/* Read the next glob of compressed data: */
			void* compressedBuffer;
			size_t compressedSize=gzippedFile->readInBuffer(compressedBuffer);
			stream.next_in=static_cast<Bytef*>(compressedBuffer);
			stream.avail_in=compressedSize;
			
			/* Need to assign an output buffer, even though no output will be produced: */
			Bytef outBuffer[1];
			stream.next_out=outBuffer;
			stream.avail_out=1;
			
			/* Try processing the header: */
			int result=inflate(&stream,Z_BLOCK);
			if(result==Z_STREAM_END)
				break;
			else if(result!=Z_OK)
				throw OpenError("IO::GzipFilter: File is not gzip-compressed");
			}
		}
	else if(canWrite)
		{
		/* Install an input buffer for uncompressed data: */
		resizeWriteBuffer(gzippedFile->getWriteBufferSize()*2);
		
		/* Initialize the zlib stream object: */
		stream.next_in=Z_NULL;
		stream.avail_in=0;
		stream.zalloc=Z_NULL;
		stream.zfree=Z_NULL;
		stream.opaque=0;
		if(deflateInit2(&stream,Z_DEFAULT_COMPRESSION,Z_DEFLATED,15+16,8,Z_DEFAULT_STRATEGY)!=Z_OK)
			{
			if(stream.msg!=0)
				{
				char buffer[512];
				throw OpenError(Misc::printStdErrMsgReentrant(buffer,sizeof(buffer),"IO::GzipFilter: Error \"%s\" during initialization",stream.msg));
				}
			else
				throw OpenError("IO::GzipFilter: Internal zlib error during initialization");
			}
		}
	}

GzipFilter::GzipFilter(FilePtr sGzippedFile)
	:File(),
	 gzippedFile(sGzippedFile),
	 readEof(false)
	{
	init();
	}

GzipFilter::GzipFilter(const char* gzippedFileName,File::AccessMode sAccessMode)
	:File(),
	 gzippedFile(IO::openFile(gzippedFileName,sAccessMode)),
	 readEof(false)
	{
	init();
	}

GzipFilter::~GzipFilter(void)
	{
	/* Clean out the compressor/decompressor: */
	if(getReadBufferSize()!=0&&!readEof)
		inflateEnd(&stream);
	if(getWriteBufferSize()!=0)
		{
		/* Flush the write buffer: */
		flush();
		
		/* Continue compressing until the compressor says it's done: */
		int result=Z_OK;
		while(result!=Z_STREAM_END)
			{
			/* Set up the compressor's outpuf buffer: */
			void* outputBuffer;
			size_t outputSize=gzippedFile->writeInBufferPrepare(outputBuffer);
			stream.next_out=static_cast<Bytef*>(outputBuffer);
			stream.avail_out=outputSize;
			
			/* Compress into the gzipped file's buffer: */
			result=deflate(&stream,Z_FINISH);
			if(result!=Z_STREAM_END&&result!=Z_OK)
				{
				/* Print an error message and bail out: */
				if(stream.msg!=0)
					Misc::formattedUserError("IO::GzipFilter: Error \"%s\" while compressing",stream.msg);
				else
					Misc::userError("IO::GzipFilter: Internal zlib error while compressing");
				
				break;
				}
			gzippedFile->writeInBufferFinish(outputSize-stream.avail_out);
			}
		
		/* Clean out the compressor: */
		deflateEnd(&stream);
		}
	}

int GzipFilter::getFd(void) const
	{
	/* Return the gzipped file's file descriptor: */
	return gzippedFile->getFd();
	}

}
