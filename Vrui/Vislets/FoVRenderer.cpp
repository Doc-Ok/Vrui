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

#include <Vrui/Vislets/FoVRenderer.h>

#include <stdlib.h>
#include <string.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/OrthonormalTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Viewer.h>
#include <Vrui/VisletManager.h>
#include <Vrui/DisplayState.h>

namespace Vrui {

namespace Vislets {

/***********************************
Methods of class FoVRendererFactory:
***********************************/

FoVRendererFactory::FoVRendererFactory(VisletManager& visletManager)
	:VisletFactory("FoVRenderer",visletManager),
	 lineWidth(3.0f),lineColor(0.0f,1.0f,0.0f)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	lineWidth=cfs.retrieveValue<float>("./lineWidth",lineWidth);
	lineColor=cfs.retrieveValue<Color>("./lineColor",lineColor);
	
	/* Set tool class' factory pointer: */
	FoVRenderer::factory=this;
	}

FoVRendererFactory::~FoVRendererFactory(void)
	{
	/* Reset tool class' factory pointer: */
	FoVRenderer::factory=0;
	}

Vislet* FoVRendererFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new FoVRenderer(numArguments,arguments);
	}

void FoVRendererFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveFoVRendererDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createFoVRendererFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	FoVRendererFactory* fovRendererFactory=new FoVRendererFactory(*visletManager);
	
	/* Return factory object: */
	return fovRendererFactory;
	}

extern "C" void destroyFoVRendererFactory(VisletFactory* factory)
	{
	delete factory;
	}

/************************************
Static elements of class FoVRenderer:
************************************/

FoVRendererFactory* FoVRenderer::factory=0;

/****************************
Methods of class FoVRenderer:
****************************/

FoVRenderer::FoVRenderer(int numArguments,const char* const arguments[])
	:renderCircles(false)
	{
	/* Parse the command line: */
	for(int arg=0;arg<numArguments;++arg)
		{
		if(arguments[arg][0]=='-')
			{
			if(strcasecmp(arguments[arg]+1,"circles")==0||strcasecmp(arguments[arg]+1,"C")==0)
				renderCircles=true;
			}
		else
			{
			/* Try to parse a FoV rectangle: */
			if(arg+2<=numArguments&&arguments[arg+1][0]!='-')
				{
				/* Parse the angle pair in degrees: */
				FoV newFov;
				for(int i=0;i<2;++i)
					newFov[i]=Math::abs(Scalar(atof(arguments[arg+i])));
				
				/* Check for tangent-space validity: */
				if(newFov[0]<Scalar(180)&&newFov[1]<Scalar(180))
					{
					/* Convert angle to tangent space: */
					for(int i=0;i<2;++i)
						newFov[i]=Math::tan(Math::div2(Math::rad(newFov[i])));
					
					/* Store the new FoV pair: */
					fovs.push_back(newFov);
					}
				}
			++arg;
			}
		}
	}

FoVRenderer::~FoVRenderer(void)
	{
	}

VisletFactory* FoVRenderer::getFactory(void) const
	{
	return factory;
	}

void FoVRenderer::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(factory->lineWidth);
	
	/* Temporarily disable all clipping planes: */
	contextData.getClipPlaneTracker()->pause();
	
	/* Access current display state: */
	const DisplayState& ds=getDisplayState(contextData);
	
	/* Move into the current viewer's eye space: */
	glPushMatrix();
	glLoadMatrix(ds.modelviewPhysical);
	ONTransform eyeTransform=ONTransform::translateFromOriginTo(ds.eyePosition);
	eyeTransform*=ONTransform::rotate(ds.viewer->getHeadTransformation().getRotation());
	Vector up=ds.viewer->getDeviceUpDirection();
	Vector right=ds.viewer->getDeviceViewDirection()^up;
	eyeTransform*=ONTransform::rotate(Rotation::fromBaseVectors(right,up));
	glMultMatrix(eyeTransform);
	
	/* Draw all FoV rectangles: */
	Scalar z=getFrontplaneDist()*Scalar(-1.01);
	for(std::vector<FoV>::const_iterator fIt=fovs.begin();fIt!=fovs.end();++fIt)
		{
		glBegin(GL_LINE_LOOP);
		glColor(factory->lineColor);
		if(renderCircles)
			{
			/* Render a circle: */
			for(int i=0;i<360;++i)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(i)/Scalar(360);
				glVertex3d((*fIt)[0]*Math::cos(angle)*z,(*fIt)[0]*Math::sin(angle)*z,z);
				}
			}
		else
			{
			/* Render a rectangle: */
			glVertex3d(-(*fIt)[0]*z,-(*fIt)[1]*z,z);
			glVertex3d((*fIt)[0]*z,-(*fIt)[1]*z,z);
			glVertex3d((*fIt)[0]*z,(*fIt)[1]*z,z);
			glVertex3d(-(*fIt)[0]*z,(*fIt)[1]*z,z);
			}
		glEnd();
		}
	
	/* Return to physical coordinates: */
	glPopMatrix();
	
	/* Re-enable clipping: */
	contextData.getClipPlaneTracker()->resume();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

}

}
