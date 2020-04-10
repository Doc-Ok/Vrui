/***********************************************************************
ReadTIFFImage - Functions to read RGB images from image files in TIFF
formats over an IO::SeekableFile abstraction.
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

#include <Images/ReadTIFFImage.h>

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <tiffio.h>
#include <stdexcept>
#include <Misc/SelfDestructArray.h>
#include <Misc/ThrowStdErr.h>
#include <GL/gl.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/TIFFReader.h>

namespace Images {

RGBImage readTIFFImage(IO::File& source)
	{
	/* Create a TIFF image reader for the given source file: */
	TIFFReader reader(source);
	
	/* Create the result image: */
	RGBImage result(reader.getWidth(),reader.getHeight());
	
	/* Read the TIFF image into a temporary RGBA buffer: */
	Misc::SelfDestructArray<uint32> rgbaBuffer(reader.getHeight()*reader.getWidth());
	reader.readRgba(rgbaBuffer);
	
	/* Copy the RGB image data into the result image: */
	uint32* sPtr=rgbaBuffer;
	RGBImage::Color* dPtr=result.replacePixels();
	for(uint32 y=0;y<reader.getHeight();++y)
		for(uint32 x=0;x<reader.getWidth();++x,++sPtr,++dPtr)
			{
			(*dPtr)[0]=RGBImage::Color::Scalar(TIFFGetR(*sPtr));
			(*dPtr)[1]=RGBImage::Color::Scalar(TIFFGetG(*sPtr));
			(*dPtr)[2]=RGBImage::Color::Scalar(TIFFGetB(*sPtr));
			}
	
	return result;
	}

RGBAImage readTransparentTIFFImage(IO::File& source)
	{
	/* Create a TIFF image reader for the given source file: */
	TIFFReader reader(source);
	
	/* Create the result image: */
	RGBAImage result(reader.getWidth(),reader.getHeight());
	
	/* Read the TIFF image into a temporary RGBA buffer: */
	Misc::SelfDestructArray<uint32> rgbaBuffer(reader.getHeight()*reader.getWidth());
	reader.readRgba(rgbaBuffer);
	
	/* Copy the RGB image data into the result image: */
	uint32* sPtr=rgbaBuffer;
	RGBAImage::Color* dPtr=result.replacePixels();
	for(uint32 y=0;y<reader.getHeight();++y)
		for(uint32 x=0;x<reader.getWidth();++x,++sPtr,++dPtr)
			{
			(*dPtr)[0]=RGBAImage::Color::Scalar(TIFFGetR(*sPtr));
			(*dPtr)[1]=RGBAImage::Color::Scalar(TIFFGetG(*sPtr));
			(*dPtr)[2]=RGBAImage::Color::Scalar(TIFFGetB(*sPtr));
			(*dPtr)[3]=RGBAImage::Color::Scalar(TIFFGetA(*sPtr));
			}
	
	return result;
	}

BaseImage readGenericTIFFImage(IO::File& source,GeoTIFFMetadata* metadata)
	{
	/* Create a TIFF image reader for the given source file: */
	TIFFReader reader(source);
	
	/* Check if the caller wants to retrieve metadata: */
	if(metadata!=0)
		{
		/* Extract GeoTIFF metadata from the TIFF file: */
		reader.readMetadata(*metadata);
		}
	
	/* Determine the result image's pixel format: */
	GLenum format=GL_RGB;
	switch(reader.getNumSamples())
		{
		case 1:
			format=GL_LUMINANCE;
			break;
		
		case 2:
			format=GL_LUMINANCE_ALPHA;
			break;
		
		case 3:
			format=GL_RGB;
			break;
		
		case 4:
			format=GL_RGBA;
			break;
		
		default:
			Misc::throwStdErr("Images::readGenericTIFFImage: Unsupported number %u of channels",(unsigned int)(reader.getNumSamples()));
		}
	GLenum scalarType=GL_UNSIGNED_BYTE;
	switch(reader.getNumBits())
		{
		case 8:
			if(reader.hasUnsignedIntSamples())
				scalarType=GL_UNSIGNED_BYTE;
			else if(reader.hasSignedIntSamples())
				scalarType=GL_BYTE;
			else
				throw std::runtime_error("Images::readGenericTIFFImage: Unsupported 8-bit sample format");
			break;
		
		case 16:
			if(reader.hasUnsignedIntSamples())
				scalarType=GL_UNSIGNED_SHORT;
			else if(reader.hasSignedIntSamples())
				scalarType=GL_SHORT;
			else
				throw std::runtime_error("Images::readGenericTIFFImage: Unsupported 16-bit sample format");
			break;
		
		case 32:
			if(reader.hasUnsignedIntSamples())
				scalarType=GL_UNSIGNED_INT;
			else if(reader.hasSignedIntSamples())
				scalarType=GL_INT;
			else if(reader.hasFloatSamples())
				scalarType=GL_FLOAT;
			else
				throw std::runtime_error("Images::readGenericTIFFImage: Unsupported 32-bit sample format");
			break;
		
		default:
			Misc::throwStdErr("Images::readGenericTIFFImage: Unsupported bit depth %u",(unsigned int)(reader.getNumBits()));
		}
	
	/* Create the result image and read the TIFF image: */
	BaseImage result(reader.getWidth(),reader.getHeight(),reader.getNumSamples(),(reader.getNumBits()+7)/8,format,scalarType);
	reader.readImage(static_cast<uint8*>(result.replacePixels()),result.getRowStride());
	
	return result;
	}

}

#endif
