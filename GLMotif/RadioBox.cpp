/***********************************************************************
RadioBox - Subclass of RowColumn that contains only mutually exclusive
ToggleButton objects.
Copyright (c) 2001-2020 Oliver Kreylos

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

#include <stdio.h>
#include <Misc/ThrowStdErr.h>
#include <GLMotif/ToggleButton.h>

#include <GLMotif/RadioBox.h>

namespace GLMotif {

/*************************
Methods of class RadioBox:
*************************/

void RadioBox::childrenValueChangedCallbackWrapper(Misc::CallbackData* callbackData,void* userData)
	{
	/* Extract the widget pointers: */
	ToggleButton::ValueChangedCallbackData* cbStruct=static_cast<ToggleButton::ValueChangedCallbackData*>(callbackData);
	RadioBox* thisPtr=static_cast<RadioBox*>(userData);
	
	/* Change the radio box' state: */
	ToggleButton* oldSelectedToggle=thisPtr->selectedToggle;
	if(cbStruct->set)
		{
		/* Unset the previously selected toggle: */
		if(oldSelectedToggle!=0&&oldSelectedToggle!=cbStruct->toggle)
			oldSelectedToggle->setToggle(false);
		
		/* Set the new toggle: */
		thisPtr->selectedToggle=cbStruct->toggle;
		}
	else if(cbStruct->toggle==oldSelectedToggle)
		{
		if(thisPtr->selectionMode==ALWAYS_ONE) // We can't allow the selected toggle to just unselect itself!
			oldSelectedToggle->setToggle(true);
		else
			thisPtr->selectedToggle=0;
		}
	
	/* Check if variable tracking is active: */
	if(thisPtr->isTracking())
		{
		/* Set the tracked variable to the index of the new selected toggle, or -1: */
		thisPtr->setTrackedSInt(thisPtr->getToggleIndex(thisPtr->selectedToggle));
		}
	
	/* Call the value changed callbacks: */
	RadioBox::ValueChangedCallbackData cbData(thisPtr,oldSelectedToggle,thisPtr->selectedToggle);
	thisPtr->valueChangedCallbacks.call(&cbData);
	}

bool RadioBox::findAndSelectToggle(int toggleIndex)
	{
	bool result=false;
	
	if(toggleIndex<0)
		{
		/* Unset the previously selected toggle if there was one and the selection mode allows it: */
		if(selectionMode==ATMOST_ONE&&selectedToggle!=0)
			{
			/* Unset the previously selected toggle: */
			selectedToggle->setToggle(false);
			
			/* Reset the selected toggle: */
			selectedToggle=0;
			
			result=true;
			}
		}
	else
		{
		/* Find the child toggle button of the given index: */
		for(WidgetList::iterator chIt=children.begin();chIt!=children.end();++chIt)
			{
			/* Check if the child is a toggle button: */
			ToggleButton* toggle=dynamic_cast<ToggleButton*>(*chIt);
			if(toggle!=0)
				{
				/* Check if this is the requested toggle: */
				if(toggleIndex==0)
					{
					/* Check if the selection actually changed: */
					if(selectedToggle!=toggle)
						{
						/* Unset the previously selected toggle: */
						if(selectedToggle!=0)
							selectedToggle->setToggle(false);
						
						/* Set the found toggle: */
						toggle->setToggle(true);
						
						/* Select the toggle: */
						selectedToggle=toggle;
						
						result=true;
						}
					
					/* Stop searching: */
					break;
					}
				
				--toggleIndex;
				}
			}
		}
	
	return result;
	}

