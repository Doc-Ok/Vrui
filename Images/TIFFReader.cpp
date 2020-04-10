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

#include <Images/TIFFReader.h>

#if IMAGES_CONFIG_HAVE_TIFF

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>
#include <Misc/SelfDestructArray.h>
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/SeekableFilter.h>
#include <Math/Math.h>
#include <Images/GeoTIFF.h>
#include <Images/GeoTIFFMetadata.h>

namespace Images {

namespace {

/**************
Helper classes:
**************/

class GeoTIFFExtender // Helper class to hook a tag extender for GeoTIFF tags into the TIFF library upon program loading
	{
	/* Elements: */
	private:
	static GeoTIFFExtender theTiffExtender; // Static TIFF tag extender object
	static TIFFExtendProc parentTagExtender; // Pointer to a previously-registered tag extender
	
	/* Private methods: */
	static void tagExtender(TIFF* tiff) // The tag extender function
		{
		/* Define the GeoTIFF and related GDAL tags: */
		static const TIFFFieldInfo geotiffFieldInfo[]=
			{
			{TIFFTAG_GEOPIXELSCALE,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoPixelScale"},
			{TIFFTAG_GEOTRANSMATRIX,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoTransformationMatrix"},
			{TIFFTAG_GEOTIEPOINTS,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoTiePoints"},
			{TIFFTAG_GEOKEYDIRECTORY,-1,-1,TIFF_SHORT,FIELD_CUSTOM,true,true,(char*)"GeoKeyDirectory"},
			{TIFFTAG_GEODOUBLEPARAMS,-1,-1,TIFF_DOUBLE,FIELD_CUSTOM,true,true,(char*)"GeoDoubleParams"},
			{TIFFTAG_GEOASCIIPARAMS,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GeoASCIIParams"},
			{TIFFTAG_GDAL_METADATA,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GDALMetadataValue"},
			{TIFFTAG_GDAL_NODATA,-1,-1,TIFF_ASCII,FIELD_CUSTOM,true,false,(char*)"GDALNoDataValue"}
			};
		
		/* Merge them into the new TIFF file's tag directory: */
		TIFFMergeFieldInfo(tiff,geotiffFieldInfo,sizeof(geotiffFieldInfo)/sizeof(TIFFFieldInfo));
		
		/* Call the parent tag extender, if there was one: */
		if(parentTagExtender!=0)
			(*parentTagExtender)(tiff);
		}
	
	/* Constructors and destructors: */
	public:
	GeoTIFFExtender(void)
		{
		/* Register this class's TIFF tag extender and remember the previous one: */
		parentTagExtender=TIFFSetTagExtender(tagExtender);
		}
	};

GeoTIFFExtender GeoTIFFExtender::theTiffExtender;
TIFFExtendProc GeoTIFFExtender::parentTagExtender=0;

}

/***************************
Methods of class TIFFReader:
***************************/

void TIFFReader::tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	throw std::runtime_error(msg);
	}

void TIFFReader::tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Show warning to user: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	Misc::userWarning(msg);
	}

tsize_t TIFFReader::tiffReadFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Libtiff expects to always get the amount of data it wants: */
	source->readRaw(buffer,size);
	return size;
	}

tsize_t TIFFReader::tiffWriteFunction(thandle_t handle,tdata_t buffer,tsize_t size)
	{
	/* Ignore silently */
	return size;
	}

toff_t TIFFReader::tiffSeekFunction(thandle_t handle,toff_t offset,int whence)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	/* Seek to the requested position: */
	switch(whence)
		{
		case SEEK_SET:
			source->setReadPosAbs(offset);
			break;

		case SEEK_CUR:
			source->setReadPosRel(offset);
			break;

		case SEEK_END:
			source->setReadPosAbs(source->getSize()-offset);
			break;
		}
	
