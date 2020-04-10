/***********************************************************************
WidgetManager - Class to manage top-level GLMotif UI components and user
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

#include <GLMotif/WidgetManager.h>

#include <string.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLLabel.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/WidgetArranger.h>
#include <GLMotif/TextEntryMethod.h>
#include <GLMotif/Event.h>
#include <GLMotif/Widget.h>
#include <GLMotif/WidgetAlgorithms.h>

namespace GLMotif {

/********************************************
Methods of class WidgetManager::PopupBinding:
********************************************/

WidgetManager::PopupBinding::PopupBinding(Widget* sTopLevelWidget,const WidgetManager::Transformation& sWidgetToWorld,WidgetManager::PopupBinding* sParent,WidgetManager::PopupBinding* sSucc)
	:topLevelWidget(sTopLevelWidget),widgetToWorld(sWidgetToWorld),visible(true),
	 parent(sParent),pred(0),succ(sSucc),firstSecondary(0)
	{
	}

WidgetManager::PopupBinding::~PopupBinding(void)
	{
	/* Delete all this binding's secondary bindings: */
	while(firstSecondary!=0)
		{
		PopupBinding* next=firstSecondary->succ;
		delete firstSecondary;
		firstSecondary=next;
		}
	}

const WidgetManager::PopupBinding* WidgetManager::PopupBinding::getSucc(void) const
	{
	if(firstSecondary!=0)
		return firstSecondary;
	else if(succ!=0)
		return succ;
	else
		{
		const PopupBinding* bPtr=parent;
		while(bPtr!=0&&bPtr->succ==0)
			bPtr=bPtr->parent;
		if(bPtr!=0)
			return bPtr->succ;
		else
			return 0;
		}
	}

WidgetManager::PopupBinding* WidgetManager::PopupBinding::getSucc(void)
	{
	if(firstSecondary!=0)
		return firstSecondary;
	else if(succ!=0)
		return succ;
	else
		{
		PopupBinding* bPtr=parent;
		while(bPtr!=0&&bPtr->succ==0)
			bPtr=bPtr->parent;
		if(bPtr!=0)
			return bPtr->succ;
		else
			return 0;
		}
	}

