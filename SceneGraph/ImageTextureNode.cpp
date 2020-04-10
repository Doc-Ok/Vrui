/***********************************************************************
ImageTextureNode - Class for textures loaded from external image files.
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

#include <SceneGraph/ImageTextureNode.h>

#include <string.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/*******************************************
Methods of class ImageTextureNode::DataItem:
*******************************************/

ImageTextureNode::DataItem::DataItem(void)
	:textureObjectId(0),
	 version(0)
	{
	glGenTextures(1,&textureObjectId);
	}

ImageTextureNode::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&textureObjectId);
	}

/*********************************
Methods of class ImageTextureNode:
*********************************/

ImageTextureNode::ImageTextureNode(void)
	:repeatS(true),repeatT(true),filter(true),mipmapLevel(0),
	 version(0)
	{
	}

const char* ImageTextureNode::getStaticClassName(void)
	{
	return "ImageTexture";
	}

const char* ImageTextureNode::getClassName(void) const
	{
	return "ImageTexture";
	}

void ImageTextureNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		{
		vrmlFile.parseField(url);
		
		/* Remember the VRML file's base directory: */
		baseDirectory=&vrmlFile.getBaseDirectory();
		}
	else if(strcmp(fieldName,"repeatS")==0)
		{
		vrmlFile.parseField(repeatS);
		}
	else if(strcmp(fieldName,"repeatT")==0)
		{
		vrmlFile.parseField(repeatT);
		}
	else if(strcmp(fieldName,"filter")==0)
		{
		vrmlFile.parseField(filter);
		}
	else if(strcmp(fieldName,"mipmapLevel")==0)
		{
		vrmlFile.parseField(mipmapLevel);
		}
	else
		TextureNode::parseField(fieldName,vrmlFile);
	}

void ImageTextureNode::update(void)
	{
	/* Clamp the mipmap level: */
	if(mipmapLevel.getValue()<0)
		mipmapLevel.setValue(0);
	
	/* Bump up the texture's version number: */
	++version;
	}

void ImageTextureNode::setGLState(GLRenderState& renderState) const
	{
	if(url.getNumValues()>0)
		{
		/* Enable 2D textures: */
		renderState.enableTexture2D();
		
		/* Get the data item: */
		DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
		
		/* Bind the texture object: */
		renderState.bindTexture2D(dataItem->textureObjectId);
		
		/* Check if the texture object needs to be updated: */
		if(dataItem->version!=version)
			{
			/* Load the texture image: */
			Images::BaseImage texture=Images::readGenericImageFile(*baseDirectory,url.getValue(0).c_str());
			
			/* Upload the texture image: */
			int mml=mipmapLevel.getValue();
			texture.glTexImage2D(GL_TEXTURE_2D,0,false);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,mml);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter.getValue()?(mml>0?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR):GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter.getValue()?GL_LINEAR:GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,repeatS.getValue()?GL_REPEAT:GL_CLAMP);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,repeatT.getValue()?GL_REPEAT:GL_CLAMP);
			
			/* Check if mipmapping was requested and mipmap generation is supported: */
			if(mml>0&&GLEXTFramebufferObject::isSupported())
				{
				/* Initialize the framebuffer extension: */
				GLEXTFramebufferObject::initExtension();
				
				/* Auto-generate all requested mipmap levels: */
				glGenerateMipmapEXT(GL_TEXTURE_2D);
				}
			
			/* Mark the texture object as up-to-date: */
			dataItem->version=version;
			}
		
		#if 0
		
		/* Enable alpha testing for now: */
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL,0.5f);
		
		#endif
		}
	else
		{
		/* Disable texture mapping: */
		renderState.disableTextures();
		}
	}

void ImageTextureNode::resetGLState(GLRenderState& renderState) const
	{
	#if 0
	
	/* Disable alpha testing for now: */
	glDisable(GL_ALPHA_TEST);
	
	#endif
	
	/* Don't do anything; next guy cleans up */
	}

void ImageTextureNode::initContext(GLContextData& contextData) const
	{
	/* Create a data item and store it in the GL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

void ImageTextureNode::setUrl(const std::string& newUrl,IO::Directory& newBaseDirectory)
	{
	/* Store the URL and its base directory: */
	url.setValue(newUrl);
	baseDirectory=&newBaseDirectory;
	}

void ImageTextureNode::setUrl(const std::string& newUrl)
	{
	/* Store the URL and the current directory: */
	url.setValue(newUrl);
	baseDirectory=IO::Directory::getCurrent();
	}

}
