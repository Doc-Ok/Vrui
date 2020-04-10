/***********************************************************************
ReadPNGImage - Functions to read RGB or RGBA images from image files in
PNG formats over an IO::File abstraction.
Copyright (c) 2011-2019 Oliver Kreylos

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

#include <Images/ReadPNGImage.h>

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_PNG

#include <png.h>
#include <stdexcept>
#include <Misc/Endianness.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>

namespace Images {

namespace {

/**************
Helper classes:
**************/

class PNGReader
	{
	/* Elements: */
	private:
	IO::File& source; // Handle for the PNG image file
	png_structp pngReadStruct; // Structure representing state of an open PNG image file inside the PNG library
	png_infop pngInfoStruct; // Structure containing information about the image in an open PNG image file
	png_byte** rowPointers; // Array of row pointers to vertically flip an image during reading
	public:
	png_uint_32 imageSize[2];
	int elementSize;
	int colorType;
	unsigned int numChannels;
	
	/* Private methods: */
	static void readDataFunction(png_structp pngReadStruct,png_bytep buffer,png_size_t size) // Called by the PNG library to read additional data from the source
		{
		/* Get the pointer to the IO::File object: */
		IO::File* source=static_cast<IO::File*>(png_get_io_ptr(pngReadStruct));
		
		/* Read the requested number of bytes from the source, and let the source handle errors: */
		source->read(buffer,size);
		}
	static void errorFunction(png_structp pngReadStruct,png_const_charp errorMsg) // Called by the PNG library to report an error
		{
		/* Throw an exception: */
		throw std::runtime_error(errorMsg);
		}
	static void warningFunction(png_structp pngReadStruct,png_const_charp warningMsg) // Called by the PNG library to report a recoverable error
		{
		/* Show warning to user: */
		Misc::consoleWarning(warningMsg);
		}
	
	/* Constructors and destructors: */
	public:
	PNGReader(IO::File& sSource)
		:source(sSource),
		 pngReadStruct(0),pngInfoStruct(0),rowPointers(0)
		{
		/* Check for PNG file signature: */
		unsigned char pngSignature[8];
		source.read(pngSignature,8);
		if(!png_check_sig(pngSignature,8))
			throw std::runtime_error("Images::PNGReader: illegal PNG header");
		
		/* Allocate the PNG library data structures: */
		if((pngReadStruct=png_create_read_struct(PNG_LIBPNG_VER_STRING,0,errorFunction,warningFunction))==0
		   ||(pngInfoStruct=png_create_info_struct(pngReadStruct))==0)
		  {
		  /* Clean up and throw an exception: */
			png_destroy_read_struct(&pngReadStruct,0,0);
			throw std::runtime_error("Images::PNGReader: Internal error in PNG library");
			}
		
		/* Initialize PNG I/O to read from the supplied data source: */
		png_set_read_fn(pngReadStruct,&source,readDataFunction);
		
		/* Tell the PNG library that the signature has been read: */
		png_set_sig_bytes(pngReadStruct,8);
		}
	~PNGReader(void)
		{
		/* Delete the image row pointers: */
		delete[] rowPointers;
		
		/* Destroy the PNG library structures: */
		png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
		}
	
	/* Methods: */
	void readHeader(void) // Reads a PNG image header from the source file
		{
		/* Read PNG image header: */
		png_read_info(pngReadStruct,pngInfoStruct);
		png_get_IHDR(pngReadStruct,pngInfoStruct,&imageSize[0],&imageSize[1],&elementSize,&colorType,0,0,0);
		
		/* Create the row pointers array: */
		rowPointers=new png_byte*[imageSize[1]];
		}
	void setupProcessing(bool forceRGB,bool force8Bit,bool stripAlpha,bool forceAlpha)
		{
		/* Determine image format and set up image processing: */
		bool isGray=colorType==PNG_COLOR_TYPE_GRAY||colorType==PNG_COLOR_TYPE_GRAY_ALPHA;
		bool haveAlpha=colorType==PNG_COLOR_TYPE_GRAY_ALPHA||colorType==PNG_COLOR_TYPE_RGB_ALPHA;
		if(colorType==PNG_COLOR_TYPE_PALETTE)
			{
			/* Expand paletted images to RGB: */
			png_set_palette_to_rgb(pngReadStruct);
			}
		if(isGray&&elementSize<8)
			{
			/* Expand bitmaps to 8-bit grayscale: */
			png_set_expand_gray_1_2_4_to_8(pngReadStruct);
			}
		if(forceRGB&&isGray)
			{
			/* Convert grayscale pixels to RGB: */
			png_set_gray_to_rgb(pngReadStruct);
			}
		if(force8Bit&&elementSize==16)
			{
			/* Reduce 16-bit pixels to 8-bit: */
			png_set_strip_16(pngReadStruct);
			}
		#if __BYTE_ORDER==__LITTLE_ENDIAN
		if(!force8Bit&&elementSize==16)
			{
			/* Swap 16-bit pixels from big endian network order to little endian host order: */
			png_set_swap(pngReadStruct);
			}
		#endif
		if(stripAlpha&&haveAlpha)
			{
			/* Strip the alpha channel: */
			png_set_strip_alpha(pngReadStruct);
			}
		if(!stripAlpha&&png_get_valid(pngReadStruct,pngInfoStruct,PNG_INFO_tRNS))
			{
			/* Create a full alpha channel from tRNS chunk: */
			png_set_tRNS_to_alpha(pngReadStruct);
			haveAlpha=true;
			}
		if(forceAlpha&&!haveAlpha)
			{
			/* Create a dummy alpha channel: */
			png_set_filler(pngReadStruct,0xff,PNG_FILLER_AFTER);
			}
		
		/* Apply the image's stored gamma curve: */
		double gamma;
		if(png_get_gAMA(pngReadStruct,pngInfoStruct,&gamma))
			png_set_gamma(pngReadStruct,2.2,gamma);
		
		/* Update the PNG processor and retrieve the potentially changed image format: */
		png_read_update_info(pngReadStruct,pngInfoStruct);
		numChannels=png_get_channels(pngReadStruct,pngInfoStruct);
		elementSize=png_get_bit_depth(pngReadStruct,pngInfoStruct);
		}
	void readImage(png_byte* pixels,ptrdiff_t rowStride) // Reads image into the given pixel buffer of appropriate size
		{
		/* Set up the row pointers array: */
		rowPointers[0]=pixels+(imageSize[1]-1)*rowStride;
		for(png_uint_32 y=1;y<imageSize[1];++y)
			rowPointers[y]=rowPointers[y-1]-rowStride;
		
		/* Read the PNG image: */
		png_read_image(pngReadStruct,rowPointers);
		
		/* Finish reading the image: */
		png_read_end(pngReadStruct,0);
		}
	};

}

