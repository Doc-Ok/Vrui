/***********************************************************************
ImageFileFormats - Types and functions to represent image file formats
handled by the Image Handling Library.
Copyright (c) 2018-2019 Oliver Kreylos

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

#include <Images/ImageFileFormats.h>

#include <ctype.h>
#include <string.h>
#include <Misc/FileNameExtensions.h>

namespace Images {

ImageFileFormat getImageFileFormat(const char* imageFileName)
	{
	/* Retrieve the file name extension: */
	const char* ext=Misc::getExtension(imageFileName);
	int extLen=strlen(ext);
	if(strcasecmp(ext,".gz")==0)
		{
		/* Strip the gzip extension and try again: */
		const char* gzExt=ext;
		ext=Misc::getExtension(imageFileName,gzExt);
		extLen=gzExt-ext;
		}
	
	/* Try to determine image file format from file name extension: */
	ImageFileFormat iff=IFF_UNKNOWN;
	if(extLen==4
	   &&ext[0]=='.'
	   &&tolower(ext[1])=='p'
	   &&(tolower(ext[2])=='b'
	      ||tolower(ext[2])=='g'
	      ||tolower(ext[2])=='n'
	      ||tolower(ext[2])=='p')
	   &&tolower(ext[3])=='m') // It's a Portable AnyMap image
		iff=IFF_PNM;
	else if(extLen==4
	        &&ext[0]=='.'
	        &&tolower(ext[1])=='b'
	        &&((tolower(ext[2])=='i'
	            &&(tolower(ext[3])=='p'
	               ||tolower(ext[3])=='l'))
	           ||(tolower(ext[2])=='s'
	              &&tolower(ext[3])=='q'))) // It's a BIP/BIL/BSQ image
		iff=IFF_BIL;
	else if(extLen==4&&strncasecmp(ext,".img",extLen)==0) // Chances are it's a BIP/BIL/BSQ image
		iff=IFF_BIL;
	else if(extLen==4&&strncasecmp(ext,".png",extLen)==0) // It's a PNG image
		iff=IFF_PNG;
	else if((extLen==4&&strncasecmp(ext,".jpg",extLen)==0)
	        ||(extLen==5&&strncasecmp(ext,".jpeg",extLen)==0)) // It's a JPEG image
		iff=IFF_JPEG;
	else if((extLen==4&&strncasecmp(ext,".tif",extLen)==0)
	        ||(extLen==5&&strncasecmp(ext,".tiff",extLen)==0)) // It's a TIFF image
		iff=IFF_TIFF;
	
	return iff;
	}

}
