/***********************************************************************
Event - Class to provide widgets with information they need to handle
events.
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

#include <GLMotif/Event.h>

#include <GLMotif/WidgetManager.h>
#include <GLMotif/Widget.h>

namespace GLMotif {

/**********************
Methods of class Event:
**********************/

Event::Event(bool sButtonState)
	:worldLocationType(NONE),
	 buttonState(sButtonState),targetWidget(0)
	{
	}

Event::Event(const Point& sWorldLocationPoint,bool sButtonState)
	:worldLocationType(POINT),worldLocationPoint(sWorldLocationPoint),
	 buttonState(sButtonState),targetWidget(0)
	{
	}

Event::Event(const Ray& sWorldLocationRay,bool sButtonState)
	:worldLocationType(RAY),worldLocationRay(sWorldLocationRay),
	 buttonState(sButtonState),targetWidget(0)
	{
	}

bool Event::setTargetWidget(Widget* newTargetWidget,const Event::WidgetPoint& newWidgetPoint)
	{
	/* If the widget point is ray-based, only set the target widget if the intersection is valid and closer than the current one: */
	if(worldLocationType==POINT||(newWidgetPoint.lambda>=Scalar(0)&&newWidgetPoint.lambda<widgetPoint.lambda))
		{
		/* Set the target widget: */
		widgetPoint=newWidgetPoint;
		targetWidget=newTargetWidget;
		
		return true;
		}
	else
		return false;
	}

Event::WidgetPoint Event::calcWidgetPoint(const Widget* widget) const
	{
	/* Check if the given widget is the same as the current target widget: */
	if(widget==targetWidget)
		{
		/* Return the stored point: */
		return widgetPoint;
		}
	else
		{
		/* Convert the world location to the widget's coordinate system: */
		const WidgetManager* manager=widget->getManager();
		WidgetManager::Transformation t=manager->calcWidgetTransformation(widget);
		
		WidgetPoint result;
		switch(worldLocationType)
			{
			case NONE:
				/* Whatshallwedoaboutit? */
				break;
			
			case POINT:
				result.point=t.inverseTransform(worldLocationPoint);
				break;
			
			case RAY:
				Ray ray=worldLocationRay;
				ray.inverseTransform(t);
				result.lambda=widget->intersectRay(ray,result.point);
				break;
			}
		
		return result;
		}
	}

}
