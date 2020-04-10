/***********************************************************************
ReadMtlFile - Helper function to read a material library file in
Wavefront OBJ format into a material library node.
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

#include <SceneGraph/Internal/ReadMtlFile.h>

#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <Misc/FileNameExtensions.h>
#include <SceneGraph/MaterialNode.h>
#include <SceneGraph/ImageTextureNode.h>
#include <SceneGraph/AppearanceNode.h>
#include <SceneGraph/MaterialLibraryNode.h>
#include <SceneGraph/Internal/OBJValueSource.h>

namespace SceneGraph {

namespace {

class MTLFileReader // Helper class to maintain state while parsing a material library file
	{
	/* Embedded classes: */
	private:
	typedef Misc::HashTable<std::string,ImageTextureNodePointer> ImageTextureMap;
	
	/* Elements: */
	IO::Directory& directory; // Base directory for relative URLs
	OBJValueSource mtlFile; // Value source representing the parsed OBJ file
	std::string currentName; // Name of the current material
	MaterialNodePointer currentMaterial; // Current Phong material properties
	Color black; // Shortcut for black colors
	Color ambientColor; // VRML doesn't really support ambient colors; approximate it when storing a material
	ImageTextureMap imageTextureMap; // Map of image names to already-created image texture nodes, to facilitate image sharing
	ImageTextureNodePointer currentTexture; // Current diffuse texture image
	MaterialLibraryNode& materialLibrary; // Library of defined named material properties
	
	/* Private methods: */
	void storeMaterial(void) // Stores the current material definition in the library
		{
		/* Check if there is a current material definition: */
		if(currentMaterial!=0||currentTexture!=0||ambientColor!=black)
			{
			/* Create an appearance node: */
			AppearanceNodePointer appearance=new AppearanceNode;
			
			if(currentMaterial==0&&ambientColor!=black)
				{
				/* Create a material node and set its diffuse color to the requested ambient color: */
				currentMaterial=new MaterialNode;
				currentMaterial->diffuseColor.setValue(ambientColor);
				}
			if(currentMaterial!=0)
				{
				/* If there is a texture node, reset the diffuse color to white to replace it with the texture color instead of modulating it: */
				if(currentTexture!=0)
					currentMaterial->diffuseColor.setValue(Color(1.0f,1.0f,1.0f));
				
				/* Calculate an ambient intensity from the luminance ratio of the ambient and diffuse color: */
				Scalar al=ambientColor[0]+ambientColor[1]+ambientColor[2];
				Scalar dl=currentMaterial->diffuseColor.getValue()[0]+currentMaterial->diffuseColor.getValue()[1]+currentMaterial->diffuseColor.getValue()[2];
				currentMaterial->ambientIntensity.setValue(Math::max(al/dl,Scalar(1)));
				
				/* Finalize the material node and add it to the appearance node: */
				currentMaterial->update();
				appearance->material.setValue(currentMaterial);
				}
			if(currentTexture!=0)
				{
				/* Finalize the texture node and add it to the appearance node: */
				currentTexture->update();
				appearance->texture.setValue(currentTexture);
				}
			
			/* Finalize the appearance node and store it in the material library node: */
			appearance->update();
			materialLibrary.setMaterial(currentName,appearance);
			}
		
		/* Reset the current material properties: */
		currentMaterial=0;
		ambientColor=black;
		currentTexture=0;
		}
	
	/* Constructors and destructors: */
	public:
	MTLFileReader(IO::Directory& sDirectory,const std::string& fileName,MaterialLibraryNode& sMaterialLibrary)
		:directory(sDirectory),mtlFile(sDirectory,fileName),
		 black(0.0f,0.0f,0.0f),
		 ambientColor(black),
		 imageTextureMap(17),
		 materialLibrary(sMaterialLibrary)
		{
		}
	
	/* Methods: */
	void parse(bool disableTextures) // Parses the material library file and adds its material definitions to the library
		{
		/* Process the entire material file: */
		while(!mtlFile.eof())
			{
			/* Parse the next tag: */
			if(mtlFile.peekc()=='n') // Probably a material name
				{
				mtlFile.getChar();
				
				/* Read the full tag: */
				std::string tag=mtlFile.readString();
				if(tag=="ewmtl")
					{
					/* Store the current material in the library: */
					storeMaterial();
					
					/* Read the next material's name: */
					currentName=mtlFile.readLine();
					}
				}
			else if(mtlFile.peekc()=='K') // It's some Phong material property
				{
				mtlFile.getChar();
				if(mtlFile.peekc()=='a')
					{
					mtlFile.readChar();
					
					/* Read an ambient color: */
					ambientColor=mtlFile.readColor();
					}
				else if(mtlFile.peekc()=='d')
					{
					mtlFile.readChar();
					
					/* Set the current material's diffuse color: */
					if(currentMaterial==0)
						currentMaterial=new MaterialNode;
					currentMaterial->diffuseColor.setValue(mtlFile.readColor());
					}
				else if(mtlFile.peekc()=='s')
					{
					mtlFile.readChar();
					
					/* Set the current material's specular color: */
					if(currentMaterial==0)
						currentMaterial=new MaterialNode;
					currentMaterial->specularColor.setValue(mtlFile.readColor());
					}
				else if(mtlFile.peekc()=='e')
					{
					mtlFile.readChar();
					
					/* Set the current material's emissive color: */
					if(currentMaterial==0)
						currentMaterial=new MaterialNode;
					currentMaterial->emissiveColor.setValue(mtlFile.readColor());
					}
				}
			else if(mtlFile.peekc()=='N')
				{
				mtlFile.getChar();
				if(mtlFile.peekc()=='s')
					{
					mtlFile.readChar();
					
					/* Set the current material's shininess: */
					if(currentMaterial==0)
						currentMaterial=new MaterialNode;
					Scalar shininessExponent=Math::max(Scalar(mtlFile.readNumber()),Scalar(128));
					currentMaterial->shininess.setValue(shininessExponent/Scalar(128));
					}
				}
			else if(mtlFile.peekc()=='m') // Probably a texture map
				{
				mtlFile.getChar();
				
				/* Read the full tag: */
				std::string tag=mtlFile.readString();
				if(tag=="ap_Kd")
					{
					if(!disableTextures)
						{
						/* Read the texture image URL: */
						std::string url=mtlFile.readLine();
						
						/* Check if the requested texture image has already been defined: */
						ImageTextureMap::Iterator itIt=imageTextureMap.findEntry(url);
						if(itIt.isFinished())
							{
							/* Create a new image texture node: */
							currentTexture=new ImageTextureNode;
							currentTexture->setUrl(url,directory);
							
							/* Store the image texture node for re-use: */
							imageTextureMap[url]=currentTexture;
							}
						else
							{
							/* Re-use the existing image texture node: */
							currentTexture=itIt->getDest();
							}
						}
					}
				}
			
			/* Finish the current line: */
			mtlFile.finishLine();
			}
		
		/* Store the current material in the library: */
		storeMaterial();
		}
	};

}

void readMtlFile(IO::Directory& directory,const std::string& fileName,MaterialLibraryNode& materialLibrary,bool disableTextures)
	{
	/* Open the directory containing the OBJ file: */
	IO::DirectoryPtr mtlDirectory=directory.openFileDirectory(fileName.c_str());
	
	/* Remove the relative path from the OBJ file name: */
	std::string mtlFileName=Misc::getFileName(fileName.c_str());
	
	/* Open the material library file for parsing: */
	MTLFileReader mtl(*mtlDirectory,mtlFileName,materialLibrary);
	
	/* Parse the material library file: */
	mtl.parse(disableTextures);
	}

}