WidgetManager::PopupBinding* WidgetManager::PopupBinding::findTopLevelWidget(const Point& point)
	{
	if(!visible)
		return 0;
	
	Point widgetPoint=widgetToWorld.inverseTransform(point);
	PopupBinding* foundBinding=0;
	
	/* Check if our widget contains the given point: */
	if(topLevelWidget->isInside(widgetPoint))
		foundBinding=this;
	
	/* Traverse through all secondary bindings: */
	for(PopupBinding* bPtr=firstSecondary;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(widgetPoint);
	
	return foundBinding;
	}

WidgetManager::PopupBinding* WidgetManager::PopupBinding::findTopLevelWidget(const Ray& ray,Scalar& lambda)
	{
	if(!visible)
		return 0;
	
	Ray widgetRay=ray;
	widgetRay.inverseTransform(widgetToWorld);
	PopupBinding* foundBinding=0;
	
	/* Check if our widget intersects the given ray and is closer than the given lambda: */
	Point intersection;
	Scalar l=topLevelWidget->intersectRay(widgetRay,intersection);
	if(l>=Scalar(0)&&l<lambda&&topLevelWidget->isInside(intersection))
		{
		foundBinding=this;
		lambda=l;
		}
	
	/* Traverse through all secondary bindings: */
	for(PopupBinding* bPtr=firstSecondary;bPtr!=0;bPtr=bPtr->succ)
		{
		/* Find a top-level widget in the secondary binding and update our result if one is found: */
		PopupBinding* fb=bPtr->findTopLevelWidget(widgetRay,lambda);
		if(fb!=0)
			foundBinding=fb;
		}
	
	return foundBinding;
	}

void WidgetManager::PopupBinding::draw(bool overlayWidgets,GLContextData& contextData) const
	{
	if(visible)
		{
		glPushMatrix();
		
		/* Go to the top-level widget's coordinate system: */
		glMultMatrix(widgetToWorld);
		
		/* Draw all its secondary top level widgets: */
		for(PopupBinding* bPtr=firstSecondary;bPtr!=0;bPtr=bPtr->succ)
			bPtr->draw(overlayWidgets,contextData);
		
		/* Draw the top level widget: */
		{
		GLLabel::DeferredRenderer dr(contextData);
		topLevelWidget->draw(contextData);
		dr.draw();
		}
		
		if(overlayWidgets)
			{
			/* Draw the top level widget again to squash the z buffer: */
			GLdouble depthRange[2];
			glGetDoublev(GL_DEPTH_RANGE,depthRange);
			glDepthRange(0.0,0.0);
			GLboolean colorMask[4];
			glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			{
			GLLabel::DeferredRenderer dr(contextData);
			topLevelWidget->draw(contextData);
			dr.draw();
			}
			glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
			glDepthRange(depthRange[0],depthRange[1]);
			}
		
		glPopMatrix();
		}
	}

/******************************
Methods of class WidgetManager:
******************************/

const WidgetManager::PopupBinding* WidgetManager::getRootBinding(const Widget* widget) const
	{
	PopupBindingMap::ConstIterator pbmIt=popupBindingMap.findEntry(widget->getRoot());
	return pbmIt.isFinished()?0:pbmIt->getDest();
	}

WidgetManager::PopupBinding* WidgetManager::getRootBinding(const Widget* widget)
	{
	PopupBindingMap::Iterator pbmIt=popupBindingMap.findEntry(widget->getRoot());
	return pbmIt.isFinished()?0:pbmIt->getDest();
	}

void WidgetManager::popupPrimaryWidgetAt(Widget* topLevelWidget,const WidgetManager::Transformation& widgetToWorld)
	{
	/* Check if the widget is already popped up: */
	if(!popupBindingMap.isEntry(topLevelWidget))
		{
		/* Pop up the widget: */
		PopupBinding* newBinding=new PopupBinding(topLevelWidget,widgetToWorld,0,firstBinding);
		if(firstBinding!=0)
			firstBinding->pred=newBinding;
		firstBinding=newBinding;
		popupBindingMap.setEntry(PopupBindingMap::Entry(topLevelWidget,newBinding));
		
		{
		/* Call the pop-up callbacks: */
		WidgetPopCallbackData cbData(this,true,topLevelWidget,true);
		widgetPopCallbacks.call(&cbData);
		}
		
		{
		/* Call the widget move callbacks: */
		WidgetMoveCallbackData cbData(this,widgetToWorld,newBinding->topLevelWidget,true);
		widgetMoveCallbacks.call(&cbData);
		
		/* Recurse into the primary binding: */
		moveSecondaryWidgets(newBinding,widgetToWorld);
		}
		}
	}

void WidgetManager::moveSecondaryWidgets(WidgetManager::PopupBinding* parent,const WidgetManager::Transformation& parentTransform)
	{
	/* Iterate through the parent's child bindings: */
	for(PopupBinding* pbPtr=parent->firstSecondary;pbPtr!=0;pbPtr=pbPtr->succ)
		{
		/* Calculate the secondary binding's transformation: */
		Transformation childTransform=parentTransform;
		childTransform*=pbPtr->widgetToWorld;
		
		/* Call the move callback: */
		WidgetMoveCallbackData cbData(this,childTransform,pbPtr->topLevelWidget,false);
		widgetMoveCallbacks.call(&cbData);
		
		/* Recurse into the child binding: */
		moveSecondaryWidgets(pbPtr,childTransform);
		}
	}

void WidgetManager::removeFocusFromChild(Widget* widget)
	{
	/* Bail out if there is no text focus widget: */
	if(textFocusWidget==0)
		return;
	
	/* Traverse upwards from the text focus widget until the given widget is reached: */
	Widget* tfwParent=textFocusWidget;
	while(tfwParent!=0)
		{
		if(tfwParent==widget)
			{
			/* Take text focus away from the widget: */
			textFocusWidget->takeTextFocus();
			textFocusWidget=0;
			}
		
		/* Go up to the next parent: */
		tfwParent=tfwParent->getParent();
		}
	}

void WidgetManager::deleteWidgetImmediately(Widget* widget)
	{
	/* Release a pointer grab held by the widget: */
	if(pointerGrabWidget==widget)
		{
		hardGrab=false;
		pointerGrabWidget=0;
		}
	
	/* Release the focus if the widget or one of its children had it: */
	removeFocusFromChild(widget);
	
	/* Delete a widget attribute associated with the widget: */
	WidgetAttributeMap::Iterator waIt=widgetAttributeMap.findEntry(widget);
	if(!waIt.isFinished())
		{
		delete waIt->getDest();
		widgetAttributeMap.removeEntry(waIt);
		}

	delete widget;
	}

void WidgetManager::deleteQueuedWidgets(void)
	{
	/* Delete all queued-up widgets: */
	for(std::vector<Widget*>::iterator dlIt=deletionList.begin();dlIt!=deletionList.end();++dlIt)
		deleteWidgetImmediately(*dlIt);
	deletionList.clear();
	}

WidgetManager::WidgetManager(void)
	:styleSheet(0),arranger(0),textEntryMethod(0),
	 timerEventScheduler(0),drawOverlayWidgets(false),
	 widgetAttributeMap(101),
	 firstBinding(0),popupBindingMap(31),
	 time(0.0),
	 hardGrab(false),pointerGrabWidget(0),
	 textFocusWidget(0),
	 inEventProcessing(false),
	 textBufferLength(0),textBuffer(new char[1])
	{
	/* Initialize the cut & paste buffer: */
	textBuffer[0]='\0';
	}

WidgetManager::~WidgetManager(void)
	{
	/* Delete all widget attributes: */
	for(WidgetAttributeMap::Iterator waIt=widgetAttributeMap.begin();!waIt.isFinished();++waIt)
		delete waIt->getDest();
	widgetAttributeMap.clear();
	
	/* Delete all bindings: */
	while(firstBinding!=0)
		{
		PopupBinding* next=firstBinding->succ;
		delete firstBinding;
		firstBinding=next;
		}
	
	/* Delete the cut & paste buffer: */
	delete[] textBuffer;
	
	/* Delete other helper objects: */
	delete arranger;
	delete textEntryMethod;
	}

void WidgetManager::setStyleSheet(const StyleSheet* newStyleSheet)
	{
	styleSheet=newStyleSheet;
	}

void WidgetManager::setArranger(WidgetArranger* newArranger)
	{
	delete arranger;
	arranger=newArranger;
	}

void WidgetManager::setTextEntryMethod(TextEntryMethod* newTextEntryMethod)
	{
	delete textEntryMethod;
	textEntryMethod=newTextEntryMethod;
	}

void WidgetManager::setTimerEventScheduler(Misc::TimerEventScheduler* newTimerEventScheduler)
	{
	timerEventScheduler=newTimerEventScheduler;
	}

void WidgetManager::setDrawOverlayWidgets(bool newDrawOverlayWidgets)
	{
	drawOverlayWidgets=newDrawOverlayWidgets;
	}

void WidgetManager::unmanageWidget(Widget* widget)
	{
	/* Check if the widget has an attribute: */
	WidgetAttributeMap::Iterator waIt=widgetAttributeMap.findEntry(widget);
	if(!waIt.isFinished())
		{
		/* Delete the attribute: */
		delete waIt->getDest();
		widgetAttributeMap.removeEntry(waIt);
		}
	}

void WidgetManager::popupPrimaryWidget(Widget* topLevelWidget)
	{
	/* Pop up with a default widget transformation: */
	popupPrimaryWidgetAt(topLevelWidget,arranger->calcTopLevelTransform(topLevelWidget));
	}

void WidgetManager::popupPrimaryWidget(Widget* topLevelWidget,const Point& hotspot)
	{
	/* Pop up with a hot spot widget transformation: */
	popupPrimaryWidgetAt(topLevelWidget,arranger->calcTopLevelTransform(topLevelWidget,hotspot));
	}

void WidgetManager::popupPrimaryWidget(Widget* topLevelWidget,const WidgetManager::Transformation& widgetToWorld)
	{
	/* Pop up with a full widget transformation: */
	popupPrimaryWidgetAt(topLevelWidget,arranger->calcTopLevelTransform(topLevelWidget,widgetToWorld));
	}

void WidgetManager::popupSecondaryWidget(const Widget* owner,Widget* topLevelWidget,const Vector& offset)
	{
	/* Check if the widget is already popped up: */
	if(!popupBindingMap.isEntry(topLevelWidget))
		{
		/* Find the owner's binding: */
		PopupBinding* ownerBinding=getRootBinding(owner);
		if(ownerBinding!=0)
			{
			Transformation widgetToWorld=Transformation::translate(Transformation::Vector(offset.getXyzw()));
			PopupBinding* newBinding=new PopupBinding(topLevelWidget,widgetToWorld,ownerBinding,ownerBinding->firstSecondary);
			if(ownerBinding->firstSecondary!=0)
				ownerBinding->firstSecondary->pred=newBinding;
			ownerBinding->firstSecondary=newBinding;
			popupBindingMap.setEntry(PopupBindingMap::Entry(topLevelWidget,newBinding));
			
			{
			/* Call the pop-up callbacks: */
			WidgetPopCallbackData cbData(this,true,topLevelWidget,false);
			widgetPopCallbacks.call(&cbData);
			}
			
			{
			/* Call the widget move callbacks: */
			WidgetMoveCallbackData cbData(this,widgetToWorld,newBinding->topLevelWidget,false);
			widgetMoveCallbacks.call(&cbData);

			/* Recurse into the primary binding: */
			moveSecondaryWidgets(newBinding,widgetToWorld);
			}
			}
		}
	}

void WidgetManager::popdownWidget(Widget* widget)
	{
	/* Find the widget's binding: */
	Widget* topLevelWidget=widget->getRoot();
	PopupBindingMap::Iterator pbmIt=popupBindingMap.findEntry(topLevelWidget);
	if(!pbmIt.isFinished())
		{
		PopupBinding* binding=pbmIt->getDest();
		
		/* Pop down all secondary widgets belonging to this popup binding: */
		while(binding->firstSecondary!=0)
			popdownWidget(binding->firstSecondary->topLevelWidget);
		
		/* Take text focus away from any child of the popped-down widget: */
		removeFocusFromChild(topLevelWidget);
		
		/* Call the pop-down callbacks: */
		WidgetPopCallbackData cbData(this,false,topLevelWidget,binding->parent==0);
		widgetPopCallbacks.call(&cbData);
		
		/* Remove the binding from the list: */
		if(binding->pred!=0)
			binding->pred->succ=binding->succ;
		else if(binding->parent!=0)
			binding->parent->firstSecondary=binding->succ;
		else
			firstBinding=binding->succ;
		if(binding->succ!=0)
			binding->succ->pred=binding->pred;
		delete binding;
		popupBindingMap.removeEntry(pbmIt);
		}
	}

void WidgetManager::show(Widget* widget)
	{
	/* Find the widget's binding: */
	PopupBinding* binding=getRootBinding(widget);
	if(binding!=0)
		{
		/* Mark the binding as visible: */
		binding->visible=true;
		}
	}

void WidgetManager::hide(Widget* widget)
	{
	/* Find the widget's binding: */
	PopupBinding* binding=getRootBinding(widget);
	if(binding!=0)
		{
		/* Mark the binding as invisible: */
		binding->visible=false;
		}
	}

bool WidgetManager::isManaged(const Widget* widget) const
	{
	/* Check if the widget's root has an entry in the popup binding map: */
	return popupBindingMap.isEntry(widget->getRoot());
	}

bool WidgetManager::isVisible(const Widget* widget) const
	{
	/* Find the widget's binding: */
	const PopupBinding* binding=getRootBinding(widget);
	
	/* Return true if binding exists and has visible flag set: */
	return binding!=0&&binding->visible;
	}

Widget* WidgetManager::findPrimaryWidget(const Point& point)
	{
	/* Find a recipient for this event amongst all primary bindings: */
	PopupBinding* foundBinding=0;
	for(PopupBinding* bPtr=firstBinding;bPtr!=0&&foundBinding==0;bPtr=bPtr->succ)
		foundBinding=bPtr->findTopLevelWidget(point);
	
	/* Bail out if no widget was found: */
	if(foundBinding==0)
		return 0;
	
	/* Find the primary top level widget containing the found widget: */
	while(foundBinding->parent!=0)
		foundBinding=foundBinding->parent;
	
	/* Return the top level widget: */
	return foundBinding->topLevelWidget;
	}

Widget* WidgetManager::findPrimaryWidget(const Ray& ray,Scalar& lambda)
	{
	/* Initialize lambda to the invalid value: */
	lambda=Math::Constants<Scalar>::max;
	
	/* Find a recipient for this event amongst all primary bindings: */
	PopupBinding* foundBinding=0;
	for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
		{
		PopupBinding* fb=bPtr->findTopLevelWidget(ray,lambda);
		if(fb!=0)
			foundBinding=fb;
		}
	
	/* Bail out if no widget was found: */
	if(foundBinding==0)
		return 0;
	
	/* Find the primary top level widget containing the found widget: */
	while(foundBinding->parent!=0)
		foundBinding=foundBinding->parent;
	
	/* Return the top level widget: */
	return foundBinding->topLevelWidget;
	}

WidgetManager::Transformation WidgetManager::calcWidgetTransformation(const Widget* widget) const
	{
	Transformation result=Transformation::identity;
	
	/* Find the widget's binding: */
	const PopupBinding* bPtr=getRootBinding(widget);
	
	/* Concatenate all transformations up to the primary top level widget: */
	while(bPtr!=0)
		{
		result.leftMultiply(bPtr->widgetToWorld);
		bPtr=bPtr->parent;
		}
	
	return result;
	}

void WidgetManager::setPrimaryWidgetTransformation(Widget* widget,const WidgetManager::Transformation& newWidgetToWorld)
	{
	/* Find the widget's binding: */
	PopupBinding* bPtr=getRootBinding(widget);
	
	/* Check if the binding exists and is a top-level binding: */
	if(bPtr!=0&&bPtr->parent==0)
		{
		/* Adjust and set the binding's widget transformation: */
		bPtr->widgetToWorld=arranger->calcTopLevelTransform(bPtr->topLevelWidget,newWidgetToWorld);
		
		/* Call the widget move callbacks: */
		WidgetMoveCallbackData cbData(this,newWidgetToWorld,bPtr->topLevelWidget,true);
		widgetMoveCallbacks.call(&cbData);
		
		/* Recurse into the primary binding: */
		moveSecondaryWidgets(bPtr,bPtr->widgetToWorld);
		}
	}

void WidgetManager::deleteWidget(Widget* widget)
	{
	if(inEventProcessing)
		{
		/* Mark the widget for deletion: */
		deletionList.push_back(widget);
		}
	else
		{
		/* Delete the widget immediately: */
		deleteWidgetImmediately(widget);
		}
	}

void WidgetManager::setTime(double newTime)
	{
	/* Set the time: */
	time=newTime;
	}

void WidgetManager::draw(GLContextData& contextData) const
	{
	/* Traverse all primary top level widgets: */
	for(const PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
		bPtr->draw(drawOverlayWidgets,contextData);
	}

bool WidgetManager::pointerButtonDown(Event& event)
	{
	EventProcessingLocker epl(this);
	
	bool result=false;
	
	/* Find a recipient for the event: */
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event, or release its pointer grab: */
		pointerGrabWidget->findRecipient(event);
		}
	
	if(pointerGrabWidget==0)
		{
		/* Find a recipient for this event amongst the primary top-level windows: */
		if(drawOverlayWidgets)
			{
			/* Find the first visible top-level widget in the stacking order that is hit by the event: */
			PopupBinding* bPtr;
			for(bPtr=firstBinding;bPtr!=0&&!(bPtr->visible&&bPtr->topLevelWidget->findRecipient(event));bPtr=bPtr->succ)
				;
			
			if(bPtr!=0&&bPtr!=firstBinding)
				{
				/* Move the found top level widget to the front of the stacking order: */
				bPtr->pred->succ=bPtr->succ;
				if(bPtr->succ!=0)
					bPtr->succ->pred=bPtr->pred;
				bPtr->pred=0;
				bPtr->succ=firstBinding;
				firstBinding->pred=bPtr;
				firstBinding=bPtr;
				}
			}
		else
			{
			/* Ask each visible top-level widget to inspect the event to find the closest hit: */
			for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
				if(bPtr->visible)
					bPtr->topLevelWidget->findRecipient(event);
			}
		}
	
	if(pointerGrabWidget!=0||event.getTargetWidget()!=0)
		{
		/* Initiate a "soft" pointer grab if there is no grab active: */
		if(pointerGrabWidget==0)
			pointerGrabWidget=event.getTargetWidget();
		
		/* Pass the event to the grabbing widget (either the found target widget, or a hard grabber): */
		pointerGrabWidget->pointerButtonDown(event);
		
		result=true;
		}
	
	return result;
	}

bool WidgetManager::pointerButtonUp(Event& event)
	{
	EventProcessingLocker epl(this);
	
	bool result=false;
	
	/* Deliver the event to the widget that holds the hard or "soft" pointer grab: */
	Widget* targetWidget=pointerGrabWidget;
	if(targetWidget!=0)
		{
		/* Allow the grabbing widget to modify the event, or release its pointer grab: */
		targetWidget->findRecipient(event);
		
		/* Release a "soft" grab initiated by a pointerButtonDown event: */
		if(!hardGrab)
			pointerGrabWidget=0;
		
		/* Pass the event to the current or former grabbing widget: */
		targetWidget->pointerButtonUp(event);
		
		result=true;
		}
	
	return result;
	}

bool WidgetManager::pointerMotion(Event& event)
	{
	EventProcessingLocker epl(this);
	
	/* Find a recipient for the event: */
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event, or release its pointer grab: */
		pointerGrabWidget->findRecipient(event);
		}
	
	if(pointerGrabWidget==0)
		{
		/* Find a recipient for this event amongst the primary top-level windows: */
		if(drawOverlayWidgets)
			{
			/* Find the first visible top-level widget in the stacking order that is hit by the event: */
			PopupBinding* bPtr;
			for(bPtr=firstBinding;bPtr!=0&&!(bPtr->visible&&bPtr->topLevelWidget->findRecipient(event));bPtr=bPtr->succ)
				;
			}
		else
			{
			/* Ask each visible top-level widget to inspect the event to find the closest hit: */
			for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
				if(bPtr->visible)
					bPtr->topLevelWidget->findRecipient(event);
			}
		}
	
	/* Check if a widget claimed the event: */
	if(pointerGrabWidget!=0)
		{
		/* Pass the event to the grabbing widget: */
		pointerGrabWidget->pointerMotion(event);
		return true;
		}
	else if(event.getTargetWidget()!=0)
		{
		/* Pass the event to the found target: */
		event.getTargetWidget()->pointerMotion(event);
		return true;
		}
	else
		return false;
	}

