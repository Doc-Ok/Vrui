/***********************************************************************
InlineNode - Class for group nodes that read their children from an
external VRML file.
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

#include <SceneGraph/InlineNode.h>

#include <string.h>
#include <stdexcept>
#include <Misc/MessageLogger.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/***************************
Methods of class InlineNode:
***************************/

InlineNode::InlineNode(void)
	{
	}

const char* InlineNode::getStaticClassName(void)
	{
	return "Inline";
	}

const char* InlineNode::getClassName(void) const
	{
	return "Inline";
	}

void InlineNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		try
			{
			/* Load the external VRML file: */
			VRMLFile externalVrmlFile(vrmlFile.getBaseDirectory(),url.getValue(0),vrmlFile.getNodeCreator());
			externalVrmlFile.parse(this);
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message and delete all partially-read file contents: */
			Misc::formattedUserError("SceneGraph::InlineNode: Unable to load file %s due to exception %s",url.getValue(0).c_str(),err.what());
			children.clearValues();
			}
		}
	else
		GroupNode::parseField(fieldName,vrmlFile);
	}

void InlineNode::update(void)
	{
	}

}
