/***********************************************************************
Small image viewer using Vrui.
Copyright (c) 2011-2019 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef IMAGEVIEWER_INCLUDED
#define IMAGEVIEWER_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <GL/gl.h>
#include <GL/GLColor.h>
#include <Images/TextureSet.h>
#include <Vrui/Application.h>
#include <Vrui/Tool.h>
#include <Vrui/GenericToolFactory.h>

class ImageViewer:public Vrui::Application
	{
	/* Embedded classes: */
	public:
	typedef double Scalar;
	typedef Geometry::Point<Scalar,2> Point; // Type for points in image space
	typedef Geometry::Vector<Scalar,2> Vector; // Type for vectors in image space
	typedef GLColor<GLfloat,4> Color; // Type for RGBA image colors
	
	private:
	class PipetteTool; // Forward declaration
	typedef Vrui::GenericToolFactory<PipetteTool> PipetteToolFactory; // Pipette tool class uses the generic factory class
	
	class PipetteTool:public Vrui::Tool,public Vrui::Application::Tool<ImageViewer> // A tool class to pick color values from an image, derived from application tool class
		{
		friend class Vrui::GenericToolFactory<PipetteTool>;
		
		/* Elements: */
		private:
		static PipetteToolFactory* factory; // Pointer to the factory object for this class
		bool dragging; // Flag whether there is a current dragging operation
		int x0,y0; // Initial pixel position for dragging operations
		int x,y; // Current pixel position during dragging operations
		
		/* Private methods: */
		void setPixelPos(void); // Sets the current pixel position based on current input device selection ray
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the pipette tool factory class
		PipetteTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	friend class HomographySamplerTool;
	
	class HomographySamplerTool; // Forward declaration
	typedef Vrui::GenericToolFactory<HomographySamplerTool> HomographySamplerToolFactory; // HomographySampler tool class uses the generic factory class
	
	class HomographySamplerTool:public Vrui::Tool,public Vrui::Application::Tool<ImageViewer> // A tool class to sample a perspective-distorted image quadrilateral into a rectangular sub-image
		{
		friend class Vrui::GenericToolFactory<HomographySamplerTool>;
		
		/* Elements: */
		private:
		static HomographySamplerToolFactory* factory; // Pointer to the factory object for this class
		int numVertices; // Number of quad vertices that have already been set
		Point quad[4]; // Four corners of the distorted quadrilateral in order lower-left, lower-right, upper-right, upper-left
		Point edge[4]; // Four points along the edges of the distorted quadrilateral to approximate lens distortion
		unsigned int size[2]; // Size of the rectangular result image in pixels
		bool dragging; // Flag whether there is a current dragging operation
		int dragIndex; // Index of quad corner currently being dragged
		Vector dragOffset; // Offset from device position to dragged vertex position during dragging
		
		/* Private methods: */
		Point calcPixelPos(void) const; // Returns the current pixel position based on current input device selection ray
		void resample(void); // Resamples the image based on the current quad
		void draw(void) const; // Draws the current quad
		
		/* Constructors and destructors: */
		public:
		static void initClass(void); // Initializes the pipette tool factory class
		HomographySamplerTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment);
		
		/* Methods: */
		virtual const Vrui::ToolFactory* getFactory(void) const;
		virtual void buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData);
		virtual void frame(void);
		virtual void display(GLContextData& contextData) const;
		};
	
	friend class HomographySamplerTool;
	
	/* Elements: */
	Images::TextureSet textures; // Texture set containing the image to be displayed
	const Images::BaseImage* image; // Pointer to the image
	
	/* Private methods: */
	Color getPixel(unsigned int x,unsigned int y) const; // Returns an RGBA color for the given pixel position
	
	/* Constructors and destructors: */
	public:
	ImageViewer(int& argc,char**& argv);
	virtual ~ImageViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

#endif
