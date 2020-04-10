/***********************************************************************
FancyTextNode - Class for nodes to render fancy 3D text as solid
polyhedral characters using high-quality outline fonts.
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

#ifndef SCENEGRAPH_FANCYTEXTNODE_INCLUDED
#define SCENEGRAPH_FANCYTEXTNODE_INCLUDED

#include <stddef.h>
#include <GL/gl.h>
#include <GL/GLObject.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GeometryNode.h>
#include <SceneGraph/FancyFontStyleNode.h>

namespace SceneGraph {

class FancyTextNode:public GeometryNode,public GLObject
	{
	/* Embedded classes: */
	public:
	typedef SF<FancyFontStyleNodePointer> SFFancyFontStyleNode;
	
	protected:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint vertexBufferId; // ID of buffer holding triangle vertices
		GLuint indexBufferId; // ID of buffer holding triangle indices
		unsigned int version; // Version number of geometry contained in buffers
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	
	/* Fields: */
	public:
	MFString string;
	SFFancyFontStyleNode fontStyle;
	SFFloat depth;
	SFBool front;
	SFBool outline;
	SFBool back;
	MFFloat length;
	SFFloat maxExtent;
	
	/* Derived elements: */
	protected:
	size_t numVertices; // Number of vertices required to render the current set of strings
	size_t numIndices; // Number of triangle vertex indices required to render the current set of strings
	Box boundingBox; // Bounding box around current set of strings
	unsigned int version; // Version number of node state
	
	/* Constructors and destructors: */
	public:
	FancyTextNode(void); // Creates a default fancy text node
	
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

typedef Misc::Autopointer<FancyTextNode> FancyTextNodePointer;

}

#endif
