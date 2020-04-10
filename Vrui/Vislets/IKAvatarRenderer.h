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

#ifndef VRUI_VISLETS_IKAVATARRENDERER_INCLUDED
#define VRUI_VISLETS_IKAVATARRENDERER_INCLUDED

#include <Vrui/Vislet.h>
#include <Vrui/IKAvatar.h>
#include <Vrui/IKAvatarDriver.h>

/* Forward declarations: */
namespace Vrui {
class VisletManager;
}

namespace Vrui {

namespace Vislets {

class IKAvatarRenderer;

class IKAvatarRendererFactory:public Vrui::VisletFactory
	{
	friend class IKAvatarRenderer;
	
	/* Constructors and destructors: */
	public:
	IKAvatarRendererFactory(Vrui::VisletManager& visletManager);
	virtual ~IKAvatarRendererFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class IKAvatarRenderer:public Vrui::Vislet
	{
	friend class IKAvatarRendererFactory;
	
	/* Elements: */
	static IKAvatarRendererFactory* factory; // Pointer to the factory object for this class
	
	IKAvatar avatar; // The avatar
	IKAvatarDriver driver; // An inverse kinematics driver for the avatar
	
	/* Constructors and destructors: */
	public:
	IKAvatarRenderer(int numArguments,const char* const arguments[]);
	virtual ~IKAvatarRenderer(void);
	
	/* Methods: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
