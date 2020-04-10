/***********************************************************************
GLBuffer - Common base class for OpenGL vertex (GL_ARRAY_BUFFER_ARB) or
index (GL_ELEMENT_ARRAY_BUFFER_ARB) buffers.
Copyright (c) 2018-2020 Oliver Kreylos

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

#ifndef GLBUFFER_INCLUDED
#define GLBUFFER_INCLUDED

#include <stddef.h>
#include <GL/gl.h>
#include <GL/GLObject.h>

/* Forward declarations: */
class GLContextData;

class GLBuffer:public GLObject // Base class for OpenGL buffer objects
	{
	/* Embedded classes: */
	public:
	struct DataItem:public GLObject::DataItem
		{
		friend class GLBuffer;
		
		/* Elements: */
		private:
		GLuint bufferObjectId; // ID of the buffer object storing the vertex or index array in GPU memory
		unsigned int parameterVersion; // Version number of the buffer's parameters (size and usage pattern)
		unsigned int version; // Version number of the vertex or index data in GPU memory
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	GLenum bufferType; // Type of buffer (GL_ARRAY_BUFFER_ARB or GL_ELEMENT_ARRAY_BUFFER_ARB)
	size_t elementSize; // Size of a single buffer element (vertex or index) in bytes
	size_t numElements; // Number of elements in the buffer
	const void* sourceElements; // Pointer to source vertex or index data in CPU memory; if zero, data needs to be manually uploaded at bind()-time
	GLenum bufferUsage; // Usage pattern for the data buffer
	unsigned int parameterVersion; // Version number of buffer parameters (size and usage pattern)
	unsigned int version; // Version number of the vertex or index data in CPU memory
	
	/* Constructors and destructors: */
	public:
	GLBuffer(GLenum sBufferType,size_t sElementSize); // Creates zero-sized buffer with default parameters for elements of the given size
	GLBuffer(GLenum sBufferType,size_t sElementSize,size_t sNumElements,const void* sSourceElements,GLenum sBufferUsage =GL_DYNAMIC_DRAW_ARB); // Creates a buffer for elements of the given size with the given source vertex or index array and usage pattern
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	
	/* Methods to be called from application code: */
	size_t getNumElements(void) const // Returns the number of vertices or indices in the buffer
		{
		return numElements;
		}
	const void* getSourceElements(void) const // Returns a pointer to the source vertex or index data in CPU memory
		{
		return sourceElements;
		}
	GLenum getBufferUsage(void) const // Returns the buffer usage pattern
		{
		return bufferUsage;
		}
	void invalidate(void); // Invalidates the buffer when the source vertex array is changed externally
	void setSource(size_t newNumElements,const void* newSourceElements); // Changes the source vertex or index data; causes a re-upload of buffer contents on the next bind()
	void setBufferUsage(GLenum newBufferUsage); // Changes the buffer usage pattern; causes a re-upload of buffer contents on next bind()
	
	/* Methods to be called from inside an active OpenGL context: */
	DataItem* bind(GLContextData& contextData) const; // Binds the buffer to the current OpenGL context and returns a data item for subsequent manual update or drawing calls
	bool needsUpdate(DataItem* dataItem) const // Returns true if the buffer's contents need to be updated manually
		{
		return dataItem->version!=version;
		}
	void* startUpdate(DataItem* dataItem) const; // Returns a pointer to upload vertex or index data into the buffer
	void finishUpdate(DataItem* dataItem) const; // Finishes updating the buffer and prepares it for subsequent drawing operations
	void unbind(void) const; // Unbinds any active buffers of this buffer's type from the current OpenGL context
	};

#endif
