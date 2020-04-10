/***********************************************************************
DOGTransformNode - Class for group nodes that apply a double-precision
orthogonal transformation to their children, with a simplified field
interface for direct control through application software.
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

#ifndef SCENEGRAPH_DOGTRANSFORMNODE_INCLUDED
#define SCENEGRAPH_DOGTRANSFORMNODE_INCLUDED

#include <Geometry/OrthogonalTransformation.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/GroupNode.h>

namespace SceneGraph {

class DOGTransformNode:public GroupNode
	{
	/* Embedded classes: */
	public:
	typedef Geometry::OrthogonalTransformation<double,3> DOGTransform; // Type for double-precision orthogonal (rigid body) transformations
	typedef SF<DOGTransform> SFDOGTransform;
	
	/* Elements: */
	
	/* Fields: */
	public:
	SFDOGTransform transform;
	
	/* Derived state: */
	protected:
	
	/* Constructors and destructors: */
	public:
	DOGTransformNode(void); // Creates an empty transform node with an identity transformation
	
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

typedef Misc::Autopointer<DOGTransformNode> DOGTransformNodePointer;

}

#endif
