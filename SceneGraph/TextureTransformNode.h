/***********************************************************************
TextureTransformNode - Class for nodes that apply an orthogonal
transformation to model texture coordinates provided by geometry nodes.
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

#ifndef SCENEGRAPH_TEXTURETRANSFORMNODE_INCLUDED
#define SCENEGRAPH_TEXTURETRANSFORMNODE_INCLUDED

#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/AttributeNode.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

class TextureTransformNode:public AttributeNode
	{
	/* Embedded classes: */
	public:
	typedef Geometry::ComponentArray<Scalar,2> Size2; // 2D size
	typedef Geometry::Point<Scalar,2> Point2; // 2D point
	typedef Geometry::Vector<Scalar,2> Vector2; // 2D vector
	typedef SF<Size2> SFSize2;
	typedef SF<Point2> SFPoint2;
	typedef SF<Vector2> SFVector2;
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFPoint2 center;
	SFFloat rotation;
	SFSize2 scale;
	SFVector2 translation;
	
	/* Derived state: */
	protected:
	GLRenderState::TextureTransform transform; // The current transformation extended to 3D
	
	/* Constructors and destructors: */
	public:
	TextureTransformNode(void); // Creates an empty texture transform node with an identity transformation
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from AttributeNode: */
	virtual void setGLState(GLRenderState& renderState) const;
	virtual void resetGLState(GLRenderState& renderState) const;
	
	/* New methods: */
	const GLRenderState::TextureTransform& getTransform(void) const // Returns the current derived texture transformation
		{
		return transform;
		}
	};

typedef Misc::Autopointer<TextureTransformNode> TextureTransformNodePointer;

}

#endif
