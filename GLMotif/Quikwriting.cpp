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

#include <GLMotif/Quikwriting.h>

#include <stdio.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLVertexTemplates.h>
#include <GL/GLContextData.h>
#include <GL/GLLightTracker.h>
#include <GL/Extensions/GLEXTFramebufferObject.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>
#include <GLMotif/Config.h>
#include <GLMotif/Event.h>
#include <GLMotif/TextEvent.h>
#include <GLMotif/TextControlEvent.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>

namespace GLMotif {

/**************************************
Methods of class Quikwriting::DataItem:
**************************************/

Quikwriting::DataItem::DataItem(void)
	{
	/* Create the texture objects: */
	glGenTextures(4,textures);
	}

Quikwriting::DataItem::~DataItem(void)
	{
	/* Destroy the texture objects: */
	glDeleteTextures(4,textures);
	}

/************************************
Static elements of class Quikwriting:
************************************/

const int Quikwriting::characterTable[4][8][8]=
	{
	{ // Shift level 0 - lowercase characters
	{'t',Upper,0,0,0,0,0,Symbols},
	{'f','n','r','p',0,0,0,'x'},
	{0,'u',' ','y',0,0,0,0},
	{0,'j','l','i','d','b',0,0},
	{0,0,0,Numerical,'e',Enter,0,0},
	{0,0,0,'z','g','o','w','v'},
	{0,0,0,0,0,'c',Backspace,'h'},
	{'s','k',0,0,0,'q','m','a'}
	},
	{ // Shift level 1 - uppercase characters
	{'T',Upper,0,0,0,0,0,Symbols},
	{'F','N','R','P',0,0,0,'X'},
	{0,'U',' ','Y',0,0,0,0},
	{0,'J','L','I','D','B',0,0},
	{0,0,0,Numerical,'E',Enter,0,0},
	{0,0,0,'Z','G','O','W','V'},
	{0,0,0,0,0,'C',Backspace,'H'},
	{'S','K',0,0,0,'Q','M','A'}
	},
	{ // Shift level 2 - numerical symbols
	{'2',Upper,0,0,0,0,0,Symbols},
	{')','3','/','*',0,0,0,']'},
	{0,'5',' ','7',0,0,0,0},
	{0,'#','=','0','.','>',0,0},
	{0,0,0,Numerical,'9',Enter,0,0},
	{0,0,0,'<',',','8','-','+'},
	{0,0,0,0,0,'6',Backspace,'4'},
	{'(','[',0,0,0,'$','%','1'}
	},
	{ // Shift level 3 - punctuation symbols
	{':',Upper,0,0,0,0,0,Symbols},
	{'}','!','/','*',0,0,0,']'},
	{0,'"',' ','&',0,0,0,0},
	{0,'#','|','.','^','\\',0,0},
	{0,0,0,Numerical,';',Enter,0,0},
	{0,0,0,'@','`',',','-','~'},
	{0,0,0,0,0,'_',Backspace,'\''},
	{'{','[',0,0,0,'$','%','?'}
	}
	};

/****************************
Methods of class Quikwriting:
****************************/

void  Quikwriting::calcLayout(void)
	{
	/* Calculate the widget's center point and radius: */
	center=getInterior().origin;
	for(int i=0;i<2;++i)
		center[i]+=getInterior().size[i]*0.5f;
	GLfloat radius=Math::min(getInterior().size[0],getInterior().size[1])*0.5f;
	
	/* Calculate the radius of the central area: */
	centralRadius=radius*5.0f/11.0f;
	centralRadius2=Math::sqr(centralRadius);
	
	/* Calculate the parabola equation for the "north" outer area: */
	a=1.4f*2.75f/(Math::sqr(1.025f)*radius);
	c=1.35f*radius/2.75f;
	}

int Quikwriting::findArea(const Event& event) const
	{
	/* Check if the event happened inside the central area: */
	GLfloat dx=event.getWidgetPoint().getPoint()[0]-center[0];
	GLfloat dy=event.getWidgetPoint().getPoint()[1]-center[1];
	GLfloat r2=Math::sqr(dx)+Math::sqr(dy);
	if(r2<=centralRadius2)
		return 0;
	
	/* Determine the index of the outer area potentially containing the event: */
	static const GLfloat ca=0.92387953f; // cos(22.5)
	static const GLfloat sa=0.38268343f; // sin(22.5)
	static const GLfloat scb=0.70710678f; // sin(45)=cos(45)
	if(dx*ca+dy*sa>=0.0f) // Areas 1-4
		{
		if(dx*sa-dy*ca>=0.0f) // Areas 3-4
			{
			if(-dx*sa-dy*ca>=0.0f)
				{
				if(a*Math::sqr(-scb*(dx+dy))+c<=scb*(dx-dy))
					return 4;
				}
			else
				{
				if(a*Math::sqr(-dy)+c<=dx)
					return 3;
				}
			}
		else // Areas 1-2
			{
			if(dx*ca-dy*sa>=0.0f)
				{
				if(a*Math::sqr(scb*(dx-dy))+c<=scb*(dx+dy))
					return 2;
				}
			else
				{
				if(a*Math::sqr(dx)+c<=dy)
					return 1;
				}
			}
		}
	else // Areas 5-8
		{
		if(dx*sa-dy*ca>=0.0f) // Areas 5-6
			{
			if(dx*ca-dy*sa>=0.0f)
				{
				if(a*Math::sqr(-dx)+c<=-dy)
					return 5;
				}
			else
				{
				if(a*Math::sqr(scb*(dy-dx))+c<=-scb*(dx+dy))
					return 6;
				}
			}
		else // Areas 7-8
			{
			if(-dx*sa-dy*ca>=0.0f)
				{
				if(a*Math::sqr(dy)+c<=-dx)
					return 7;
				}
			else
				{
				if(a*Math::sqr(scb*(dx+dy))+c<=scb*(dy-dx))
					return 8;
				}
			}
		}
	
	return -1;
	}

Quikwriting::Quikwriting(const char* sName,Container* sParent,bool sManageChild)
	:Widget(sName,sParent,false),
	 buttonDownTime(0),state(Inactive),area(-1),
	 shiftLevel(Lower),shiftLevelLocked(false),
	 targetWidget(0)
	{
	/* Manage me: */
	if(sManageChild)
		manageChild();
	}

Quikwriting::~Quikwriting(void)
	{
	}

Vector Quikwriting::calcNaturalSize(void) const
	{
	GLfloat size=getManager()->getStyleSheet()->fontHeight*11.0f;
	return Vector(size,size,0.0f);
	}

void Quikwriting::resize(const Box& newExterior)
	{
	/* Call the base class method: */
	Widget::resize(newExterior);
	
	/* Re-calculate the widget layout: */
	calcLayout();
	}

void Quikwriting::setBorderWidth(GLfloat newBorderWidth)
	{
	/* Call the base class method: */
	Widget::setBorderWidth(newBorderWidth);
	
	/* Re-calculate the widget layout: */
	calcLayout();
	}

void Quikwriting::setBorderType(Widget::BorderType newBorderType)
	{
	/* Call the base class method: */
	Widget::setBorderType(newBorderType);
	
	/* Re-calculate the widget layout: */
	calcLayout();
	}

void Quikwriting::draw(GLContextData& contextData) const
	{
	/* Call the base class method: */
	Widget::draw(contextData);
	
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	GLLightTracker* lt=contextData.getLightTracker();
	if(lt->isLightingEnabled()&&!lt->isSpecularColorSeparate())
		{
		/* Temporarily turn on separate specular color handling: */
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
		}
	
	/* Draw the border around the Quikwriting texture: */
	glColor(backgroundColor);
	GLfloat x0=getInterior().origin[0];
	GLfloat x1=x0+getInterior().size[0];
	GLfloat y0=getInterior().origin[1];
	GLfloat y1=y0+getInterior().size[1];
	GLfloat z=getInterior().origin[2];
	if(x1-x0>y1-y0)
		{
		GLfloat d=((x1-x0)-(y1-y0))*0.5f;
		x0+=d;
		x1-=d;
		}
	else
		{
		GLfloat d=((y1-y0)-(x1-x0))*0.5f;
		y0+=d;
		y1-=d;
		}
	glBegin(GL_QUAD_STRIP);
	glVertex3f(x0,y0,z);
	glVertex(getInterior().getCorner(0));
	glVertex3f(x1,y0,z);
	glVertex(getInterior().getCorner(1));
	glVertex3f(x1,y1,z);
	glVertex(getInterior().getCorner(3));
	glVertex3f(x0,y1,z);
	glVertex(getInterior().getCorner(2));
	glVertex3f(x0,y0,z);
	glVertex(getInterior().getCorner(0));
	glEnd();
	
	/* Bind the appropriate Quikwriting texture: */
	glBindTexture(GL_TEXTURE_2D,dataItem->textures[shiftLevel-Lower]);
	
	/* Draw the widget's interior: */
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,lt->isLightingEnabled()?GL_MODULATE:GL_REPLACE);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f,0.0f);
	glVertex3f(x0,y0,z);
	glTexCoord2f(1.0f,0.0f);
	glVertex3f(x1,y0,z);
	glTexCoord2f(1.0f,1.0f);
	glVertex3f(x1,y1,z);
	glTexCoord2f(0.0f,1.0f);
	glVertex3f(x0,y1,z);
	glEnd();
	
	/* Protect the texture: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Reset OpenGL state: */
	if(lt->isLightingEnabled()&&!lt->isSpecularColorSeparate())
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
	glPopAttrib();
	}

