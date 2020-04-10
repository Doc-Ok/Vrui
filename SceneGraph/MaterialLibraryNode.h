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

#ifndef SCENEGRAPH_MATERIALLIBRARYNODE_INCLUDED
#define SCENEGRAPH_MATERIALLIBRARYNODE_INCLUDED

#include <Misc/Autopointer.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <IO/Directory.h>
#include <SceneGraph/FieldTypes.h>
#include <SceneGraph/Node.h>
#include <SceneGraph/AppearanceNode.h>

namespace SceneGraph {

class MaterialLibraryNode:public Node
	{
	/* Embedded classes: */
	public:
	typedef MF<AppearanceNodePointer> MFAppearanceNode;
	
	/* Elements: */
	
	/* Fields: */
	MFString urls; // List of URLs of material library files to load
	SFBool disableTextures; // Flag to ignore texture images when creating materials
	MFAppearanceNode materials; // List of Appearance nodes defining material properties
	MFString materialNames; // List of material names, paired with Appearance nodes in the materials list; override those loaded from URLs
	
	/* Derived state: */
	protected:
	IO::DirectoryPtr baseDirectory; // Base directory for material library file URLs
	Misc::HashTable<std::string,AppearanceNodePointer> materialLibrary; // Hash table mapping material names to appearance nodes
	
	/* Constructors and destructors: */
	public:
	MaterialLibraryNode(void); // Creates empty material library
	
	/* Methods from Node: */
	static const char* getStaticClassName(void);
	virtual const char* getClassName(void) const;
	virtual EventOut* getEventOut(const char* fieldName) const;
	virtual EventIn* getEventIn(const char* fieldName);
	virtual void parseField(const char* fieldName,VRMLFile& vrmlFile);
	virtual void update(void);
	
	/* New methods: */
	void setMaterial(const std::string& materialName,AppearanceNodePointer material) // Adds or replaces a material definition
		{
		/* Store the (name, material) pair in the material map: */
		materialLibrary[materialName]=material;
		}
	AppearanceNodePointer getMaterial(const std::string& materialName) // Returns an appearance node for the given material name, or 0 if no matching material is found
		{
		/* Find the named appearance node and return it or a null pointer: */
		Misc::HashTable<std::string,AppearanceNodePointer>::Iterator mIt=materialLibrary.findEntry(materialName);
		if(!mIt.isFinished())
			return mIt->getDest();
		else
			return 0;
		}
	};

typedef Misc::Autopointer<MaterialLibraryNode> MaterialLibraryNodePointer;

}

#endif
