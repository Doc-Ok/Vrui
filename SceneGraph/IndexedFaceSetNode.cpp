/***********************************************************************
IndexedFaceSetNode - Class for sets of polygonal faces as renderable
geometry.
Copyright (c) 2009-2020 Oliver Kreylos

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

#include <SceneGraph/IndexedFaceSetNode.h>

#include <string.h>
#include <GL/gl.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLContextData.h>
#include <GL/GLExtensionManager.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*********************************************
Methods of class IndexedFaceSetNode::DataItem:
*********************************************/

IndexedFaceSetNode::DataItem::DataItem(void)
	:vertexBufferObjectId(0),indexBufferObjectId(0),
	 numVertexIndices(0),
	 version(0)
	{
	if(GLARBVertexBufferObject::isSupported())
		{
		/* Initialize the vertex buffer object extension: */
		GLARBVertexBufferObject::initExtension();
		
		/* Create the vertex buffer object: */
		glGenBuffersARB(1,&vertexBufferObjectId);
		
		/* Create the index buffer object: */
		glGenBuffersARB(1,&indexBufferObjectId);
		}
	}

IndexedFaceSetNode::DataItem::~DataItem(void)
	{
	/* Destroy the vertex and index buffer objects: */
	if(vertexBufferObjectId!=0)
		glDeleteBuffersARB(1,&vertexBufferObjectId);
	if(indexBufferObjectId!=0)
		glDeleteBuffersARB(1,&indexBufferObjectId);
	}

/***********************************
Methods of class IndexedFaceSetNode:
***********************************/

#if 0

NormalNode* IndexedFaceSetNode::calcNormals(size_t numFaces,int viMin,int viMax) const
	{
	/* Create a temporary normal node: */
	NormalNode* result=new NormalNode;
	MFVector::ValueList& norms=result->vector.getValues();
	
	/* Get a handle to the face set's vertex coordinates: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	
	/* Get a handle to the face set's face vertex indices: */
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	if(normalPerVertex.getValue())
		{
		/* Calculate per-vertex normal vectors: */
		norms.reserve(viMax+1-viMin);
		for(int i=viMin;i<=viMax;++i)
			norms.push_back(Vector::zero);
		
		/* Calculate per-face normal vectors and accumulate them in each face's vertices: */
		for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
			{
			/* Get the current face's first three vertices: */
			const Point* vs[3];
			int numVertices;
			MFInt::ValueList::const_iterator fciIt;
			for(numVertices=0,fciIt=ciIt;numVertices<3&&fciIt!=coordIndices.end()&&*fciIt>=0;++numVertices,++fciIt)
				vs[numVertices]=&coords[*fciIt];
			
			/* Check if the face has at least three vertices: */
			Vector faceNormal;
			if(numVertices==3)
				{
				/* Calculate the first three vertices' normal vector: */
				faceNormal=(*vs[1]-*vs[0])^(*vs[2]-*vs[1]);
				}
			else
				{
				/* Use a dummy normal: */
				faceNormal=Vector::zero;
				}
			
			/* Accumulate the face normal with the face's vertices: */
			for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
				norms[*ciIt-viMin]+=faceNormal;
			
			/* Skip the face terminator: */
			if(ciIt!=coordIndices.end())
				++ciIt;
			}
		}
	else
		{
		/* Calculate per-face normal vectors: */
		norms.reserve(numFaces);
		for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
			{
			/* Get the current face's first three vertices: */
			const Point* vs[3];
			int numVertices;
			for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
				vs[numVertices]=&coords[*ciIt];
			
			/* Check if the face has at least three vertices: */
			if(numVertices==3)
				{
				/* Calculate the first three vertices' normal vector: */
				norms.push_back((*vs[1]-*vs[0])^(*vs[2]-*vs[1]));
				}
			else
				{
				/* Assign a dummy normal: */
				norms.push_back(Vector::zero);
				}
			
			/* Skip the rest of the face's vertices: */
			for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
				;
			
			/* Skip the face terminator: */
			if(ciIt!=coordIndices.end())
				++ciIt;
			}
		}
	
	/* Return the temporary normal node: */
	return result;
	}

