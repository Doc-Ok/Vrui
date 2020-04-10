/***********************************************************************
Quikwriting - Widget for text entry using the Quikwriting method.
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

#ifndef GLMOTIF_QUIKWRITING_INCLUDED
#define GLMOTIF_QUIKWRITING_INCLUDED

#include <GL/gl.h>
#include <GL/GLObject.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

class Quikwriting:public Widget,public GLObject
	{
	/* Embedded classes: */
	private:
	enum Specials // Enumerated type for special symbols
		{
		Backspace=1,Enter,Lower,Upper,Numerical,Symbols,SpecialsEnd
		};
	
	enum State // Enumerated type for Quikwriting state
		{
		Inactive,Start,Outer1
		};
	
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint textures[4]; // IDs of texture objects containing the Quikwriting wheels
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	private:
	static const int characterTable[4][8][8]; // Character table mapping shift planes and first and second areas to symbols
	Vector center; // Center point of Quikwriting panel
	GLfloat centralRadius,centralRadius2; // Radius of central area
	GLfloat a,c; // Parabola coefficients of "north" outer area
	double buttonDownTime; // System time at which the button was last pressed
	State state; // Current Quikwriting state
	int area; // Index of area in which the last event happened (-1-8)
	int outer1; // Index of outer area first entered (1-8)
	int outer2; // Index of outer area last entered (1-8)
	int shiftLevel; // Current shift level (0: lowercase, 1: uppercase, 2: numeric, 3: punctuation
	bool shiftLevelLocked; // Flag if the current shift level is locked for multiple characters
	Widget* targetWidget; // Widget supposed to receive text entry events from the Quikwriting widget
	Widget* motionTarget; // Pointer to the widget holding a "soft" grab on the current pointer motion sequence
	
	/* Private methods: */
	void calcLayout(void); // Calculates the Quikwriting area's layout after a geometry change
	int findArea(const Event& event) const; // Returns the index of the area containing the given event, -1: no area; 0: central area; 1-8: outer areas, starting "north" and going clockwise
	
	/* Constructors and destructors: */
	public:
	Quikwriting(const char* sName,Container* sParent,bool sManageChild =true);
	virtual ~Quikwriting(void);
	
	/* Methods from class GLMotif::Widget: */
	virtual Vector calcNaturalSize(void) const;
	virtual void resize(const Box& newExterior);
	virtual void setBorderWidth(GLfloat newBorderWidth);
	virtual void setBorderType(BorderType newBorderType);
	virtual void draw(GLContextData& contextData) const;
	virtual bool findRecipient(Event& event);
	virtual void pointerButtonDown(Event& event);
	virtual void pointerButtonUp(Event& event);
	virtual void pointerMotion(Event& event);
	
	/* Methods from class GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	
	/* New methods: */
	void setShiftLevel(int newShiftLevel,bool newShiftLevelLocked); // Sets the current shift level and lock flag
	void setTargetWidget(Widget* newTargetWidget); // Sets a new target widget
	};

}

#endif
