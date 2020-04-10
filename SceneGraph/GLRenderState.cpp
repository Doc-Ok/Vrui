/***********************************************************************
GLRenderState - Class encapsulating the traversal state of a scene graph
during OpenGL rendering.
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

#include <SceneGraph/GLRenderState.h>

#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLTexEnvTemplates.h>
#include <GL/GLVertexArrayParts.h>
#include <GL/GLTransformationWrappers.h>

namespace SceneGraph {

void GLRenderState::changeVertexArraysMask(int currentMask,int newMask)
	{
	/* Enable those vertex arrays that are active in the new state but not in the current state: */
	int onMask=newMask&(~currentMask);
	GLVertexArrayParts::enable(onMask);
	
	/* Disable those vertex arrays that are active in the current state but not in the new state: */
	int offMask=currentMask&(~newMask);
	GLVertexArrayParts::disable(offMask);
	}

/******************************
Methods of class GLRenderState:
******************************/

GLRenderState::GLRenderState(GLContextData& sContextData,const GLRenderState::DOGTransform& initialTransform,const Point& sBaseViewerPos,const Vector& sBaseUpVector)
	:contextData(sContextData),
	 baseViewerPos(sBaseViewerPos),baseUpVector(sBaseUpVector),
	 currentTransform(initialTransform)
	{
	/* Install the initial model transformations: */
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrix(currentTransform);
	
	/* Initialize the view frustum from the current OpenGL context: */
	baseFrustum.setFromGL();
	
	/* Initialize OpenGL state tracking elements: */
	GLint tempFrontFace;
	glGetIntegerv(GL_FRONT_FACE,&tempFrontFace);
	initialState.frontFace=tempFrontFace;
	initialState.cullingEnabled=glIsEnabled(GL_CULL_FACE);
	GLint tempCulledFace;
	glGetIntegerv(GL_CULL_FACE_MODE,&tempCulledFace);
	initialState.culledFace=tempCulledFace;
	initialState.lightingEnabled=glIsEnabled(GL_LIGHTING);
	initialState.emissiveColor=Color(0.0f,0.0f,0.0f);
	if(initialState.lightingEnabled)
		{
		glEnable(GL_NORMALIZE);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,initialState.cullingEnabled?GL_FALSE:GL_TRUE);
		}
	else
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	initialState.colorMaterialEnabled=glIsEnabled(GL_COLOR_MATERIAL);
	initialState.highestTexturePriority=-1;
	if(glIsEnabled(GL_TEXTURE_1D))
		initialState.highestTexturePriority=0;
	if(glIsEnabled(GL_TEXTURE_2D))
		initialState.highestTexturePriority=1;
	initialState.boundTextures[3]=initialState.boundTextures[2]=initialState.boundTextures[1]=initialState.boundTextures[0]=0;
	GLint lightModelColorControl;
	glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL,&lightModelColorControl);
	initialState.separateSpecularColorEnabled=lightModelColorControl==GL_SEPARATE_SPECULAR_COLOR;
	initialState.matrixMode=GL_MODELVIEW;
	initialState.activeVertexArraysMask=0x0;
	initialState.vertexBuffer=0;
	initialState.indexBuffer=0;
	GLint programHandle;
	glGetIntegerv(GL_CURRENT_PROGRAM,&programHandle);
	initialState.shaderProgram=GLhandleARB(programHandle);
	
	/* Copy current state from initial state: */
	currentState=initialState;
	}

namespace {

/****************
Helper functions:
****************/

void setGLState(GLenum flag,bool value)
	{
	/* Enable or disable the state component: */
	if(value)
		glEnable(flag);
	else
		glDisable(flag);
	}
}

