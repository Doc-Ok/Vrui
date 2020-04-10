/***********************************************************************
ReadObjFile - Helper function to read a 3D polygon file in Wavefront OBJ
format into a list of shape nodes.
Copyright (c) 2018-2019 Oliver Kreylos

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

#include <SceneGraph/Internal/ReadObjFile.h>

#include <ctype.h>
#include <Misc/StringPrintf.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <IO/Directory.h>
#include <IO/ValueSource.h>
#include <Math/Math.h>
#include <SceneGraph/TextureCoordinateNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/IndexedFaceSetNode.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/ImageTextureNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/MaterialLibraryNode.h>
#include <SceneGraph/ShapeNode.h>
#include <SceneGraph/MeshFileNode.h>
#include <SceneGraph/Internal/OBJValueSource.h>
#include <SceneGraph/Internal/ReadMtlFile.h>

namespace SceneGraph {

namespace {

class OBJFileReader // Helper class to maintain state while parsing an OBJ file
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<AppearanceNode*,IndexedFaceSetNode*> FaceSetMap; // Type for maps from appearance node pointers to indexed face sets using that appearance
	
	/* Elements: */
	IO::Directory& directory; // Base directory for relative URLs
	OBJValueSource objFile; // Value source representing the parsed OBJ file
	
	/* Property nodes collecting vertex properties: */
	TextureCoordinateNodePointer texCoord;
	MFTexCoord::ValueList& texCoords;
	int numTexCoords;
	ColorNodePointer color;
	MFColor::ValueList& colors;
	int numColors;
	NormalNodePointer normal;
	MFVector::ValueList& normals;
	int numNormals;
	CoordinateNodePointer coord;
	MFPoint::ValueList& coords;
	int numCoords;
	
	/* Temporary material library node: */
	MaterialLibraryNodePointer materialLibrary;
	
	/* Appearance node representing the current material properties: */
	AppearanceNodePointer currentAppearance;
	
	/* Geometry nodes collecting geometric primitives: */
	FaceSetMap faceSetMap; // Map of indexed face sets by appearances
	Misc::Autopointer<IndexedFaceSetNode> currentFaceSet;
	bool newFaceSet; // Flag indicating whether the current face set is not yet part of a shape node
	bool haveTexCoords;
	int lastTexCoordIndex;
	bool haveNormals;
	int lastNormalIndex;
	
	/* Output state: */
	MeshFileNode& node; // Mesh file node in which to collect created shapes
	
	/* Private methods: */
	void storeFaceSet(void) // Adds the current face set as a shape to the mesh file node
		{
		/* Bail out if there is no current face set: */
		if(currentFaceSet==0)
			return;
		
		if(newFaceSet)
			{
			/* Attach the current property nodes to the face set node: */
			if(haveTexCoords)
				currentFaceSet->texCoord.setValue(texCoord);
			if(numColors>0)
				currentFaceSet->color.setValue(color);
			if(haveNormals)
				currentFaceSet->normal.setValue(normal);
			currentFaceSet->coord.setValue(coord);
			
			/* Set up face set parameters, copying values from the mesh file node: */
			currentFaceSet->colorPerVertex.setValue(true);
			currentFaceSet->normalPerVertex.setValue(true);
			currentFaceSet->ccw.setValue(node.ccw.getValue());
			currentFaceSet->solid.setValue(node.solid.getValue());
			currentFaceSet->creaseAngle.setValue(node.creaseAngle.getValue());
			}
		
		/* Finalize the face set: */
		currentFaceSet->update();
		
		if(newFaceSet)
			{
			/* Create a new shape node: */
			ShapeNodePointer shape=new ShapeNode;
			
			/* Set the shape node's appearance to the current material properties: */
			shape->appearance.setValue(currentAppearance);
			
			/* Set the shape node's geometry to the current face set node: */
			shape->geometry.setValue(currentFaceSet);
			
			/* Finalize the shape node and add it to the mesh file node's representation: */
			shape->update();
			node.addShape(shape);
			
			/* If there is a current appearance node, add a mapping from it to the face set to the face set map: */
			if(currentAppearance!=0)
				faceSetMap[currentAppearance.getPointer()]=currentFaceSet.getPointer();
			}
		
		/* Reset face set state: */
		currentFaceSet=0;
		}
	
	/* Constructors and destructors: */
	public:
	OBJFileReader(IO::Directory& sDirectory,const std::string& fileName,MeshFileNode& sNode)
		:directory(sDirectory),
		 objFile(sDirectory,fileName),
		 texCoord(new TextureCoordinateNode),texCoords(texCoord->point.getValues()),numTexCoords(0),
		 color(new ColorNode),colors(color->color.getValues()),numColors(0),
		 normal(new NormalNode),normals(normal->vector.getValues()),numNormals(0),
		 coord(new CoordinateNode),coords(coord->point.getValues()),numCoords(0),
		 materialLibrary(sNode.materialLibrary.getValue()==0?new MaterialLibraryNode:0),
		 currentAppearance(sNode.appearance.getValue()),
		 faceSetMap(17),currentFaceSet(0),
		 node(sNode)
		{
		}
	
	/* Methods: */
	void parse(void) // Parses the OBJ file and creates shapes
		{
		/* Process the entire OBJ file: */
		while(!objFile.eof())
			{
			/* Parse the next tag: */
			if(objFile.peekc()=='v') // It's some type of vertex property
				{
				objFile.getChar();
				if(objFile.peekc()=='t') // Texture coordinate
					{
					objFile.readChar();
					
					/* Read texture coordinate components: */
					TexCoord tc=TexCoord::origin;
					for(int i=0;i<2&&!objFile.eol();++i)
						tc[i]=Scalar(objFile.readNumber());
					
					texCoords.push_back(tc);
					++numTexCoords;
					}
				else if(objFile.peekc()=='n') // Normal vector
					{
					objFile.readChar();
					
					/* Read normal vector components: */
					Vector n=Vector::zero;
					for(int i=0;i<3&&!objFile.eol();++i)
						n[i]=Scalar(objFile.readNumber());
					
					normals.push_back(n);
					++numNormals;
					}
				else if(objFile.peekc()==' ') // Vertex position
					{
					objFile.skipWs();
					
					/* Read vertex position components and optional vertex colors: */
					Scalar vc[6];
					vc[2]=vc[1]=vc[0]=Scalar(0);
					int numComponents;
					for(numComponents=0;numComponents<6&&!objFile.eol();++numComponents)
						vc[numComponents]=Scalar(objFile.readNumber());
					
					/* Check which vertex type this is: */
					if(numComponents==6)
						{
						/* Store a vertex position and a vertex color: */
						coords.push_back(Point(vc[0],vc[1],vc[2]));
						++numCoords;
						colors.push_back(Color(vc[3],vc[4],vc[5]));
						++numColors;
						}
					else
						{
						/* Store a vertex position, ignoring any homogeneous weights etc.: */
						coords.push_back(Point(vc[0],vc[1],vc[2]));
						++numCoords;
						}
					}
				}
			else if(objFile.peekc()=='f')
				{
				objFile.getChar();
				if(objFile.peekc()==' ') // Face definition
					{
					objFile.skipWs();
					
					/* Check whether this is the first face in a new face set: */
					if(currentFaceSet==0)
						{
						/* Check whether there is already a face set node compatible with the current appearance: */
						newFaceSet=true;
						if(currentAppearance!=0)
							{
							FaceSetMap::Iterator fsIt=faceSetMap.findEntry(currentAppearance.getPointer());
							if(!fsIt.isFinished())
								{
								/* Append this group's faces to the existing face set: */
								currentFaceSet=fsIt->getDest();
								newFaceSet=false;
								
								/* Check whether the existing face set uses texture coordinates and/or normal vectors: */
								haveTexCoords=currentFaceSet->texCoord.getValue()!=0;
								haveNormals=currentFaceSet->normal.getValue()!=0;
								}
							}
						
						if(newFaceSet)
							{
							/* Start a new face set: */
							currentFaceSet=new IndexedFaceSetNode;
							
							/***********************************************************
							Read the first vertex of the first face to determine whether
							the new face set will have texture coordinates and/or normal
							vectors:
							***********************************************************/
							
							/* Read a vertex position index: */
							int coordIndex=objFile.readInteger();
							currentFaceSet->coordIndex.appendValue(coordIndex>0?coordIndex-1:numCoords+coordIndex); // Negative indices count back from most recent
							
							/* Check for a texture coordinate index: */
							if((haveTexCoords=objFile.peekc()=='/'&&objFile.getCharAndPeekc()!='/'))
								{
								/* Read a texture coordinate index: */
								int texCoordIndex=objFile.readInteger();
								lastTexCoordIndex=texCoordIndex>0?texCoordIndex-1:numTexCoords+texCoordIndex; // Negative indices count back from most recent
								currentFaceSet->texCoordIndex.appendValue(lastTexCoordIndex);
								}
							
							/* Check for a normal vector index: */
							if((haveNormals=objFile.peekc()=='/'&&!objFile.isWs(objFile.getCharAndPeekc())))
								{
								/* Read a normal vector index: */
								int normalIndex=objFile.readInteger();
								lastNormalIndex=normalIndex>0?normalIndex-1:numNormals+normalIndex; // Negative indices count back from most recent
								currentFaceSet->normalIndex.appendValue(lastNormalIndex);
								}
							objFile.skipWs();
							}
						}
					
					/* Read face vertex definitions until the end of the line: */
					while(!objFile.eol())
						{
						/* Read a vertex position index: */
						int coordIndex=objFile.readInteger();
						currentFaceSet->coordIndex.appendValue(coordIndex>0?coordIndex-1:numCoords+coordIndex); // Negative indices count back from most recent
						
						/* Check for a texture coordinate index: */
						if(objFile.peekc()=='/'&&objFile.getCharAndPeekc()!='/')
							{
							/* Read a texture coordinate index: */
							int texCoordIndex=objFile.readInteger();
							lastTexCoordIndex=texCoordIndex>0?texCoordIndex-1:numTexCoords+texCoordIndex; // Negative indices count back from most recent
							}
						
						/* Check for a normal vector index: */
						if(objFile.peekc()=='/'&&!objFile.isWs(objFile.getCharAndPeekc()))
							{
							/* Read a normal vector index: */
							int normalIndex=objFile.readInteger();
							lastNormalIndex=normalIndex>0?normalIndex-1:numNormals+normalIndex; // Negative indices count back from most recent
							}
						objFile.skipWs();
						
						/* Store this vertex's texture coordinate and/or normal vector if the face set requires them: */
						if(haveTexCoords)
							currentFaceSet->texCoordIndex.appendValue(lastTexCoordIndex);
						if(haveNormals)
							currentFaceSet->normalIndex.appendValue(lastNormalIndex);
						}
					
					/* Finish the face: */
					if(haveTexCoords)
						currentFaceSet->texCoordIndex.appendValue(-1);
					if(haveNormals)
						currentFaceSet->normalIndex.appendValue(-1);
					currentFaceSet->coordIndex.appendValue(-1);
					}
				}
			else if(objFile.peekc()=='g')
				{
				objFile.getChar();
				if(objFile.peekc()==' ') // Group definition
					{
					objFile.skipWs();
					
					/* Add the current face set to the mesh file node: */
					storeFaceSet();
					}
				}
			else if(objFile.peekc()=='m')
				{
				objFile.getChar();
				
				/* Read the rest of the tag: */
				std::string tag=objFile.readString();
				if(tag=="tllib") // Material library file name
					{
					/* Read the material library file name: */
					std::string materialLibraryFileName=objFile.readLine();
					
					/* Check if the mesh file node does not have a defined material library node: */
					if(node.materialLibrary.getValue()==0)
						{
						/* Read the material library file into the temporary node: */
						readMtlFile(directory,materialLibraryFileName,*materialLibrary,node.disableTextures.getValue());
						}
					}
				}
			else if(objFile.peekc()=='u')
				{
				objFile.getChar();
				
				/* Read the rest of the tag: */
				std::string tag=objFile.readString();
				if(tag=="semtl") // Use named material from the material library
					{
					/* Add the current face set to the mesh file node: */
					storeFaceSet();
					
					/* Read the name of the new material and get its appearance node from the active material library: */
					std::string materialName=objFile.readLine();
					if(node.materialLibrary.getValue()!=0)
						currentAppearance=node.materialLibrary.getValue()->getMaterial(materialName);
					else
						currentAppearance=materialLibrary->getMaterial(materialName);
					}
				}
			
			/* Finish the current line: */
			objFile.finishLine();
			}
		
		/* Add the current face set to the mesh file node: */
		storeFaceSet();
		}
	};

}

void readObjFile(const IO::Directory& directory,const std::string& fileName,MeshFileNode& node)
	{
	/* Open the directory containing the OBJ file: */
	IO::DirectoryPtr objDirectory=directory.openFileDirectory(fileName.c_str());
	
	/* Remove the relative path from the OBJ file name: */
	std::string objFileName=Misc::getFileName(fileName.c_str());
	
	/* Create a reader for the OBJ file: */
	OBJFileReader objFileReader(*objDirectory,objFileName,node);
	
	/* Parse the OBJ file: */
	objFileReader.parse();
	}

}
