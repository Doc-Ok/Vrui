/***********************************************************************
MultiDeviceNavigationTool - Class to use multiple 3-DOF devices for full
navigation (translation, rotation, scaling).
Copyright (c) 2007-2019 Oliver Kreylos

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

#include <Vrui/Tools/MultiDeviceNavigationTool.h>

#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Geometry/Vector.h>
#include <Geometry/AffineCombiner.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

namespace Vrui {

/****************************************************************
Methods of class MultiDeviceNavigationToolFactory::Configuration:
****************************************************************/

MultiDeviceNavigationToolFactory::Configuration::Configuration(void)
	:translationFactor(1),
	 minRotationScalingDistance(getPointPickDistance()*getNavigationTransformation().getScaling()),
	 rotationFactor(1),
	 scalingFactor(1),
	 mutualExclusion(false)
	{
	}

void MultiDeviceNavigationToolFactory::Configuration::read(const Misc::ConfigurationFileSection& cfs)
	{
	translationFactor=cfs.retrieveValue<Scalar>("./translationFactor",translationFactor);
	minRotationScalingDistance=cfs.retrieveValue<Scalar>("./minRotationScalingDistance",minRotationScalingDistance);
	rotationFactor=cfs.retrieveValue<Scalar>("./rotationFactor",rotationFactor);
	scalingFactor=cfs.retrieveValue<Scalar>("./scalingFactor",scalingFactor);
	mutualExclusion=cfs.retrieveValue<bool>("./mutualExclusion",mutualExclusion);
	}

void MultiDeviceNavigationToolFactory::Configuration::write(Misc::ConfigurationFileSection& cfs) const
	{
	cfs.storeValue<Scalar>("./translationFactor",translationFactor);
	cfs.storeValue<Scalar>("./minRotationScalingDistance",minRotationScalingDistance);
	cfs.storeValue<Scalar>("./rotationFactor",rotationFactor);
	cfs.storeValue<Scalar>("./scalingFactor",scalingFactor);
	cfs.storeValue<bool>("./mutualExclusion",mutualExclusion);
	}

/*************************************************
Methods of class MultiDeviceNavigationToolFactory:
*************************************************/

MultiDeviceNavigationToolFactory::MultiDeviceNavigationToolFactory(ToolManager& toolManager)
	:ToolFactory("MultiDeviceNavigationTool",toolManager)
	{
	/* Initialize tool layout: */
	layout.setNumButtons(1,true);
	
	/* Insert class into class hierarchy: */
	ToolFactory* navigationToolFactory=toolManager.loadClass("NavigationTool");
	navigationToolFactory->addChildClass(this);
	addParentClass(navigationToolFactory);
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=toolManager.getToolClassSection(getClassName());
	configuration.read(cfs);
	
	/* Set tool class' factory pointer: */
	MultiDeviceNavigationTool::factory=this;
	}

MultiDeviceNavigationToolFactory::~MultiDeviceNavigationToolFactory(void)
	{
	/* Reset tool class' factory pointer: */
	MultiDeviceNavigationTool::factory=0;
	}

const char* MultiDeviceNavigationToolFactory::getName(void) const
	{
	return "Multiple 3-DOF Devices";
	}

const char* MultiDeviceNavigationToolFactory::getButtonFunction(int) const
	{
	return "Move / Rotate / Zoom";
	}

Tool* MultiDeviceNavigationToolFactory::createTool(const ToolInputAssignment& inputAssignment) const
	{
	return new MultiDeviceNavigationTool(this,inputAssignment);
	}

void MultiDeviceNavigationToolFactory::destroyTool(Tool* tool) const
	{
	delete tool;
	}

extern "C" void resolveMultiDeviceNavigationToolDependencies(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Load base classes: */
	manager.loadClass("NavigationTool");
	}

extern "C" ToolFactory* createMultiDeviceNavigationToolFactory(Plugins::FactoryManager<ToolFactory>& manager)
	{
	/* Get pointer to tool manager: */
	ToolManager* toolManager=static_cast<ToolManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	MultiDeviceNavigationToolFactory* multiDeviceNavigationToolFactory=new MultiDeviceNavigationToolFactory(*toolManager);
	
	/* Return factory object: */
	return multiDeviceNavigationToolFactory;
	}

extern "C" void destroyMultiDeviceNavigationToolFactory(ToolFactory* factory)
	{
	delete factory;
	}

/**************************************************
Static elements of class MultiDeviceNavigationTool:
**************************************************/

MultiDeviceNavigationToolFactory* MultiDeviceNavigationTool::factory=0;

/******************************************
Methods of class MultiDeviceNavigationTool:
******************************************/

