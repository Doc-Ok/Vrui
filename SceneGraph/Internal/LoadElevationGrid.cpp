/***********************************************************************
LoadElevationGrid - Function to load an elevation grid's height values
from an external file.
Copyright (c) 2010-2019 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/Internal/LoadElevationGrid.h>

#include <utility>
#include <string>
#include <Misc/SizedTypes.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <IO/ValueSource.h>
#include <Images/Config.h>
#include <Images/BaseImage.h>
#include <Images/ImageFileFormats.h>
#include <Images/ReadBILImage.h>
#if IMAGES_CONFIG_HAVE_TIFF
#include <Images/GeoTIFFMetadata.h>
#include <Images/ReadTIFFImage.h>
#endif
#include <Images/ReadImageFile.h>
#include <SceneGraph/ElevationGridNode.h>

namespace SceneGraph {

namespace {

/****************************************************************************
Helper function to extract elevation data from generic single-channel images:
****************************************************************************/

template <class ImageScalarParam>
inline
void
copyImageGrid(
	const Images::BaseImage& image,
	std::vector<Scalar>& heights)
	{
	/* Stuff all image pixels into the given vector: */
	size_t numPixels=size_t(image.getSize(1))*size_t(image.getSize(0));
	heights.reserve(numPixels);
	const ImageScalarParam* iPtr=static_cast<const ImageScalarParam*>(image.getPixels());
	for(size_t i=numPixels;i>0;--i,++iPtr)
		heights.push_back(Scalar(*iPtr));
	}

void installImageGrid(ElevationGridNode& node,Images::BaseImage& image)
	{
	/* Convert the image to single channel: */
	image=image.toGrey().dropAlpha();
	
	/* Copy the grid: */
	std::vector<Scalar> heights;
	switch(image.getScalarType())
		{
		case GL_BYTE:
			copyImageGrid<GLbyte>(image,heights);
			break;
		
		case GL_UNSIGNED_BYTE:
			copyImageGrid<GLubyte>(image,heights);
			break;
		
		case GL_SHORT:
			copyImageGrid<GLshort>(image,heights);
			break;
		
		case GL_UNSIGNED_SHORT:
			copyImageGrid<GLushort>(image,heights);
			break;
		
		case GL_INT:
			copyImageGrid<GLint>(image,heights);
			break;
		
		case GL_UNSIGNED_INT:
			copyImageGrid<GLuint>(image,heights);
			break;
		
		case GL_FLOAT:
			copyImageGrid<GLfloat>(image,heights);
			break;
		
		case GL_DOUBLE:
			copyImageGrid<GLdouble>(image,heights);
			break;
		
		default:
			throw std::runtime_error("SceneGraph::loadElevationGrid: Source image has unsupported pixel type");
		}
	
	/* Install the height field: */
	node.xDimension.setValue(image.getWidth());
	node.zDimension.setValue(image.getHeight());
	std::swap(node.height.getValues(),heights);
	}

void loadBILGrid(ElevationGridNode& node)
	{
	/* Load an elevation grid from a BIL file and retrieve its metadata: */
	Images::BILMetadata metadata;
	Images::BaseImage image=Images::readGenericBILImage(*node.baseDirectory,node.heightUrl.getValue(0).c_str(),&metadata);
	
	/* Install the elevation grid: */
	installImageGrid(node,image);
	
	/* Apply the BIL file's metadata: */
	if(metadata.haveMap&&!(node.propMask&0x1U))
		{
		Point origin=node.origin.getValue();
		origin[0]=Scalar(metadata.map[0]);
		int y=node.heightIsY.getValue()?2:1;
		origin[y]=Scalar(metadata.map[1]-double(image.getHeight()-1)*metadata.dim[1]);
		node.origin.setValue(origin);
		}
	if(metadata.haveDim)
		{
		if(!(node.propMask&0x2U))
			node.xSpacing.setValue(Scalar(metadata.dim[0]));
		if(!(node.propMask&0x4U))
			node.zSpacing.setValue(Scalar(metadata.dim[1]));
		}
	if(!(node.propMask&0x8U))
		{
		node.removeInvalids.setValue(metadata.haveNoData);
		if(metadata.haveNoData)
			node.invalidHeight.setValue(Scalar(metadata.noData));
		}
	}

#if IMAGES_CONFIG_HAVE_TIFF