GLRenderState::~GLRenderState(void)
	{
	/* Unbind all bound texture objects: */
	if(currentState.boundTextures[0]!=0)
		glBindTexture(GL_TEXTURE_1D,0);
	if(currentState.boundTextures[1]!=0)
		glBindTexture(GL_TEXTURE_2D,0);
	if(currentState.boundTextures[2]!=0)
		glBindTexture(GL_TEXTURE_3D,0);
	if(currentState.boundTextures[3]!=0)
		glBindTexture(GL_TEXTURE_CUBE_MAP,0);
	
	/* Reset texture mapping: */
	if(initialState.highestTexturePriority<3&&currentState.highestTexturePriority>=3)
		glDisable(GL_TEXTURE_CUBE_MAP);
	if(initialState.highestTexturePriority<2&&currentState.highestTexturePriority>=2)
		glDisable(GL_TEXTURE_3D);
	if(initialState.highestTexturePriority<1&&currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(initialState.highestTexturePriority<0&&currentState.highestTexturePriority>=0)
		glDisable(GL_TEXTURE_1D);
	if(initialState.highestTexturePriority==3&&currentState.highestTexturePriority<3)
		glEnable(GL_TEXTURE_CUBE_MAP);
	if(initialState.highestTexturePriority==2&&currentState.highestTexturePriority<2)
		glEnable(GL_TEXTURE_3D);
	if(initialState.highestTexturePriority==1&&currentState.highestTexturePriority<1)
		glEnable(GL_TEXTURE_2D);
	if(initialState.highestTexturePriority==0&&currentState.highestTexturePriority<0)
		glEnable(GL_TEXTURE_1D);
	
	/* Reset other state back to initial state: */
	if(initialState.frontFace!=currentState.frontFace)
		glFrontFace(initialState.frontFace);
	if(initialState.cullingEnabled!=currentState.cullingEnabled)
		setGLState(GL_CULL_FACE,initialState.cullingEnabled);
	if(initialState.culledFace!=currentState.culledFace)
		glCullFace(initialState.culledFace);
	if(initialState.lightingEnabled!=currentState.lightingEnabled)
		{
		setGLState(GL_LIGHTING,initialState.lightingEnabled);
		if(initialState.lightingEnabled)
			glEnable(GL_NORMALIZE);
		}
	if(currentState.lightingEnabled&&!currentState.cullingEnabled)
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
	if(initialState.colorMaterialEnabled!=currentState.colorMaterialEnabled)
		setGLState(GL_COLOR_MATERIAL,initialState.colorMaterialEnabled);
	if(initialState.separateSpecularColorEnabled!=currentState.separateSpecularColorEnabled)
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,initialState.separateSpecularColorEnabled?GL_SEPARATE_SPECULAR_COLOR:GL_SINGLE_COLOR);
	
	/* Reset the matrix mode: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		glMatrixMode(GL_MODELVIEW);
	
	/* Reset active vertex arrays: */
	changeVertexArraysMask(currentState.activeVertexArraysMask,initialState.activeVertexArraysMask);
	
	/* Unbind active vertex and index buffers: */
	if(currentState.vertexBuffer!=initialState.vertexBuffer)
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,initialState.vertexBuffer);
	if(currentState.indexBuffer!=initialState.indexBuffer)
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,initialState.indexBuffer);
	
	/* Reset the bound shader program: */
	if(initialState.shaderProgram!=currentState.shaderProgram)
		glUseProgramObjectARB(initialState.shaderProgram);
	}

GLRenderState::DOGTransform GLRenderState::pushTransform(const OGTransform& deltaTransform)
	{
	/* Update the current transformation: */
	DOGTransform result=currentTransform;
	currentTransform*=deltaTransform;
	currentTransform.renormalize();
	
	/* Set up the new transformation: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		{
		glMatrixMode(GL_MODELVIEW);
		currentState.matrixMode=GL_MODELVIEW;
		}
	glLoadMatrix(currentTransform);
	
	return result;
	}

GLRenderState::DOGTransform GLRenderState::pushTransform(const GLRenderState::DOGTransform& deltaTransform)
	{
	/* Update the current transformation: */
	DOGTransform result=currentTransform;
	currentTransform*=deltaTransform;
	currentTransform.renormalize();
	
	/* Set up the new transformation: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		{
		glMatrixMode(GL_MODELVIEW);
		currentState.matrixMode=GL_MODELVIEW;
		}
	glLoadMatrix(currentTransform);
	
	return result;
	}

GLRenderState::DOGTransform GLRenderState::pushTransform(const Geometry::OrthonormalTransformation<Scalar,3>& deltaTransform)
	{
	/* Update the current transformation: */
	DOGTransform result=currentTransform;
	currentTransform*=deltaTransform;
	currentTransform.renormalize();
	
	/* Set up the new transformation: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		{
		glMatrixMode(GL_MODELVIEW);
		currentState.matrixMode=GL_MODELVIEW;
		}
	glLoadMatrix(currentTransform);
	
	return result;
	}

void GLRenderState::popTransform(const GLRenderState::DOGTransform& previousTransform)
	{
	/* Reinstate the current transformation: */
	currentTransform=previousTransform;
	
	/* Set up the new transformation: */
	if(currentState.matrixMode!=GL_MODELVIEW)
		{
		glMatrixMode(GL_MODELVIEW);
		currentState.matrixMode=GL_MODELVIEW;
		}
	glLoadMatrix(currentTransform);
	}

bool GLRenderState::doesBoxIntersectFrustum(const Box& box) const
	{
	/* Get the current transformation's direction axes: */
	Vector axis[3];
	for(int i=0;i<3;++i)
		axis[i]=currentTransform.getDirection(i);
	
	/* Check the box against each frustum plane: */
	for(int planeIndex=0;planeIndex<6;++planeIndex)
		{
		/* Get the frustum plane's normal vector: */
		const Vector& normal=baseFrustum.getFrustumPlane(planeIndex).getNormal();
		
		/* Find the point on the bounding box which is closest to the frustum plane: */
		Point p;
		for(int i=0;i<3;++i)
			p[i]=normal*axis[i]>Scalar(0)?box.max[i]:box.min[i];
		
		/* Check if the point is inside the view frustum: */
		if(!baseFrustum.getFrustumPlane(planeIndex).contains(currentTransform.transform(p)))
			return false;
		}
	
	return true;
	}

