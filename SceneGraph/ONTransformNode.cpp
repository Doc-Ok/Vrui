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

#include <SceneGraph/ONTransformNode.h>

#include <string.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/********************************
Methods of class ONTransformNode:
********************************/

ONTransformNode::ONTransformNode(void)
	:transform(ONTransform::identity)
	{
	}

const char* ONTransformNode::getStaticClassName(void)
	{
	return "ONTransform";
	}

const char* ONTransformNode::getClassName(void) const
	{
	return "ONTransform";
	}

EventOut* ONTransformNode::getEventOut(const char* fieldName) const
	{
	return GroupNode::getEventOut(fieldName);
	}

EventIn* ONTransformNode::getEventIn(const char* fieldName)
	{
	return GroupNode::getEventIn(fieldName);
	}

void ONTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	GroupNode::parseField(fieldName,vrmlFile);
	}

void ONTransformNode::update(void)
	{
	/* Do nothing */
	}

Box ONTransformNode::calcBoundingBox(void) const
	{
	/* Return the explicit bounding box if there is one: */
	if(haveExplicitBoundingBox)
		return explicitBoundingBox;
	else
		{
		/* Calculate the group's bounding box as the union of the transformed children's boxes: */
		Box result=Box::empty;
		for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
			{
			Box childBox=(*chIt)->calcBoundingBox();
			childBox.transform(transform.getValue());
			result.addBox(childBox);
			}
		return result;
		}
	}

void ONTransformNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Push the transformation onto the matrix stack: */
	GLRenderState::DOGTransform previousTransform=renderState.pushTransform(transform.getValue());
	
	/* Call the render actions of all children in order: */
	for(MFGraphNode::ValueList::const_iterator chIt=children.getValues().begin();chIt!=children.getValues().end();++chIt)
		(*chIt)->glRenderAction(renderState);
		
	/* Pop the transformation off the matrix stack: */
	renderState.popTransform(previousTransform);
	}

}
