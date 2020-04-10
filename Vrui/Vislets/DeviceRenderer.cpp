/***********************************************************************
DeviceRenderer - Vislet class to render input devices using fancy
representations.
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

#include <Vrui/Vislets/DeviceRenderer.h>

#include <string.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <IO/OpenFile.h>
#include <GL/gl.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLContextData.h>
#include <GL/GLTransformationWrappers.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <SceneGraph/GLRenderState.h>
#include <Vrui/Vrui.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VisletManager.h>
#include <Vrui/DisplayState.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

namespace Vislets {

/**************************************
Methods of class DeviceRendererFactory:
**************************************/

DeviceRendererFactory::DeviceRendererFactory(VisletManager& visletManager)
	:VisletFactory("DeviceRenderer",visletManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	deviceGlyphs=cfs.retrieveValue<std::vector<std::pair<std::string,std::string> > >("./deviceGlyphs");
	
	/* Set tool class' factory pointer: */
	DeviceRenderer::factory=this;
	}

DeviceRendererFactory::~DeviceRendererFactory(void)
	{
	/* Reset tool class' factory pointer: */
	DeviceRenderer::factory=0;
	}

Vislet* DeviceRendererFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new DeviceRenderer(numArguments,arguments);
	}

void DeviceRendererFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveDeviceRendererDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createDeviceRendererFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	DeviceRendererFactory* deviceRendererFactory=new DeviceRendererFactory(*visletManager);
	
	/* Return factory object: */
	return deviceRendererFactory;
	}

extern "C" void destroyDeviceRendererFactory(VisletFactory* factory)
	{
	delete factory;
	}

/***************************************
Static elements of class DeviceRenderer:
***************************************/

DeviceRendererFactory* DeviceRenderer::factory=0;

/*******************************
Methods of class DeviceRenderer:
*******************************/

DeviceRenderer::DeviceRenderer(int numArguments,const char* const arguments[])
	{
	/* Create a hash table to associate scene graph file names with scene graphs: */
	Misc::HashTable<std::string,SceneGraph::GroupNodePointer> sceneGraphMap(17);
	
	/* Process the list of device/scene graph associations: */
	for(std::vector<std::pair<std::string,std::string> >::const_iterator dgIt=factory->deviceGlyphs.begin();dgIt!=factory->deviceGlyphs.end();++dgIt)
		{
		/* Get a pointer to the referenced input device: */
		DeviceGlyph dg;
		dg.device=findInputDevice(dgIt->first.c_str());
		if(dg.device!=0)
			{
			/* Check if the referenced scene graph has already been loaded: */
			Misc::HashTable<std::string,SceneGraph::GroupNodePointer>::Iterator sgIt=sceneGraphMap.findEntry(dgIt->second);
			if(sgIt.isFinished())
				{
				try
					{
					/* Open the installation's share directory: */
					IO::DirectoryPtr shareDir=IO::openDirectory(VRUI_INTERNAL_CONFIG_SHAREDIR);
					
					/* Load the scene graph file: */
					SceneGraph::NodeCreator nodeCreator;
					SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
					SceneGraph::VRMLFile vrmlFile(*shareDir,dgIt->second,nodeCreator);
					vrmlFile.parse(root);
					
					/* Associate the new scene graph with the input device: */
					sceneGraphMap[dgIt->second]=root;
					dg.glyph=root;
					deviceGlyphs.push_back(dg);
					}
				catch(const std::runtime_error& err)
					{
					}
				}
			else
				{
				/* Associate the existing scene graph with the input device: */
				dg.glyph=sgIt->getDest();
				deviceGlyphs.push_back(dg);
				}
			}
		else
			{
			}
		}
	}

DeviceRenderer::~DeviceRenderer(void)
	{
	}

VisletFactory* DeviceRenderer::getFactory(void) const
	{
	return factory;
	}

void DeviceRenderer::display(GLContextData& contextData) const
	{
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);
	
	/* Temporarily disable all clipping planes: */
	contextData.getClipPlaneTracker()->pause();
	
	/* Create a render state object: */
	glPushMatrix();
	const NavTransform& mvp=getDisplayState(contextData).modelviewPhysical;
	SceneGraph::GLRenderState renderState(contextData,mvp,mvp.transform(getMainViewer()->getHeadPosition()),mvp.transform(getUpDirection()));
	
	/* Render all enabled input devices that have an associated scene graph: */
	for(std::vector<DeviceGlyph>::const_iterator dgIt=deviceGlyphs.begin();dgIt!=deviceGlyphs.end();++dgIt)
		if(getInputGraphManager()->isEnabled(dgIt->device))
			{
			/* Transform to the input device's current transformation: */
			SceneGraph::GLRenderState::DOGTransform deviceTrans(dgIt->device->getTransformation());
			SceneGraph::GLRenderState::DOGTransform previous=renderState.pushTransform(deviceTrans);
			
			/* Render the scene graph: */
			dgIt->glyph->glRenderAction(renderState);
			
			/* Go back to physical coordinates: */
			renderState.popTransform(previous);
			}
	
	/* Restore OpenGL state: */
	glPopMatrix();
	glPopAttrib();
	
	/* Re-enable clipping: */
	contextData.getClipPlaneTracker()->resume();
	}

}

}
