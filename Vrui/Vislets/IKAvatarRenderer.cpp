/***********************************************************************
IKAvatarRenderer - Vislet class to render an IK-controlled avatar for
the local user.
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

#include <Vrui/Vislets/IKAvatarRenderer.h>

#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/VisletManager.h>
#include <Vrui/SceneGraphSupport.h>

namespace Vrui {

namespace Vislets {

/****************************************
Methods of class IKAvatarRendererFactory:
****************************************/

IKAvatarRendererFactory::IKAvatarRendererFactory(VisletManager& visletManager)
	:VisletFactory("IKAvatarRenderer",visletManager)
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Set tool class' factory pointer: */
	IKAvatarRenderer::factory=this;
	}

IKAvatarRendererFactory::~IKAvatarRendererFactory(void)
	{
	/* Reset tool class' factory pointer: */
	IKAvatarRenderer::factory=0;
	}

Vislet* IKAvatarRendererFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new IKAvatarRenderer(numArguments,arguments);
	}

void IKAvatarRendererFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveIKAvatarRendererDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createIKAvatarRendererFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	IKAvatarRendererFactory* ikAvatarRendererFactory=new IKAvatarRendererFactory(*visletManager);
	
	/* Return factory object: */
	return ikAvatarRendererFactory;
	}

extern "C" void destroyIKAvatarRendererFactory(VisletFactory* factory)
	{
	delete factory;
	}

/*****************************************
Static elements of class IKAvatarRenderer:
*****************************************/

IKAvatarRendererFactory* IKAvatarRenderer::factory=0;

/*********************************
Methods of class IKAvatarRenderer:
*********************************/

IKAvatarRenderer::IKAvatarRenderer(int numArguments,const char* const arguments[])
	{
	/* Parse the command line: */
	const char* driverConfigName=0;
	if(numArguments>=1)
		driverConfigName=arguments[0];
	
	/* Configure the avatar driver and scale it from meters to physical coordinate units: */
	driver.configure(driverConfigName);
	driver.scaleAvatar(getMeterFactor());
	
	/* Load the avatar, scale it from meters, and apply the avatar driver's configuration: */
	avatar.loadAvatar(driver.getAvatarModelFileName().c_str());
	avatar.scaleAvatar(getMeterFactor());
	avatar.configureAvatar(driver.getConfiguration());
	}

IKAvatarRenderer::~IKAvatarRenderer(void)
	{
	}

VisletFactory* IKAvatarRenderer::getFactory(void) const
	{
	return factory;
	}

void IKAvatarRenderer::frame(void)
	{
	if(isActive()&&driver.needsUpdate())
		{
		/* Update the avatar: */
		IKAvatar::State newState;
		if(driver.calculateState(newState))
			Vrui::scheduleUpdate(Vrui::getNextAnimationTime());
		avatar.updateState(newState);
		
		/* Attach the avatar to the viewer: */
		avatar.setRootTransform(getMainViewer()->getHeadTransformation());
		}
	}

void IKAvatarRenderer::display(GLContextData& contextData) const
	{
	if(isActive()&&avatar.isValid())
		{
		/* Save OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);
		
		/* Render the avatar's scene graph in navigational or physical space: */
		renderSceneGraph(avatar.getSceneGraph(),false,contextData);
		
		glPopAttrib();
		}
	}

}

}