void loadTIFFGrid(ElevationGridNode& node)
	{
	/* Load an elevation grid from a TIFF file and retrieve its metadata: */
	Images::GeoTIFFMetadata metadata;
	Images::BaseImage image=Images::readGenericTIFFImage(*node.baseDirectory->openFile(node.heightUrl.getValue(0).c_str()),&metadata);
	
	/* Install the elevation grid: */
	installImageGrid(node,image);
	
	/* Apply the GeoTIFF file's metadata: */
	if(metadata.haveMap&&!(node.propMask&0x1U))
		{
		Point origin=node.origin.getValue();
		origin[0]=Scalar(metadata.map[0]);
		int y=node.heightIsY.getValue()?2:1;
		origin[y]=Scalar(metadata.map[1]-double(image.getHeight()-1)*metadata.dim[1]);
		node.origin.setValue(origin);
		}
	if(metadata.haveDim)
		{
		if(!(node.propMask&0x2U))
			node.xSpacing.setValue(Scalar(metadata.dim[0]));
		if(!(node.propMask&0x4U))
			node.zSpacing.setValue(Scalar(metadata.dim[1]));
		}
	if(!(node.propMask&0x8U))
		{
		node.removeInvalids.setValue(metadata.haveNoData);
		if(metadata.haveNoData)
			node.invalidHeight.setValue(Scalar(metadata.noData));
		}
	}

#endif

void loadAGRGrid(ElevationGridNode& node)
	{
	/* Open the grid file: */
	IO::ValueSource grid(node.baseDirectory->openFile(node.heightUrl.getValue(0).c_str()));
	grid.skipWs();
	
	/* Read the grid header: */
	unsigned int gridSize[2]={0,0};
	double gridOrigin[2]={0.0,0.0};
	double cellSize=1.0;
	double nodata=9999.0;
	bool ok=grid.readString()=="ncols";
	if(ok)
		gridSize[0]=grid.readUnsignedInteger();
	ok=ok&&grid.readString()=="nrows";
	if(ok)
		gridSize[1]=grid.readUnsignedInteger();
	ok=ok&&grid.readString()=="xllcorner";
	if(ok)
		gridOrigin[0]=grid.readNumber();
	ok=ok&&grid.readString()=="yllcorner";
	if(ok)
		gridOrigin[1]=grid.readNumber();
	ok=ok&&grid.readString()=="cellsize";
	if(ok)
		cellSize=grid.readNumber();
	ok=ok&&grid.readString()=="NODATA_value";
	if(ok)
		nodata=grid.readNumber();
	if(!ok)
		Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s is not an ARC/INFO ASCII grid",node.heightUrl.getValue(0).c_str());
	
	/* Read the grid: */
	std::vector<Scalar> heights;
	heights.reserve(size_t(gridSize[0])*size_t(gridSize[1]));
	for(size_t i=size_t(gridSize[0])*size_t(gridSize[1]);i>0;--i)
		heights.push_back(Scalar(0));
	for(unsigned int y=gridSize[1];y>0;--y)
		for(unsigned int x=0;x<gridSize[0];++x)
			heights[(y-1)*gridSize[0]+x]=grid.readNumber();
	
	/* Install the height field: */
	if(!(node.propMask&0x1U))
		{
		Point origin=node.origin.getValue();
		origin[0]=Scalar(gridOrigin[0]+cellSize*Scalar(0.5));
		if(node.heightIsY.getValue())
			origin[2]=Scalar(gridOrigin[1]+cellSize*Scalar(0.5));
		else
			origin[1]=Scalar(gridOrigin[1]+cellSize*Scalar(0.5));
		node.origin.setValue(origin);
		}
	node.xDimension.setValue(gridSize[0]);
	if(!(node.propMask&0x2U))
		node.xSpacing.setValue(cellSize);
	node.zDimension.setValue(gridSize[1]);
	if(!(node.propMask&0x4U))
		node.zSpacing.setValue(cellSize);
	std::swap(node.height.getValues(),heights);
	
	if(!(node.propMask&0x8U))
		{
		/* Set the node's invalid removal flag and invalid height value: */
		node.removeInvalids.setValue(true);
		node.invalidHeight.setValue(nodata);
		}
	}

template <class ValueParam>
inline
void
loadRawGrid(
	ElevationGridNode& node,
	Misc::Endianness endianness)
	{
	/* Open the grid file: */
	IO::FilePtr gridFile=node.baseDirectory->openFile(node.heightUrl.getValue(0).c_str());
	gridFile->setEndianness(endianness);
	
	/* Read the grid: */
	int width=node.xDimension.getValue();
	int height=node.zDimension.getValue();
	std::vector<Scalar> heights;
	heights.reserve(size_t(height)*size_t(width));
	ValueParam* row=new ValueParam[width];
	for(int y=0;y<height;++y)
		{
		/* Read a row of values in the file format: */
		gridFile->read(row,width);
		
		/* Convert the row values to elevation grid format: */
		for(int x=0;x<width;++x)
			heights.push_back(Scalar(row[x]));
		}
	delete[] row;
	
	/* Install the height field: */
	std::swap(node.height.getValues(),heights);
	}

