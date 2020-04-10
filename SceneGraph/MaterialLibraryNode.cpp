/***********************************************************************
MaterialLibraryNode - Class for nodes associating material properties,
represented as Appearance nodes, with named materials.
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

#include <SceneGraph/MaterialLibraryNode.h>

#include <string.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/Internal/ReadMtlFile.h>

namespace SceneGraph {

/************************************
Methods of class MaterialLibraryNode:
************************************/

MaterialLibraryNode::MaterialLibraryNode(void)
	:disableTextures(false),
	 materialLibrary(17)
	{
	}

const char* MaterialLibraryNode::getStaticClassName(void)
	{
	return "MaterialLibrary";
	}

const char* MaterialLibraryNode::getClassName(void) const
	{
	return "MaterialLibrary";
	}

EventOut* MaterialLibraryNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"urls")==0)
		return makeEventOut(this,urls);
	else if(strcmp(fieldName,"disableTextures")==0)
		return makeEventOut(this,disableTextures);
	else if(strcmp(fieldName,"materials")==0)
		return makeEventOut(this,materials);
	else if(strcmp(fieldName,"materialNames")==0)
		return makeEventOut(this,materialNames);
	else
		return Node::getEventOut(fieldName);
	}

EventIn* MaterialLibraryNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"urls")==0)
		return makeEventIn(this,urls);
	else if(strcmp(fieldName,"disableTextures")==0)
		return makeEventIn(this,disableTextures);
	else if(strcmp(fieldName,"materials")==0)
		return makeEventIn(this,materials);
	else if(strcmp(fieldName,"materialNames")==0)
		return makeEventIn(this,materialNames);
	else
		return Node::getEventIn(fieldName);
	}

void MaterialLibraryNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"urls")==0)
		{
		vrmlFile.parseField(urls);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"disableTextures")==0)
		vrmlFile.parseField(disableTextures);
	else if(strcmp(fieldName,"materials")==0)
		vrmlFile.parseMFNode(materials);
	else if(strcmp(fieldName,"materialNames")==0)
		vrmlFile.parseField(materialNames);
	else
		Node::parseField(fieldName,vrmlFile);
	}

void MaterialLibraryNode::update(void)
	{
	/* Load all material library files referenced by the URL field: */
	for(std::vector<std::string>::iterator uIt=urls.getValues().begin();uIt!=urls.getValues().end();++uIt)
		{
		/* Read the material library file and add its materials to this node: */
		readMtlFile(*baseDirectory,*uIt,*this,disableTextures.getValue());
		}
	
	/* Add in-line defined materials to the material library: */
	for(size_t i=0;i<materials.getNumValues()&&i<materialNames.getNumValues();++i)
		materialLibrary[materialNames.getValue(i)]=materials.getValue(i);
	}

}
