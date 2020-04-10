/***********************************************************************
GLBuffer - Common base class for OpenGL vertex (GL_ARRAY_BUFFER_ARB) or
index (GL_ELEMENT_ARRAY_BUFFER_ARB) buffers.
Copyright (c) 2018 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLBuffer.h>

#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>

/***********************************
Methods of class GLBuffer::DataItem:
***********************************/

GLBuffer::DataItem::DataItem(void)
	:bufferObjectId(0),
	 parameterVersion(0),version(0)
	{
	/* Initialize the GL_ARB_vertex_buffer_object extension: */
	GLARBVertexBufferObject::initExtension();
	
	/* Allocate a buffer object: */
	glGenBuffersARB(1,&bufferObjectId);
	}

GLBuffer::DataItem::~DataItem(void)
	{
	/* Release the buffer object: */
	glDeleteBuffersARB(1,&bufferObjectId);
	}

/*************************
Methods of class GLBuffer:
*************************/

GLBuffer::GLBuffer(GLenum sBufferType,size_t sElementSize)
	:bufferType(sBufferType),
	 elementSize(sElementSize),
	 numElements(0),sourceElements(0),
	 bufferUsage(GL_DYNAMIC_DRAW_ARB),
	 parameterVersion(0),version(0)
	{
	}

GLBuffer::GLBuffer(GLenum sBufferType,size_t sElementSize,size_t sNumElements,const void* sSourceElements,GLenum sBufferUsage)
	:bufferType(sBufferType),
	 elementSize(sElementSize),
	 numElements(sNumElements),sourceElements(sSourceElements),
	 bufferUsage(sBufferUsage),
	 parameterVersion(0),version(0)
	{
	}

void GLBuffer::initContext(GLContextData& contextData) const
	{
	/* Create an OpenGL context data item and associate it with the OpenGL context: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Initialize the buffer object: */
	glBindBufferARB(bufferType,dataItem->bufferObjectId);
	glBufferDataARB(bufferType,numElements*elementSize,sourceElements,bufferUsage);
	glBindBufferARB(bufferType,0);
	
	/* Mark the buffer as up-to-date: */
	dataItem->parameterVersion=parameterVersion;
	if(sourceElements!=0)
		dataItem->version=version;
	}

void GLBuffer::invalidate(void)
	{
	/* Invalidate the buffer contents: */
	++version;
	}

void GLBuffer::setSource(size_t newNumElements,const void* newSourceElements)
	{
	/* Copy the new source parameters: */
	numElements=newNumElements;
	sourceElements=newSourceElements;
	
	/* Invalidate the buffer format and contents: */
	++parameterVersion;
	++version;
	}

void GLBuffer::setBufferUsage(GLenum newBufferUsage)
	{
	/* Copy the new buffer usage: */
	bufferUsage=newBufferUsage;
	
	/* Invalidate the buffer format and contents: */
	++parameterVersion;
	++version;
	}

GLBuffer::DataItem* GLBuffer::bind(GLContextData& contextData) const
	{
	/* Retrieve the OpenGL context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Bind the buffer: */
	glBindBufferARB(bufferType,dataItem->bufferObjectId);
	
	/* Check if the buffer format and/or contents are outdated: */
	if(dataItem->parameterVersion!=parameterVersion||(sourceElements!=0&&dataItem->version!=version))
		{
		/* Create a new buffer and mark its format as up-to-date: */
		glBufferDataARB(bufferType,numElements*elementSize,sourceElements,bufferUsage);
		dataItem->parameterVersion=parameterVersion;
		
		/* If the source elements pointer is non-null, the buffer contents are now up-to-date, too: */
		if(sourceElements!=0)
			dataItem->version=version;
		}
	
	/* Return the data item to speed up subsequent buffer operations: */
	return dataItem;
	}

void* GLBuffer::startUpdate(GLBuffer::DataItem* dataItem) const
	{
	/* Map the buffer's vertices to CPU memory for update: */
	return glMapBufferARB(bufferType,GL_WRITE_ONLY);
	}

void GLBuffer::finishUpdate(GLBuffer::DataItem* dataItem) const
	{
	/* Unmap the buffer and mark it as up-to-date: */
	glUnmapBufferARB(bufferType);
	dataItem->version=version;
	}

void GLBuffer::unbind(void) const
	{
	/* Unbind any bound buffers of this buffer's type: */
	glBindBufferARB(bufferType,0);
	}
