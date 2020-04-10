/***********************************************************************
KeyboardTextEntryMethod - Class to enter text using a real keyboard.
Copyright (c) 2019 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_KEYBOARDTEXTENTRYMETHOD_INCLUDED
#define VRUI_INTERNAL_KEYBOARDTEXTENTRYMETHOD_INCLUDED

#include <GLMotif/TextEntryMethod.h>

/* Forward declarations: */
namespace Vrui {
class InputDeviceAdapterMouse;
}

namespace Vrui {

class KeyboardTextEntryMethod:public GLMotif::TextEntryMethod
	{
	/* Elements: */
	private:
	InputDeviceAdapterMouse* mouseAdapter; // Pointer to the input device adapter representing the real keyboard
	
	/* Constructors and destructors: */
	public:
	KeyboardTextEntryMethod(InputDeviceAdapterMouse* sMouseAdapter); // Creates a text entry method for the keyboard represented by the given input device adapter
	
	/* Methods from class GLMotif::TextEntryMethod: */
	virtual void requestNumericEntry(const GLMotif::WidgetManager::Transformation& transform,GLMotif::Widget* widget);
	virtual void requestAlphaNumericEntry(const GLMotif::WidgetManager::Transformation& transform,GLMotif::Widget* widget);
	virtual void entryFinished(void);
	};

}

#endif
