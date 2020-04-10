/***********************************************************************
GetImageFileSize - Functions to extract the image size from a variety of
file formats reading the minimal required amount of data.
Copyright (c) 2007-2018 Oliver Kreylos

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

#include <Images/GetImageFileSize.h>

#include <Images/Config.h>

#include <stdio.h>
#include <string.h>
#if IMAGES_CONFIG_HAVE_PNG
#include <png.h>
#endif
#if IMAGES_CONFIG_HAVE_JPEG
#include <jpeglib.h>
#endif
#if IMAGES_CONFIG_HAVE_TIFF
#include <tiffio.h>
#endif
#include <Misc/ThrowStdErr.h>
#include <Misc/File.h>
#include <Images/ImageFileFormats.h>

namespace Images {

namespace {

/**********************************************
Function to extract image size from PNM images:
**********************************************/

void getPnmFileSize(const char* imageFileName,unsigned int& width,unsigned int& height)
	{
	/* Open the image file: */
	Misc::File pnmFile(imageFileName,"rb");
	char line[80];
	
	/* Check the PNM file type: */
	pnmFile.gets(line,sizeof(line));
	if(line[0]!='P'||line[1]<'4'||line[1]>'6'||line[2]!='\n')
		Misc::throwStdErr("Images::getPnmFileSize: illegal PNM header in image file \"%s\"",imageFileName);
	
	/* Skip any comment lines in the PNM header: */
	do
		{
		pnmFile.gets(line,sizeof(line));
		}
	while(line[0]=='#');
	
	/* Read the image size: */
	sscanf(line,"%u %u",&width,&height);
	}

#if IMAGES_CONFIG_HAVE_PNG

/**********************************************
Function to extract image size from PNG images:
**********************************************/

void getPngFileSize(const char* imageFileName,unsigned int& width,unsigned int& height)
	{
	/* Open input file: */
	Misc::File pngFile(imageFileName,"rb");
	
	/* Check for PNG file signature: */
	unsigned char pngSignature[8];
	pngFile.read(pngSignature,8);
	if(!png_check_sig(pngSignature,8))
		Misc::throwStdErr("Images::getPngFileSize: illegal PNG header in image file \"%s\"",imageFileName);
	
	/* Allocate the PNG library data structures: */
	png_structp pngReadStruct=png_create_read_struct(PNG_LIBPNG_VER_STRING,0,0,0);
	if(pngReadStruct==0)
		Misc::throwStdErr("Images::getPngFileSize: Internal error in PNG library");
	png_infop pngInfoStruct=png_create_info_struct(pngReadStruct);
	if(pngInfoStruct==0)
		{
		png_destroy_read_struct(&pngReadStruct,0,0);
		Misc::throwStdErr("Images::getPngFileSize: Internal error in PNG library");
		}
	
	/* Set up longjump facility for PNG error handling (ouch): */
	if(setjmp(png_jmpbuf(pngReadStruct)))
		{
		png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
		Misc::throwStdErr("Images::getPngFileSize: Error while setting up PNG library error handling");
		}
	
	/* Initialize PNG I/O: */
	png_init_io(pngReadStruct,pngFile.getFilePtr());
	
	/* Read PNG image header: */
	png_set_sig_bytes(pngReadStruct,8);
	png_read_info(pngReadStruct,pngInfoStruct);
	png_uint_32 imageSize[2];
	int elementSize;
	int colorType;
	png_get_IHDR(pngReadStruct,pngInfoStruct,&imageSize[0],&imageSize[1],&elementSize,&colorType,0,0,0);
	
	/* Set the image size: */
	width=imageSize[0];
	height=imageSize[1];
	
	/* Clean up: */
	png_destroy_read_struct(&pngReadStruct,&pngInfoStruct,0);
	}

#endif

#if IMAGES_CONFIG_HAVE_JPEG

/***************************************************
Helper structures and functions for the JPEG reader:
***************************************************/

