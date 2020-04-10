/***********************************************************************
SwitchNode - Class for group nodes that traverse zero or one of their
children based on a selection field.
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

#include <SceneGraph/SwitchNode.h>

#include <string.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/***************************
Methods of class SwitchNode:
***************************/

SwitchNode::SwitchNode(void)
	:whichChoice(-1)
	{
	}

const char* SwitchNode::getStaticClassName(void)
	{
	return "Switch";
	}

const char* SwitchNode::getClassName(void) const
	{
	return "Switch";
	}

EventOut* SwitchNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventOut(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventOut(this,whichChoice);
	else
		return GraphNode::getEventOut(fieldName);
	}

EventIn* SwitchNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"choice")==0)
		return makeEventIn(this,choice);
	else if(strcmp(fieldName,"whichChoice")==0)
		return makeEventIn(this,whichChoice);
	else
		return GraphNode::getEventIn(fieldName);
	}

void SwitchNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"choice")==0)
		{
		vrmlFile.parseMFNode(choice);
		}
	else if(strcmp(fieldName,"whichChoice")==0)
		{
		vrmlFile.parseField(whichChoice);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

Box SwitchNode::calcBoundingBox(void) const
	{
	/* Calculate the group's bounding box as the union of the children's boxes: */
	Box result=Box::empty;
	for(MFGraphNode::ValueList::const_iterator cIt=choice.getValues().begin();cIt!=choice.getValues().end();++cIt)
		result.addBox((*cIt)->calcBoundingBox());
	return result;
	}

void SwitchNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Bail out if the children list is empty or the choice index is out of range: */
	if(choice.getValues().empty()||whichChoice.getValue()<0||whichChoice.getValue()>=int(choice.getNumValues()))
		return;
	
	/* Call the render action of the selected choice: */
	choice.getValue(whichChoice.getValue())->glRenderAction(renderState);
	}

}
