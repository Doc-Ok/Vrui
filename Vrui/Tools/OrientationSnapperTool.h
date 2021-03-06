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

#ifndef VRUI_ORIENTATIONSNAPPERTOOL_INCLUDED
#define VRUI_ORIENTATIONSNAPPERTOOL_INCLUDED

#include <Vrui/TransformTool.h>

namespace Vrui {

class OrientationSnapperTool;

class OrientationSnapperToolFactory:public ToolFactory
	{
	friend class OrientationSnapperTool;
	
	/* Constructors and destructors: */
	public:
	OrientationSnapperToolFactory(ToolManager& toolManager);
	virtual ~OrientationSnapperToolFactory(void);
	
	/* Methods from ToolFactory: */
	virtual const char* getName(void) const;
	virtual Tool* createTool(const ToolInputAssignment& inputAssignment) const;
	virtual void destroyTool(Tool* tool) const;
	};

class OrientationSnapperTool:public TransformTool
	{
	friend class OrientationSnapperToolFactory;
	
	/* Elements: */
	private:
	static OrientationSnapperToolFactory* factory; // Pointer to the factory object for this class
	
	/* Constructors and destructors: */
	public:
	OrientationSnapperTool(const ToolFactory* factory,const ToolInputAssignment& inputAssignment);
	virtual ~OrientationSnapperTool(void);
	
	/* Methods from Tool: */
	virtual void initialize(void);
	virtual const ToolFactory* getFactory(void) const;
	virtual void frame(void);
	};

}

#endif
