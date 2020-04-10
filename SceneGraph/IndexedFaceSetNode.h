/***********************************************************************
IndexedFaceSetNode - Class for sets of polygonal faces as renderable
geometry.
Copyright (c) 2009-2018 Oliver Kreylos

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

#ifndef SCENEGRAPH_INDEXEDFACESETNODE_INCLUDED
#define SCENEGRAPH_INDEXEDFACESETNODE_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/ColorNode.h>
#include <SceneGraph/CoordinateNode.h>
#include <SceneGraph/NormalNode.h>
#include <SceneGraph/TextureCoordinateNode.h>

namespace SceneGraph {

class IndexedFaceSetNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<ColorNodePointer> SFColorNode;
	typedef SF<CoordinateNodePointer> SFCoordinateNode;
	typedef SF<NormalNodePointer> SFNormalNode;
	typedef SF<TextureCoordinateNodePointer> SFTextureCoordinateNode;
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferObjectId; // ID of vertex buffer object containing the face set's vertices, if supported
		GLuint indexBufferObjectId; // ID of index buffer object containing the face set's triangle vertex indices, if supported
		ptrdiff_t texCoordOffset; // Offset of texture coordinate in interleaved vertex buffer
		ptrdiff_t colorOffset; // Offset of color in interleaved vertex buffer
		ptrdiff_t normalOffset; // Offset of normal vector in interleaved vertex buffer
		ptrdiff_t coordOffset; // Offset of vertex position in interleaved vertex buffer
		size_t vertexSize; // Total vertex size in interleaved vertex buffer
		int vertexArrayPartsMask; // Bit mask of use vertex properties in vertex buffer
		GLsizei numVertexIndices; // Number of vertex indices in the index buffer
		unsigned int version; // Version of face set stored in the buffer objects
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFTextureCoordinateNode texCoord;
	SFColorNode color;
	SFNormalNode normal;
	SFCoordinateNode coord;
	MFInt texCoordIndex;
	MFInt colorIndex;
	SFBool colorPerVertex;
	MFInt normalIndex;
	SFBool normalPerVertex;
	MFInt coordIndex;
	SFBool ccw;
	SFBool convex;
	SFBool solid;
	SFFloat creaseAngle;
	
	/* Derived state: */
	protected:
	bool haveColors; // Flag if the face set's vertices have per-vertex color values
	size_t numTriangles; // Total number of triangles defined by the indexed face set
	unsigned int version; // Version number of face set
	
	#if 0
	/* Private methods: */
	NormalNode* calcNormals(size_t numFaces,int viMin,int viMax) const; // Returns a temporary normal node if normal vectors are needed for rendering, but none were provided
	#endif
	
	/* Protected methods: */
	protected:
	void uploadFaceSet(DataItem* dataItem) const; // Uploads new face set into OpenGL buffers
	
	/* Constructors and destructors: */
	public:
	IndexedFaceSetNode(void); // Creates a default face set
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from GeometryNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

}

#endif
