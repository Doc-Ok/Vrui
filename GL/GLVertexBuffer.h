/***********************************************************************
GLVertexBuffer - Class to represent OpenGL vertex buffer objects
containg typed vertices.
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

#ifndef GLVERTEXBUFFER_INCLUDED
#define GLVERTEXBUFFER_INCLUDED

#include <GL/gl.h>
#include <GL/GLBuffer.h>

template <class VertexParam>
class GLVertexBuffer:public GLBuffer
	{
	/* Embedded classes: */
	public:
	typedef VertexParam Vertex; // Type for vertices stored in the vertex buffer
	
	/* Constructors and destructors: */
	public:
	GLVertexBuffer(void); // Creates zero-sized buffer with default parameters
	GLVertexBuffer(size_t sNumVertices,const Vertex* sSourceVertices,GLenum sBufferUsage =GL_DYNAMIC_DRAW_ARB); // Creates a buffer for the given source vertex array and usage pattern
	
	/* Methods from GLBuffer: */
	size_t getNumVertices(void) const
		{
		/* Call the base class method: */
		return GLBuffer::getNumElements();
		}
	const Vertex* getSourceVertices(void) const
		{
		/* Call the base class method and cast the result to a pointer of correct type: */
		return static_cast<const Vertex*>(GLBuffer::getSourceElements());
		}
	void setSource(size_t newNumVertices,const Vertex* newSourceVertices)
		{
		/* Call the base class method: */
		GLBuffer::setSource(newNumVertices,newSourceVertices);
		}
	DataItem* bind(GLContextData& contextData) const; // Binds the buffer and prepares for vertex rendering
	Vertex* startUpdate(DataItem* dataItem) const
		{
		/* Call the base class method and cast the result to a pointer of correct type: */
		return static_cast<Vertex*>(GLBuffer::startUpdate(dataItem));
		}
	void unbind(void) const; // Disables vertex rendering and unbinds the buffer
	
	/* New methods: */
	void draw(GLenum mode,DataItem* dataItem) const; // Draws the bound and up-to-date buffer's vertices using the given primitive mode
	void draw(GLenum mode,GLint first,GLsizei count,DataItem* dataItem) const; // Draws the given subset of the bound and up-to-date buffer's vertices using the given primitive mode
	};

#ifndef GLVERTEXBUFFER_IMPLEMENTATION
#include <GL/GLVertexBuffer.icpp>
#endif

#endif
