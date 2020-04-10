/***********************************************************************
GLNVPrimitiveRestart - OpenGL extension class for the
GL_NV_primitive_restart extension.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef GLEXTENSIONS_GLNVPRIMITIVERESTART_INCLUDED
#define GLEXTENSIONS_GLNVPRIMITIVERESTART_INCLUDED

#include <GL/gl.h>
#include <GL/TLSHelper.h>
#include <GL/Extensions/GLExtension.h>

/********************************
Extension-specific parts of gl.h:
********************************/

#ifndef GL_NV_primitive_restart
#define GL_NV_primitive_restart 1

/* Extension-specific functions: */
typedef void (APIENTRY * PFNGLPRIMITIVERESTARTNVPROC) (void);
typedef void (APIENTRY * PFNGLPRIMITIVERESTARTINDEXNVPROC) (GLuint index);

/* Extension-specific constants: */
#define GL_PRIMITIVE_RESTART_NV           0x8558
#define GL_PRIMITIVE_RESTART_INDEX_NV     0x8559

#endif

/* Forward declarations of friend functions: */
void glPrimitiveRestartNV(void);
void glPrimitiveRestartIndexNV(GLuint index);

class GLNVPrimitiveRestart:public GLExtension
	{
	/* Elements: */
	private:
	static GL_THREAD_LOCAL(GLNVPrimitiveRestart*) current; // Pointer to extension object for current OpenGL context
	static const char* name; // Extension name
	PFNGLPRIMITIVERESTARTNVPROC glPrimitiveRestartNVProc;
	PFNGLPRIMITIVERESTARTINDEXNVPROC glPrimitiveRestartIndexNVProc;
	
	/* Constructors and destructors: */
	private:
	GLNVPrimitiveRestart(void);
	public:
	virtual ~GLNVPrimitiveRestart(void);
	
	/* Methods: */
	public:
	virtual const char* getExtensionName(void) const;
	virtual void activate(void);
	virtual void deactivate(void);
	static bool isSupported(void); // Returns true if the extension is supported in the current OpenGL context
	static void initExtension(void); // Initializes the extension in the current OpenGL context
	
	/* Extension entry points: */
	inline friend void glPrimitiveRestartNV(void)
		{
		GLNVPrimitiveRestart::current->glPrimitiveRestartNVProc();
		}
	inline friend void glPrimitiveRestartIndexNV(GLuint index)
		{
		GLNVPrimitiveRestart::current->glPrimitiveRestartIndexNVProc(index);
		}
	};

/*******************************
Extension-specific entry points:
*******************************/

#endif