void WidgetManager::grabPointer(Widget* widget)
	{
	if(pointerGrabWidget==0)
		{
		/* Check if the widget is popped up: */
		if(isManaged(widget))
			{
			/* Set a hard grab: */
			hardGrab=true;
			pointerGrabWidget=widget;
			pointerGrabWidgetToWorld=calcWidgetTransformation(widget);
			}
		}
	else if(pointerGrabWidget==widget)
		{
		/* Turn a soft grab into a hard grab: */
		hardGrab=true;
		}
	}

void WidgetManager::releasePointer(Widget* widget)
	{
	/* Check if the given widget has a hard grab: */
	if(widget==pointerGrabWidget&&hardGrab)
		{
		hardGrab=false;
		pointerGrabWidget=0;
		}
	}

bool WidgetManager::requestFocus(Widget* widget)
	{
	/* Change the text focus widget: */
	if(textFocusWidget!=0)
		textFocusWidget->takeTextFocus();
	textFocusWidget=widget;
	
	return true;
	}

void WidgetManager::releaseFocus(Widget* widget)
	{
	if(textFocusWidget==widget)
		{
		/* TODO: Move the text focus to the next widget: */
		textFocusWidget=0;
		}
	}

void WidgetManager::focusPreviousWidget(void)
	{
	}

