/***********************************************************************
OrientationSnapperTool - Class to snap the orientation of an input
device such that its axes are all aligned with primary axes.
Copyright (c) 2020 Oliver Kreylos

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

#include <Vrui/Tools/OrientationSnapperTool.h>

#include <Geometry/Vector.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/**********************************************
Methods of class OrientationSnapperToolFactory:
**********************************************/

OrientationSnapperToolFactory::OrientationSnapperToolFactory(ToolManager& toolManager)
	:ToolFactory("OrientationSnapperTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(0,true);
	layout.setNumValuators(0,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* transformToolFactory=toolManager.loadClass("TransformTool");
	transformToolFactory->addChildClass(this);
	addParentClass(transformToolFactory);
	
	/* Set tool class' factory pointer: */
	OrientationSnapperTool::factory=this;
	}

OrientationSnapperToolFactory::~OrientationSnapperToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	OrientationSnapperTool::factory=0;
	}

const char* OrientationSnapperToolFactory::getName(void) const
	{
	return "Orientation Snapper";
	}

Tool* OrientationSnapperToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new OrientationSnapperTool(this,inputAssignment);
	}

void OrientationSnapperToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveOrientationSnapperToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("TransformTool");
	}

extern "C" ToolFactory* createOrientationSnapperToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	OrientationSnapperToolFactory* orientationSnapperToolFactory=new OrientationSnapperToolFactory(*toolManager);
	
	/* Return factory object: */
	return orientationSnapperToolFactory;
	}

extern "C" void destroyOrientationSnapperToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/***********************************************
Static elements of class OrientationSnapperTool:
***********************************************/

OrientationSnapperToolFactory* OrientationSnapperTool::factory=0;

/***************************************
Methods of class OrientationSnapperTool:
***************************************/

OrientationSnapperTool::OrientationSnapperTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:TransformTool(sFactory,inputAssignment)
	{
	}

OrientationSnapperTool::~OrientationSnapperTool(void)
	{
	}

void OrientationSnapperTool::initialize(void)
	{
	/* Let the base class do its thing: */
	TransformTool::initialize();
	
	/* Disable the transformed device's glyph: */
	getInputGraphManager()->getInputDeviceGlyph(transformedDevice).disable();
	
	/* Initialize the virtual input device's position and orientation: */
	// TODO
	}

const ToolFactory* OrientationSnapperTool::getFactory(void) const
	{
	return factory;
	}

void OrientationSnapperTool::frame(void)
	{
	/* Get the source input device's current orientation: */
	const TrackerState::Rotation& rot=sourceDevice->getOrientation();
	
	/* Sort the orientation's axes by their closeness to their closest primary axis: */
	Vector axes[3];
	int comps[3];
	Scalar dists[3];
	Vector cans[3];
	for(int i=0;i<3;++i)
		{
		axes[i]=rot.getDirection(i);
		comps[i]=Geometry::findParallelAxis(axes[i]);
		dists[i]=Math::abs(axes[i][comps[i]]);
		cans[i]=Vector::zero;
		cans[i][comps[i]]=axes[i][comps[i]]>=Scalar(0)?Scalar(1):Scalar(-1);
		}
	
	/* Find the base vectors for the snapped orientation: */
	Vector b0,b1;
	if(dists[0]>=dists[1]&&dists[0]>=dists[2]) // X is closest
		{
		b0=cans[0];
		if(dists[1]>=dists[2]) // Y is next closest
			b1=cans[1];
		else // Z is next closest
			b1=cans[2]^cans[0];
		}
	else if(dists[1]>=dists[2]) // Y is closest
		{
		b1=cans[1];
		if(dists[0]>=dists[2]) // X is next closest
			b0=cans[0];
		else // Z is next closest
			b0=cans[1]^cans[2];
		}
	else // Z is closest
		{
		if(dists[0]>=dists[1]) // X is next closest
			{
			b0=cans[0];
			b1=cans[2]^cans[0];
			}
		else // Y is next closest
			{
			b0=cans[1]^cans[2];
			b1=cans[1];
			}
		}
	
	/* Update the virtual input device's tracking state: */
	TrackerState::Rotation snapped=TrackerState::Rotation::fromBaseVectors(b0,b1);
	transformedDevice->setTrackingState(TrackerState(sourceDevice->getTransformation().getTranslation(),snapped),sourceDevice->getLinearVelocity(),sourceDevice->getAngularVelocity());
	
	/* Update the virtual input device's device ray state so that the selection ray still points in the same direction: */
	transformedDevice->setDeviceRay(snapped.inverseTransform(rot.transform(sourceDevice->getDeviceRayDirection())),sourceDevice->getDeviceRayStart());
	}

}