bool Quikwriting::findRecipient(Event& event)
	{
	/* Find the event's point in our coordinate system: */
	Event::WidgetPoint wp=event.calcWidgetPoint(this);
	
	/* If the point is inside our bounding box, put us down as recipient; otherwise, delegate to a potential target widget: */
	if(isInside(wp.getPoint()))
		return event.setTargetWidget(this,wp);
	else if(targetWidget!=0)
		{
		/* Check if the target widget wants the event: */
		if(targetWidget->findRecipient(event))
			{
			/* Let the target widget have the event: */
			return true;
			}
		else
			{
			/* Don't set a recipient and let the event method sort it out: */
			return false;
			}
		}
	else
		return false;
	}

void Quikwriting::pointerButtonDown(Event& event)
	{
	/* Remember the real target of this event: */
	motionTarget=event.getTargetWidget();
	
	/* Distribute the event to the intended target: */
	if(motionTarget==targetWidget)
		targetWidget->pointerButtonDown(event);
	else if(motionTarget==this)
		{
		if(state==Inactive)
			{
			/* Start a gesture sequence: */
			buttonDownTime=getManager()->getTime();
			area=findArea(event);
			if(area<=0)
				state=Start;
			else
				{
				state=Outer1;
				outer2=outer1=area;
				}
			}
		else
			{
			/* Finish the gesture sequence: */
			state=Inactive;
			shiftLevel=Lower;
			shiftLevelLocked=false;
			}
		}
	}