void WidgetManager::focusNextWidget(void)
	{
	/* Go to the next focus-accepting widget inside the same top-level widget: */
	PopupBinding* initialBinding=0;
	if(textFocusWidget!=0)
		{
		/* Take focus away from the widget: */
		textFocusWidget->takeTextFocus();
		
		/* Go to the next widget that will accept focus: */
		Widget* w=textFocusWidget;
		do
			w=getNextWidget(w);
		while(w!=0&&!w->giveTextFocus());
		if(w==0)
			initialBinding=getRootBinding(textFocusWidget);
		textFocusWidget=w;
		}
	
	/* Traverse the hierarchy of top-level widgets until a focus widget is found: */
	PopupBinding* bPtr=initialBinding;
	while(textFocusWidget==0)
		{
		/* Go to the next top-level widget: */
		bPtr=bPtr!=0?bPtr->getSucc():firstBinding;
		if(bPtr==0)
			bPtr=firstBinding;
		
		/* Bail out if the entire hierarchy was traversed: */
		if(bPtr==initialBinding)
			break;
		
		/* Go to the first widget that will accept focus: */
		textFocusWidget=bPtr->topLevelWidget;
		while(textFocusWidget!=0&&!textFocusWidget->giveTextFocus())
			textFocusWidget=getNextWidget(textFocusWidget);
		}
	}

