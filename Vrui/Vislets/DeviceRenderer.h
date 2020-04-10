/***********************************************************************
DeviceRenderer - Vislet class to render input devices using fancy
representations.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef VRUI_VISLETS_DEVICERENDERER_INCLUDED
#define VRUI_VISLETS_DEVICERENDERER_INCLUDED

#include <utility>
#include <string>
#include <vector>
#include <SceneGraph/GroupNode.h>
#include <Vrui/Vislet.h>

/* Forward declarations: */
namespace Vrui {
class InputDevice;
class VisletManager;
}

namespace Vrui {

namespace Vislets {

class DeviceRenderer;

class DeviceRendererFactory:public Vrui::VisletFactory
	{
	friend class DeviceRenderer;
	
	/* Elements: */
	private:
	std::vector<std::pair<std::string,std::string> > deviceGlyphs; // List of pairs of device names and associated scene graph file names
	
	/* Constructors and destructors: */
	public:
	DeviceRendererFactory(Vrui::VisletManager& visletManager);
	virtual ~DeviceRendererFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class DeviceRenderer:public Vrui::Vislet
	{
	friend class DeviceRendererFactory;
	
	/* Embedded classes: */
	private:
	struct DeviceGlyph // Structure to associate an input device with a scene graph
		{
		/* Elements: */
		InputDevice* device; // Pointer to the input device
		SceneGraph::GroupNodePointer glyph; // Pointer to the root node of the device's scene graph
		};
	
	/* Elements: */
	static DeviceRendererFactory* factory; // Pointer to the factory object for this class
	
	std::vector<DeviceGlyph> deviceGlyphs; // List of device/scene graph associations
	
	/* Constructors and destructors: */
	public:
	DeviceRenderer(int numArguments,const char* const arguments[]);
	virtual ~DeviceRenderer(void);
	
	/* Methods: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void display(GLContextData& contextData) const;
	};

}

}

#endif
