/***********************************************************************
WriteImageFile - Functions to write RGB images to a variety of file
formats.
Copyright (c) 2006-2020 Oliver Kreylos

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

#include <Images/WriteImageFile.h>

#include <Images/Config.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>
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
#include <Misc/MessageLogger.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>

namespace Images {

namespace {

/****************************************
Function to write binary PNM image files:
****************************************/

void writePnmFile(unsigned int width,unsigned int height,const unsigned char* image,IO::File& imageFile)
	{
	/* Assemble the PNM header: */
	char header[1024];
	int headerSize=snprintf(header,sizeof(header),"P6\n%u %u\n255\n",width,height);
	
	/* Write the assembled header to the file: */
	imageFile.write(header,headerSize);
	
	/* Write each row of the image file in order to flip the image vertically: */
	ptrdiff_t rowStride=width*3;
	const unsigned char* rowPtr=image+(height-1)*rowStride;
	for(unsigned int row=0;row<height;++row,rowPtr-=rowStride)
		imageFile.write(rowPtr,width*3);
	}

#if IMAGES_CONFIG_HAVE_PNG

class PNGWriter
	{
	/* Elements: */
	private:
	IO::File& sink; // Handle for the PNG image file
	png_structp pngWriteStruct; // Structure representing state of an open PNG image file inside the PNG library
	png_infop pngInfoStruct; // Structure containing information about the image in an open PNG image file
	unsigned int imageSize[2]; // Size of image to be written
	
	/* Private methods: */
	static void writeDataFunction(png_structp pngWriteStruct,png_bytep buffer,png_size_t size) // Called by the PNG library to write additional data to the sink
		{
		/* Get the pointer to the IO::File object: */
		IO::File* sink=static_cast<IO::File*>(png_get_io_ptr(pngWriteStruct));
		
		/* Write the requested number of bytes to the sink, and let the sink handle errors: */
		sink->write(buffer,size);
		}
	static void flushSinkFunction(png_structp pngWriteStruct) // Called by the PNG library to flush the sink
		{
		/* Get the pointer to the IO::File object: */
		IO::File* sink=static_cast<IO::File*>(png_get_io_ptr(pngWriteStruct));
		
		/* Flush the sink buffer: */
		sink->flush();
		}
	static void errorFunction(png_structp pngReadStruct,png_const_charp errorMsg) // Called by the PNG library to report an error
		{
		/* Throw an exception: */
		throw std::runtime_error(errorMsg);
		}
	static void warningFunction(png_structp pngReadStruct,png_const_charp warningMsg) // Called by the PNG library to report a recoverable error
		{
		/* Show warning to user: */
		Misc::userWarning(warningMsg);
		}
	
	/* Constructors and destructors: */
	public:
	PNGWriter(IO::File& sSink)
		:sink(sSink),
		 pngWriteStruct(0),pngInfoStruct(0)
		{
		/* Allocate the PNG library data structures: */
		if((pngWriteStruct=png_create_write_struct(PNG_LIBPNG_VER_STRING,0,errorFunction,warningFunction))==0
		   ||(pngInfoStruct=png_create_info_struct(pngWriteStruct))==0)
		  {
		  /* Clean up and throw an exception: */
			png_destroy_write_struct(&pngWriteStruct,0);
			throw std::runtime_error("Images::PNGWriter: Internal error in PNG library");
			}
		
		/* Initialize PNG I/O to write to the supplied data sink: */
		png_set_write_fn(pngWriteStruct,&sink,writeDataFunction,flushSinkFunction);
		}
	~PNGWriter(void)
		{
		/* Clean up: */
		png_destroy_write_struct(&pngWriteStruct,&pngInfoStruct);
		}
	
	/* Methods: */
	void writeHeader(unsigned int width,unsigned int height)
		{
		/* Remember image size: */
		imageSize[0]=width;
		imageSize[1]=height;
		
		/* Set and write PNG image header: */
		png_set_IHDR(pngWriteStruct,pngInfoStruct,imageSize[0],imageSize[1],8,PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
		png_write_info(pngWriteStruct,pngInfoStruct);
		}
	void writeImage(const png_byte* pixels,ptrdiff_t rowStride)
		{
		/* Write all image rows in reverse order: */
		const png_byte* rowPtr=pixels+(imageSize[1]-1)*rowStride;
		for(unsigned int row=0;row<imageSize[1];++row,rowPtr-=rowStride)
			png_write_row(pngWriteStruct,rowPtr);
		
		/* Finish writing image: */
		png_write_end(pngWriteStruct,0);
		}
	};

/*********************************
Function to write PNG image files:
*********************************/

void writePngFile(unsigned int width,unsigned int height,const unsigned char* image,IO::File& imageFile)
	{
	/* Create a PNG writer: */
	PNGWriter writer(imageFile);
	
	/* Write a PNG header: */
	writer.writeHeader(width,height);
	
	/* Write the PNG image: */
	writer.writeImage(image,width*3);
	}

#endif

#if IMAGES_CONFIG_HAVE_JPEG

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

class JPEGFileDestinationManager:public jpeg_destination_mgr
	{
	/* Methods: */
	private:
	IO::File& dest; // Reference to the destination stream
	size_t bufferSize; // Size of the currently used output buffer
	
	/* Private methods: */
	void initBuffer(void)
		{
		/* Set the output buffer to the destination file's output buffer: */
		void* destBuffer;
		free_in_buffer=bufferSize=dest.writeInBufferPrepare(destBuffer);
		next_output_byte=static_cast<JOCTET*>(destBuffer);
		}
	static void initDestinationFunction(j_compress_ptr cinfo)
		{
		JPEGFileDestinationManager* thisPtr=static_cast<JPEGFileDestinationManager*>(cinfo->dest);
		
		/* Clear the output buffer: */
		thisPtr->initBuffer();
		}
	static boolean emptyOutputBufferFunction(j_compress_ptr cinfo)
		{
		JPEGFileDestinationManager* thisPtr=static_cast<JPEGFileDestinationManager*>(cinfo->dest);
		
		/* Write the JPEG encoder's output buffer to the destination file: */
		thisPtr->dest.writeInBufferFinish(thisPtr->bufferSize);
		
		/* Clear the output buffer: */
		thisPtr->initBuffer();
		
		return TRUE;
		}
	static void termDestinationFunction(j_compress_ptr cinfo)
		{
		JPEGFileDestinationManager* thisPtr=static_cast<JPEGFileDestinationManager*>(cinfo->dest);
		
		/* Write the JPEG encoder's final (partial) output buffer to the destination file: */
		thisPtr->dest.writeInBufferFinish(thisPtr->bufferSize-thisPtr->free_in_buffer);
		}
	
	/* Constructors and destructors: */
	public:
	JPEGFileDestinationManager(IO::File& sDest)
		:dest(sDest)
		{
		/* Install the hook functions: */
		init_destination=initDestinationFunction;
		empty_output_buffer=emptyOutputBufferFunction;
		term_destination=termDestinationFunction;
		}
	~JPEGFileDestinationManager(void)
		{
		}
	};

class JPEGWriter:public jpeg_compress_struct
	{
	/* Elements: */
	private:
	JPEGExceptionErrorManager exceptionManager;
	JPEGFileDestinationManager fileDestinationManager;
	JSAMPLE const** rowPointers;
	
	/* Constructors and destructors: */
	public:
	JPEGWriter(IO::File& destination)
		:fileDestinationManager(destination),
		 rowPointers(0)
		{
		/* Throw an exception if the JPEG library consumes anything but 8-bit samples: */
		#if BITS_IN_JSAMPLE!=8
		throw std::runtime_error("Images::JPEGWriter: Unsupported bit depth in JPEG library");
		#endif
		
		/* Initialize the JPEG library parts of this object: */
		err=&exceptionManager;
		client_data=0;
		jpeg_create_compress(this);
		dest=&fileDestinationManager;
		}
	~JPEGWriter(void)
		{
		/* Clean up: */
		delete[] rowPointers;
		jpeg_destroy_compress(this);
		}
	
	/* Methods: */
	void writeImage(JDIMENSION width,JDIMENSION height,const JSAMPLE* pixels,ptrdiff_t rowStride)
		{
		/* Set JPEG library image descriptor: */
		image_width=width;
		image_height=height;
		input_components=3;
		in_color_space=JCS_RGB;
		
		/* Create default compression parameters: */
		jpeg_set_defaults(this);
		
		/* Override some defaults with other defaults: */
		jpeg_set_quality(this,90,TRUE);
		arith_code=FALSE;
		dct_method=JDCT_FASTEST;
		optimize_coding=FALSE;
		
		/* Start compressing to a complete JPEG interchange datastream: */
		jpeg_start_compress(this,TRUE);
		
		/* Create row pointers to flip the image during writing: */
		rowPointers=new const JSAMPLE*[image_height];
		rowPointers[0]=pixels+(image_height-1)*rowStride;
		for(JDIMENSION y=1;y<image_height;++y)
			rowPointers[y]=rowPointers[y-1]-rowStride;
		
		/* Read the JPEG image's scan lines: */
		JDIMENSION scanline=0;
		while(scanline<image_height)
			scanline+=jpeg_write_scanlines(this,const_cast<JSAMPARRAY>(rowPointers+scanline),image_height-scanline);
		
		/* Finish writing image: */
		jpeg_finish_compress(this);
		}
	};

void writeJpegFile(unsigned int width,unsigned int height,const unsigned char* image,IO::File& imageFile)
	{
	/* Create a JPEG writer: */
	JPEGWriter writer(imageFile);
	
	/* Write the JPEG image: */
	writer.writeImage(width,height,image,width*3);
	}

#endif

#if IMAGES_CONFIG_HAVE_TIFF

/**********************************
Function to write TIFF image files:
**********************************/

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

void writeTiffFile(unsigned int width,unsigned int height,const unsigned char* image,const char* imageFileName)
	{
	/* Set the TIFF error handler: */
	TIFFSetErrorHandler(tiffErrorFunction);
	TIFFSetWarningHandler(tiffWarningFunction);
	
	TIFF* tiff=0;
	try
		{
		/* Open the TIFF file: */
		tiff=TIFFOpen(imageFileName,"w");
		if(tiff==0)
			throw std::runtime_error("Error while opening image file");
		
		/* Write the TIFF image layout tags: */
		TIFFSetField(tiff,TIFFTAG_IMAGEWIDTH,width);
		TIFFSetField(tiff,TIFFTAG_IMAGELENGTH,height);
		TIFFSetField(tiff,TIFFTAG_BITSPERSAMPLE,8);
		TIFFSetField(tiff,TIFFTAG_SAMPLESPERPIXEL,3);
		TIFFSetField(tiff,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
		TIFFSetField(tiff,TIFFTAG_COMPRESSION,COMPRESSION_NONE);
		TIFFSetField(tiff,TIFFTAG_PHOTOMETRIC,PHOTOMETRIC_RGB);
		TIFFSetField(tiff,TIFFTAG_ROWSPERSTRIP,TIFFDefaultStripSize(tiff,width*3));
		
		/* Write the image data to the TIFF file one scan line at a time, top-to-bottom: */
		for(unsigned int row=height;row>0;--row)
			if(TIFFWriteScanline(tiff,reinterpret_cast<tdata_t>(const_cast<unsigned char*>(image+(row-1)*width*3)),height-row)<0)
				throw std::runtime_error("Error while writing image");
		
		/* Close the TIFF file: */
		TIFFClose(tiff);
		}
	catch(const std::runtime_error& err)
		{
		/* Clean up: */
		if(tiff!=0)
			TIFFClose(tiff);
		
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::writeTiffFile: Caught exception \"%s\" while writing image \"%s\"",err.what(),imageFileName);
		}
	}

#endif

}

