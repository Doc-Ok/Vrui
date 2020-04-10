/***********************************************************************
ReadPlyFile - Helper function to read a 3D polygon file in PLY format
into a list of shape nodes.
Copyright (c) 2018 Oliver Kreylos

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

#include <SceneGraph/Internal/ReadPlyFile.h>

#include <stdexcept>
#include <Misc/Autopointer.h>
#include <Misc/ThrowStdErr.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/ValueSource.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/PointSetNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/MeshFileNode.h>
#include <SceneGraph/Internal/PlyFileStructures.h>

namespace SceneGraph {

namespace {

/****************
Helper functions:
****************/

template <class PLYFileParam>
void readPlyFileElements(const PLYFileHeader& header,PLYFileParam& ply,MeshFileNode& node)
	{
	/* Collect attribute and geometry nodes extracted from the PLY file: */
	ColorNodePointer color;
	NormalNodePointer normal;
	CoordinateNodePointer coord;
	Misc::Autopointer<IndexedFaceSetNode> faceSet;
	
	/* Process all PLY file elements in order: */
	for(size_t elementIndex=0;elementIndex<header.getNumElements();++elementIndex)
		{
		/* Get the next element: */
		const PLYElement& element=header.getElement(elementIndex);
		
		/* Check if it's the vertex or face element: */
		if(element.isElement("vertex")&&element.getNumValues()>0)
			{
			/* Get the indices of all supported vertex properties: */
			const char* colorNames[3]={"red","green","blue"};
			unsigned int colorIndex[3];
			int colorMask=0x0;
			bool colorIsUInt=false;
			Color::Scalar colorScale(1);
			const char* normalNames[3]={"nx","ny","nz"};
			unsigned int normalIndex[3];
			int normalMask=0x0;
			const char* coordNames[3]={"x","y","z"};
			unsigned int coordIndex[3];
			int coordMask=0x0;
			unsigned int propertyIndex=0;
			for(PLYElement::PropertyList::const_iterator pIt=element.propertiesBegin();pIt!=element.propertiesEnd();++pIt,++propertyIndex)
				if(pIt->getPropertyType()==PLYProperty::SCALAR)
					{
					/* Check for any of the supported properties: */
					for(int i=0;i<3;++i)
						if(pIt->getName()==colorNames[i])
							{
							colorIndex[i]=propertyIndex;
							colorMask|=0x1<<i;
							if(colorMask==0x7)
								{
								/* Determine the color component scalar type: */
								switch(pIt->getScalarType())
									{
									case PLY_UINT8:
										colorIsUInt=true;
										colorScale=Color::Scalar(1)/Color::Scalar(255);
										break;
									
									case PLY_UINT16:
										colorIsUInt=true;
										colorScale=Color::Scalar(1)/Color::Scalar(65535);
										break;
									
									case PLY_FLOAT32:
									case PLY_FLOAT64:
										colorIsUInt=false;
										break;
									
									default:
										/* Ignore color values due to unsupported scalar type: */
										colorMask=0x0;
									}
								}
							}
					for(int i=0;i<3;++i)
						if(pIt->getName()==normalNames[i])
							{
							normalIndex[i]=propertyIndex;
							normalMask|=0x1<<i;
							}
					for(int i=0;i<3;++i)
						if(pIt->getName()==coordNames[i])
							{
							coordIndex[i]=propertyIndex;
							coordMask|=0x1<<i;
							}
					}
			
			/* Check that the PLY file at least defines vertex positions: */
			if(coordMask!=0x7)
				throw std::runtime_error("Vertex element does not contain x, y, z properties");
			
			/* Create property nodes for defined properties: */
			if(colorMask==0x7)
				color=new ColorNode;
			if(normalMask==0x7)
				normal=new NormalNode;
			coord=new CoordinateNode;
			
			/* Read vertices based on their defined properties: */
			if(colorMask==0x7)
				{
				/* Check if colors are stored as unsigned integers: */
				if(colorIsUInt)
					{
					if(normalMask==0x7)
						{
						/* Read vertex colors, normal vectors, and positions: */
						MFColor::ValueList& colors=color->color.getValues();
						colors.reserve(element.getNumValues());
						MFVector::ValueList& normals=normal->vector.getValues();
						normals.reserve(element.getNumValues());
						MFPoint::ValueList& coords=coord->point.getValues();
						coords.reserve(element.getNumValues());
						PLYElement::Value vertexValue(element);
						for(size_t i=0;i<element.getNumValues();++i)
							{
							/* Read vertex element from file: */
							vertexValue.read(ply);
							
							/* Extract vertex color: */
							Color color;
							for(int i=0;i<3;++i)
								color[i]=Color::Scalar(vertexValue.getValue(colorIndex[i]).getScalar()->getUnsignedInt())*colorScale;
							colors.push_back(color);
							
							/* Extract vertex normal vector: */
							Vector normal;
							for(int i=0;i<3;++i)
								normal[i]=Scalar(vertexValue.getValue(normalIndex[i]).getScalar()->getDouble());
							normals.push_back(normal);
							
							/* Extract vertex position: */
							Point coord;
							for(int i=0;i<3;++i)
								coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
							coords.push_back(coord);
							}
						}
					else
						{
						/* Read vertex colors and positions: */
						MFColor::ValueList& colors=color->color.getValues();
						colors.reserve(element.getNumValues());
						MFPoint::ValueList& coords=coord->point.getValues();
						coords.reserve(element.getNumValues());
						PLYElement::Value vertexValue(element);
						for(size_t i=0;i<element.getNumValues();++i)
							{
							/* Read vertex element from file: */
							vertexValue.read(ply);
							
							/* Extract vertex color: */
							Color color;
							for(int i=0;i<3;++i)
								color[i]=Color::Scalar(vertexValue.getValue(colorIndex[i]).getScalar()->getUnsignedInt())*colorScale;
							colors.push_back(color);
							
							/* Extract vertex position: */
							Point coord;
							for(int i=0;i<3;++i)
								coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
							coords.push_back(coord);
							}
						}
					}
				else
					{
					if(normalMask==0x7)
						{
						/* Read vertex colors, normal vectors, and positions: */
						MFColor::ValueList& colors=color->color.getValues();
						colors.reserve(element.getNumValues());
						MFVector::ValueList& normals=normal->vector.getValues();
						normals.reserve(element.getNumValues());
						MFPoint::ValueList& coords=coord->point.getValues();
						coords.reserve(element.getNumValues());
						PLYElement::Value vertexValue(element);
						for(size_t i=0;i<element.getNumValues();++i)
							{
							/* Read vertex element from file: */
							vertexValue.read(ply);
							
							/* Extract vertex color: */
							Color color;
							for(int i=0;i<3;++i)
								color[i]=Color::Scalar(vertexValue.getValue(colorIndex[i]).getScalar()->getDouble());
							colors.push_back(color);
							
							/* Extract vertex normal vector: */
							Vector normal;
							for(int i=0;i<3;++i)
								normal[i]=Scalar(vertexValue.getValue(normalIndex[i]).getScalar()->getDouble());
							normals.push_back(normal);
							
							/* Extract vertex position: */
							Point coord;
							for(int i=0;i<3;++i)
								coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
							coords.push_back(coord);
							}
						}
					else
						{
						/* Read vertex colors and positions: */
						MFColor::ValueList& colors=color->color.getValues();
						colors.reserve(element.getNumValues());
						MFPoint::ValueList& coords=coord->point.getValues();
						coords.reserve(element.getNumValues());
						PLYElement::Value vertexValue(element);
						for(size_t i=0;i<element.getNumValues();++i)
							{
							/* Read vertex element from file: */
							vertexValue.read(ply);
							
							/* Extract vertex color: */
							Color color;
							for(int i=0;i<3;++i)
								color[i]=Color::Scalar(vertexValue.getValue(colorIndex[i]).getScalar()->getDouble());
							colors.push_back(color);
							
							/* Extract vertex position: */
							Point coord;
							for(int i=0;i<3;++i)
								coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
							coords.push_back(coord);
							}
						}
					}
				}
			else
				{
				if(normalMask==0x7)
					{
					/* Read vertex normal vectors and positions: */
					MFVector::ValueList& normals=normal->vector.getValues();
					normals.reserve(element.getNumValues());
					MFPoint::ValueList& coords=coord->point.getValues();
					coords.reserve(element.getNumValues());
					PLYElement::Value vertexValue(element);
					for(size_t i=0;i<element.getNumValues();++i)
						{
						/* Read vertex element from file: */
						vertexValue.read(ply);
						
						/* Extract vertex normal vector: */
						Vector normal;
						for(int i=0;i<3;++i)
							normal[i]=Scalar(vertexValue.getValue(normalIndex[i]).getScalar()->getDouble());
						normals.push_back(normal);
						
						/* Extract vertex position: */
						Point coord;
						for(int i=0;i<3;++i)
							coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
						coords.push_back(coord);
						}
					}
				else
					{
					/* Read vertex positions: */
					MFPoint::ValueList& coords=coord->point.getValues();
					coords.reserve(element.getNumValues());
					PLYElement::Value vertexValue(element);
					for(size_t i=0;i<element.getNumValues();++i)
						{
						/* Read vertex element from file: */
						vertexValue.read(ply);
						
						/* Extract vertex position: */
						Point coord;
						for(int i=0;i<3;++i)
							coord[i]=Scalar(vertexValue.getValue(coordIndex[i]).getScalar()->getDouble());
						coords.push_back(coord);
						}
					}
				}
			
			/* Finalize the property nodes: */
			if(colorMask==0x7)
				color->update();
			if(normalMask==0x7)
				normal->update();
			coord->update();
			}
		else if(element.isElement("face")&&element.getNumValues()>0)
			{
			/* Create an indexed face set node: */
			faceSet=new IndexedFaceSetNode;
			MFInt::ValueList& coordIndices=faceSet->coordIndex.getValues();
			coordIndices.reserve(element.getNumValues()*4); // Educated guess
			
			/* Read all face vertex indices: */
			PLYElement::Value faceValue(element);
			unsigned int vertexIndicesIndex=element.getPropertyIndex("vertex_indices");
			if(vertexIndicesIndex>=element.getNumProperties())
				throw std::runtime_error("Face element does not contain vertex_indices property");
			for(size_t i=0;i<element.getNumValues();++i)
				{
				/* Read face element from file: */
				faceValue.read(ply);
				
				/* Extract vertex indices from face element: */
				unsigned int numFaceVertices=faceValue.getValue(vertexIndicesIndex).getListSize()->getUnsignedInt();
				for(unsigned int j=0;j<numFaceVertices;++j)
					coordIndices.push_back(int(faceValue.getValue(vertexIndicesIndex).getListElement(j)->getUnsignedInt()));
				coordIndices.push_back(-1);
				}
			}
		else
			{
			/* Skip the entire element: */
			skipElement(element,ply);
			}
		}
	
	/* Check if the PLY file defined vertex coordinates: */
	if(coord!=0)
		{
		/* Create a new shape node: */
		ShapeNodePointer shape=new ShapeNode;
		
		/* Set the shape node's appearance to the mesh file node's appearance: */
		shape->appearance.setValue(node.appearance.getValue());
		
		/* Check if the PLY file defined faces: */
		if(faceSet!=0)
			{
			/* Attach the property nodes to the face set node: */
			faceSet->color.setValue(color);
			faceSet->normal.setValue(normal);
			faceSet->coord.setValue(coord);
			
			/* Set up face set parameters: */
			faceSet->colorPerVertex.setValue(true);
			faceSet->normalPerVertex.setValue(true);
			
			/* Copy face set parameters from the mesh file node: */
			faceSet->ccw.setValue(node.ccw.getValue());
			faceSet->solid.setValue(node.solid.getValue());
			faceSet->creaseAngle.setValue(node.creaseAngle.getValue());
			
			/* Finalize the face set and set it as the shape's geometry node: */
			faceSet->update();
			shape->geometry.setValue(faceSet);
			}
		else
			{
			/* Create a point set node to render the vertices read from the PLY file: */
			Misc::Autopointer<PointSetNode> pointSet=new PointSetNode;
			
			/* Attach the property nodes to the point set node: */
			pointSet->color.setValue(color);
			pointSet->coord.setValue(coord);
			
			/* Copy point set parameters from the mesh file node: */
			pointSet->pointSize.setValue(node.pointSize.getValue());
			
			/* Finalize the point set and set it as the shape's geometry node: */
			pointSet->update();
			shape->geometry.setValue(pointSet);
			}
		
		/* Finalize the shape node and add it to the mesh file node's shape list: */
		shape->update();
		node.addShape(shape);
		}
	}

}

void readPlyFile(const IO::Directory& directory,const std::string& fileName,MeshFileNode& node)
	{
	/* Open the input file: */
	IO::FilePtr plyFile(directory.openFile(fileName.c_str()));
	
	/* Read the PLY file's header: */
	PLYFileHeader header(*plyFile);
	if(!header.isValid())
		Misc::throwStdErr("SceneGraph::readPlyFile: File %s is not a valid PLY file",fileName.c_str());
	
	try
		{
		/* Read the PLY file in ASCII or binary mode: */
		if(header.getFileType()==PLYFileHeader::Ascii)
			{
			/* Attach a value source to the PLY file: */
			IO::ValueSource ply(plyFile);
			
			/* Read the PLY file in ASCII mode: */
			readPlyFileElements(header,ply,node);
			}
		else
			{
			/* Set the PLY file's endianness: */
			plyFile->setEndianness(header.getFileEndianness());
			
			/* Read the PLY file in binary mode: */
			readPlyFileElements(header,*plyFile,node);
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Wrap and re-throw the exception: */
		Misc::throwStdErr("SceneGraph::readPlyFile: Error %s while reading PLY file %s",err.what(),fileName.c_str());
		}
	}

}
