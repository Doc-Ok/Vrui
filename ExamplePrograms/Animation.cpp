/***********************************************************************
Animation - Example program demonstrating data exchange between a
background animation thread and the foreground rendering thread using
a triple buffer, and retained-mode OpenGL rendering using vertex and
index buffers.
Copyright (c) 2014-2018 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <unistd.h>
#include <iostream>
#include <Threads/Thread.h>
#include <Threads/TripleBuffer.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLGeometryVertex.h>
#include <GL/GLVertexBuffer.h>
#include <GL/GLIndexBuffer.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class Animation:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	typedef GLGeometry::Vertex<void,0,void,0,float,float,3> MeshVertex; // Type for mesh vertices storing normal vectors and positions
	typedef GLVertexBuffer<MeshVertex> VertexBuffer; // Type for OpenGL buffers holding mesh vertices
	typedef GLIndexBuffer<GLuint> IndexBuffer; // Type for OpenGL buffers holding mesh vertex indices
	
	/* Elements: */
	private:
	int meshSize[2]; // Width and height of mesh
	Threads::TripleBuffer<MeshVertex*> meshVertices;
	float phase; // Phase angle for mesh animation
	GLMaterial meshMaterialFront,meshMaterialBack; // Material properties to render the mesh from the front and back
	VertexBuffer vertexBuffer; // Buffer holding mesh vertices
	IndexBuffer indexBuffer; // Buffer holding mesh vertex indices
	Threads::Thread animationThread; // Thread object for the background animation thread
	
	/* Private methods: */
	void updateMesh(MeshVertex* mv); // Recalculates all mesh vertices based on the current phase angle
	void* animationThreadMethod(void); // Thread method for the background animation thread
	
	/* Constructors and destructors: */
	public:
	Animation(int& argc,char**& argv);
	virtual ~Animation(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

/**************************
Methods of class Animation:
**************************/

void Animation::updateMesh(Animation::MeshVertex* mv)
	{
	/* Update the z coordinate and normal vector of all mesh vertices: */
	MeshVertex* mPtr=mv;
	for(int y=0;y<meshSize[1];++y)
		for(int x=0;x<meshSize[0];++x,++mPtr)
			{
			/* Calculate the mesh vertex elevation: */
			float radius=Math::sqrt(Math::sqr(mPtr->position[0])+Math::sqr(mPtr->position[1]));
			float z=Math::cos(radius-phase)*(3.0f*Math::Constants<float>::pi-radius*0.5f);
			mPtr->position[2]=z;
			
			/* Calculate the elevation derivative and mesh normal vector: */
			float zprime=-Math::cos(radius-phase)*0.5f-Math::sin(radius-phase)*(3.0f*Math::Constants<float>::pi-radius*0.5f);
			mPtr->normal[0]=-mPtr->position[0]*zprime/radius;
			mPtr->normal[1]=-mPtr->position[1]*zprime/radius;
			mPtr->normal[2]=1.0f;
			mPtr->normal.normalize();
			}
	}

void* Animation::animationThreadMethod(void)
	{
	while(true)
		{
		/* Sleep for approx. 1/60th of a second: */
		usleep(1000000/60);
		
		/* Increment the phase angle by 1 radians/second (assuming we slept 1/60s) and wrap to [-pi, +pi): */
		phase=Math::wrapRad(phase+1.0f/60.0f);
		
		/* Start a new value in the mesh triple buffer: */
		MeshVertex* mv=meshVertices.startNewValue();
		
		/* Recalculate the mesh vertices in the new triple buffer slot based on the current phase angle: */
		updateMesh(mv);
		
		/* Push the new triple buffer slot to the foreground thread: */
		meshVertices.postNewValue();
		
		/* Wake up the foreground thread by requesting a Vrui frame immediately: */
		Vrui::requestUpdate();
		}
	
	return 0;
	}

Animation::Animation(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 meshMaterialFront(GLMaterial::Color(1.0f,0.5f,0.5f),GLMaterial::Color(0.25f,0.25f,0.25f),8.0f),
	 meshMaterialBack(GLMaterial::Color(0.5f,0.5f,1.0f),GLMaterial::Color(0.25f,0.25f,0.25f),8.0f)
	{
	/* Initialize the mesh: */
	meshSize[0]=meshSize[1]=129;
	
	/* Create initial mesh structures in all three slots of the triple buffer (vertices' x and y positions will be reused): */
	for(int i=0;i<3;++i)
		{
		/* Access the i-th triple buffer slot: */
		MeshVertex*& mv=meshVertices.getBuffer(i);
		
		/* Allocate the in-memory vertex array: */
		mv=new MeshVertex[meshSize[1]*meshSize[0]];
		
		/* Fully initialize all mesh vertices: */
		MeshVertex* mPtr=mv;
		float centerx=float(meshSize[0]-1)*0.5f;
		float centery=float(meshSize[1]-1)*0.5f;
		float scale=6.0f*Math::Constants<float>::pi/Math::min(centerx,centery);
		for(int y=0;y<meshSize[1];++y)
			for(int x=0;x<meshSize[0];++x,++mPtr)
				{
				mPtr->normal=MeshVertex::Normal::zero;
				mPtr->position=MeshVertex::Position((float(x)-centerx)*scale,(float(y)-centery)*scale,0.0f);
				}
		}
	
	/* Initialize animation state: */
	phase=0.0f;
	
	/* Calculate the first full mesh state in a new triple buffer slot: */
	updateMesh(meshVertices.startNewValue());
	meshVertices.postNewValue();
	
	/* Initialize the index buffer for manual updates: */
	indexBuffer.setSource((meshSize[1]-1)*meshSize[0]*2,0);
	
	/* Start the background animation thread: */
	animationThread.start(this,&Animation::animationThreadMethod);
	}

Animation::~Animation(void)
	{
	/* Shut down the background animation thread: */
	animationThread.cancel();
	animationThread.join();
	
	/* Delete the in-memory vertex arrays in all three triple buffer slots: */
	for(int i=0;i<3;++i)
		delete[] meshVertices.getBuffer(i);
	}

void Animation::frame(void)
	{
	/* Check if there is a new entry in the triple buffer and lock it: */
	if(meshVertices.lockNewValue())
		{
		/* Point the vertex buffer to the new mesh vertices: */
		vertexBuffer.setSource(meshSize[1]*meshSize[0],meshVertices.getLockedValue());
		}
	}

void Animation::display(GLContextData& contextData) const
	{
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT);
	
	/* Enable double-sided lighting: */
	glDisable(GL_CULL_FACE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
	
	/* Set up a surface material: */
	glMaterial(GLMaterialEnums::FRONT,meshMaterialFront);
	glMaterial(GLMaterialEnums::BACK,meshMaterialBack);
	
	/* Bind the vertex buffer (which automatically uploads any new vertex data): */
	vertexBuffer.bind(contextData);
	
	/* Bind the index buffer and check if it is outdated (happens only on first call): */
	IndexBuffer::DataItem* ibdi=indexBuffer.bind(contextData);
	if(indexBuffer.needsUpdate(ibdi))
		{
		/* Upload mesh vertex indices into the index buffer: */
		IndexBuffer::Index* indexPtr=indexBuffer.startUpdate(ibdi);
		for(int y=1;y<meshSize[1];++y)
			for(int x=0;x<meshSize[0];++x,indexPtr+=2)
				{
				indexPtr[0]=IndexBuffer::Index(y*meshSize[0]+x);
				indexPtr[1]=IndexBuffer::Index((y-1)*meshSize[0]+x);
				}
		
		/* Mark the index buffer as up-to-date: */
		indexBuffer.finishUpdate(ibdi);
		}
	
	/* Draw the mesh as a sequence of quad strips: */
	GLint start=0;
	for(int y=1;y<meshSize[1];++y,start+=meshSize[0]*2)
		indexBuffer.draw(GL_QUAD_STRIP,start,meshSize[0]*2,ibdi);
	
	/* Unbind the buffers: */
	indexBuffer.unbind();
	vertexBuffer.unbind();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void Animation::resetNavigation(void)
	{
	/* Center and scale the object: */
	Vrui::setNavigationTransformation(Vrui::Point::origin,9.0*Math::Constants<double>::pi,Vrui::Vector(0,1,0));
	}

VRUI_APPLICATION_RUN(Animation)