bool canWriteImageFileFormat(ImageFileFormat imageFileFormat)
	{
	/* Check if the format is supported: */
	if(imageFileFormat==IFF_PNM)
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

/***********************************************************
Function to write images files in several supported formats:
***********************************************************/

void writeImageFile(const RGBImage& image,IO::File& file,ImageFileFormat imageFileFormat)
	{
	/* Delegate to specific readers based on the given format: */
	switch(imageFileFormat)
		{
		case IFF_PNM:
			writePnmFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),file);
			break;
		
		#if IMAGES_CONFIG_HAVE_PNG
		case IFF_PNG:
			writePngFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),file);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_JPEG
		case IFF_JPEG:
			writeJpegFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),file);
			break;
		#endif
		
		#if IMAGES_CONFIG_HAVE_TIFF
		case IFF_TIFF:
			throw std::runtime_error("Images::writeImageFile: Can not write TIFF images to already-open files");
		#endif
		
		default:
			throw std::runtime_error("Images::writeImageFile: Unsupported image file format");
		}
	}

void writeImageFile(const RGBImage& image,const char* imageFileName)
	{
	try
		{
		/* Determine the type of the given image file: */
		ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
		
		/* Delegate to specific readers based on the given format: */
		switch(imageFileFormat)
			{
			case IFF_PNM:
				writePnmFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				writePngFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
		
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				writeJpegFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				writeTiffFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),imageFileName);
				break;
			#endif
			
			default:
				throw std::runtime_error("Unsupported image file format");
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::writeImageFile: Error %s while writing image file %s",err.what(),imageFileName);
		}
	}

