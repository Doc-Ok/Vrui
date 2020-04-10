/***********************************************************************
CollisionNode - Class for group nodes that can disable collision queries
with their children.
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

#include <SceneGraph/CollisionNode.h>

#include <string.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/******************************
Methods of class CollisionNode:
******************************/

CollisionNode::CollisionNode(void)
	:collide(true)
	{
	}

const char* CollisionNode::getStaticClassName(void)
	{
	return "Collision";
	}

const char* CollisionNode::getClassName(void) const
	{
	return "Collision";
	}

EventOut* CollisionNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"collide")==0)
		return makeEventOut(this,collide);
	else
		return GroupNode::getEventOut(fieldName);
	}

EventIn* CollisionNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"collide")==0)
		return makeEventIn(this,collide);
	else
		return GroupNode::getEventIn(fieldName);
	}

void CollisionNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"collide")==0)
		{
		vrmlFile.parseField(collide);
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

}
