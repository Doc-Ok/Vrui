/***********************************************************************
ImageFileFormats - Types and functions to represent image file formats
handled by the Image Handling Library.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef IMAGES_IMAGEFILEFORMATS_INCLUDED
#define IMAGES_IMAGEFILEFORMATS_INCLUDED

namespace Images {

enum ImageFileFormat // Enumerated type for potentially supported image file formats
	{
	IFF_UNKNOWN=0,
	IFF_PNM, // Any flavor of Portable AnyMap images, always supported
	IFF_BIL, // BIL (Band Interleaved Line), BIP (Band Interleaved Pixel), or BSQ (Band SeQuential) images, always supported
	IFF_PNG, // PNG (Portable Network Graphics) images, supported if PNG library is present
	IFF_JPEG, // JPEG (Joint Photographic Experts Group)/JFIF (JPEG File Interchange Format) images, supported if JPEG library is present
	IFF_TIFF // TIFF (Tagged Image File Format) images, supported if TIFF library is present
	};

ImageFileFormat getImageFileFormat(const char* imageFileName); // Detects the file format of an image file based on its file name extension

}

#endif
