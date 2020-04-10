/***********************************************************************
ReadImageFile - Functions to read generic images from image files in a
variety of formats.
Copyright (c) 2005-2018 Oliver Kreylos

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

#ifndef IMAGES_READIMAGEFILE_INCLUDED
#define IMAGES_READIMAGEFILE_INCLUDED

#include <Images/ImageFileFormats.h>

/* Forward declarations: */
namespace IO {
class File;
class Directory;
}
namespace Images {
class BaseImage;
class RGBImage;
class RGBAImage;
}

namespace Images {

/* Functions to query image file format support: */
bool canReadImageFileFormat(ImageFileFormat imageFileFormat); // Returns true if the image reader supports the given image file format
inline bool canReadImageFileFormat(const char* imageFileName) // Convenience function
	{
	return canReadImageFileFormat(getImageFileFormat(imageFileName));
	}

/* Functions to read generic images from image files: */
BaseImage readGenericImageFile(IO::File& file,ImageFileFormat imageFileFormat); // Reads a generic image from an already-opened image file of the given format
BaseImage readGenericImageFile(const char* imageFileName); // Reads a generic image from the file of the given name
BaseImage readGenericImageFile(const IO::Directory& directory,const char* imageFileName); // Reads a generic image from the file of the given name relative to the given directory

/* Functions to read transparent images from cursor definition files: */
RGBAImage readCursorFile(IO::File& file,unsigned int nominalSize,unsigned int* hotspot =0); // Reads an RGBA image from an already-opened cursor file in Xcursor format
RGBAImage readCursorFile(const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot =0); // Reads an RGBA image from the Xcursor file of the given name
RGBAImage readCursorFile(const IO::Directory& directory,const char* cursorFileName,unsigned int nominalSize,unsigned int* hotspot =0); // Reads an RGBA image from the Xcursor file of the given name relative to the given directory

/* Deprecated functions to read images in specific formats from image files: */
RGBImage readImageFile(IO::File& file,ImageFileFormat imageFileFormat); // Reads an RGB image from an already-open file; auto-detects file format based on file name extension
RGBImage readImageFile(const char* imageFileName); // Ditto, but opens the given file itself relative to the current directory
RGBImage readImageFile(const IO::Directory& directory,const char* imageFileName); // Ditto, but opens the file itself relative to the given directory

RGBAImage readTransparentImageFile(IO::File& file,ImageFileFormat imageFileFormat); // Reads an RGB image with alpha layer from an already-open file; auto-detects file format based on file name extension
RGBAImage readTransparentImageFile(const char* imageFileName); // Ditto, but opens the given file itself relative to the current directory

}

#endif
