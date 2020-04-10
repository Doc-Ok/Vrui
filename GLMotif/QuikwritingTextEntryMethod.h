/***********************************************************************
QuikwritingTextEntryMethod - Class to enter text using the Quikwriting
method.
Copyright (c) 2019 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef GLMOTIF_QUIKWRITINGTEXTENTRYMETHOD_INCLUDED
#define GLMOTIF_QUIKWRITINGTEXTENTRYMETHOD_INCLUDED

#include <GLMotif/TextEntryMethod.h>

/* Forward declarations: */
namespace GLMotif {
class Widget;
class WidgetManager;
class Quikwriting;
}

namespace GLMotif {

class QuikwritingTextEntryMethod:public TextEntryMethod
	{
	/* Elements: */
	private:
	WidgetManager* widgetManager; // The widget manager
	Widget* quikwritingPanel; // The top-level Quikwriting widget
	Quikwriting* quikwriting; // The Quikwriting widget
	
	/* Private methods: */
	void popupQuikwritingPanel(const WidgetManager::Transformation& transform,Widget* widget);
	
	/* Constructors and destructors: */
	public:
	QuikwritingTextEntryMethod(WidgetManager* sWidgetManager); // Creates a Quikwriting text entry method
	virtual ~QuikwritingTextEntryMethod(void);
	
	/* Methods from class TextEntryMethod: */
	virtual void requestNumericEntry(const WidgetManager::Transformation& transform,Widget* widget);
	virtual void requestAlphaNumericEntry(const WidgetManager::Transformation& transform,Widget* widget);
	virtual void entryFinished(void);
	};

}

#endif
