/***********************************************************************
RadioBox - Subclass of RowColumn that contains only mutually exclusive
ToggleButton objects.
Copyright (c) 2001-2019 Oliver Kreylos

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

#ifndef GLMOTIF_RADIOBOX_INCLUDED
#define GLMOTIF_RADIOBOX_INCLUDED

#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <GLMotif/VariableTracker.h>
#include <GLMotif/RowColumn.h>

/* Forward declarations: */
namespace GLMotif {
class ToggleButton;
}

namespace GLMotif {

class RadioBox:public RowColumn,public VariableTracker
	{
	/* Embedded classes: */
	public:
	enum SelectionMode // Different modes of radio boxes
		{
		ATMOST_ONE,ALWAYS_ONE
		};
	
	class CallbackData:public Misc::CallbackData // Base class for callback data sent by radio boxes
		{
		/* Elements: */
		public:
		RadioBox* radioBox; // Pointer to the radio box that caused the event
		
		/* Constructors and destructors: */
		CallbackData(RadioBox* sRadioBox)
			:radioBox(sRadioBox)
			{
			}
		};
	
	class ValueChangedCallbackData:public CallbackData
		{
		/* Elements: */
		public:
		ToggleButton* oldSelectedToggle; // Pointer to the old selected toggle
		ToggleButton* newSelectedToggle; // Pointer to the new selected toggle
		
		/* Constructors and destructors: */
		ValueChangedCallbackData(RadioBox* sRadioBox,ToggleButton* sOldSelectedToggle,ToggleButton* sNewSelectedToggle)
			:CallbackData(sRadioBox),
			 oldSelectedToggle(sOldSelectedToggle),
			 newSelectedToggle(sNewSelectedToggle)
			{
			}
		};
	
	/* Elements: */
	protected:
	SelectionMode selectionMode; // Radio box' selection mode
	ToggleButton* selectedToggle; // Currently selected toggle button
	Misc::CallbackList valueChangedCallbacks; // List of callbacks to be called when a different button is selected
	
	/* Protected methods: */
	static void childrenValueChangedCallbackWrapper(Misc::CallbackData* callbackData,void* userData); // Callback that is called when a child changes value by user interaction
	bool findAndSelectToggle(int toggleIndex); // Selects the toggle button of the given index; deselects all toggles if index<0 and selection mode is ATMOST_ONE; does nothing if index is too large; returns true if selected toggle changed
	
	/* Constructors and destructors: */
	public:
	RadioBox(const char* sName,Container* sParent,bool manageChild =true);
	
	/* Methods from class Widget: */
	virtual void updateVariables(void);
	
	/* Methods from class Container: */
	virtual void addChild(Widget* newChild);
	
	/* Methods from class VariableTracker: */
	template <class VariableTypeParam>
	void track(VariableTypeParam& newVariable) // Tracks the given variable and sets its initial value
		{
		findAndSelectToggle(newVariable);
		VariableTracker::track(newVariable);
		}
	
	/* New methods: */
	void addToggle(const char* newToggleLabel); // Adds a new toggle button with the given label
	SelectionMode getSelectionMode(void) const // Returns the current selection mode
		{
		return selectionMode;
		}
	void setSelectionMode(SelectionMode newSelectionMode); // Sets a new selection mode
	const ToggleButton* getSelectedToggle(void) const // Returns the currently selected button
		{
		return selectedToggle;
		}
	ToggleButton* getSelectedToggle(void) // Ditto
		{
		return selectedToggle;
		}
	int getToggleIndex(const ToggleButton* toggle) const; // Returns the index of the given toggle
	void setSelectedToggle(ToggleButton* newSelectedToggle); // Changes the currently selected toggle
	void setSelectedToggle(int newSelectedToggleIndex) // Changes the currently selected toggle based on the given child index
		{
		/* Select the toggle of the requested index and update a potential tracked variable if the selection changed: */
		if(findAndSelectToggle(newSelectedToggleIndex))
			setTrackedSInt(newSelectedToggleIndex);
		}
	Misc::CallbackList& getValueChangedCallbacks(void) // Returns list of value changed callbacks
		{
		return valueChangedCallbacks;
		}
	};

}

#endif
