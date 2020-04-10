/***********************************************************************
ShapeNode - Class for shapes represented as a combination of a geometry
node and an attribute node defining the geometry's appearance.
Copyright (c) 2009-2019 Oliver Kreylos

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

#include <SceneGraph/ShapeNode.h>

#include <string.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/**************************
Methods of class ShapeNode:
**************************/

ShapeNode::ShapeNode(void)
	{
	}

const char* ShapeNode::getStaticClassName(void)
	{
	return "Shape";
	}

const char* ShapeNode::getClassName(void) const
	{
	return "Shape";
	}

void ShapeNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"appearance")==0)
		{
		vrmlFile.parseSFNode(appearance);
		}
	else if(strcmp(fieldName,"geometry")==0)
		{
		vrmlFile.parseSFNode(geometry);
		}
	else
		GraphNode::parseField(fieldName,vrmlFile);
	}

void ShapeNode::update(void)
	{
	/* Check if there are both an appearance node and a geometry node: */
	if(appearance.getValue()!=0&&geometry.getValue()!=0)
		{
		/* Tell the geometry node whether it requires per-vertex texture coordinates, colors, and/or normal vectors: */
		if(appearance.getValue()->requiresTexCoords())
			geometry.getValue()->mustProvideTexCoords();
		if(appearance.getValue()->requiresColors())
			geometry.getValue()->mustProvideColors();
		if(appearance.getValue()->requiresNormals())
			geometry.getValue()->mustProvideNormals();
		}
	}

Box ShapeNode::calcBoundingBox(void) const
	{
	/* Return the geometry node's bounding box or an empty box if there is no geometry node: */
	if(geometry.getValue()!=0)
		return geometry.getValue()->calcBoundingBox();
	else
		return Box::empty;
	}

void ShapeNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Set the appearance node's OpenGL state: */
	if(appearance.getValue()!=0)
		appearance.getValue()->setGLState(renderState);
	else
		{
		/* Turn off all appearance aspects: */
		renderState.disableMaterials();
		renderState.setEmissiveColor(GLRenderState::Color(1.0f,1.0f,1.0f));
		renderState.disableTextures();
		}
	
	/* Render the geometry node: */
	if(geometry.getValue()!=0)
		geometry.getValue()->glRenderAction(renderState);
	
	/* Reset the appearance node's OpenGL state: */
	if(appearance.getValue()!=0)
		appearance.getValue()->resetGLState(renderState);
	}

}