	return source->getReadPos();
	}

int TIFFReader::tiffCloseFunction(thandle_t handle)
	{
	/* Ignore silently */
	return 0;
	}

toff_t TIFFReader::tiffSizeFunction(thandle_t handle)
	{
	IO::SeekableFile* source=static_cast<IO::SeekableFile*>(handle);
	
	return source->getSize();
	}

int TIFFReader::tiffMapFileFunction(thandle_t handle,tdata_t* buffer,toff_t* size)
	{
	/* Ignore silently */
	return -1;
	}

void TIFFReader::tiffUnmapFileFunction(thandle_t handle,tdata_t buffer,toff_t size)
	{
	/* Ignore silently */
	}

TIFFReader::TIFFReader(IO::File& source,unsigned int imageIndex)
	:tiff(0),
	 rowsPerStrip(0),tileWidth(0),tileHeight(0)
	{
	/* Check if the source file is seekable: */
	seekableSource=IO::SeekableFilePtr(&source);
	if(seekableSource==0)
		{
		/* Create a seekable filter for the source file: */
		seekableSource=new IO::SeekableFilter(&source);
		}
	
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	/* Pretend to open the TIFF file and register the hook functions: */
	tiff=TIFFClientOpen("Foo.tif","rm",seekableSource.getPointer(),tiffReadFunction,tiffWriteFunction,tiffSeekFunction,tiffCloseFunction,tiffSizeFunction,tiffMapFileFunction,tiffUnmapFileFunction);
	if(tiff==0)
		throw std::runtime_error("Images::TIFFReader: Error while opening image");
	
	/* Select the requested image: */
	if(imageIndex!=0)
		{
		/* Select directory; errors will be handled by the TIFF error handler: */
		TIFFSetDirectory(tiff,uint16(imageIndex));
		}
	
	/* Get the image size and format: */
	TIFFGetField(tiff,TIFFTAG_IMAGEWIDTH,&width);
	TIFFGetField(tiff,TIFFTAG_IMAGELENGTH,&height);
	TIFFGetField(tiff,TIFFTAG_BITSPERSAMPLE,&numBits);
	TIFFGetField(tiff,TIFFTAG_SAMPLESPERPIXEL,&numSamples);
	TIFFGetFieldDefaulted(tiff,TIFFTAG_SAMPLEFORMAT,&sampleFormat);
	
	/* Check if pixel values are color map indices: */
	int16 indexedTagValue;
	bool haveIndexedTag=TIFFGetField(tiff,TIFFTAG_INDEXED,&indexedTagValue)!=0;
	indexed=haveIndexedTag&&indexedTagValue!=0;
	int16 photometricTagValue;
	bool havePhotometricTag=TIFFGetField(tiff,TIFFTAG_PHOTOMETRIC,&photometricTagValue)!=0;
	colorSpace=Invalid;
	if(havePhotometricTag&&photometricTagValue==PHOTOMETRIC_PALETTE)
		{
		if(!indexed)
			colorSpace=RGB;
		indexed=true;
		}
	else if(havePhotometricTag&&photometricTagValue<=10&&photometricTagValue!=7)
		colorSpace=ColorSpace(photometricTagValue);
	
	/* Query whether samples are layed out in planes or interleaved per pixel: */
	uint16 planarConfig;
	TIFFGetFieldDefaulted(tiff,TIFFTAG_PLANARCONFIG,&planarConfig);
	planar=planarConfig==PLANARCONFIG_SEPARATE;
	
	/* Query whether the image is organized in strips or tiles: */
	tiled=TIFFIsTiled(tiff);
	if(tiled)
		{
		/* Get the image's tile layout: */
		TIFFGetField(tiff,TIFFTAG_TILEWIDTH,&tileWidth);
		TIFFGetField(tiff,TIFFTAG_TILELENGTH,&tileHeight);
		}
	else
		{
		/* Get the image's strip layout: */
		TIFFGetField(tiff,TIFFTAG_ROWSPERSTRIP,&rowsPerStrip);
		}
	}

TIFFReader::~TIFFReader(void)
	{
	/* Close the TIFF file: */
	TIFFClose(tiff);
	}

bool TIFFReader::getColorMap(uint16*& red,uint16*& green,uint16*& blue)
	{
	/* Get the color map: */
	bool result=TIFFGetField(tiff,TIFFTAG_COLORMAP,&red,&green,&blue)!=0;
	if(!result)
		{
		/* Null the given pointers, just in case: */
		blue=green=red=0;
		}
	
	return result;
	}

bool TIFFReader::getCMYKColorMap(uint16*& cyan,uint16*& magenta,uint16*& yellow,uint16*& black)
	{
	/* Get the color map: */
	bool result=TIFFGetField(tiff,TIFFTAG_COLORMAP,&cyan,&magenta,&yellow,&black)!=0;
	if(!result)
		{
		/* Null the given pointers, just in case: */
		black=yellow=magenta=cyan=0;
		}
	
	return result;
	}

void TIFFReader::readMetadata(GeoTIFFMetadata& metadata)
	{
	/* Reset the metadata structure: */
	metadata.haveMap=false;
	metadata.haveDim=false;
	metadata.haveNoData=false;
	
	/* Query GeoTIFF tags: */
	uint16 elemCount=0;
	double* dElems=0;
	char* cElems=0;
	if(TIFFGetField(tiff,TIFFTAG_GEOPIXELSCALE,&elemCount,&dElems)&&elemCount>=2)
		{
		metadata.haveDim=true;
		metadata.dim[0]=dElems[0];
		metadata.dim[1]=dElems[1];
		}
	if(TIFFGetField(tiff,TIFFTAG_GEOTIEPOINTS,&elemCount,&dElems)&&elemCount==6&&dElems[0]==0.0&&dElems[1]==0.0&&dElems[2]==0.0&&dElems[5]==0.0)
		{
		/* Assume point pixels for now: */
		metadata.haveMap=true;
		metadata.map[0]=dElems[3];
		metadata.map[1]=dElems[4];
		}
	if(TIFFGetField(tiff,TIFFTAG_GEOTRANSMATRIX,&elemCount,&dElems)&&elemCount==16)
		{
		/* Assume point pixels for now: */
		metadata.haveMap=true;
		metadata.map[0]=dElems[3];
		metadata.map[1]=dElems[7];
		metadata.haveDim=true;
		metadata.dim[0]=dElems[0];
		metadata.dim[1]=dElems[5];
		}
	if(TIFFGetField(tiff,TIFFTAG_GDAL_NODATA,&cElems))
		{
		metadata.haveNoData=true;
		metadata.noData=atof(cElems);
		}
	
	/* Extract and parse GeoTIFF key directory: */
	bool pixelIsArea=true; // GeoTIFF pixels are area by default
	uint16 numKeys=0;
	uint16* keys=0;
	if(TIFFGetField(tiff,TIFFTAG_GEOKEYDIRECTORY,&numKeys,&keys)&&numKeys>=4)
		{
		uint16 numDoubleParams=0;
		double* doubleParams=0;
		if(!TIFFGetField(tiff,TIFFTAG_GEODOUBLEPARAMS,&numDoubleParams,&doubleParams))
			doubleParams=0;
		char* asciiParams=0;
		if(!TIFFGetField(tiff,TIFFTAG_GEOASCIIPARAMS,&asciiParams))
			asciiParams=0;
		
		/* Parse key directory header: */
		uint16* keyPtr=keys;
		numKeys=keyPtr[3]; // Actual number of keys is in the header
		keyPtr+=4;
		
		/* Parse all keys: */
		for(uint16 key=0;key<numKeys;++key,keyPtr+=4)
			{
			/* Check for relevant keys: */
			if(keyPtr[0]==GEOTIFFKEY_RASTERTYPE&&keyPtr[1]==0&&keyPtr[3]==GEOTIFFCODE_RASTERPIXELISPOINT)
				pixelIsArea=false;
			}
		}
	
	/* Check if the metadata's map must be adjusted: */
	if(pixelIsArea&&metadata.haveMap&&metadata.haveDim)
		{
		metadata.map[0]+=metadata.dim[0]*0.5;
		metadata.map[1]+=metadata.dim[1]*0.5;
		}
	}

void TIFFReader::readRgba(uint32* rgbaBuffer)
	{
	if(!TIFFReadRGBAImage(tiff,width,height,rgbaBuffer))
		throw std::runtime_error("Images::TIFFReader::readRgba: Error while reading image");
	}

namespace {

/****************
Helper functions:
****************/

template <class ScalarParam>
inline
void
copyRowChannel( // Helper function to read a single channel for an image row
	uint32 width,
	uint16 numChannels,
	uint16 channel,
	ScalarParam* rowPtr,
	ScalarParam* stripPtr)
	{
	rowPtr+=channel;
	for(uint32 x=0;x<width;++x,rowPtr+=numChannels,++stripPtr)
		*rowPtr=*stripPtr;
	}

}

void TIFFReader::readStrips(uint8* image,ptrdiff_t rowStride)
	{
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Create a buffer to hold a strip of image data: */
		Misc::SelfDestructArray<uint8> stripBuffer(TIFFStripSize(tiff));
		
		/* Read image data by channels: */
		for(uint16 channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by rows: */
			uint8* rowPtr=image+(height-1)*rowStride;
			uint32 rowStart=0;
			for(uint32 strip=0;rowStart<height;++strip)
				{
				/* Read the next strip of image data: */
				uint8* stripPtr=stripBuffer;
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Copy image data from the strip buffer into the result image: */
				uint32 rowEnd=rowStart+rowsPerStrip;
				if(rowEnd>height)
					rowEnd=height;
				switch(numBits)
					{
					case 8:
						for(uint32 row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,rowPtr,stripPtr);
						break;
					
					case 16:
						for(uint32 row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,reinterpret_cast<uint16*>(rowPtr),reinterpret_cast<uint16*>(stripPtr));
						break;
					
					case 32:
						for(uint32 row=rowStart;row<rowEnd;++row,rowPtr-=rowStride,stripPtr+=rowStride)
							copyRowChannel(width,numSamples,channel,reinterpret_cast<uint32*>(rowPtr),reinterpret_cast<uint32*>(stripPtr));
						break;
					}
				
				/* Prepare for the next strip: */
				rowStart=rowEnd;
				}
			}
		}
	else
		{
		/* Read image data by rows directly into the result image: */
		uint32 rowEnd=height;
		for(uint32 strip=0;rowEnd>0;++strip)
			{
			/* Read the next strip of image data into the appropriate region of the result image: */
			uint32 rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
			uint8* stripPtr=image+rowStart*rowStride;
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Flip rows in the just-read image region in-place: */
			uint32 row0=rowStart;
			uint32 row1=rowEnd-1;
			while(row0<row1)
				{
				uint8* row0Ptr=image+row0*rowStride;
				uint8* row1Ptr=image+row1*rowStride;
				for(size_t i=size_t(rowStride);i>0;--i,++row0Ptr,++row1Ptr)
					{
					uint8 t=*row0Ptr;
					*row0Ptr=*row1Ptr;
					*row1Ptr=t;
					}
				++row0;
				--row1;
				}
			
			/* Prepare for the next strip: */
			rowEnd=rowStart;
			}
		}
	}

namespace {

/****************
Helper functions:
****************/

template <class ScalarParam>
inline
void
copyTileChannel( // Helper function to read a single channel for an image tile
	uint32 width,
	uint32 height,
	uint16 numChannels,
	uint16 channel,
	ScalarParam* rowPtr,
	ptrdiff_t rowStride,
	ScalarParam* tilePtr,
	ptrdiff_t tileStride)
	{
	rowPtr+=channel;
	for(uint32 y=0;y<height;++y,rowPtr+=rowStride,tilePtr+=tileStride)
		{
		ScalarParam* rPtr=rowPtr;
		ScalarParam* tPtr=tilePtr;
		for(uint32 x=0;x<width;++x,rPtr+=numChannels,++tPtr)
			*rPtr=*tPtr;
		}
	}

}

uint8* TIFFReader::createTileBuffer(void)
	{
	/* Return a buffer to hold a tile of image data: */
	return new uint8[TIFFTileSize(tiff)];
	}

void TIFFReader::readTile(uint32 tileIndexX,uint32 tileIndexY,uint8* tileBuffer,uint8* image,ptrdiff_t rowStride)
	{
	/* Query tile memory layout: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	tmsize_t pixelSize=tmsize_t(numSamples*((numBits+7)/8));
	uint32 tilesPerRow=(width+tileWidth-1)/tileWidth;
	
	/* Calculate the index of the tile to read, or the tile to read in the first plane of a planar image: */
	uint32 tileIndex=tileIndexY*tilesPerRow+tileIndexX;
	
	/* Determine actually used tile size of the requested tile: */
	uint32 tw=Math::min(width-tileIndexX*tileWidth,tileWidth);
	tmsize_t ts=tw*pixelSize;
	uint32 th=Math::min(height-tileIndexY*tileHeight,tileHeight);
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Calculate the tile index increment to the next plane: */
		uint32 tilesPerPlane=((height+tileHeight-1)/tileHeight)*tilesPerRow;
		
		/* Read tile data by channels: */
		for(uint16 channel=0;channel<numSamples;++channel,tileIndex+=tilesPerPlane)
			{
			/* Read the requested tile into the tile buffer: */
			TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
			
			/* Copy tile data by row: */
			uint8* rowPtr=image;
			uint8* tilePtr=tileBuffer;
			switch(numBits)
				{
				case 8:
					for(uint32 y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,rowPtr,tilePtr);
					break;
				
				case 16:
					for(uint32 y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint16*>(rowPtr),reinterpret_cast<uint16*>(tilePtr));
					break;
				
				case 32:
					for(uint32 y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
						copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint32*>(rowPtr),reinterpret_cast<uint32*>(tilePtr));
					break;
				}
			}
		}
	else
		{
		/* Read the requested tile into the tile buffer: */
		TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
		
		/* Copy tile data by row: */
		uint8* rowPtr=image;
		uint8* tilePtr=tileBuffer;
		for(uint32 y=0;y<th;++y,rowPtr+=rowStride,tilePtr+=tileRowStride)
			{
			/* Copy the tile row: */
			memcpy(rowPtr,tilePtr,ts);
			}
		}
	}