struct ExceptionErrorManager:public jpeg_error_mgr
	{
	/* Constructors and destructors: */
	public:
	ExceptionErrorManager(void)
		{
		/* Set the method pointer(s) in the base class object: */
		jpeg_std_error(this);
		error_exit=exit;
		}
	
	/* Methods: */
	static void exit(j_common_ptr cinfo) // Method called when JPEG library encounters a fatal error
		{
		/* Get pointer to the error manager object: */
		// ExceptionErrorManager* myManager=static_cast<ExceptionErrorManager*>(cinfo->err);
		
		/* Throw an exception: */
		Misc::throwStdErr("Images::getJpegFileSize: JPEG library encountered fatal error");
		}
	};

/*********************************************************************************
Function to extract image size from JPEG (actually, JFIF interchange) image files:
*********************************************************************************/

void getJpegFileSize(const char* imageFileName,unsigned int& width,unsigned int& height)
	{
	/* Open input file: */
	Misc::File jpegFile(imageFileName,"rb");
	
	/* Create a JPEG error handler and a JPEG decompression object: */
	ExceptionErrorManager jpegErrorManager;
	jpeg_decompress_struct jpegDecompressStruct;
	jpegDecompressStruct.err=&jpegErrorManager;
	jpeg_create_decompress(&jpegDecompressStruct);
	
	try
		{
		/* Associate the decompression object with the input file: */
		jpeg_stdio_src(&jpegDecompressStruct,jpegFile.getFilePtr());
		
		/* Read the JPEG file header: */
		jpeg_read_header(&jpegDecompressStruct,TRUE);
		
		/* Prepare for decompression: */
		jpeg_start_decompress(&jpegDecompressStruct);
		
		/* Store the image size: */
		width=jpegDecompressStruct.output_width;
		height=jpegDecompressStruct.output_height;
		
		/* Finish reading image: */
		jpeg_finish_decompress(&jpegDecompressStruct);
		
		/* Clean up: */
		jpeg_destroy_decompress(&jpegDecompressStruct);
		}
	catch(...)
		{
		/* Clean up: */
		jpeg_destroy_decompress(&jpegDecompressStruct);
		
		/* Re-throw the exception: */
		throw;
		}
	}

#endif

#if IMAGES_CONFIG_HAVE_TIFF

namespace {

/******************************************
Helper functions for the TIFF image reader:
******************************************/

void tiffErrorFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Throw an exception with the error message: */
	char msg[1024];
	vsnprintf(msg,sizeof(msg),fmt,ap);
	throw std::runtime_error(msg);
	}

void tiffWarningFunction(const char* module,const char* fmt,va_list ap)
	{
	/* Ignore warnings */
	}

}

/***********************************************
Function to extract image size from TIFF images:
***********************************************/

void getTiffFileSize(const char* imageFileName,unsigned int& width,unsigned int& height)
	{
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	/* Open the TIFF image: */
	TIFF* image=TIFFOpen(imageFileName,"r");
	if(image==0)
		Misc::throwStdErr("Images::getTiffFileSize: Unable to open image file %s",imageFileName);
	
	/* Get the image size: */
	uint32 tiffWidth,tiffHeight;
	TIFFGetField(image,TIFFTAG_IMAGEWIDTH,&tiffWidth);
	TIFFGetField(image,TIFFTAG_IMAGELENGTH,&tiffHeight);
	width=tiffWidth;
	height=tiffHeight;
	}

#endif

}

/*****************************************************************************
Function to extract image sizes from image files in several supported formats:
*****************************************************************************/

void getImageFileSize(const char* imageFileName,unsigned int& width,unsigned int& height)
	{
	/* Determine the type of the given image file: */
	ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
	
	/* Delegate to specific readers based on the given format: */
	switch(imageFileFormat)
		{
		case IFF_PNM:
			getPnmFileSize(imageFileName,width,height);
			break;
		
		#if IMAGES_CONFIG_HAVE_PNG
		case IFF_PNG:
			getPngFileSize(imageFileName,width,height);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_JPEG
		case IFF_JPEG:
			getJpegFileSize(imageFileName,width,height);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_TIFF
		case IFF_TIFF:
			getTiffFileSize(imageFileName,width,height);
			break;
		#endif
		
		default:
			Misc::throwStdErr("Images::getImageFileSize: Image file \"%s\" has unsupported format",imageFileName);
		}
	}

}