void GLRenderState::setTextureTransform(const GLRenderState::TextureTransform& newTextureTransform)
	{
	/* Set up the new texture transformation: */
	if(currentState.matrixMode!=GL_TEXTURE)
		{
		glMatrixMode(GL_TEXTURE);
		currentState.matrixMode=GL_TEXTURE;
		}
	glLoadMatrix(newTextureTransform);
	}

void GLRenderState::resetTextureTransform(void)
	{
	/* Reset the texture transformation: */
	if(currentState.matrixMode!=GL_TEXTURE)
		{
		glMatrixMode(GL_TEXTURE);
		currentState.matrixMode=GL_TEXTURE;
		}
	glLoadIdentity();
	}

void GLRenderState::setFrontFace(GLenum newFrontFace)
	{
	if(currentState.frontFace!=newFrontFace)
		{
		glFrontFace(newFrontFace);
		currentState.frontFace=newFrontFace;
		}
	}

void GLRenderState::enableCulling(GLenum newCulledFace)
	{
	if(!currentState.cullingEnabled)
		{
		glEnable(GL_CULL_FACE);
		if(currentState.lightingEnabled)
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		currentState.cullingEnabled=true;
		}
	if(currentState.culledFace!=newCulledFace)
		{
		glCullFace(newCulledFace);
		currentState.culledFace=newCulledFace;
		}
	}

void GLRenderState::disableCulling(void)
	{
	if(currentState.cullingEnabled)
		{
		glDisable(GL_CULL_FACE);
		if(currentState.lightingEnabled)
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		currentState.cullingEnabled=false;
		}
	}

void GLRenderState::enableMaterials(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	if(!currentState.lightingEnabled)
		{
		glEnable(GL_LIGHTING);
		glEnable(GL_NORMALIZE);
		if(!currentState.cullingEnabled)
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
		if(currentState.highestTexturePriority>=0)
			glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::MODULATE);
		currentState.lightingEnabled=true;
		}
	if(!currentState.colorMaterialEnabled)
		{
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
		currentState.colorMaterialEnabled=true;
		}
	if(currentState.highestTexturePriority>=0&&!currentState.separateSpecularColorEnabled)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.separateSpecularColorEnabled=true;
		}
	}

void GLRenderState::disableMaterials(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	if(currentState.lightingEnabled)
		{
		glDisable(GL_LIGHTING);
		glDisable(GL_NORMALIZE);
		if(!currentState.cullingEnabled)
			glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);
		if(currentState.highestTexturePriority>=0)
			glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,GLTexEnvEnums::REPLACE);
		currentState.lightingEnabled=false;
		}
	if(currentState.colorMaterialEnabled)
		{
		glDisable(GL_COLOR_MATERIAL);
		currentState.colorMaterialEnabled=false;
		}
	if(currentState.separateSpecularColorEnabled)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		currentState.separateSpecularColorEnabled=false;
		}
	glColor(currentState.emissiveColor);
	}

void GLRenderState::setEmissiveColor(const Color& newEmissiveColor)
	{
	currentState.emissiveColor=newEmissiveColor;
	glColor(newEmissiveColor);
	}

void GLRenderState::enableTexture1D(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	bool textureEnabled=currentState.highestTexturePriority>=0;
	if(currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(currentState.highestTexturePriority<0)
		glEnable(GL_TEXTURE_1D);
	currentState.highestTexturePriority=0;
	
	if(!textureEnabled)
		glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,currentState.lightingEnabled?GLTexEnvEnums::MODULATE:GLTexEnvEnums::REPLACE);
	if(currentState.lightingEnabled&&!currentState.separateSpecularColorEnabled)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.separateSpecularColorEnabled=true;
		}
	}

void GLRenderState::enableTexture2D(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	bool textureEnabled=currentState.highestTexturePriority>=0;
	if(currentState.highestTexturePriority<1)
		glEnable(GL_TEXTURE_2D);
	currentState.highestTexturePriority=1;
	
	if(!textureEnabled)
		glTexEnvMode(GLTexEnvEnums::TEXTURE_ENV,currentState.lightingEnabled?GLTexEnvEnums::MODULATE:GLTexEnvEnums::REPLACE);
	if(currentState.lightingEnabled&&!currentState.separateSpecularColorEnabled)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		currentState.separateSpecularColorEnabled=true;
		}
	}

void GLRenderState::disableTextures(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	if(currentState.highestTexturePriority>=1)
		glDisable(GL_TEXTURE_2D);
	if(currentState.highestTexturePriority>=0)
		glDisable(GL_TEXTURE_1D);
	currentState.highestTexturePriority=-1;
	
	if(currentState.separateSpecularColorEnabled)
		{
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
		currentState.separateSpecularColorEnabled=false;
		}
	}

void GLRenderState::bindShader(GLhandleARB newShaderProgram)
	{
	if(currentState.shaderProgram!=newShaderProgram)
		{
		glUseProgramObjectARB(newShaderProgram);
		currentState.shaderProgram=newShaderProgram;
		}
	}

void GLRenderState::disableShaders(void)
	{
	if(currentState.shaderProgram!=0)
		{
		glUseProgramObjectARB(0);
		currentState.shaderProgram=0;
		}
	}

}