void TIFFReader::readTiles(uint8* image,ptrdiff_t rowStride)
	{
	/* Create a buffer to hold a tile of image data: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	Misc::SelfDestructArray<uint8> tileBuffer(tileSize);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	tmsize_t pixelSize=tmsize_t(numSamples*((numBits+7)/8));
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Read image data by channels: */
		uint32 tileIndex=0;
		for(uint16 channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by tiles: */
			for(uint32 ty=0;ty<height;ty+=tileHeight)
				{
				/* Determine actual tile height in this tile row: */
				uint32 th=Math::min(height-ty,tileHeight);
				
				/* Read all tiles in this tile row: */
				for(uint32 tx=0;tx<width;tx+=tileWidth,++tileIndex)
					{
					/* Read the next tile: */
					TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
					
					/* Determine actual tile width in this tile column: */
					uint32 tw=Math::min(width-tx,tileWidth);
					
					/* Copy tile data by row: */
					uint8* rowPtr=image+(height-1-ty)*rowStride+tx*pixelSize;
					uint8* tilePtr=tileBuffer;
					switch(numBits)
						{
						case 8:
							for(uint32 y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,rowPtr,tilePtr);
							break;
						
						case 16:
							for(uint32 y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint16*>(rowPtr),reinterpret_cast<uint16*>(tilePtr));
							break;
						
						case 32:
							for(uint32 y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
								copyRowChannel(tw,numSamples,channel,reinterpret_cast<uint32*>(rowPtr),reinterpret_cast<uint32*>(tilePtr));
							break;
						}
					}
				}
			}
		}
	else
		{
		/* Read image data by tiles: */
		uint32 tileIndex=0;
		for(uint32 ty=0;ty<height;ty+=tileHeight)
			{
			/* Determine actual tile height in this tile row: */
			uint32 th=Math::min(height-ty,tileHeight);
			
			/* Read all tiles in this tile row: */
			for(uint32 tx=0;tx<width;tx+=tileWidth,++tileIndex)
				{
				/* Read the next tile: */
				TIFFReadEncodedTile(tiff,tileIndex,tileBuffer,tileSize);
				
				/* Determine actual tile width in this tile column: */
				uint32 tw=Math::min(width-tx,tileWidth);
				tmsize_t ts=tw*pixelSize;
				
				/* Copy tile data by row: */
				uint8* rowPtr=image+(height-1-ty)*rowStride+tx*pixelSize;
				uint8* tilePtr=tileBuffer;
				for(uint32 y=0;y<th;++y,rowPtr-=rowStride,tilePtr+=tileRowStride)
					{
					/* Copy the tile row: */
					memcpy(rowPtr,tilePtr,ts);
					}
				}
			}
		}
	}