void writeImageFile(const RGBImage& image,const IO::Directory& directory,const char* imageFileName)
	{
	try
		{
		/* Determine the type of the given image file: */
		ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
		
		/* Delegate to specific readers based on the given format: */
		switch(imageFileFormat)
			{
			case IFF_PNM:
				writePnmFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*directory.openFile(imageFileName,IO::File::WriteOnly));
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				writePngFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*directory.openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				writeJpegFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),*directory.openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				writeTiffFile(image.getWidth(),image.getHeight(),image.getPixels()[0].getRgba(),directory.getPath(imageFileName).c_str());
				break;
			#endif
			
			default:
				throw std::runtime_error("Unsupported image file format");
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::writeImageFile: Error %s while writing image file %s",err.what(),directory.getPath(imageFileName).c_str());
		}
	}

void writeImageFile(unsigned int width,unsigned int height,const unsigned char* image,const char* imageFileName)
	{
	try
		{
		/* Determine the type of the given image file: */
		ImageFileFormat imageFileFormat=getImageFileFormat(imageFileName);
		
		/* Delegate to specific readers based on the given format: */
		switch(imageFileFormat)
			{
			case IFF_PNM:
				writePnmFile(width,height,image,*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			
			#if IMAGES_CONFIG_HAVE_PNG
			case IFF_PNG:
				writePngFile(width,height,image,*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_JPEG
			case IFF_JPEG:
				writeJpegFile(width,height,image,*IO::openFile(imageFileName,IO::File::WriteOnly));
				break;
			#endif
			
			#if IMAGES_CONFIG_HAVE_TIFF
			case IFF_TIFF:
				writeTiffFile(width,height,image,imageFileName);
				break;
			#endif
			
			default:
				throw std::runtime_error("Unsupported image file format");
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("Images::writeImageFile: Error %s while writing image file %s",err.what(),imageFileName);
		}
	}

}