MultiDeviceNavigationTool::MultiDeviceNavigationTool(const ToolFactory* sFactory,const ToolInputAssignment& inputAssignment)
	:NavigationTool(factory,inputAssignment),
	 configuration(MultiDeviceNavigationTool::factory->configuration),
	 numPressedButtons(0),selectNavMode(false),
	 firstDevicePositions(new Point[input.getNumButtonSlots()]),
	 lastDeviceButtonStates(new bool[input.getNumButtonSlots()]),
	 lastDevicePositions(new Point[input.getNumButtonSlots()]),
	 devicePositions(new Point[input.getNumButtonSlots()])
	{
	/* Initialize old button states: */
	for(int i=0;i<input.getNumButtonSlots();++i)
		lastDeviceButtonStates[i]=false;
	}

MultiDeviceNavigationTool::~MultiDeviceNavigationTool(void)
	{
	delete[] firstDevicePositions;
	delete[] lastDeviceButtonStates;
	delete[] lastDevicePositions;
	delete[] devicePositions;
	}

void MultiDeviceNavigationTool::configure(const Misc::ConfigurationFileSection& configFileSection)
	{
	/* Override private configuration data from given configuration file section: */
	configuration.read(configFileSection);
	}

void MultiDeviceNavigationTool::storeState(Misc::ConfigurationFileSection& configFileSection) const
	{
	/* Write private configuration data to given configuration file section: */
	configuration.write(configFileSection);
	}

const ToolFactory* MultiDeviceNavigationTool::getFactory(void) const
	{
	return factory;
	}

void MultiDeviceNavigationTool::buttonCallback(int buttonSlotIndex,InputDevice::ButtonCallbackData* cbData)
	{
	/* Update the number of pressed buttons: */
	if(cbData->newButtonState)
		{
		/* Activate navigation when the first button is pressed: */
		if(numPressedButtons==0)
			{
			activate();
			
			/* Retrieve the current navigation transformation: */
			nav=getNavigationTransformation();
			}
		else if(numPressedButtons==1)
			{
			/* Store the current positions of all active devices: */
			for(int i=0;i<input.getNumButtonSlots();++i)
				if(getButtonState(i))
					firstDevicePositions[i]=getButtonDevicePosition(i);
			
			/* Start the navigation mode selection process: */
			initialNav=getNavigationTransformation();
			selectNavMode=configuration.mutualExclusion;
			allowRotation=true;
			allowScaling=true;
			}
		else
			{
			/* Store the current position of the newly-activated device: */
			firstDevicePositions[buttonSlotIndex]=getButtonDevicePosition(buttonSlotIndex);
			}
		
		++numPressedButtons;
		}
	else
		{
		if(numPressedButtons>0)
			--numPressedButtons;
		
		if(numPressedButtons<=1)
			selectNavMode=false;
		
		/* Deactivate navigation and reset all button states when the last button is released: */
		if(numPressedButtons==0)
			{
			deactivate();
			for(int i=0;i<input.getNumButtonSlots();++i)
				lastDeviceButtonStates[i]=false;
			}
		}
	}