inline bool startsWith(std::string::const_iterator string,std::string::const_iterator stringEnd,const char* prefix)
	{
	while(string!=stringEnd&&*prefix!='\0')
		{
		if(*string!=*prefix)
			return false;
		++string;
		++prefix;
		}
	return *prefix=='\0';
	}

}

void loadElevationGrid(ElevationGridNode& node)
	{
	/* Determine the format of the height file: */
	if(node.heightUrlFormat.getNumValues()>=1&&node.heightUrlFormat.getValue(0)=="BIL")
		{
		/* Load an elevation grid in BIL/BIP/BSQ format: */
		loadBILGrid(node);
		}
	else if(node.heightUrlFormat.getNumValues()>=1&&node.heightUrlFormat.getValue(0)=="ARC/INFO ASCII GRID")
		{
		/* Load an elevation grid in ARC/INFO ASCII GRID format: */
		loadAGRGrid(node);
		}
	else if(node.heightUrlFormat.getNumValues()>=1&&startsWith(node.heightUrlFormat.getValue(0).begin(),node.heightUrlFormat.getValue(0).end(),"RAW "))
		{
		std::string::const_iterator sIt=node.heightUrlFormat.getValue(0).begin()+4;
		std::string::const_iterator sEnd=node.heightUrlFormat.getValue(0).end();
		std::string format;
		while(sIt!=sEnd&&!isspace(*sIt))
			{
			format.push_back(*sIt);
			++sIt;
			}
		while(sIt!=sEnd&&isspace(*sIt))
			++sIt;
		std::string endiannessCode;
		while(sIt!=sEnd&&!isspace(*sIt))
			{
			endiannessCode.push_back(*sIt);
			++sIt;
			}
		Misc::Endianness endianness=Misc::HostEndianness;
		if(endiannessCode=="LE")
			endianness=Misc::LittleEndian;
		else if(endiannessCode=="BE")
			endianness=Misc::BigEndian;
		else if(!endiannessCode.empty())
			Misc::throwStdErr("SceneGraph::loadElevationGrid: Unknown endianness %s",endiannessCode.c_str());
		
		/* Load an elevation grid in raw format, containing values of the requested type: */
		if(format=="UINT8")
			loadRawGrid<Misc::UInt8>(node,endianness);
		else if(format=="SINT8")
			loadRawGrid<Misc::SInt8>(node,endianness);
		else if(format=="UINT16")
			loadRawGrid<Misc::UInt16>(node,endianness);
		else if(format=="SINT16")
			loadRawGrid<Misc::SInt16>(node,endianness);
		else if(format=="UINT32")
			loadRawGrid<Misc::UInt32>(node,endianness);
		else if(format=="SINT32")
			loadRawGrid<Misc::SInt32>(node,endianness);
		else if(format=="FLOAT32")
			loadRawGrid<Misc::Float32>(node,endianness);
		else if(format=="FLOAT64")
			loadRawGrid<Misc::Float64>(node,endianness);
		else
			Misc::throwStdErr("SceneGraph::loadElevationGrid: Unknown raw data type %s",format.c_str());
		}
	else
		{
		/* Determine the height file name's image file format: */
		Images::ImageFileFormat heightIff=Images::getImageFileFormat(node.heightUrl.getValue(0).c_str());
		
		if(heightIff==Images::IFF_BIL)
			{
			/* Load an elevation grid in BIL format: */
			loadBILGrid(node);
			}
		#if IMAGES_CONFIG_HAVE_TIFF
		else if(heightIff==Images::IFF_TIFF)
			{
			/* Load an elevation grid in TIFF format: */
			loadTIFFGrid(node);
			}
		#endif
		else if(Images::canReadImageFileFormat(heightIff))
			{
			/* Load the elevation grid as an image file with height defined by luminance: */
			Images::BaseImage image=Images::readGenericImageFile(*node.baseDirectory,node.heightUrl.getValue(0).c_str());
			
			/* Install the elevation grid: */
			installImageGrid(node,image);
			}
		else
			Misc::throwStdErr("SceneGraph::loadElevationGrid: File %s has unknown format",node.heightUrl.getValue(0).c_str());
		}
	}

}
