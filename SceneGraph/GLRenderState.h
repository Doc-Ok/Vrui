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

#ifndef SCENEGRAPH_GLRENDERSTATE_INCLUDED
#define SCENEGRAPH_GLRENDERSTATE_INCLUDED

#include <Geometry/OrthonormalTransformation.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <GL/GLFrustum.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <GL/Extensions/GLARBShaderObjects.h>
#include <SceneGraph/Geometry.h>

/* Forward declarations: */
class GLContextData;

namespace SceneGraph {

class GLRenderState
	{
	/* Embedded classes: */
	public:
	typedef GLColor<GLfloat,4> Color; // Type for RGBA colors
	typedef Geometry::OrthogonalTransformation<double,3> DOGTransform; // Double-precision orthogonal transformations as internal representations
	typedef GLFrustum<Scalar> Frustum; // Class describing the rendering context's view frustum
	typedef Geometry::AffineTransformation<Scalar,3> TextureTransform; // Affine texture transformation
	
	private:
	struct GLState // Structure to track current OpenGL state to minimize changes
		{
		/* Elements: */
		public:
		GLenum frontFace;
		bool cullingEnabled;
		GLenum culledFace;
		bool lightingEnabled;
		Color emissiveColor;
		bool colorMaterialEnabled;
		int highestTexturePriority; // Priority level of highest enabled texture unit (None=-1, 1D=0, 2D, 3D, cube map)
		GLuint boundTextures[4]; // Texture object IDs of currently bound 1D, 2D, 3D, and cube map textures
		bool separateSpecularColorEnabled;
		GLenum matrixMode; // Current matrix mode
		int activeVertexArraysMask; // Bit mask of currently active vertex arrays, from GLVertexArrayParts
		GLuint vertexBuffer; // ID of currently bound vertex buffer
		GLuint indexBuffer; // ID of currently bound index buffer
		GLhandleARB shaderProgram; // Currently bound shader program, or null
		};
	
	/* Elements: */
	public:
	GLContextData& contextData; // Context data of the current OpenGL context
	private:
	Frustum baseFrustum; // The rendering context's view frustum in initial model coordinates
	Point baseViewerPos; // Viewer position in initial model coordinates
	Vector baseUpVector; // Up vector in initial model coordinates
	DOGTransform currentTransform; // Transformation from initial model coordinates to current model coordinates
	
	/* Elements shadowing current OpenGL state: */
	public:
	GLState initialState; // OpenGL state when render state object was created
	GLState currentState; // Current OpenGL state
	
	/* Private methods: */
	private:
	void changeVertexArraysMask(int currentMask,int newMask); // Changes the set of active vertex arrays
	
	/* Constructors and destructors: */
	public:
	GLRenderState(GLContextData& sContextData,const DOGTransform& initialTransform,const Point& sBaseViewerPos,const Vector& sBaseUpVector); // Creates a render state object
	~GLRenderState(void); // Releases OpenGL state and destroys render state object
	
	/* Methods: */
	Point getViewerPos(void) const // Returns the viewer position in current model coordinates
		{
		return Point(currentTransform.inverseTransform(baseViewerPos));
		}
	Vector getUpVector(void) const // Returns the up direction in current model coordinates
		{
		return Vector(currentTransform.inverseTransform(baseUpVector));
		}
	const DOGTransform& getTransform(void) const // Returns the current model transformation matrix
		{
		return currentTransform;
		}
	DOGTransform pushTransform(const OGTransform& deltaTransform); // Pushes the given transformation onto the matrix stack and returns the previous transformation
	DOGTransform pushTransform(const DOGTransform& deltaTransform); // Ditto, with a double-precision transformation
	DOGTransform pushTransform(const Geometry::OrthonormalTransformation<Scalar,3>& deltaTransform); // Ditto, with an orthonormal transformation
	void popTransform(const DOGTransform& previousTransform); // Resets the matrix stack to the given transformation; must be result from previous pushTransform call
	bool doesBoxIntersectFrustum(const Box& box) const; // Returns true if the given box in current model coordinates intersects the view frustum
	void setTextureTransform(const TextureTransform& newTextureTransform); // Sets the given transformation as the new texture transformation
	void resetTextureTransform(void); // Resets the texture transformation
	
	/* OpenGL state management methods: */
	void setFrontFace(GLenum newFrontFace); // Selects whether counter-clockwise or clockwise polygons are front-facing
	void enableCulling(GLenum newCulledFace); // Enables OpenGL face culling
	void disableCulling(void); // Disables OpenGL face culling
	void enableMaterials(void); // Enables OpenGL material rendering
	void disableMaterials(void); // Disables OpenGL material rendering
	void setEmissiveColor(const Color& newEmissiveColor); // Sets the current emissive color
	void enableTexture1D(void); // Enables OpenGL 1D texture mapping
	void bindTexture1D(GLuint textureObjectId) // Binds a 1D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[0]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_1D,textureObjectId);
			currentState.boundTextures[0]=textureObjectId;
			}
		}
	void enableTexture2D(void); // Enables OpenGL 2D texture mapping
	void bindTexture2D(GLuint textureObjectId) // Binds a 2D texture
		{
		/* Check if the texture to bind is not currently bound: */
		if(currentState.boundTextures[1]!=textureObjectId)
			{
			glBindTexture(GL_TEXTURE_2D,textureObjectId);
			currentState.boundTextures[1]=textureObjectId;
			}
		}
	void disableTextures(void); // Disables OpenGL texture mapping
	void enableVertexArrays(int vertexArraysMask) // Enables the given set of vertex arrays
		{
		/* Activate/deactivate arrays as needed and update the current state: */
		changeVertexArraysMask(currentState.activeVertexArraysMask,vertexArraysMask);
		currentState.activeVertexArraysMask=vertexArraysMask;
		}
	void bindVertexBuffer(GLuint newVertexBuffer) // Binds the given vertex buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.vertexBuffer!=newVertexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,newVertexBuffer);
			currentState.vertexBuffer=newVertexBuffer;
			}
		}
	void bindIndexBuffer(GLuint newIndexBuffer) // Binds the given index buffer
		{
		/* Check if the new buffer is different from the current one: */
		if(currentState.indexBuffer!=newIndexBuffer)
			{
			/* Bind the new buffer and update the current state: */
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,newIndexBuffer);
			currentState.indexBuffer=newIndexBuffer;
			}
		}
	void bindShader(GLhandleARB newShaderProgram); // Binds the shader program of the current handle by calling glUseProgramObjectARB
	void disableShaders(void); // Unbinds any currently-bound shaders and returns to OpenGL fixed functionality
	};

}

#endif
