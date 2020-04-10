/***********************************************************************
ImageViewer - Vislet class to display a zoomable and scrollable image in
a GLMotif dialog window.
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

#ifndef VRUI_VISLETS_IMAGEVIEWER_INCLUDED
#define VRUI_VISLETS_IMAGEVIEWER_INCLUDED

#include <GLMotif/TextFieldSlider.h>
#include <Vrui/Geometry.h>
#include <Vrui/Vislet.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace GLMotif {
class PopupWindow;
class ScrolledImage;
class Button;
}

namespace Vrui {

namespace Vislets {

class ImageViewer;

class ImageViewerFactory:public Vrui::VisletFactory
	{
	friend class ImageViewer;
	
	/* Elements: */
	private:
	Scalar minWindowSize; // Minimum image window size in Vrui physical units; defaults to 1/4 of display size
	
	/* Constructors and destructors: */
	public:
	ImageViewerFactory(Vrui::VisletManager& visletManager);
	virtual ~ImageViewerFactory(void);
	
	/* Methods: */
	virtual Vislet* createVislet(int numVisletArguments,const char* const visletArguments[]) const;
	virtual void destroyVislet(Vislet* vislet) const;
	};

class ImageViewer:public Vrui::Vislet
	{
	friend class ImageViewerFactory;
	
	/* Elements: */
	private:
	static ImageViewerFactory* factory; // Pointer to the factory object for this class
	GLMotif::PopupWindow* imageDialog;
	GLMotif::ScrolledImage* imageViewer;
	GLMotif::Button* zoomInButton;
	GLMotif::TextFieldSlider* zoomFactor;
	GLMotif::Button* zoomOutButton;
	
	/* Private methods: */
	void zoomInCallback(Misc::CallbackData* cbData);
	void zoomFactorCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	void zoomOutCallback(Misc::CallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	ImageViewer(int numArguments,const char* const arguments[]);
	virtual ~ImageViewer(void);
	
	/* Methods from Vislet: */
	public:
	virtual VisletFactory* getFactory(void) const;
	virtual void enable(bool startup);
	virtual void disable(bool shutdown);
	};

}

}

#endif
