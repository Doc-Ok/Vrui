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

#include <GLMotif/QuikwritingTextEntryMethod.h>

#include <Geometry/Point.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/Popup.h>
#include <GLMotif/Quikwriting.h>

namespace GLMotif {

/*******************************************
Methods of class QuikwritingTextEntryMethod:
*******************************************/

void QuikwritingTextEntryMethod::popupQuikwritingPanel(const WidgetManager::Transformation& transform,Widget* widget)
	{
	/* Position the Quikwriting panel in the vicinity of the entry-requesting widget: */
	Vector offset;
	const Box& widgetExt=widget->getExterior();
	offset[0]=widgetExt.origin[0]+widgetExt.size[0]*0.5f;
	offset[1]=widgetExt.origin[1];
	offset[2]=widget->getZRange().second;
	const Box& panelExt=quikwritingPanel->getExterior();
	offset[0]-=panelExt.origin[0]+panelExt.size[0]*0.5f;
	offset[1]-=panelExt.origin[1]+panelExt.size[0]*1.05f;
	offset[2]-=quikwritingPanel->getZRange().first;
	
	/* Pop up the Quikwriting panel: */
	widgetManager->popupSecondaryWidget(widget,quikwritingPanel,offset);
	
	/* Set the Quikwriting widget's target widget: */
	quikwriting->setTargetWidget(widget);
	}

QuikwritingTextEntryMethod::QuikwritingTextEntryMethod(WidgetManager* sWidgetManager)
	:widgetManager(sWidgetManager),quikwritingPanel(0)
	{
	/* Create the Quikwriting panel: */
	Popup* panel=new Popup("QuikwritingPanel",widgetManager);
	
	quikwriting=new Quikwriting("Quikwriting",panel);
	
	quikwritingPanel=panel;
	}

QuikwritingTextEntryMethod::~QuikwritingTextEntryMethod(void)
	{
	/* Delete the Quikwriting panel: */
	delete quikwritingPanel;
	}

void QuikwritingTextEntryMethod::requestNumericEntry(const WidgetManager::Transformation& transform,Widget* widget)
	{
	/* Pop up the Quikwriting panel: */
	quikwriting->setShiftLevel(2,true);
	popupQuikwritingPanel(transform,widget);
	}

void QuikwritingTextEntryMethod::requestAlphaNumericEntry(const WidgetManager::Transformation& transform,Widget* widget)
	{
	/* Pop up the Quikwriting panel: */
	quikwriting->setShiftLevel(0,false);
	popupQuikwritingPanel(transform,widget);
	}

void QuikwritingTextEntryMethod::entryFinished(void)
	{
	/* Pop down the Quikwriting panel: */
	widgetManager->popdownWidget(quikwritingPanel);
	}

}
