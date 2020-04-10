/***********************************************************************
TIFFReader - Helper class for low-level access to image files in TIFF
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

#ifndef IMAGES_TIFFREADER_INCLUDED
#define IMAGES_TIFFREADER_INCLUDED

#include <Images/Config.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <stddef.h>
#include <tiffio.h>
#include <IO/SeekableFile.h>

/* Forward declarations: */
namespace IO {
class File;
}
namespace Images {
struct GeoTIFFMetadata;
}
namespace Images {

class TIFFReader // Class to read the first image from a TIFF image file
	{
	/* Embedded classes: */
	public:
	enum ColorSpace // Enumerated type for color spaces supported by TIFF
		{
		WhiteIsZero=0,BlackIsZero,RGB,
		TransparencyMask=4,CMYK,YCbCr,
		CIE_Lab=8,ICC_Lab,ITU_Lab,
		Invalid
		};
	typedef void (*PixelStreamingCallback)(uint32 x,uint32 y,uint32 width,uint16 channel,const uint8* pixels,void* userData); // Type of callback function receiving a row of samples for the given channel in streaming mode; if channel==uint16(-1), pixel channels are interleaved
	
	/* Elements: */
	private:
	TIFF* tiff; // The TIFF library object
	IO::SeekableFilePtr seekableSource; // A seekable version of the source file
	uint32 width,height; // Image width and height in pixels
	uint16 numBits,numSamples,sampleFormat; // Number of bits per sample, samples per pixel, and sample format
	bool indexed; // Flag if pixel values are indices into a color map
	ColorSpace colorSpace; // Color space to interpret pixel values
	bool planar; // Flag whether sample data is stored separately by sample, or interleaved by pixel
	bool tiled; // Flag whether image data is organized in tiles instead of in strips
	uint32 rowsPerStrip; // Number of rows per image strip if image data is organized in strips
	uint32 tileWidth,tileHeight; // Width and height of an image tile if image data is organized in tiles
	
	/* Private methods: */
	static void tiffErrorFunction(const char* module,const char* fmt,va_list ap);
	static void tiffWarningFunction(const char* module,const char* fmt,va_list ap);
	static tsize_t tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size);
	static tsize_t tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size);
	static toff_t tiffSeekFunction(thandle_t handle,toff_t offset,int whence);
	static int tiffCloseFunction(thandle_t handle);
	static toff_t tiffSizeFunction(thandle_t handle);
	static int tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size);
	static void tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size);
	
	/* Constructors and destructors: */
	public:
	TIFFReader(IO::File& source,unsigned int imageIndex =0); // Creates a TIFF reader for the given file source and the image of the given index inside the file
	~TIFFReader(void); // Destroys the TIFF reader
	
	/* Methods: */
	uint32 getWidth(void) const // Returns the image's width in pixels
		{
		return width;
		}
	uint32 getHeight(void) const // Returns the image's height in pixels
		{
		return height;
		}
	uint16 getNumBits(void) const // Returns the number of bits per image sample
		{
		return numBits;
		}
	uint16 getNumSamples(void) const // Returns the number of samples per image pixel
		{
		return numSamples;
		}
	uint16 getSampleFormat(void) const // Returns the sample format as a libtiff enumerant
		{
		return sampleFormat;
		}
	bool hasUnsignedIntSamples(void) const // Returns true if the image's sample format is unsigned integer
		{
		return sampleFormat==SAMPLEFORMAT_UINT;
		}
	bool hasSignedIntSamples(void) const // Returns true if the image's sample format is signed integer
		{
		return sampleFormat==SAMPLEFORMAT_INT;
		}
	bool hasFloatSamples(void) const // Returns true if the image's sample format is IEEE floating-point
		{
		return sampleFormat==SAMPLEFORMAT_IEEEFP;
		}
	bool isIndexed(void) const // Returns true if pixel values are color map indices
		{
		return indexed;
		}
	ColorSpace getColorSpace(void) const // Returns the image's color space
		{
		return colorSpace;
		}
	bool isPlanar(void) const // Returns true if pixel data is organized in planes of samples or interleaved per pixel
		{
		return planar;
		}
	bool isTiled(void) const // Returns true if image data is organized in tiles
		{
		return tiled;
		}
	uint32 getRowsPerStrip(void) const // Returns the number of image rows per image strip
		{
		return rowsPerStrip;
		}
	uint32 getTileWidth(void) const // Returns the width of an image tile
		{
		return tileWidth;
		}
	uint32 getTileHeight(void) const // Returns the height of an image tile
		{
		return tileHeight;
		}
	bool getColorMap(uint16*& red,uint16*& green,uint16*& blue); // Puts pointers to the TIFF file's color map into the provided pointers; returns true if TIFF file does contain a color map; nulls pointers if no color map present
	bool getCMYKColorMap(uint16*& cyan,uint16*& magenta,uint16*& yellow,uint16*& black); // Puts pointers to the TIFF file's color map into the provided pointers if the file's color space is CMYK; returns true if TIFF file does contain a color map; nulls pointers if no color map present
	void readMetadata(GeoTIFFMetadata& metadata); // Reads GeoTIFF metadata associated with the image file, if it has any
	void readRgba(uint32* rgbaBuffer); // Reads a complete TIFF image into a 32-bit RGBA buffer
	void readStrips(uint8* image,ptrdiff_t rowStride); // Reads a stripped TIFF image into a channel-interleaved image of appropriate size
	uint8* createTileBuffer(void); // Returns a new-allocated buffer for temporary storage when reading a single TIFF image tile
	void readTile(uint32 tileIndexX,uint32 tileIndexY,uint8* tileBuffer,uint8* image,ptrdiff_t rowStride); // Reads a single TIFF image tile into a channel-interleaved image of appropriate size; tile buffer must have been created using createTileBuffer method
	void readTiles(uint8* image,ptrdiff_t rowStride); // Reads a tiled TIFF image into a channel-interleaved image of appropriate size
	void readImage(uint8* image,ptrdiff_t rowStride) // Reads a stripped or tiled TIFF image into a channel-interleaved image of appropriate size
		{
		/* Check if the TIFF image is tiled: */
		if(tiled)
			{
			/* Read the image in tiles: */
			readTiles(image,rowStride);
			}
		else
			{
			/* Read the image in strips: */
			readStrips(image,rowStride);
			}
		}
	void streamStrips(PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData); // Streams image pixels strip-by-strip to a callback function
	void streamTiles(PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData); // Streams image pixels tile-by-tile to a callback function
	void streamImage(PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData) // Streams image pixels to a callback function
		{
		/* Check if the TIFF image is tiled: */
		if(tiled)
			{
			/* Stream the image by tiles: */
			streamTiles(pixelStreamingCallback,pixelStreamingUserData);
			}
		else
			{
			/* Stream the image by strips: */
			streamStrips(pixelStreamingCallback,pixelStreamingUserData);
			}
		}
	};

}

#endif

#endif
