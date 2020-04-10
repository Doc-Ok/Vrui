/***********************************************************************
ONTransformNode - Class for group nodes that apply an orthonormal
transformation to their children, with a simplified field interface for
direct control through application software.
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

#ifndef SCENEGRAPH_ONTRANSFORMNODE_INCLUDED
#define SCENEGRAPH_ONTRANSFORMNODE_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GroupNode.h>

namespace SceneGraph {

class ONTransformNode:public GroupNode
	{
	/* Embedded classes: */
	public:
	typedef Geometry::OrthonormalTransformation<Scalar,3> ONTransform; // Type for orthonormal (rigid body) transformations
	typedef SF<ONTransform> SFONTransform;
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFONTransform transform;
	
	/* Derived state: */
	protected:
	
	/* Constructors and destructors: */
	public:
	ONTransformNode(void); // Creates an empty transform node with an identity transformation
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* Methods from GraphNode: */
	virtual Box calcBoundingBox(void) const;
	virtual void glRenderAction(GLRenderState& renderState) const;
	};

typedef Misc::Autopointer<ONTransformNode> ONTransformNodePointer;

}

#endif
