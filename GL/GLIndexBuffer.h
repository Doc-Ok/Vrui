/***********************************************************************
GLIndexBuffer - Class to represent OpenGL index buffer objects containg
typed indices.
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

#ifndef GLINDEXBUFFER_INCLUDED
#define GLINDEXBUFFER_INCLUDED

#include <GL/gl.h>
#include <GL/GLBuffer.h>

template <class IndexParam>
class GLIndexBuffer:public GLBuffer
	{
	/* Embedded classes: */
	public:
	typedef IndexParam Index; // Type for indices stored in the index buffer
	
	/* Constructors and destructors: */
	public:
	GLIndexBuffer(void); // Creates zero-sized buffer with default parameters
	GLIndexBuffer(size_t sNumIndices,const Index* sSourceIndices,GLenum sBufferUsage =GL_DYNAMIC_DRAW_ARB); // Creates a buffer for the given source index array and usage pattern
	
	/* Methods from GLBuffer: */
	size_t getNumIndices(void) const
		{
		/* Call the base class method: */
		return GLBuffer::getNumElements();
		}
	const Index* getSourceIndices(void) const
		{
		/* Call the base class method and cast the result to a pointer of correct type: */
		return static_cast<const Index*>(GLBuffer::getSourceElements());
		}
	void setSource(size_t newNumIndices,const Index* newSourceIndices)
		{
		/* Call the base class method: */
		GLBuffer::setSource(newNumIndices,newSourceIndices);
		}
	Index* startUpdate(DataItem* dataItem) const
		{
		/* Call the base class method and cast the result to a pointer of correct type: */
		return static_cast<Index*>(GLBuffer::startUpdate(dataItem));
		}
	
	/* New methods: */
	void draw(GLenum mode,GLint first,GLsizei count,DataItem* dataItem) const; // Draws vertices from a bound vertex buffer using indices from the bound and up-to-date index buffer
	};

#ifndef GLINDEXBUFFER_IMPLEMENTATION
#include <GL/GLIndexBuffer.icpp>
#endif

#endif
