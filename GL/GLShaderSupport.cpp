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

#include <GL/GLShaderSupport.h>

#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBVertexShader.h>

bool glAreShaderExtensionsSupported(void)
	{
	/* Check all required extensions: */
	return GLARBShaderObjects::isSupported()&&GLARBFragmentShader::isSupported()&&GLARBVertexShader::isSupported();
	}

void glInitShaderExtensions(void)
	{
	/* Initialize all required extensions: */
	GLARBShaderObjects::initExtension();
	GLARBFragmentShader::initExtension();
	GLARBVertexShader::initExtension();
	}

GLhandleARB glCompileAndLinkShaderFromStrings(const char* vertexShaderSource,const char* fragmentShaderSource)
	{
	/* Create a vertex shader object and compile the vertex shader source: */
	GLhandleARB vertexShader=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	glCompileShaderFromString(vertexShader,vertexShaderSource);
	
	/* Create a fragment shader object and compile the fragment shader source: */
	GLhandleARB fragmentShader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	glCompileShaderFromString(fragmentShader,fragmentShaderSource);
	
	/* Link the vertex and fragment shaders into a shader program: */
	GLhandleARB programObject=glLinkShader(vertexShader,fragmentShader);
	
	/* Delete the vertex and fragment shaders: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(fragmentShader);
	
	/* Return the shader program object: */
	return programObject;
	}

