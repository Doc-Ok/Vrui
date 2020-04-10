/***********************************************************************
FancyTextNode - Class for nodes to render fancy 3D text as solid
polyhedral characters using high-quality outline fonts.
Copyright (c) 2020 Oliver Kreylos

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

#include <SceneGraph/FancyTextNode.h>

#include <Misc/UTF8.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>

namespace SceneGraph {

/****************************************
Methods of class FancyTextNode::DataItem:
****************************************/

FancyTextNode::DataItem::DataItem(void)
	:vertexBufferId(0),indexBufferId(0),
	 version(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBVertexBufferObject::initExtension();
	
	/* Create buffer objects: */
	glGenBuffersARB(1,&vertexBufferId);
	glGenBuffersARB(1,&indexBufferId);
	}

FancyTextNode::DataItem::~DataItem(void)
	{
	/* Destroy buffer objects: */
	glDeleteBuffersARB(1,&vertexBufferId);
	glDeleteBuffersARB(1,&indexBufferId);
	}

/******************************
Methods of class FancyTextNode:
******************************/

FancyTextNode::FancyTextNode(void)
	:depth(0),front(true),outline(false),back(false),
	 maxExtent(0),
	 numVertices(0),numIndices(0),
	 boundingBox(Box::empty),
	 version(0)
	{
	}

const char* FancyTextNode::getStaticClassName(void)
	{
	return "FancyText";
	}

const char* FancyTextNode::getClassName(void) const
	{
	return "FancyText";
	}

void FancyTextNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"string")==0)
		vrmlFile.parseField(string);
	else if(strcmp(fieldName,"fontStyle")==0)
		vrmlFile.parseSFNode(fontStyle);
	else if(strcmp(fieldName,"depth")==0)
		vrmlFile.parseField(depth);
	else if(strcmp(fieldName,"front")==0)
		vrmlFile.parseField(front);
	else if(strcmp(fieldName,"outline")==0)
		vrmlFile.parseField(outline);
	else if(strcmp(fieldName,"back")==0)
		vrmlFile.parseField(back);
	else if(strcmp(fieldName,"length")==0)
		vrmlFile.parseField(length);
	else if(strcmp(fieldName,"maxExtent")==0)
		vrmlFile.parseField(maxExtent);
	else
		GeometryNode::parseField(fieldName,vrmlFile);
	}

void FancyTextNode::update(void)
	{
	/* Check all strings for valid UTF-8 encoding: */
	for(MFString::ValueList::const_iterator sIt=string.getValues().begin();sIt!=string.getValues().end();++sIt)
		if(!Misc::UTF8::isValid(sIt->begin(),sIt->end()))
			throw std::runtime_error("FancyTextNode::update: String is not a valid UTF-8 string");
	
	/* Cache all glyphs required to render the strings: */
	FancyFontStyleNode::GBox stringBox=fontStyle.getValue()->prepareStrings(string,front.getValue(),outline.getValue(),back.getValue(),numVertices,numIndices);
	
	/* Expand the bounding box by the font's depth: */
	FancyFontStyleNode::GScalar z=Math::div2(depth.getValue());
	boundingBox.min=Point(stringBox.min[0],stringBox.min[1],-z);
	boundingBox.max=Point(stringBox.max[0],stringBox.max[1],z);
	
	/* Invalidate OpenGL state: */
	++version;
	}

Box FancyTextNode::calcBoundingBox(void) const
	{
	return boundingBox;
	}

void FancyTextNode::glRenderAction(GLRenderState& renderState) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=renderState.contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	renderState.enableCulling(GL_BACK);
	
	/* Bind the vertex and index buffers: */
	renderState.bindVertexBuffer(dataItem->vertexBufferId);
	renderState.bindIndexBuffer(dataItem->indexBufferId);
	
	/* Check if the buffers are outdated: */
	if(dataItem->version!=version)
		{
		/* Upload the current string geometry into the buffers: */
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,numVertices*sizeof(FancyFontStyleNode::GLVertex),0,GL_STATIC_DRAW_ARB);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numIndices*sizeof(FancyFontStyleNode::GLIndex),0,GL_STATIC_DRAW_ARB);
		fontStyle.getValue()->uploadStrings(string,depth.getValue(),front.getValue(),outline.getValue(),back.getValue());
		
		/* Mark the context data item as up-to-date: */
		dataItem->version=version;
		}
	
	/* Render the strings' geometry: */
	renderState.enableVertexArrays(FancyFontStyleNode::GLVertex::getPartsMask());
	glVertexPointer(static_cast<const FancyFontStyleNode::GLVertex*>(0));
	glDrawElements(GL_TRIANGLES,numIndices,GL_UNSIGNED_INT,static_cast<const FancyFontStyleNode::GLIndex*>(0));
	}

void FancyTextNode::initContext(GLContextData& contextData) const
	{
	/* Create a context data item and associate it with this node: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	}

}
