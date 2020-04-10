/***********************************************************************
GLSphereRenderer - Class to render spheres as ray-cast impostors.
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

#ifndef GLSPHERERENDERER_INCLUDED
#define GLSPHERERENDERER_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>

/* Forward declarations: */
class GLLightTracker;

class GLSphereRenderer:public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem // Structure to store per-context state
		{
		/* Elements: */
		public:
		GLhandleARB vertexShader; // Vertex shader to render impostor spheres
		GLhandleARB geometryShader; // Geometry shader to render impostor spheres
		GLhandleARB fragmentShader; // Fragment shader to render impostor spheres
		GLhandleARB shaderProgram; // Shader program to render impostor spheres
		GLint shaderProgramUniforms[1]; // Locations of the sphere shader program's uniform variables
		unsigned int settingsVersion; // Version number of current sphere renderer settings reflected in the shader program
		unsigned int lightStateVersion; // Version number for current lighting state reflected in the shader program
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	bool fixedRadius; // Flag if all spheres are rendered with the same model-space radius
	GLfloat radius; // Model-space radius for all spheres
	bool colorMaterial; // Flag whether each sphere's ambient and diffuse material components follow the OpenGL color
	unsigned int settingsVersion; // Version number of current sphere renderer settings
	
	/* Private methods: */
	void compileShader(DataItem* dataItem,const GLLightTracker& lightTracker) const; // Compiles the sphere shader program based on current settings and lighting state
	
	/* Constructors and destructors: */
	public:
	GLSphereRenderer(void);
	virtual ~GLSphereRenderer(void);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	bool isFixedRadius(void) const // Returns true if all spheres use the same model-space radius
		{
		return fixedRadius;
		}
	GLfloat getFixedRadius(void) const // Returns the model-space radius used for all spheres if fixed radius is enabled
		{
		return radius;
		}
	bool isColorMaterial(void) const // Returns true if spheres' ambient and diffuse material properties follow the current color
		{
		return colorMaterial;
		}
	void setFixedRadius(GLfloat newFixedRadius); // Forces to render all spheres using the given model-space radius
	void setVariableRadius(void); // Renders each sphere with the model-view radius defined by its position's w component
	void setColorMaterial(bool newColorMaterial); // Sets the color material flag
	void enable(GLfloat modelViewScale,GLContextData& contextData) const; // Enables sphere rendering for subsequent GL_POINT primitives with the given model view matrix scale factor
	void disable(GLContextData& contextData) const; // Disables sphere rendering
	};

#endif