void IndexedFaceSetNode::uploadFaceSet(DataItem* dataItem) const
	{
	/* Get a handle to the face set's vertex coordinates: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	
	/* Get a handle to the face set's face vertex indices: */
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Count the number of faces and vertex index range of the face set, and the total number of triangles that will be generated: */
	size_t numFaces=0;
	int viMin=int(coords.size());
	int viMax=-1;
	size_t numTriangles=0;
	for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Count the face even if it has fewer than three vertices: */
		++numFaces;
		
		/* Count the number of vertices in the current face: */
		size_t numVertices=0;
		for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
			{
			/* Update the vertex index range: */
			if(viMin>*ciIt)
				viMin=*ciIt;
			if(viMax<*ciIt)
				viMax=*ciIt;
			
			++numVertices;
			}
		
		/* Count the number of triangles, assuming trivial face triangulation: */
		if(numVertices>2)
			numTriangles+=numVertices-2;
		
		/* Skip the face terminator: */
		if(ciIt!=coordIndices.end())
			++ciIt;
		}
	
	/* Get a handle on the face set's normal node: */
	const NormalNode* normalNode=normal.getValue().getPointer();
	bool normalsIndexed=!normalIndex.getValues().empty();
	
	/* Check whether normal vectors need to be calculated: */
	bool tempNormals=needNormals&&normalNode==0;
	if(tempNormals)
		{
		/* Create a temporary normal node: */
		normalNode=calcNormals(numFaces,viMin,viMax);
		}
	
	/* Upload triangle vertices into the vertex buffer depending on which components are required: */
	if(needTexCoords)
		{
		if(haveColors)
			{
			if(needNormals)
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<GLfloat,2,GLubyte,4,GLfloat,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Upload triangles for all faces: */
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
					{
					/* Retrieve the first triangle's first two vertices: */
					int vis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						vis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third vertex: */
							vis[2]=*ciIt;
							
							/* Add three vertices to the vertex buffer: */
							for(int i=0;i<3;++i,++vPtr)
								{
								//vPtr->texCoord=...;
								//vPtr->color=...;
								//vPtr->normal=...;
								//vPtr->position=coords[vis[i]];
								}
							
							/* Prepare for the next triangle: */
							vis[1]=vis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			else // !needNormals
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<GLfloat,2,GLubyte,4,void,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			}
		else // !haveColors
			{
			if(needNormals)
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<GLfloat,2,void,0,GLfloat,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			else // !needNormals
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<GLfloat,2,void,0,void,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			}
		}
	else // !needTexCoords
		{
		if(haveColors)
			{
			if(needNormals)
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<void,0,GLubyte,4,GLfloat,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			else // !needNormals
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<void,0,GLubyte,4,void,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			}
		else // !haveColors
			{
			if(needNormals)
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Upload triangles for all faces: */
				const MFVector::ValueList& norms=normalNode->vector.getValues();
				MFVector::ValueList::const_iterator nIt=norms.begin();
				bool npv=normalPerVertex.getValue();
				MFInt::ValueList::const_iterator niIt=normalIndex.getValues().begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
					{
					/* Retrieve the first triangle's first two vertices: */
					int vis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						vis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third vertex: */
							vis[2]=*ciIt;
							
							/* Add three vertices to the vertex buffer: */
							for(int i=0;i<3;++i,++vPtr)
								{
								if(tempNormals)
									{
									if(npv)
										vPtr->normal=norms[vis[i]-viMin];
									else
										vPtr->normal=*nIt;
									}
								else if(normalsIndexed)
									{
									vPtr->normal=norms[*niIt];
									if(npv)
										++niIt;
									}
								else
									{
									if(npv)
										vPtr->normal=norms[vis[i]];
									else
										vPtr->normal=*nIt;
									}
								vPtr->position=coords[vis[i]];
								}
							
							/* Prepare for the next triangle: */
							vis[1]=vis[2];
							}
						}
					
					/* Advance the normal vector iterator if normals are per-face and either temporary or not indexed: */
					if(!npv&&(tempNormals||!normalsIndexed))
						++nIt;
					
					/* Advance the normal index pointer; if normals are per-face, it's done once per face; if they are per-vertex, this skips the face terminator index: */
					if(normalsIndexed)
						++niIt;
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			else // !needNormals
				{
				/* Select the appropriate vertex type: */
				typedef GLGeometry::Vertex<void,0,void,0,void,GLfloat,3> Vertex;
				
				/* Create a vertex buffer: */
				glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*sizeof(Vertex),0,GL_STATIC_DRAW_ARB);
				Vertex* vPtr=static_cast<Vertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
				
				/* Upload triangles for all faces: */
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
					{
					/* Retrieve the first triangle's first two vertices: */
					int vis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						vis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third vertex: */
							vis[2]=*ciIt;
							
							/* Add three vertices to the vertex buffer: */
							for(int i=0;i<3;++i,++vPtr)
								vPtr->position=coords[vis[i]];
							
							/* Prepare for the next triangle: */
							vis[1]=vis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				
				/* Finalize the buffer: */
				glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
				}
			}
		}
	
	/* Remember the number of vertices in the vertex array: */
	dataItem->numVertexIndices=numTriangles*3;
	
	/* Delete the temporary normal node if one was created: */
	if(tempNormals)
		delete normalNode;
	}

