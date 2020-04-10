/***********************************************************************
GLShaderSupport - Helper functions to simplify managing GLSL shaders.
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

#ifndef GLSHADERSUPPORT_INCLUDED
#define GLSHADERSUPPORT_INCLUDED

#include <GL/gl.h>
#include <GL/Extensions/GLARBShaderObjects.h>

/****************
Helper functions:
****************/

bool glAreShaderExtensionsSupported(void); // Returns true if all OpenGL extensions required for simplified shader support are supported by local OpenGL
void glInitShaderExtensions(void); // Initializes OpenGL extensions required for simplified shader support; throws exception if not all are supported by local OpenGL
GLhandleARB glCompileAndLinkShaderFromStrings(const char* vertexShaderSource,const char* fragmentShaderSource); // Compiles one vertex and one fragment shader from strings and links them into a shader program; throws exception on errors

#endif