void TIFFReader::streamStrips(TIFFReader::PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData)
	{
	/* Create a buffer to hold a strip of image data: */
	Misc::SelfDestructArray<uint8> stripBuffer(TIFFStripSize(tiff));
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Calculate the strip stride: */
		ptrdiff_t stripStride=ptrdiff_t(width)*ptrdiff_t((numBits+7)/8);
		
		/* Read pixel data by channels: */
		for(uint16 channel=0;channel<numSamples;++channel)
			{
			/* Read channel data by rows: */
			uint32 rowEnd=height;
			for(uint32 strip=0;rowEnd>0;++strip)
				{
				/* Read the next strip of channel data into the strip buffer: */
				uint32 rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
				uint8* stripPtr=stripBuffer;
				TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
				
				/* Stream the channel data strip row-by-row: */
				for(uint32 row=rowEnd;row!=rowStart;--row,stripPtr+=stripStride)
					pixelStreamingCallback(0,row-1,width,channel,stripPtr,pixelStreamingUserData);
				
				/* Prepare for the next strip: */
				rowEnd=rowStart;
				}
			}
		}
	else
		{
		/* Calculate the strip stride: */
		ptrdiff_t stripStride=ptrdiff_t(width)*ptrdiff_t(numSamples)*ptrdiff_t((numBits+7)/8);
		
		/* Read pixel data by rows: */
		uint32 rowEnd=height;
		for(uint32 strip=0;rowEnd>0;++strip)
			{
			/* Read the next strip of pixel data into the strip buffer: */
			uint32 rowStart=rowEnd>=rowsPerStrip?rowEnd-rowsPerStrip:0;
			uint8* stripPtr=stripBuffer;
			TIFFReadEncodedStrip(tiff,strip,stripPtr,tsize_t(-1));
			
			/* Stream the pixel data strip row-by-row: */
			for(uint32 row=rowEnd;row!=rowStart;--row,stripPtr+=stripStride)
				pixelStreamingCallback(0,row-1,width,uint16(-1),stripPtr,pixelStreamingUserData);
			
			/* Prepare for the next strip: */
			rowEnd=rowStart;
			}
		}
	}