RadioBox::RadioBox(const char* sName,Container* sParent,bool sManageChild)
	:RowColumn(sName,sParent,false),selectionMode(ATMOST_ONE),selectedToggle(0)
	{
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

void RadioBox::updateVariables(void)
	{
	/* Check if tracking is active: */
	if(isTracking())
		{
		/* Select the toggle whose index matches the tracked variable's current value: */
		findAndSelectToggle(int(getTrackedSInt()));
		}
	}

void RadioBox::addChild(Widget* newChild)
	{
	/* If the new child is a toggle, initialize it: */
	ToggleButton* newToggle=dynamic_cast<ToggleButton*>(newChild);
	if(newToggle!=0)
		{
		/* Set the new toggle's defaults and callbacks: */
		newToggle->setBorderWidth(0.0f);
		newToggle->setToggleType(ToggleButton::RADIO_BUTTON);
		newToggle->setHAlignment(GLFont::Left);
		newToggle->getValueChangedCallbacks().add(childrenValueChangedCallbackWrapper,this);
		
		/* Set/unset the new toggle to satisfy our selection mode: */
		newToggle->setToggle(selectionMode==ALWAYS_ONE&&selectedToggle==0);
		if(newToggle->getToggle())
			{
			/* Select the new toggle and update a potentially tracked variable: */
			selectedToggle=newToggle;
			setTrackedSInt(0);
			}
		}
	
	/* Call the parent class method: */
	RowColumn::addChild(newChild);
	}

void RadioBox::addToggle(const char* newToggleLabel)
	{
	/* Create a new toggle button: */
	char newToggleName[40];
	snprintf(newToggleName,sizeof(newToggleName),"_RadioBoxToggle%d",int(children.size()));
	new ToggleButton(newToggleName,this,newToggleLabel);
	}

int RadioBox::getToggleIndex(const ToggleButton* toggle) const
	{
	int index=0;
	for(WidgetList::const_iterator chIt=children.begin();chIt!=children.end();++chIt)
		{
		/* Ignore any children that are not toggle buttons: */
		if(dynamic_cast<const ToggleButton*>(*chIt)!=0)
			{
			if(*chIt==toggle)
				return index;
			++index;
			}
		}
	
	return -1;
	}

void RadioBox::setSelectionMode(RadioBox::SelectionMode newSelectionMode)
	{
	/* Set the selection mode: */
	selectionMode=newSelectionMode;
	
	/* Enforce the new mode: */
	if(selectionMode==ALWAYS_ONE&&selectedToggle==0)
		{
		/* Select the first child toggle button: */
		for(WidgetList::const_iterator chIt=children.begin();chIt!=children.end();++chIt)
			{
			/* Check if the child is a toggle button: */
			ToggleButton* toggle=dynamic_cast<ToggleButton*>(*chIt);
			if(toggle!=0)
				{
				/* Select the toggle and update a potentially tracked variable: */
				selectedToggle=toggle;
				selectedToggle->setToggle(true);
				setTrackedSInt(0);
				
				/* Stop searching: */
				break;
				}
			}
		}
	}

void RadioBox::setSelectedToggle(ToggleButton* newSelectedToggle)
	{
	/* Bail out if the selection did not change: */
	if(selectedToggle==newSelectedToggle)
		return;
	
	if(newSelectedToggle!=0)
		{
		/* Ensure that the new selected toggle is part of the radio box, and find its toggle index: */
		int toggleIndex=0;
		for(WidgetList::iterator chIt=children.begin();chIt!=children.end();++chIt)
			{
			if(*chIt==newSelectedToggle)
				{
				/* Unset the previously selected toggle: */
				if(selectedToggle!=0)
					selectedToggle->setToggle(false);
				
				/* Set the new selected toggle: */
				newSelectedToggle->setToggle(true);
				
				/* Select the new toggle and update a potential tracked variable: */
				selectedToggle=newSelectedToggle;
				setTrackedSInt(toggleIndex);
				
				/* Stop searching: */
				break;
				}
			
			/* Increment the toggle index if the child is a toggle button: */
			if(dynamic_cast<ToggleButton*>(*chIt)!=0)
				++toggleIndex;
			}
		}
	else if(selectionMode==ATMOST_ONE)
		{
		/* Unset the previously selected toggle: */
		selectedToggle->setToggle(false);
		
		/* Reset the selected toggle and update a potential tracked variable: */
		selectedToggle=0;
		setTrackedSInt(-1);
		}
	}

}