void Quikwriting::pointerButtonUp(Event& event)
	{
	/* Distribute the event to the intended target: */
	if(motionTarget==targetWidget)
		targetWidget->pointerButtonUp(event);
	else if(motionTarget==this)
		{
		/* Check if this was an actual gesture sequence and not a click in the central area: */
		if(state!=Start||getManager()->getTime()-buttonDownTime>=getManager()->getStyleSheet()->multiClickTime)
			{
			/* Finish the gesture sequence: */
			state=Inactive;
			shiftLevel=Lower;
			shiftLevelLocked=false;
			}
		}
	else
		{
		/* This was an outside click; finish text entry: */
		WidgetManager* manager=getManager();
		manager->releasePointer(this);
		manager->requestFocus(0);
		}
	}

void Quikwriting::pointerMotion(Event& event)
	{
	/* Distribute the event to the intended target: */
	if(motionTarget==targetWidget)
		targetWidget->pointerMotion(event);
	else if(motionTarget==this&&event.getTargetWidget()==this)
		{
		/* Continue the current gesture: */
		area=findArea(event);
		switch(state)
			{
			case Start:
				if(area>0)
					{
					/* Start a new stroke: */
					state=Outer1;
					outer2=outer1=area;
					}
				break;
			
			case Outer1:
				if(area>0)
					outer2=area;
				else if(area==0)
					{
					/* Finish the current stroke: */
					int c=characterTable[shiftLevel-Lower][outer1-1][outer2-1];
					bool unshift=!shiftLevelLocked;
					if(c<SpecialsEnd)
						{
						switch(c)
							{
							case Backspace:
								getManager()->textControl(TextControlEvent(TextControlEvent::BACKSPACE));
								break;
							
							case Enter:
								if(targetWidget!=0)
									getManager()->releasePointer(this);
								getManager()->textControl(TextControlEvent(TextControlEvent::CONFIRM));
								break;
							
							case Upper:
							case Numerical:
							case Symbols:
								if(shiftLevel!=c)
									{
									shiftLevel=c;
									shiftLevelLocked=false;
									}
								else if(shiftLevelLocked)
									{
									shiftLevel=Lower;
									shiftLevelLocked=false;
									}
								else
									shiftLevelLocked=true;
								unshift=false;
								break;
							
							default:
								; // Just to make compiler happy
							}
						}
					else
						{
						char string[2];
						string[0]=char(c);
						string[1]='\0';
						getManager()->text(TextEvent(string));
						}
					
					/* Prepare for the next stroke: */
					state=Start;
					if(unshift)
						shiftLevel=Lower;
					}
				break;
			
			default:
				; // Just to make compiler happy
			}
		}
	}

