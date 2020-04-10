/***********************************************************************
ImageExtractorYpCbCr - Class to extract images from video frames in
uncompressed three-component Y'CbCr format.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Basic Video Library (Video).

The Basic Video Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The Basic Video Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Basic Video Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Video/ImageExtractorYpCbCr.h>

#include <string.h>
#include <Video/FrameBuffer.h>
#include <Video/Colorspaces.h>

namespace Video {

/*************************************
Methods of class ImageExtractorYpCbCr:
*************************************/

ImageExtractorYpCbCr::ImageExtractorYpCbCr(const unsigned int sSize[2])
	{
	/* Copy the frame size: */
	for(int i=0;i<2;++i)
		size[i]=sSize[i];
	}

void ImageExtractorYpCbCr::extractGrey(const FrameBuffer* frame,void* image)
	{
	/* Convert the frame's Y' channel to Y: */
	const unsigned char* yPtr=frame->start;
	unsigned char* gPtr=static_cast<unsigned char*>(image);
	for(unsigned int y=0;y<size[1];++y)
		for(unsigned int x=0;x<size[0];++x,yPtr+=3,++gPtr)
			{
			/* Convert from Y' to Y: */
			if(*yPtr<=16)
				*gPtr=0;
			else if(*yPtr>=236)
				*gPtr=255;
			else
				*gPtr=(unsigned char)(((int(yPtr[0])-16)*256)/220);
			}
	}

void ImageExtractorYpCbCr::extractRGB(const FrameBuffer* frame,void* image)
	{
	/* Convert each pixel from Y'CbCr to RGB: */
	const unsigned char* ypcbcrPtr=frame->start;
	unsigned char* rgbPtr=static_cast<unsigned char*>(image);
	for(unsigned int y=0;y<size[1];++y)
		for(unsigned int x=0;x<size[0];++x,ypcbcrPtr+=3,rgbPtr+=3)
			{
			/* Convert from Y'CbCr to RGB: */
			Video::ypcbcrToRgb(ypcbcrPtr,rgbPtr);
			}
	}

void ImageExtractorYpCbCr::extractYpCbCr(const FrameBuffer* frame,void* image)
	{
	/* Copy the entire image buffer: */
	memcpy(image,frame->start,size[1]*size[0]*3);
	}

void ImageExtractorYpCbCr::extractYpCbCr420(const FrameBuffer* frame,void* yp,unsigned int ypStride,void* cb,unsigned int cbStride,void* cr,unsigned int crStride)
	{
	/* Process pixels in 2x2 blocks: */
	ptrdiff_t fStride=size[0]*3;
	const unsigned char* fRowPtr=frame->start+(size[1]-1)*fStride;
	unsigned char* ypRowPtr=static_cast<unsigned char*>(yp);
	unsigned char* cbRowPtr=static_cast<unsigned char*>(cb);
	unsigned char* crRowPtr=static_cast<unsigned char*>(cr);
	for(unsigned int y=0;y<size[1];y+=2)
		{
		const unsigned char* fPtr0=fRowPtr;
		fRowPtr-=fStride;
		const unsigned char* fPtr1=fRowPtr;
		unsigned char* ypPtr=ypRowPtr;
		unsigned char* cbPtr=cbRowPtr;
		unsigned char* crPtr=crRowPtr;
		for(unsigned int x=0;x<size[0];x+=2)
			{
			/* Subsample and store the Y'CbCr components: */
			ypPtr[0]=fPtr0[0];
			ypPtr[1]=fPtr0[3];
			ypPtr[ypStride]=fPtr1[0];
			ypPtr[ypStride+1]=fPtr1[3];
			*cbPtr=(unsigned char)((int(fPtr0[1])+int(fPtr0[4])+int(fPtr1[1])+int(fPtr1[4])+2)>>2);
			*crPtr=(unsigned char)((int(fPtr0[2])+int(fPtr0[5])+int(fPtr1[2])+int(fPtr1[5])+2)>>2);
			
			/* Go to the next pixel: */
			fPtr0+=3*2;
			fPtr1+=3*2;
			ypPtr+=2;
			++cbPtr;
			++crPtr;
			}
		
		/* Go to the next pixel row: */
		fRowPtr-=fStride;
		ypRowPtr+=ypStride*2;
		cbRowPtr+=cbStride;
		crRowPtr+=crStride;
		}
	}

}