#else

void IndexedFaceSetNode::uploadFaceSet(DataItem* dataItem) const
	{
	/* Calculate the memory layout of the in-buffer vertices: */
	dataItem->vertexArrayPartsMask=0x0;
	dataItem->vertexSize=0;
	dataItem->texCoordOffset=dataItem->vertexSize;
	if(needTexCoords)
		{
		dataItem->vertexSize+=sizeof(TexCoord);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::TexCoord;
		}
	typedef GLColor<GLubyte,4> BColor; // Type for colors uploaded to vertex buffers
	dataItem->colorOffset=dataItem->vertexSize;
	if(haveColors)
		{
		dataItem->vertexSize+=sizeof(BColor);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Color;
		}
	dataItem->normalOffset=dataItem->vertexSize;
	if(needNormals)
		{
		dataItem->vertexSize+=sizeof(Vector);
		dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Normal;
		}
	dataItem->coordOffset=dataItem->vertexSize;
	dataItem->vertexSize+=sizeof(Point);
	dataItem->vertexArrayPartsMask|=GLVertexArrayParts::Position;
	
	/* Create the vertex buffer and prepare it for vertex data upload: */
	glBufferDataARB(GL_ARRAY_BUFFER_ARB,numTriangles*3*dataItem->vertexSize,0,GL_STATIC_DRAW_ARB);
	GLubyte* bPtr=static_cast<GLubyte*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	/* Access the face set's vertex coordinates and face vertex indices: */
	const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
	const MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Check if texture coordinates are needed: */
	if(needTexCoords)
		{
		/* Access the interleaved buffer's texture coordinates: */
		GLubyte* tcPtr=bPtr+dataItem->texCoordOffset;
		
		/* Check if the face set has texture coordinates: */
		if(texCoord.getValue()!=0)
			{
			/* Access the face set's texture coordinates and texture coordinate indices: */
			const MFTexCoord::ValueList& texCoords=texCoord.getValue()->point.getValues();
			const MFInt::ValueList& texCoordIndices=texCoordIndex.getValues();
			
			/* Check if there are texture coordinate indices: */
			if(texCoordIndices.empty())
				{
				/* Upload per-vertex texture coordinates using coordinate indices: */
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
					{
					/* Retrieve the first triangle's first two vertices: */
					int vis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						vis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third vertex: */
							vis[2]=*ciIt;
							
							/* Upload three texture coordinates to the vertex buffer: */
							for(int i=0;i<3;++i,tcPtr+=dataItem->vertexSize)
								*reinterpret_cast<TexCoord*>(tcPtr)=texCoords[vis[i]];
							
							/* Prepare for the next triangle: */
							vis[1]=vis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			else
				{
				/* Upload per-vertex texture coordinates using texture coordinate indices: */
				for(MFInt::ValueList::const_iterator tciIt=texCoordIndices.begin();tciIt!=texCoordIndices.end();)
					{
					/* Retrieve the first triangle's first two texture coordinates: */
					int tcis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&tciIt!=texCoordIndices.end()&&*tciIt>=0;++numVertices,++tciIt)
						tcis[numVertices]=*tciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;tciIt!=texCoordIndices.end()&&*tciIt>=0;++tciIt)
							{
							/* Retrieve the current triangle's third texture coordinate: */
							tcis[2]=*tciIt;
							
							/* Upload three texture coordinates to the vertex buffer: */
							for(int i=0;i<3;++i,tcPtr+=dataItem->vertexSize)
								*reinterpret_cast<TexCoord*>(tcPtr)=texCoords[tcis[i]];
							
							/* Prepare for the next triangle: */
							tcis[1]=tcis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(tciIt!=texCoordIndices.end())
						++tciIt;
					}
				}
			}
		else
			{
			/* Calculate the face set's bounding box: */
			Box bbox=Box::empty;
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Add the face's vertices to the bounding box: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					bbox.addPoint(coords[*ciIt]);
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			
			/* Calculate texture coordinates by mapping the largest face of the face set's bounding box to the [0, 1]^2 interval: */
			int sDim=0;
			for(int i=1;i<3;++i)
				if(bbox.getSize(i)>bbox.getSize(sDim))
					sDim=i;
			int tDim=sDim==0?1:0;
			for(int i=1;i<3;++i)
				if(i!=sDim&&bbox.getSize(i)>bbox.getSize(tDim))
					tDim=i;
			
			/* Calculate texture coordinates for all vertices: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Retrieve the first triangle's first two vertices: */
				const Point* vs[3];
				int numVertices;
				for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
					vs[numVertices]=&coords[*ciIt];
				if(numVertices==2)
					{
					/* Generate one triangle for each additional face vertex: */
					for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
						{
						/* Retrieve the current triangle's third vertex: */
						vs[2]=&coords[*ciIt];
						
						/* Upload three texture coordinates to the vertex buffer: */
						for(int i=0;i<3;++i,tcPtr+=dataItem->vertexSize)
							{
							TexCoord tc;
							tc[0]=((*vs[i])[sDim]-bbox.min[sDim])/(bbox.max[sDim]-bbox.min[sDim]);
							tc[1]=((*vs[i])[tDim]-bbox.min[tDim])/(bbox.max[tDim]-bbox.min[tDim]);
							*reinterpret_cast<TexCoord*>(tcPtr)=tc;
							}
						
						/* Prepare for the next triangle: */
						vs[1]=vs[2];
						}
					}
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			}
		}
	
	/* Check if the face set defines per-vertex or per-face colors: */
	if(haveColors)
		{
		/* Access the interleaved buffer's colors: */
		GLubyte* cPtr=bPtr+dataItem->colorOffset;
		
		/* Access the face set's colors and color indices: */
		const MFColor::ValueList& colors=color.getValue()->color.getValues();
		const MFInt::ValueList& colorIndices=colorIndex.getValues();
		
		/* Check if colors are per-vertex or per-face: */
		if(colorPerVertex.getValue())
			{
			/* Check if there are color indices: */
			if(colorIndices.empty())
				{
				/* Upload per-vertex colors using coordinate indices: */
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
					{
					/* Retrieve the first triangle's first two vertices: */
					int vis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						vis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third vertex: */
							vis[2]=*ciIt;
							
							/* Upload three colors to the vertex buffer: */
							for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
								*reinterpret_cast<BColor*>(cPtr)=BColor(colors[vis[i]]);
							
							/* Prepare for the next triangle: */
							vis[1]=vis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			else
				{
				/* Upload per-vertex colors using color indices: */
				for(MFInt::ValueList::const_iterator ciIt=colorIndices.begin();ciIt!=colorIndices.end();)
					{
					/* Retrieve the first triangle's first two colors: */
					int cis[3];
					int numVertices;
					for(numVertices=0;numVertices<2&&ciIt!=colorIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						cis[numVertices]=*ciIt;
					if(numVertices==2)
						{
						/* Generate one triangle for each additional face vertex: */
						for(;ciIt!=colorIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Retrieve the current triangle's third color: */
							cis[2]=*ciIt;
							
							/* Upload three colors to the vertex buffer: */
							for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
								*reinterpret_cast<BColor*>(cPtr)=BColor(colors[cis[i]]);
							
							/* Prepare for the next triangle: */
							cis[1]=cis[2];
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=colorIndices.end())
						++ciIt;
					}
				}
			}
		else
			{
			/* Check if there are color indices: */
			if(colorIndices.empty())
				{
				/* Upload per-face colors in the order they are provided: */
				MFColor::ValueList::const_iterator cIt=colors.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++cIt)
					{
					/* Get the current face's first three vertices: */
					int numVertices;
					for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						;
					
					/* Check if the face has at least three vertices: */
					if(numVertices==3)
						{
						BColor faceColor(*cIt);
						
						/* Assign the current face color to the first triangle's vertices: */
						for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						
						/* Process the face's remaining triangles: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Assign the current face normal vector to the current triangle's vertices: */
							for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
								*reinterpret_cast<BColor*>(cPtr)=faceColor;
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			else
				{
				/* Upload per-face colors using the provided color indices: */
				MFInt::ValueList::const_iterator coliIt=colorIndices.begin();
				for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++coliIt)
					{
					/* Get the current face's first three vertices: */
					int numVertices;
					for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
						;
					
					/* Check if the face has at least three vertices: */
					if(numVertices==3)
						{
						BColor faceColor(colors[*coliIt]);
						
						/* Assign the current face color to the first triangle's vertices: */
						for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
							*reinterpret_cast<BColor*>(cPtr)=faceColor;
						
						/* Process the face's remaining triangles: */
						for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
							{
							/* Assign the current face color to the current triangle's vertices: */
							for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
								*reinterpret_cast<BColor*>(cPtr)=faceColor;
							}
						}
					
					/* Skip the face terminator: */
					if(ciIt!=coordIndices.end())
						++ciIt;
					}
				}
			}
		}
	
	/* Check if normal vectors are needed: */
	if(needNormals)
		{
		/* Access the interleaved buffer's normal vectors: */
		GLubyte* nPtr=bPtr+dataItem->normalOffset;
		
		/* Check if the face set has normal vectors: */
		if(normal.getValue()!=0)
			{
			/* Access the face set's colors and color indices: */
			const MFVector::ValueList& normals=normal.getValue()->vector.getValues();
			const MFInt::ValueList& normalIndices=normalIndex.getValues();
			
			/* Check if normals are per-vertex or per-face: */
			if(normalPerVertex.getValue())
				{
				/* Check if there are normal indices: */
				if(normalIndices.empty())
					{
					/* Upload per-vertex normal vectors using coordinate indices: */
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
						{
						/* Retrieve the first triangle's first two vertices: */
						int vis[3];
						int numVertices;
						for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
							vis[numVertices]=*ciIt;
						if(numVertices==2)
							{
							/* Generate one triangle for each additional face vertex: */
							for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
								{
								/* Retrieve the current triangle's third vertex: */
								vis[2]=*ciIt;
								
								/* Upload three normal vectors to the vertex buffer: */
								for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
									*reinterpret_cast<Vector*>(nPtr)=normals[vis[i]];
								
								/* Prepare for the next triangle: */
								vis[1]=vis[2];
								}
							}
						
						/* Skip the face terminator: */
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				else
					{
					/* Upload per-vertex normal vectors using normal indices: */
					for(MFInt::ValueList::const_iterator niIt=normalIndices.begin();niIt!=normalIndices.end();)
						{
						/* Retrieve the first triangle's first two normal vectors: */
						int nis[3];
						int numVertices;
						for(numVertices=0;numVertices<2&&niIt!=normalIndices.end()&&*niIt>=0;++numVertices,++niIt)
							nis[numVertices]=*niIt;
						if(numVertices==2)
							{
							/* Generate one triangle for each additional face vertex: */
							for(;niIt!=normalIndices.end()&&*niIt>=0;++niIt)
								{
								/* Retrieve the current triangle's third normal vector: */
								nis[2]=*niIt;
								
								/* Upload three normal vectors to the vertex buffer: */
								for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
									*reinterpret_cast<Vector*>(nPtr)=normals[nis[i]];
								
								/* Prepare for the next triangle: */
								nis[1]=nis[2];
								}
							}
						
						/* Skip the face terminator: */
						if(niIt!=normalIndices.end())
							++niIt;
						}
					}
				}
			else
				{
				/* Check if there are normal indices: */
				if(normalIndex.getValues().empty())
					{
					/* Upload per-face normal vectors in the order they are provided: */
					MFVector::ValueList::const_iterator nIt=normals.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++nIt)
						{
						/* Get the current face's first three vertices: */
						int numVertices;
						for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
							;
						
						/* Check if the face has at least three vertices: */
						if(numVertices==3)
							{
							/* Assign the current face normal vector to the first triangle's vertices: */
							for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=*nIt;
							
							/* Process the face's remaining triangles: */
							for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
								{
								/* Assign the current face normal vector to the current triangle's vertices: */
								for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
									*reinterpret_cast<Vector*>(nPtr)=*nIt;
								}
							}
						
						/* Skip the face terminator: */
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				else
					{
					/* Upload per-face normal vectors using the provided normal vector indices: */
					MFInt::ValueList::const_iterator niIt=normalIndices.begin();
					for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();++niIt)
						{
						/* Get the current face's first three vertices: */
						int numVertices;
						for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
							;
						
						/* Check if the face has at least three vertices: */
						if(numVertices==3)
							{
							const Vector& faceNormal=normals[*niIt];
							
							/* Assign the current face normal vector to the first triangle's vertices: */
							for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
								*reinterpret_cast<Vector*>(nPtr)=faceNormal;
							
							/* Process the face's remaining triangles: */
							for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
								{
								/* Assign the current face normal vector to the current triangle's vertices: */
								for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
									*reinterpret_cast<Vector*>(nPtr)=faceNormal;
								}
							}
						
						/* Skip the face terminator: */
						if(ciIt!=coordIndices.end())
							++ciIt;
						}
					}
				}
			}
		else if(normalPerVertex.getValue())
			{
			/* Find the range of vertex indices used by the face set: */
			int viMin=int(coords.size());
			int viMax=-1;
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Process the current face: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					{
					/* Update the vertex index range: */
					if(viMin>*ciIt)
						viMin=*ciIt;
					if(viMax<*ciIt)
						viMax=*ciIt;
					}
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			
			/* Calculate normal vectors for the range of vertices used by this face set: */
			std::vector<Vector> vertexNormals;
			vertexNormals.reserve(viMax+1-viMin);
			for(int i=viMin;i<=viMax;++i)
				vertexNormals.push_back(Vector::zero);
			
			/* Calculate per-face normal vectors and accumulate them in each face's vertices: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Get the current face's first three vertices: */
				const Point* vs[3];
				int numVertices;
				MFInt::ValueList::const_iterator fciIt;
				for(numVertices=0,fciIt=ciIt;numVertices<3&&fciIt!=coordIndices.end()&&*fciIt>=0;++numVertices,++fciIt)
					vs[numVertices]=&coords[*fciIt];
				
				/* Check if the face has at least three vertices: */
				Vector faceNormal;
				if(numVertices==3)
					{
					/* Calculate the first three vertices' normal vector: */
					faceNormal=(*vs[1]-*vs[0])^(*vs[2]-*vs[1]);
					}
				else
					{
					/* Use a zero normal so the following code is a no-op: */
					faceNormal=Vector::zero;
					}
				
				/* Accumulate the face normal with the face's vertices: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					vertexNormals[*ciIt-viMin]+=faceNormal;
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			
			/* Upload the calculated per-vertex normal vectors: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Retrieve the first triangle's first two vertices: */
				int vis[3];
				int numVertices;
				for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
					vis[numVertices]=*ciIt;
				if(numVertices==2)
					{
					/* Generate one triangle for each additional face vertex: */
					for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
						{
						/* Retrieve the current triangle's third vertex: */
						vis[2]=*ciIt;
						
						/* Upload three normal vectors to the vertex buffer: */
						for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
							*reinterpret_cast<Vector*>(nPtr)=vertexNormals[vis[i]-viMin];
						
						/* Prepare for the next triangle: */
						vis[1]=vis[2];
						}
					}
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			}
		else
			{
			/* Calculate and upload per-face normal vectors: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Get the current face's first three vertices: */
				const Point* vs[3];
				int numVertices;
				for(numVertices=0;numVertices<3&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
					vs[numVertices]=&coords[*ciIt];
				
				/* Check if the face has at least three vertices: */
				if(numVertices==3)
					{
					/* Calculate the first three vertices' normal vector: */
					Vector normal=(*vs[1]-*vs[0])^(*vs[2]-*vs[1]);
					
					/* Assign the normal vector to the first triangle's vertices: */
					for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
						*reinterpret_cast<Vector*>(nPtr)=normal;
					
					/* Process the face's remaining triangles: */
					for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
						{
						/* Assign the normal vector to the current triangle's vertices: */
						for(int i=0;i<3;++i,nPtr+=dataItem->vertexSize)
							*reinterpret_cast<Vector*>(nPtr)=normal;
						}
					}
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			}
		}
	
	/* Access the interleaved buffer's vertex positions: */
	GLubyte* cPtr=bPtr+dataItem->coordOffset;
	
	/* Check if there is a point transformation: */
	if(pointTransform.getValue()!=0)
		{
		/* Upload transformed vertex positions: */
		for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
			{
			/* Retrieve the first triangle's first two vertices: */
			int vis[3];
			int numVertices;
			for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
				vis[numVertices]=*ciIt;
			if(numVertices==2)
				{
				/* Generate one triangle for each additional face vertex: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					{
					/* Retrieve the current triangle's third vertex: */
					vis[2]=*ciIt;
					
					/* Upload three transformed vertex positions to the vertex buffer: */
					for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
						*reinterpret_cast<Point*>(cPtr)=Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[vis[i]])));
					
					/* Prepare for the next triangle: */
					vis[1]=vis[2];
					}
				}
			
			/* Skip the face terminator: */
			if(ciIt!=coordIndices.end())
				++ciIt;
			}
		}
	else
		{
		/* Upload untransformed vertex positions: */
		for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
			{
			/* Retrieve the first triangle's first two vertices: */
			int vis[3];
			int numVertices;
			for(numVertices=0;numVertices<2&&ciIt!=coordIndices.end()&&*ciIt>=0;++numVertices,++ciIt)
				vis[numVertices]=*ciIt;
			if(numVertices==2)
				{
				/* Generate one triangle for each additional face vertex: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					{
					/* Retrieve the current triangle's third vertex: */
					vis[2]=*ciIt;
					
					/* Upload three vertex positions to the vertex buffer: */
					for(int i=0;i<3;++i,cPtr+=dataItem->vertexSize)
						*reinterpret_cast<Point*>(cPtr)=coords[vis[i]];
					
					/* Prepare for the next triangle: */
					vis[1]=vis[2];
					}
				}
			
			/* Skip the face terminator: */
			if(ciIt!=coordIndices.end())
				++ciIt;
			}
		}
	
	/* Finalize the buffer: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	dataItem->numVertexIndices=GLsizei(numTriangles*3);
	}

#endif

IndexedFaceSetNode::IndexedFaceSetNode(void)
	:colorPerVertex(true),normalPerVertex(true),
	 ccw(true),convex(true),solid(true),
	 haveColors(false),numTriangles(0),
	 version(0)
	{
	}

const char* IndexedFaceSetNode::getStaticClassName(void)
	{
	return "IndexedFaceSet";
	}

const char* IndexedFaceSetNode::getClassName(void) const
	{
	return "IndexedFaceSet";
	}

void IndexedFaceSetNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"texCoord")==0)
		{
		vrmlFile.parseSFNode(texCoord);
		}
	else if(strcmp(fieldName,"color")==0)
		{
		vrmlFile.parseSFNode(color);
		}
	else if(strcmp(fieldName,"normal")==0)
		{
		vrmlFile.parseSFNode(normal);
		}
	else if(strcmp(fieldName,"coord")==0)
		{
		vrmlFile.parseSFNode(coord);
		}
	else if(strcmp(fieldName,"texCoordIndex")==0)
		{
		vrmlFile.parseField(texCoordIndex);
		}
	else if(strcmp(fieldName,"colorIndex")==0)
		{
		vrmlFile.parseField(colorIndex);
		}
	else if(strcmp(fieldName,"colorPerVertex")==0)
		{
		vrmlFile.parseField(colorPerVertex);
		}
	else if(strcmp(fieldName,"normalIndex")==0)
		{
		vrmlFile.parseField(normalIndex);
		}
	else if(strcmp(fieldName,"normalPerVertex")==0)
		{
		vrmlFile.parseField(normalPerVertex);
		}
	else if(strcmp(fieldName,"coordIndex")==0)
		{
		vrmlFile.parseField(coordIndex);
		}
	else if(strcmp(fieldName,"ccw")==0)
		{
		vrmlFile.parseField(ccw);
		}
	else if(strcmp(fieldName,"convex")==0)
		{
		vrmlFile.parseField(convex);
		}
	else if(strcmp(fieldName,"solid")==0)
		{
		vrmlFile.parseField(solid);
		}
	else if(strcmp(fieldName,"creaseAngle")==0)
		{
		vrmlFile.parseField(creaseAngle);
		}
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void IndexedFaceSetNode::update(void)
	{
	/* Check if there are per-vertex colors: */
	haveColors=color.getValue()!=0;
	
	/* Access the face set's face vertex indices: */
	MFInt::ValueList& coordIndices=coordIndex.getValues();
	
	/* Calculate the total number of triangles that will be generated when the current face set is uploaded: */
	numTriangles=0;
	for(MFInt::ValueList::iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
		{
		/* Count the number of vertices in the current face: */
		size_t numVertices;
		for(numVertices=0;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt,++numVertices)
			;
		
		/* Count the number of triangles, assuming trivial face triangulation: */
		if(numVertices>2)
			numTriangles+=numVertices-2;
		
		/* Skip the face terminator: */
		if(ciIt!=coordIndices.end())
			++ciIt;
		}
	
	/* Bump up the indexed face set's version number: */
	++version;
	}

Box IndexedFaceSetNode::calcBoundingBox(void) const
	{
	Box result=Box::empty;
	
	if(coord.getValue()!=0)
		{
		/* Access the face set's vertex coordinates and face vertex indices: */
		const MFPoint::ValueList& coords=coord.getValue()->point.getValues();
		const MFInt::ValueList& coordIndices=coordIndex.getValues();
		
		if(pointTransform.getValue()!=0)
			{
			/* Return the bounding box of the transformed point coordinates: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Add the face's vertices to the bounding box: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					result.addPoint(Point(pointTransform.getValue()->transformPoint(PointTransformNode::TPoint(coords[*ciIt]))));
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			}
		else
			{
			/* Return the bounding box of the untransformed point coordinates: */
			for(MFInt::ValueList::const_iterator ciIt=coordIndices.begin();ciIt!=coordIndices.end();)
				{
				/* Add the face's vertices to the bounding box: */
				for(;ciIt!=coordIndices.end()&&*ciIt>=0;++ciIt)
					result.addPoint(coords[*ciIt]);
				
				/* Skip the face terminator: */
				if(ciIt!=coordIndices.end())
					++ciIt;
				}
			}
		}
	
	return result;
	}

void IndexedFaceSetNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set up OpenGL state: */
	renderState.setFrontFace(ccw.getValue()?GL_CCW:GL_CW);
	if(solid.getValue())
		renderState.enableCulling(GL_BACK);
	else
		renderState.disableCulling();
	
	/* Get the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	if(dataItem->vertexBufferObjectId!=0&&dataItem->indexBufferObjectId!=0)
		{
		/*******************************************************************
		Render the indexed face set from the vertex and index buffers:
		*******************************************************************/
		
		/* Bind the face set's vertex and index buffer objects: */
		renderState.bindVertexBuffer(dataItem->vertexBufferObjectId);
		renderState.bindIndexBuffer(dataItem->indexBufferObjectId);
		
		if(dataItem->version!=version)
			{
			/* Upload the new face set: */
			uploadFaceSet(dataItem);
			
			/* Mark the vertex and index buffer objects as up-to-date: */
			dataItem->version=version;
			}
		
		/* Enable vertex buffer rendering: */
		renderState.enableVertexArrays(dataItem->vertexArrayPartsMask);
		if(needTexCoords)
			glTexCoordPointer(2,GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->texCoordOffset);
		if(haveColors)
			glColorPointer(4,GL_UNSIGNED_BYTE,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->colorOffset);
		if(needNormals)
			glNormalPointer(GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->normalOffset);
		glVertexPointer(3,GL_FLOAT,dataItem->vertexSize,static_cast<const GLubyte*>(0)+dataItem->coordOffset);
		
		/* Draw the vertex array: */
		glDrawArrays(GL_TRIANGLES,0,dataItem->numVertexIndices);
		}
	else
		{
		/*******************************************************************
		Render the indexed face set directly:
		*******************************************************************/
		
		// Not even going to bother...
		}
	}

void IndexedFaceSetNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
