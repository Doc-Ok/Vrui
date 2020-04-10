/***********************************************************************
ReadJPEGImage - Functions to read RGB images from image files in JPEG
formats over an IO::File abstraction.
Copyright (c) 2011-2018 Oliver Kreylos

This file is part of the Image Handling Library (Images).

The Image Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Image Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Image Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Images/ReadJPEGImage.h>

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_JPEG

#include <stdio.h>
#include <jpeglib.h>
#include <jconfig.h>
#include <stdexcept>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>

namespace Images {

namespace {

/**************
Helper classes:
**************/

class JPEGExceptionErrorManager:public jpeg_error_mgr
	{
	/* Private methods: */
	private:
	static void errorExitFunction(j_common_ptr cinfo)
		{
		JPEGExceptionErrorManager* thisPtr=static_cast<JPEGExceptionErrorManager*>(cinfo->err);
		
		/* Throw an exception: */
		Misc::throwStdErr(thisPtr->jpeg_message_table[thisPtr->msg_code],thisPtr->msg_parm.i[0],thisPtr->msg_parm.i[1],thisPtr->msg_parm.i[2],thisPtr->msg_parm.i[3],thisPtr->msg_parm.i[4],thisPtr->msg_parm.i[5],thisPtr->msg_parm.i[6],thisPtr->msg_parm.i[7]);
		}
	
	/* Constructors and destructors: */
	public:
	JPEGExceptionErrorManager(void)
		{
		/* Set the method pointer(s) in the base class object: */
		jpeg_std_error(this);
		error_exit=errorExitFunction;
		}
	};

class JPEGFileSourceManager:public jpeg_source_mgr
	{
	/* Methods: */
	private:
	IO::File& source; // Reference to the source stream
	
	/* Private methods: */
	static void initSourceFunction(j_decompress_ptr cinfo)
		{
		// JPEGFileSourceManager* thisPtr=static_cast<JPEGFileSourceManager*>(cinfo->src);
		}
	static boolean fillInputBufferFunction(j_decompress_ptr cinfo)
		{
		JPEGFileSourceManager* thisPtr=static_cast<JPEGFileSourceManager*>(cinfo->src);
		
		/* Fill the JPEG decoder's input buffer directly from the file's read buffer: */
		void* buffer;
		size_t bufferSize=thisPtr->source.readInBuffer(buffer);
		thisPtr->bytes_in_buffer=bufferSize;
		thisPtr->next_input_byte=static_cast<JOCTET*>(buffer);
		
		/* Return true if all data has been read: */
		return bufferSize!=0;
		}
	static void skipInputDataFunction(j_decompress_ptr cinfo,long count)
		{
		JPEGFileSourceManager* thisPtr=static_cast<JPEGFileSourceManager*>(cinfo->src);
		
		size_t skip=size_t(count);
		if(skip<thisPtr->bytes_in_buffer)
			{
			/* Skip inside the decompressor's read buffer: */
			thisPtr->next_input_byte+=skip;
			thisPtr->bytes_in_buffer-=skip;
			}
		else
			{
			/* Flush the decompressor's read buffer and skip in the source: */
			skip-=thisPtr->bytes_in_buffer;
			thisPtr->bytes_in_buffer=0;
			thisPtr->source.skip<JOCTET>(skip);
			}
		}
	static void termSourceFunction(j_decompress_ptr cinfo)
		{
		// JPEGFileSourceManager* thisPtr=static_cast<JPEGFileSourceManager*>(cinfo->src);
		}
	
	/* Constructors and destructors: */
	public:
	JPEGFileSourceManager(IO::File& sSource)
		:source(sSource)
		{
		/* Install the hook functions: */
		init_source=initSourceFunction;
		fill_input_buffer=fillInputBufferFunction;
		skip_input_data=skipInputDataFunction;
		resync_to_restart=jpeg_resync_to_restart; // Use default function
		term_source=termSourceFunction;
		
		/* Clear the input buffer: */
		bytes_in_buffer=0;
		next_input_byte=0;
		}
	~JPEGFileSourceManager(void)
		{
		}
	};

class JPEGReader:public jpeg_decompress_struct
	{
	/* Elements: */
	private:
	JPEGExceptionErrorManager exceptionManager;
	JPEGFileSourceManager fileSourceManager;
	JSAMPLE** rowPointers;
	public:
	unsigned int imageSize[2];
	unsigned int numChannels;
	
	/* Constructors and destructors: */
	public:
	JPEGReader(IO::File& source)
		:fileSourceManager(source),
		 rowPointers(0)
		{
		/* Throw an exception if the JPEG library produces anything but 8-bit samples: */
		#if BITS_IN_JSAMPLE!=8
		throw std::runtime_error("Images::JPEGReader: Unsupported bit depth in JPEG library");
		#endif
		
		/* Initialize the JPEG library parts of this object: */
		err=&exceptionManager;
		client_data=0;
		jpeg_create_decompress(this);
		src=&fileSourceManager;
		
		/* Read the JPEG file header: */
		jpeg_read_header(this,TRUE);
		}
	~JPEGReader(void)
		{
		/* Clean up: */
		delete[] rowPointers;
		jpeg_destroy_decompress(this);
		}
	
	/* Methods: */
	void setupProcessing(bool forceRgb)
		{
		if(forceRgb||out_color_space!=JCS_GRAYSCALE)
			{
			/* Force output color space to RGB: */
			out_color_space=JCS_RGB;
			numChannels=3;
			}
		else
			numChannels=1;
		
		/* Prepare for decompression: */
		jpeg_start_decompress(this);
		imageSize[0]=output_width;
		imageSize[1]=output_height;
		}
	void readImage(JSAMPLE* pixels,ptrdiff_t rowStride)
		{
		/* Create row pointers to flip the image during reading: */
		rowPointers=new JSAMPLE*[imageSize[1]];
		rowPointers[0]=pixels+(imageSize[1]-1)*rowStride;
		for(unsigned int y=1;y<imageSize[1];++y)
			rowPointers[y]=rowPointers[y-1]-rowStride;
		
		/* Read the JPEG image's scan lines: */
		JDIMENSION scanline=0;
		while(scanline<output_height)
			scanline+=jpeg_read_scanlines(this,rowPointers+scanline,output_height-scanline);
		
		/* Finish reading image: */
		jpeg_finish_decompress(this);
		}
	};

}

RGBImage readJPEGImage(IO::File& source)
	{
	/* Create a JPEG image reader for the given source file: */
	JPEGReader reader(source);
	
	/* Set up image processing: */
	reader.setupProcessing(true);
	
	/* Create the result image: */
	RGBImage result(reader.imageSize[0],reader.imageSize[1]);
	
	/* Read the JPEG image: */
	reader.readImage(reinterpret_cast<JSAMPLE*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

BaseImage readGenericJPEGImage(IO::File& source)
	{
	/* Create a JPEG image reader for the given source file: */
	JPEGReader reader(source);
	
	/* Set up image processing: */
	reader.setupProcessing(false);
	
	/* Create the result image: */
	BaseImage result(reader.imageSize[0],reader.imageSize[1],reader.numChannels,1,reader.numChannels==3?GL_RGB:GL_LUMINANCE,GL_UNSIGNED_BYTE);
	
	/* Read the JPEG image: */
	reader.readImage(reinterpret_cast<JSAMPLE*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

}

#endif
