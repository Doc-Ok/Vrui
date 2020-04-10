/***********************************************************************
ReadImageFile - Functions to read generic images from image files in a
variety of formats.
Copyright (c) 2005-2019 Oliver Kreylos

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

#include <Images/ReadImageFile.h>

#include <Images/Config.h>

#include <Misc/Utility.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
#include <IO/Directory.h>
#include <Images/BaseImage.h>
#include <Images/RGBImage.h>
#include <Images/RGBAImage.h>
#include <Images/ReadPNMImage.h>
#include <Images/ReadBILImage.h>
#include <Images/ReadPNGImage.h>
#include <Images/ReadJPEGImage.h>
#include <Images/ReadTIFFImage.h>

namespace Images {

bool canReadImageFileFormat(ImageFileFormat imageFileFormat)
	{
	/* Check if the format is supported: */
	if(imageFileFormat==IFF_PNM||imageFileFormat==IFF_BIL)
		return true;
	#if IMAGES_CONFIG_HAVE_PNG
	if(imageFileFormat==IFF_PNG)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_JPEG
	if(imageFileFormat==IFF_JPEG)
		return true;
	#endif
	#if IMAGES_CONFIG_HAVE_TIFF
	if(imageFileFormat==IFF_TIFF)
		return true;
	#endif
	
	return false;
	}

BaseImage readGenericImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* Delegate to specific readers based on the given format: */
	BaseImage result;
	switch(imageFileFormat)
		{
		case IFF_PNM:
			/* Read a generic PNM image from the given file: */
			result=readGenericPNMImage(file);
			break;
		
		case IFF_BIL:
			/* Can't read BIL files through an already-open file, as we need a header file: */
			throw std::runtime_error("Images::readGenericImageFile: Cannot read BIP/BIL/BSQ image files through an already-open file");
		
		#if IMAGES_CONFIG_HAVE_PNG
		case IFF_PNG:
			/* Read a generic PNG image from the given file: */
			result=readGenericPNGImage(file);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_JPEG
		case IFF_JPEG:
			/* Read a generic JPEG image from the given file: */
			result=readGenericJPEGImage(file);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_TIFF
		case IFF_TIFF:
			/* Read a generic TIFF image from the given file: */
			result=readGenericTIFFImage(file);
			break;
		#endif
		
		default:
			throw std::runtime_error("Images::readGenericImageFile: Unsupported image file format");
		}
	
	return result;
	}

BaseImage readGenericImageFile(const char* imageFileName)
	{
	BaseImage result;
	
	try
		{
		/* Determine the type of the given image file: */
		ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
		
		/* Delegate to specific readers based on the given format: */
		switch(imageFileFormat)
			{
			case IFF_PNM:
				/* Read a generic PNM image from the given file: */
				result=readGenericPNMImage(*IO::openFile(imageFileName));
				break;
			
			case IFF_BIL:
				/* Read a generic BIL file from the given file: */
				result=readGenericBILImage(imageFileName);
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				/* Read a generic PNG image from the given file: */
				result=readGenericPNGImage(*IO::openFile(imageFileName));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				/* Read a generic JPEG image from the given file: */
				result=readGenericJPEGImage(*IO::openFile(imageFileName));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				/* Read a generic TIFF image from the given file: */
				result=readGenericTIFFImage(*IO::openFile(imageFileName));
				break;
			#endif
			
			default:
				throw std::runtime_error("Unsupported image file format");
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readGenericImageFile: Error %s while reading image file %s",err.what(),imageFileName);
		}
	
	return result;
	}

BaseImage readGenericImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	BaseImage result;
	
	try
		{
		/* Determine the type of the given image file: */
		ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
		
		/* Delegate to specific readers based on the given format: */
		switch(imageFileFormat)
			{
			case IFF_PNM:
				/* Read a generic PNM image from the given directory and file: */
				result=readGenericPNMImage(*directory.openFile(imageFileName));
				break;
			
			case IFF_BIL:
				/* Read a generic BIL file from the given directory and file: */
				result=readGenericBILImage(directory,imageFileName);
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				/* Read a generic PNG image from the given directory and file: */
				result=readGenericPNGImage(*directory.openFile(imageFileName));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				/* Read a generic JPEG image from the given directory and file: */
				result=readGenericJPEGImage(*directory.openFile(imageFileName));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				/* Read a generic TIFF image from the given directory and file: */
				result=readGenericTIFFImage(*directory.openFile(imageFileName));
				break;
			#endif
			
			default:
				throw std::runtime_error("Unsupported image file format");
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readGenericImageFile: Error %s while reading image file %s",err.what(),directory.getPath(imageFileName).c_str());
		}
	
	return result;
	}

namespace {

/********************************************
Helper structures for the cursor file reader:
********************************************/

struct CursorFileHeader
	{
	/* Elements: */
	public:
	unsigned int magic;
	unsigned int headerSize;
	unsigned int version;
	unsigned int numTOCEntries;
	};

struct CursorTOCEntry
	{
	/* Elements: */
	public:
	unsigned int chunkType;
	unsigned int chunkSubtype;
	unsigned int chunkPosition;
	};

struct CursorCommentChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffe0001U
	unsigned int chunkSubtype;
	unsigned int version;
	unsigned int commentLength;
	/* Comment characters follow */
	};

struct CursorImageChunkHeader
	{
	/* Elements: */
	public:
	unsigned int headerSize;
	unsigned int chunkType; // 0xfffd0002U
	unsigned int chunkSubtype;
	unsigned int version;
	unsigned int size[2];
	unsigned int hotspot[2];
	unsigned int delay;
	/* ARGB pixel data follows */
	};

}