void TIFFReader::streamTiles(TIFFReader::PixelStreamingCallback pixelStreamingCallback,void* pixelStreamingUserData)
	{
	/* Create a buffer to hold a tile of image data: */
	tmsize_t tileSize=TIFFTileSize(tiff);
	Misc::SelfDestructArray<uint8> tileBuffer(tileSize);
	tmsize_t tileRowStride=TIFFTileRowSize(tiff);
	
	/* Check whether the image is planed: */
	if(planar)
		{
		/* Read pixel data by channels: */
		uint32 tileIndex=0;
		for(uint16 channel=0;channel<numSamples;++channel)
			{
			for(uint32 ty=0;ty<height;ty+=tileHeight)
				{
				/* Determine actual tile height in this tile row: */
				uint32 th=Math::min(height-ty,tileHeight);
				
				/* Read all tiles in this tile row: */
				for(uint32 tx=0;tx<width;tx+=tileWidth,++tileIndex)
					{
					/* Determine actual tile width in this tile column: */
					uint32 tw=Math::min(width-tx,tileWidth);
					
					/* Read the next tile of channel data: */
					uint8* tilePtr=tileBuffer;
					TIFFReadEncodedTile(tiff,tileIndex,tilePtr,tileSize);
					
					/* Stream the channel data tile row-by-row: */
					uint32 rowEnd=height-(ty+th);
					for(uint32 row=height-ty;row!=rowEnd;--row,tilePtr+=tileRowStride)
						pixelStreamingCallback(tx,row-1,tw,channel,tilePtr,pixelStreamingUserData);
					}
				}
			}
		}
	else
		{
		/* Read pixel data by tiles: */
		uint32 tileIndex=0;
		for(uint32 ty=0;ty<height;ty+=tileHeight)
			{
			/* Determine actual tile height in this tile row: */
			uint32 th=Math::min(height-ty,tileHeight);
			
			/* Read all tiles in this tile row: */
			for(uint32 tx=0;tx<width;tx+=tileWidth,++tileIndex)
				{
				/* Determine actual tile width in this tile column: */
				uint32 tw=Math::min(width-tx,tileWidth);
				
				/* Read the next tile of pixel data: */
				uint8* tilePtr=tileBuffer;
				TIFFReadEncodedTile(tiff,tileIndex,tilePtr,tileSize);
				
				/* Stream the pixel data tile row-by-row: */
				uint32 rowEnd=height-(ty+th);
				for(uint32 row=height-ty;row!=rowEnd;--row,tilePtr+=tileRowStride)
					pixelStreamingCallback(tx,row-1,tw,uint16(-1),tilePtr,pixelStreamingUserData);
				}
			}
		}
	}

}

#endif