void Quikwriting::initContext(GLContextData& contextData) const
	{
	/* Create a context data item: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Check if automatic mipmap generation is supported: */
	bool canMipmap=GLEXTFramebufferObject::isSupported();
	if(canMipmap)
		GLEXTFramebufferObject::initExtension();
	
	/* Load the Quikwriting textures: */
	IO::DirectoryPtr textureDirectory=IO::openDirectory(GLMOTIF_CONFIG_SHAREDIR);
	for(int i=0;i<4;++i)
		{
		/* Set up the texture object: */
		glBindTexture(GL_TEXTURE_2D,dataItem->textures[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,canMipmap?9:0);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,canMipmap?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		
		/* Load the image: */
		char imageFileName[1024];
		snprintf(imageFileName,sizeof(imageFileName),"Textures/Quikwriting-%d.png",i);
		Images::BaseImage image=Images::readGenericImageFile(*textureDirectory,imageFileName);
		image.glTexImage2D(GL_TEXTURE_2D,0);
		
		if(canMipmap)
			glGenerateMipmapEXT(GL_TEXTURE_2D);
		}
	
	glBindTexture(GL_TEXTURE_2D,0);
	}

void Quikwriting::setShiftLevel(int newShiftLevel,bool newShiftLevelLocked)
	{
	shiftLevel=Lower+newShiftLevel;
	shiftLevelLocked=newShiftLevelLocked;
	}

void Quikwriting::setTargetWidget(Widget* newTargetWidget)
	{
	/* Set the target widget: */
	targetWidget=newTargetWidget;
	
	/* If there is a target widget, grab the pointer: */
	if(targetWidget!=0)
		getManager()->grabPointer(this);
	
	/* Reset gesture state: */
	state=Inactive;
	}

}