/***********************************************
Function to read cursor files in Xcursor format:
***********************************************/

RGBAImage readCursorFile(IO::File& file,unsigned int nominalSize,unsigned int* hotspot)
	{
	/* Read the magic value to determine file endianness: */
	size_t filePos=0;
	CursorFileHeader fh;
	fh.magic=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	if(fh.magic==0x58637572U)
		file.setSwapOnRead(true);
	else if(fh.magic!=0x72756358U)
		throw std::runtime_error("Images::readCursorFile: Invalid Xcursor file header");
	
	/* Read the rest of the file header: */
	fh.headerSize=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.version=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	fh.numTOCEntries=file.read<unsigned int>();
	filePos+=sizeof(unsigned int);
	
	/* Read the table of contents: */
	size_t imageChunkOffset=0;
	for(unsigned int i=0;i<fh.numTOCEntries;++i)
		{
		CursorTOCEntry te;
		te.chunkType=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkSubtype=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		te.chunkPosition=file.read<unsigned int>();
		filePos+=sizeof(unsigned int);
		
		if(te.chunkType==0xfffd0002U&&te.chunkSubtype==nominalSize)
			{
			imageChunkOffset=size_t(te.chunkPosition);
			break;
			}
		}
	
	if(imageChunkOffset==0)
		throw std::runtime_error("Images::readCursorFile: No matching image found");
	
	/* Skip ahead to the beginning of the image chunk: */
	file.skip<char>(imageChunkOffset-filePos);
	
	/* Read the image chunk: */
	CursorImageChunkHeader ich;
	ich.headerSize=file.read<unsigned int>();
	ich.chunkType=file.read<unsigned int>();
	ich.chunkSubtype=file.read<unsigned int>();
	ich.version=file.read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.size[i]=file.read<unsigned int>();
	for(int i=0;i<2;++i)
		ich.hotspot[i]=file.read<unsigned int>();
	if(hotspot!=0)
		{
		for(int i=0;i<2;++i)
			hotspot[i]=ich.hotspot[i];
		}
	ich.delay=file.read<unsigned int>();
	if(ich.headerSize!=9*sizeof(unsigned int)||ich.chunkType!=0xfffd0002U||ich.version!=1)
		throw std::runtime_error("Images::readCursorFile: Invalid image chunk header");
	
	/* Create the result image: */
	RGBAImage result(ich.size[0],ich.size[1]);
	
	/* Read the image row-by-row: */
	for(unsigned int row=result.getHeight();row>0;--row)
		{
		RGBAImage::Color* rowPtr=result.modifyPixelRow(row-1);
		file.read(rowPtr->getRgba(),result.getWidth()*4);
		
		/* Convert BGRA data into RGBA data: */
		for(unsigned int i=0;i<result.getWidth();++i)
			Misc::swap(rowPtr[i][0],rowPtr[i][2]);
		}
	
	/* Return the result image: */
	return result;
	}

RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot)
	{
	RGBAImage result;
	
	try
		{
		/* Open the cursor file and read it: */
		result=readCursorFile(*IO::openFile(cursorFileName),nominalSize,hotspot);
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readCursorFile: Error %s while reading cursor file %s",err.what(),cursorFileName);
		}
	
	return result;
	}

RGBAImage readCursorFile(const IO::Directory& directory,const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot)
	{
	RGBAImage result;
	
	try
		{
		/* Open the cursor file and read it: */
		result=readCursorFile(*directory.openFile(cursorFileName),nominalSize,hotspot);
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::readCursorFile: Error %s while reading cursor file %s",err.what(),directory.getPath(cursorFileName).c_str());
		}
	
	return result;
	}

/***********************************
Deprecated functions to read images:
***********************************/

RGBImage readImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file through deprecated RGBImage readImageFile(IO::File& file) function");
	
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(file,imageFileFormat);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const char* imageFileName)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file %s through deprecated RGBImage readImageFile(const char* fileName) function",imageFileName);
	
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(imageFileName);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBImage readImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file %s through deprecated RGBImage readImageFile(const IO::Directory& directory,const char* fileName) function",directory.getPath(imageFileName).c_str());
	
	/* This is a legacy function; read generic image and convert it to RGB: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return RGBImage(result.dropAlpha().toRgb());
	}

RGBAImage readTransparentImageFile(IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file through deprecated RGBAImage readTransparentImageFile(IO::File& file) function");
	
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(file,imageFileFormat);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const char* imageFileName)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file %s through deprecated RGBAImage readTransparentImageFile(const char* fileName) function",imageFileName);
	
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(imageFileName);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

RGBAImage readTransparentImageFile(const IO::Directory& directory,const char* imageFileName)
	{
	/* Print a warning: */
	Misc::formattedLogWarning("Images: Reading image file %s through deprecated RGBAImage readTransparentImageFile(const IO::Directory& directory,const char* fileName) function",directory.getPath(imageFileName).c_str());
	
	/* This is a legacy function; read generic image and convert it to RGBA: */
	BaseImage result=readGenericImageFile(directory,imageFileName);
	return RGBAImage(result.addAlpha(1.0).toRgb());
	}

}