void WidgetManager::requestNumericEntry(Widget* widget)
	{
	/* Forward the request to the text entry method, using the widget's top-level transformation: */
	if(textEntryMethod!=0)
		textEntryMethod->requestNumericEntry(calcWidgetTransformation(widget),widget);
	}

void WidgetManager::requestAlphaNumericEntry(Widget* widget)
	{
	/* Forward the request to the text entry method, using the widget's top-level transformation: */
	if(textEntryMethod!=0)
		textEntryMethod->requestAlphaNumericEntry(calcWidgetTransformation(widget),widget);
	}

void WidgetManager::textEntryFinished(void)
	{
	/* Forward the notification to the text entry method: */
	if(textEntryMethod!=0)
		textEntryMethod->entryFinished();
	}

bool WidgetManager::text(const TextEvent& textEvent)
	{
	EventProcessingLocker epl(this);
	
	/* Pass the event to the text focus widget: */
	if(textFocusWidget!=0)
		textFocusWidget->textEvent(textEvent);
	
	return textFocusWidget!=0;
	}

bool WidgetManager::textControl(Event& event,const TextControlEvent& textControlEvent)
	{
	EventProcessingLocker epl(this);
	
	bool result=false;
	
	if(pointerGrabWidget!=0)
		{
		/* Allow the grabbing widget to modify the event: */
		pointerGrabWidget->findRecipient(event);
		
		/* Pass the event to the grabbing widget: */
		pointerGrabWidget->textControlEvent(textControlEvent);
		
		result=pointerGrabWidget!=0;
		}
	else
		{
		/* Find a recipient for this event amongst the primary top-level windows: */
		for(PopupBinding* bPtr=firstBinding;bPtr!=0;bPtr=bPtr->succ)
			if(bPtr->visible)
				bPtr->topLevelWidget->findRecipient(event);
		
		if(event.getTargetWidget()!=0)
			{
			/* Pass the event to the found target: */
			event.getTargetWidget()->textControlEvent(textControlEvent);
			
			result=true;
			}
		}
	
	return result;
	}

bool WidgetManager::textControl(const TextControlEvent& textControlEvent)
	{
	EventProcessingLocker epl(this);
	
	/* Pass the event to the text focus widget: */
	if(textFocusWidget!=0)
		textFocusWidget->textControlEvent(textControlEvent);
	
	return textFocusWidget!=0;
	}

void WidgetManager::setTextBuffer(int newTextBufferLength,const char* newTextBuffer)
	{
	/* Reallocate the buffer: */
	if(textBufferLength!=newTextBufferLength)
		{
		delete[] textBuffer;
		textBufferLength=newTextBufferLength;
		textBuffer=new char[textBufferLength+1];
		}
	
	/* Copy the new buffer contents: */
	memcpy(textBuffer,newTextBuffer,textBufferLength+1);
	}

}
