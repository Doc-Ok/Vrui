/***********************************************************************
FancyFontStyleNode - Class for nodes defining the appearance and layout
of fancy 3D text, rendered as solid polyhedral characters using high-
quality outline fonts.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef SCENEGRAPH_FANCYFONTSTYLENODE_INCLUDED
#define SCENEGRAPH_FANCYFONTSTYLENODE_INCLUDED

#include <stddef.h>
#include <utility>
#include <vector>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <Misc/Autopointer.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Threads/Mutex.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Box.h>
#include <GL/gl.h>
#include <GL/GLGeometryVertex.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>

/* Forward declarations: */
namespace SceneGraph {
class FancyTextNode;
}

namespace SceneGraph {

class FancyFontStyleNode:public Node
	{
	friend class FancyTextNode;
	friend class FancyLabelSetNode;
	
	/* Embedded classes: */
	public:
	typedef float GScalar; // Scalar type for character glyphs
	typedef Geometry::Point<GScalar,2> GPoint; // Type for character glyph outline vertex positions
	typedef Geometry::Vector<GScalar,2> GVector; // Type for character glyph outline vectors
	typedef Geometry::Box<GScalar,2> GBox; // Type for character glyph bounding boxes
	typedef std::vector<GPoint> FaceVertexList; // Type for lists of character face vertices
	
	struct EdgeVertex // Structure representing a character outline vertex
		{
		/* Elements: */
		public:
		GVector normal; // Normalized 2D normal vector
		GPoint position; // Vertex position
		
		/* Constructors and destructors: */
		EdgeVertex(const GVector& sNormal,const GPoint& sPosition) // Elementwise constructor
			:normal(sNormal),position(sPosition)
			{
			}
		};
	
	typedef std::vector<EdgeVertex> EdgeVertexList; // Type for lists of character outline vertices
	typedef unsigned int Index; // Type for vertex indices
	typedef std::vector<Index> IndexList; // Type for lists of vertex indices
	
	protected:
	enum Justification // Enumerated type for string justifications
		{
		FIRST,BEGIN,MIDDLE,END
		};
	
	struct Glyph // Structure defining a cached character glyph
		{
		/* Elements: */
		public:
		GBox box; // Glyph's bounding box as reported by FreeType library (might not be actual bounding box of 3D geometry)
		GVector advance; // Vector by which to advance the pen position after rendering this glyph
		Index firstFaceVertex; // Index of glyph's first face vertex in the main list
		Index numFaceVertices; // Number of face vertices in the glyph
		Index firstTriangle; // Index of glyph's first triangle in the main list
		Index numTriangles; // Number of face triangles in the glyph
		Index firstEdgeVertex; // Index of glyph's first outline vertex in the main list
		Index numEdgeVertices; // Number of outline vertices in the glyph
		Index firstEdge; // Index of glyph's first outline edge in the main list
		Index numEdges; // Number of outline edges in the glyph
		};
	
	typedef Misc::HashTable<unsigned int,Glyph> GlyphMap; // Type for hash tables mapping FreeType face glyph indices to cached glyph structures
	
	class GlyphCreator; // Class to convert a FreeType character outline into a glyph structure
	
	typedef GLGeometry::Vertex<void,0,void,0,GLfloat,GLfloat,3> GLVertex; // Type for vertices uploaded into OpenGL vertex buffers
	typedef GLuint GLIndex; // Type for vertex indices uploaded into OpenGL index buffers
	
	/* Elements: */
	
	/* Fields: */
	public:
	MFString url;
	MFString family;
	SFString style;
	SFString language;
	SFFloat size;
	SFFloat spacing;
	MFString justify;
	SFBool horizontal;
	SFBool leftToRight;
	SFBool topToBottom;
	SFFloat precision;
	
	/* Derived state: */
	protected:
	static Threads::Mutex ftLibraryMutex; // Mutex serializing access to the shared FreeType library handle
	static FT_Library ftLibrary; // Handle to the FreeType font server library; shard amongst all fancy font style nodes
	static unsigned int ftLibraryRefCount; // Reference count on the shared FreeType library handle
	std::string fontFileName; // The full path name of the currently loaded font file
	FT_Face ftFace; // Handle to the FreeType font face used for text using this font style
	GScalar scale; // Scale factor from FreeType units to scene graph units
	GScalar epsilon; // Subdivision threshold for quadratic and cubic curves
	FaceVertexList faceVertices; // List of face vertices of cached glyphs
	IndexList triangles; // List of face triangles of cached glyphs
	EdgeVertexList edgeVertices; // List of outline vertices of cached glyphs
	IndexList edges; // List of outline edges of cached glyphs
	GlyphMap glyphMap; // Map from Unicode character codes to cached glyphs
	Justification justifications[2]; // Justification in major and minor directions
	
	/* Constructors and destructors: */
	public:
	FancyFontStyleNode(void); // Creates a font style node with default properties
	virtual ~FancyFontStyleNode(void); // Destroys the fancy font style
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* New methods: */
	GBox prepareStrings(const MFString& strings,bool front,bool outline,bool back,size_t& numVertices,size_t& numIndices); // Prepares to upload strings into OpenGL contexts later; returns number of vertices and triangle vertex indices required to represent the strings' geometry
	void uploadStrings(const MFString& strings,Scalar depth,bool front,bool outline,bool back) const; // Uploads 3D geometry defining a sequence of strings into a pair of bound and allocated OpenGL vertex and index buffers
	};

typedef Misc::Autopointer<FancyFontStyleNode> FancyFontStyleNodePointer;

}

#endif
