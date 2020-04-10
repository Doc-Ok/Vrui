/***********************************************************************
FoVRenderer - Vislet class to render field-of-view indicators or varying
sizes into a virtual environment.
Copyright (c) 2018-2019 Oliver Kreylos

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

#ifndef VRUI_VISLETS_FOVRENDERER_INCLUDED
#define VRUI_VISLETS_FOVRENDERER_INCLUDED

#include <vector>
#include <Geometry/ComponentArray.h>
#include <GL/gl.h>
#include <Vrui/Vrui.h>
#include <Vrui/Vislet.h>

namespace Vrui {

namespace Vislets {

class FoVRenderer;

class FoVRendererFactory:public VisletFactory
	{
	friend class FoVRenderer;
	
	/* Elements: */
	private:
	GLfloat lineWidth; // Width of lines to render FoV boundaries
	Color lineColor; // Line color for FoV boundaries
	
	/* Constructors and destructors: */
	public:
	FoVRendererFactory(VisletManager& visletManager);
	virtual ~FoVRendererFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class FoVRenderer:public Vislet
	{
	friend class FoVRendererFactory;
	
	/* Embedded classes: */
	private:
	typedef Geometry::ComponentArray<Scalar,2> FoV; // Type for a field-of-view rectangle in tangent space
	
	/* Elements: */
	private:
	static FoVRendererFactory* factory; // Pointer to the factory object for this class
	bool renderCircles; // Flag to render FoV boundaries as circles, using horizontal FoV, instead of as rectangles
	std::vector<FoV> fovs; // The FoV boundaries to render
	
	/* Constructors and destructors: */
	public:
	FoVRenderer(int numArguments,const char* const arguments[]);
	virtual ~FoVRenderer(void);
	
	/* Methods from class Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
