/***********************************************************************
CAVERenderer - Vislet class to render the default KeckCAVES backround
image seamlessly inside a VR application.
Copyright (c) 2005-2019 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_VISLETS_CAVERENDERER_INCLUDED
#define VRUI_VISLETS_CAVERENDERER_INCLUDED

#include <string>
#include <Geometry/OrthogonalTransformation.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <Images/BaseImage.h>
#include <Vrui/Geometry.h>
#include <Vrui/Vislet.h>

/* Forward declarations: */
namespace Vrui {
class Lightsource;
class VisletManager;
}

namespace Vrui {

namespace Vislets {

class CAVERenderer;

class CAVERendererFactory:public VisletFactory
	{
	friend class CAVERenderer;
	
	/* Elements: */
	private:
	bool alignToEnvironment; // Flag to enable automatic alignment of the rendered CAVE environment to the configured VR environment
	GLMaterial surfaceMaterial; // Material properties to use for the CAVE walls and floor
	int tilesPerFoot; // Number of tiles to subdivide each foot of the wall of floor rectangle into
	std::string wallTextureFileName; // Name of image file to map onto CAVE walls
	std::string floorTextureFileName; // Name of image file to map onto CAVE floor
	
	/* Constructors and destructors: */
	public:
	CAVERendererFactory(VisletManager& visletManager);
	virtual ~CAVERendererFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class CAVERenderer:public Vislet,public GLObject
	{
	friend class CAVERendererFactory;
	
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint wallTextureObjectId; // Texture object ID of a CAVE wall
		GLuint floorTextureObjectId; // Texture object ID of the CAVE floor
		GLuint wallDisplayListId; // Display list to render a CAVE wall
		GLuint floorDisplayListId; // Display list to render the CAVE floor
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	static CAVERendererFactory* factory; // Pointer to the factory object for this class
	
	/* Renderer properties: */
	OGTransform caveTransform; // Transformation to align the CAVE model with the local VR environment
	GLMaterial surfaceMaterial;
	int tilesPerFoot;
	Images::BaseImage wallTextureImage;
	Images::BaseImage floorTextureImage;
	Vrui::Lightsource* lightsources[4]; // Pointers to the four static ceiling light sources
	
	/* Saved viewer headlight states: */
	int numViewers; // Number of saved viewer headlight states
	bool* viewerHeadlightStates; // Array of enable flags for each viewer's headlight
	
	/* Animation state: */
	double angle; // Animation angle, from 0 to 720 degrees for the four folding segments
	double angleAnimStep; // Animation step value per second
	double lastFrame; // Application time of last frame
	
	/* Private methods: */
	void renderWall(DataItem* dataItem) const;
	void renderFloor(DataItem* dataItem) const;
	
	/* Constructors and destructors: */
	public:
	CAVERenderer(int numArguments,const char* const arguments[]);
	virtual ~CAVERenderer(void);
	
	/* Methods: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void enable(bool startup);
	virtual void disable(bool shutdown);
	virtual void initContext(GLContextData& contextData) const;
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