RGBImage readPNGImage(IO::File& source)
	{
	/* Create a PNG image reader for the given source file: */
	PNGReader reader(source);
	
	/* Read a PNG header: */
	reader.readHeader();
	
	/* Set up image processing: */
	reader.setupProcessing(true,true,true,false);
	
	/* Create the result image: */
	RGBImage result(reader.imageSize[0],reader.imageSize[1]);
	
	/* Read the PNG image: */
	reader.readImage(reinterpret_cast<png_byte*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

RGBAImage readTransparentPNGImage(IO::File& source)
	{
	/* Create a PNG image reader for the given source file: */
	PNGReader reader(source);
	
	/* Read a PNG header: */
	reader.readHeader();
	
	/* Set up image processing: */
	reader.setupProcessing(true,true,false,true);
	
	/* Create the result image: */
	RGBAImage result(reader.imageSize[0],reader.imageSize[1]);
	
	/* Read the PNG image: */
	reader.readImage(reinterpret_cast<png_byte*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

BaseImage readGenericPNGImage(IO::File& source)
	{
	/* Create a PNG image reader for the given source file: */
	PNGReader reader(source);
	
	/* Read a PNG header: */
	reader.readHeader();
	
	/* Set up image processing: */
	reader.setupProcessing(false,false,false,false);
	
	/* Determine the image's texture format: */
	static const GLenum formats[4]={GL_LUMINANCE,GL_LUMINANCE_ALPHA,GL_RGB,GL_RGBA};
	GLenum format=formats[reader.numChannels-1];
	GLenum channelType=reader.elementSize==16?GL_UNSIGNED_SHORT:GL_UNSIGNED_BYTE;
	
	/* Create the result image: */
	BaseImage result(reader.imageSize[0],reader.imageSize[1],reader.numChannels,(reader.elementSize+7)/8,format,channelType);
	
	/* Read the PNG image: */
	reader.readImage(reinterpret_cast<png_byte*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

}

#endif