void MultiDeviceNavigationTool::frame(void)
	{
	/* Do nothing if the tool is inactive: */
	if(isActive())
		{
		/* Calculate the previous and current centroids of all active devices, i.e., those whose buttons were pressed in both the previous and current frames: */
		int numActiveDevices=0;
		Point::AffineCombiner lastCentroidC;
		Point::AffineCombiner centroidC;
		for(int i=0;i<input.getNumButtonSlots();++i)
			{
			/* Store the device's current position: */
			devicePositions[i]=getButtonDevicePosition(i);
			
			/* Check if the device is active: */
			if(lastDeviceButtonStates[i]&&getButtonState(i))
				{
				++numActiveDevices;
				lastCentroidC.addPoint(lastDevicePositions[i]);
				centroidC.addPoint(devicePositions[i]);
				}
			}
		
		/* Check if there are any active devices: */
		if(numActiveDevices>0)
			{
			/* Get the current and previous centroids of all active devices: */
			Point lastCentroid=lastCentroidC.getPoint();
			Point centroid=centroidC.getPoint();
			
			/* Check if the tool is currently selecting a navigation mode: */
			if(selectNavMode)
				{
				/* Calculate the initial and current centroids of all currently active devices: */
				Point::AffineCombiner firstCentroidC;
				Point::AffineCombiner activeCentroidC;
				for(int i=0;i<input.getNumButtonSlots();++i)
					if(getButtonState(i))
						{
						firstCentroidC.addPoint(firstDevicePositions[i]);
						activeCentroidC.addPoint(devicePositions[i]);
						}
				Point firstCentroid=firstCentroidC.getPoint();
				Point activeCentroid=activeCentroidC.getPoint();
				
				/* Check if any of the active devices moved far enough from their initial positions to determine which mode is wanted: */
				Scalar dist2=Math::sqr(Math::div2(configuration.minRotationScalingDistance));
				Scalar dSum(0);
				Scalar dpSum(0);
				Scalar doSum(0);
				for(int i=0;i<input.getNumButtonSlots();++i)
					if(getButtonState(i)&&Geometry::sqrDist(devicePositions[i],firstDevicePositions[i])>dist2)
						{
						/* Calculate the device's displacement vector parallel and orthogonal to the centroid direction: */
						Vector dc=devicePositions[i]-activeCentroid;
						Scalar dcLen=dc.mag();
						if(dcLen>configuration.minRotationScalingDistance)
							{
							Vector delta=dc-(firstDevicePositions[i]-firstCentroid);
							Scalar d2=delta.sqr();
							Scalar deltaP=Math::abs((delta*dc))/dcLen;
							Scalar deltaO2=d2-Math::sqr(deltaP);
							Scalar deltaO=deltaO2>Scalar(0)?Math::sqrt(deltaO2):Scalar(0);
							dSum+=Math::sqrt(d2);
							dpSum+=deltaP;
							doSum+=deltaO;
							
							/* We can now select a navigation mode: */
							selectNavMode=false;
							}
						}
				
				/* Select the navigation mode based on the relative amounts of parallel and orthogonal movements: */
				if(!selectNavMode)
					{
					if(Math::sqr(Math::mul2(dSum))<dist2)
						{
						/* Translation only: */
						allowRotation=false;
						allowScaling=false;
						}
					else if(doSum>=dpSum)
						{
						/* Translation and rotation only: */
						allowScaling=false;
						}
					else
						{
						/* Translation and scaling only: */
						allowRotation=false;
						}
					
					/* Reset the disabled parts of the navigation transformation to their initial values: */
					Point navCentroid=nav.inverseTransform(centroid);
					nav*=NavTransform::translateFromOriginTo(navCentroid);
					if(!allowRotation)
						{
						nav*=NavTransform::rotate(Geometry::invert(nav.getRotation()));
						nav*=NavTransform::rotate(initialNav.getRotation());
						}
					if(!allowScaling)
						{
						nav*=NavTransform::scale(Scalar(1)/nav.getScaling());
						nav*=NavTransform::scale(initialNav.getScaling());
						}
					nav*=NavTransform::translateToOriginFrom(navCentroid);
					nav.renormalize();
					}
				}
			
			/* Calculate the average rotation vector and scaling factor of all active devices: */
			Vector rotation=Vector::zero;
			Scalar scaling(1);
			Scalar rotScaleWeight(0);
			for(int i=0;i<input.getNumButtonSlots();++i)
				if(lastDeviceButtonStates[i]&&getButtonState(i))
					{
					/* Calculate the previous vector to centroid: */
					Vector lastDist=lastDevicePositions[i]-lastCentroid;
					Scalar lastLen=Geometry::mag(lastDist);
					
					/* Calculate the new vector to centroid: */
					Vector dist=getButtonDevicePosition(i)-centroid;
					Scalar len=Geometry::mag(dist);
					
					if(lastLen>configuration.minRotationScalingDistance&&len>configuration.minRotationScalingDistance)
						{
						/* Calculate the rotation axis and angle: */
						Vector rot=lastDist^dist;
						Scalar rotLen=Geometry::mag(rot);
						if(rotLen>Scalar(0))
							{
							Scalar angle=Math::asin(rotLen/(lastLen*len));
							rot*=angle/rotLen;
							
							/* Accumulate the rotation vector: */
							rotation+=rot;
							}
						
						/* Calculate the scaling factor: */
						Scalar scal=len/lastLen;
						
						/* Accumulate the scaling factor: */
						scaling*=scal;
						
						rotScaleWeight+=Scalar(1);
						}
					}
			
			/* Update the navigation transformation: */
			NavTransform t=NavTransform::translate((centroid-lastCentroid)*configuration.translationFactor);
			if(rotScaleWeight>Scalar(0)&&(allowRotation||allowScaling))
				{
				/* Average and scale translation, rotation, and scaling: */
				rotation*=configuration.rotationFactor/rotScaleWeight;
				scaling=Math::pow(scaling*configuration.scalingFactor,Scalar(1)/rotScaleWeight);
				
				/* Apply rotation and scaling: */
				t*=NavTransform::translateFromOriginTo(centroid);
				if(allowRotation)
					t*=NavTransform::rotate(Rotation::rotateScaledAxis(rotation));
				if(allowScaling)
					t*=NavTransform::scale(scaling);
				t*=NavTransform::translateToOriginFrom(centroid);
				}
			nav.leftMultiply(t);
			nav.renormalize();
			setNavigationTransformation(nav);
			}
		
		/* Update button states and device positions for next frame: */
		for(int i=0;i<input.getNumButtonSlots();++i)
			{
			lastDeviceButtonStates[i]=getButtonState(i);
			lastDevicePositions[i]=devicePositions[i];
			}
		}
	}

}
