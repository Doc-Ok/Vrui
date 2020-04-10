/***********************************************************************
BaseAppearanceNode - Base class for nodes defining the appearance
(material properties, textures, etc.) of shape nodes.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef SCENEGRAPH_BASEAPPEARANCENODE_INCLUDED
#define SCENEGRAPH_BASEAPPEARANCENODE_INCLUDED

#include <Misc/Autopointer.h>
#include <SceneGraph/AttributeNode.h>

namespace SceneGraph {

class BaseAppearanceNode:public AttributeNode
	{
	/* New methods: */
	public:
	virtual bool requiresTexCoords(void) const =0; // Returns true if the appearance defined in this node requires per-vertex texture coordinates for rendering
	virtual bool requiresColors(void) const =0; // Returns true if the appearance defined in this node requires per-vertex colors for rendering
	virtual bool requiresNormals(void) const =0; // Returns true if the appearance defined in this node requires per-vertex normal vectors for rendering
	};

typedef Misc::Autopointer<BaseAppearanceNode> BaseAppearanceNodePointer;

}

#endif
