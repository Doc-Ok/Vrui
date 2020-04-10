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

#include <Vrui/Vislets/ImageViewer.h>

#include <Misc/MessageLogger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Margin.h>
#include <GLMotif/Button.h>
#include <GLMotif/ScrolledImage.h>
#include <Vrui/Vrui.h>
#include <Vrui/VisletManager.h>

namespace Vrui {

namespace Vislets {

/***********************************
Methods of class ImageViewerFactory:
***********************************/

ImageViewerFactory::ImageViewerFactory(VisletManager& visletManager)
	:VisletFactory("ImageViewer",visletManager),
	 minWindowSize(getDisplaySize()/Scalar(4))
	{
	#if 0
	/* Insert class into class hierarchy: */
	VisletFactory* visletFactory=visletManager.loadClass("Vislet");
	visletFactory->addChildClass(this);
	addParentClass(visletFactory);
	#endif
	
	/* Load class settings: */
	Misc::ConfigurationFileSection cfs=visletManager.getVisletClassSection(getClassName());
	
	/* Read the minimum window size: */
	minWindowSize=cfs.retrieveValue<Scalar>("./minWindowSize",minWindowSize);
	
	/* Set vislet class' factory pointer: */
	ImageViewer::factory=this;
	}

ImageViewerFactory::~ImageViewerFactory(void)
	{
	/* Reset vislet class' factory pointer: */
	ImageViewer::factory=0;
	}

Vislet* ImageViewerFactory::createVislet(int numArguments,const char* const arguments[]) const
	{
	return new ImageViewer(numArguments,arguments);
	}

void ImageViewerFactory::destroyVislet(Vislet* vislet) const
	{
	delete vislet;
	}

extern "C" void resolveImageViewerDependencies(Plugins::FactoryManager<VisletFactory>& manager)
	{
	#if 0
	/* Load base classes: */
	manager.loadClass("Vislet");
	#endif
	}

extern "C" VisletFactory* createImageViewerFactory(Plugins::FactoryManager<VisletFactory>& manager)
	{
	/* Get pointer to vislet manager: */
	VisletManager* visletManager=static_cast<VisletManager*>(&manager);
	
	/* Create factory object and insert it into class hierarchy: */
	ImageViewerFactory* factory=new ImageViewerFactory(*visletManager);
	
	/* Return factory object: */
	return factory;
	}

extern "C" void destroyImageViewerFactory(VisletFactory* factory)
	{
	delete factory;
	}

/************************************
Static elements of class ImageViewer:
************************************/

ImageViewerFactory* ImageViewer::factory=0;

/****************************
Methods of class ImageViewer:
****************************/

void ImageViewer::zoomInCallback(Misc::CallbackData* cbData)
	{
	/* Increase the image's zoom factor: */
	GLfloat newZoomFactor=imageViewer->getZoomFactor()*1.25f;
	if(newZoomFactor>1000.0f)
		newZoomFactor=1000.0f;
	imageViewer->setZoomFactor(newZoomFactor);
	
	/* Set the zoom factor display: */
	zoomFactor->setValue(newZoomFactor);
	
	/* Enable/disable the zoom in/out buttons: */
	zoomInButton->setEnabled(newZoomFactor<1000.0f);
	zoomOutButton->setEnabled(newZoomFactor>1.0f);
	}

void ImageViewer::zoomFactorCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the image's zoom factor: */
	imageViewer->setZoomFactor(cbData->value);
	
	/* Enable/disable the zoom in/out buttons: */
	zoomInButton->setEnabled(cbData->value<1000.0f);
	zoomOutButton->setEnabled(cbData->value>1.0f);
	}

void ImageViewer::zoomOutCallback(Misc::CallbackData* cbData)
	{
	/* Decrease the image's zoom factor: */
	GLfloat newZoomFactor=imageViewer->getZoomFactor()/1.25f;
	if(newZoomFactor<1.0f)
		newZoomFactor=1.0f;
	imageViewer->setZoomFactor(newZoomFactor);
	
	/* Set the zoom factor display: */
	zoomFactor->setValue(newZoomFactor);
	
	/* Enable/disable the zoom in/out buttons: */
	zoomInButton->setEnabled(newZoomFactor<1000.0f);
	zoomOutButton->setEnabled(newZoomFactor>1.0f);
	}

ImageViewer::ImageViewer(int numArguments,const char* const arguments[])
	:imageDialog(0)
	{
	if(numArguments<1)
		{
		Misc::userError("Vrui::ImageViewer: No image file name provided");
		return;
		}
	
	try
		{
		/* Load an image: */
		Images::BaseImage image=Images::readGenericImageFile(arguments[0]);
		
		/* Calculate a resolution for the image to fill the configured minimum window size: */
		GLfloat imageResolution[2];
		imageResolution[1]=imageResolution[0]=GLfloat(Math::max(Vrui::Scalar(image.getWidth()),Vrui::Scalar(image.getHeight()))/factory->minWindowSize);
		
		const GLMotif::StyleSheet& ss=*getUiStyleSheet();
		
		/* Create the image viewer dialog: */
		imageDialog=new GLMotif::PopupWindow("ImageDialog",getWidgetManager(),"Image Viewer");
		imageDialog->setHideButton(true);
		imageDialog->setCloseButton(false);
		imageDialog->setResizableFlags(true,true);
		
		GLMotif::RowColumn* imagePanel=new GLMotif::RowColumn("ImagePanel",imageDialog,false);
		imagePanel->setOrientation(GLMotif::RowColumn::VERTICAL);
		imagePanel->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		imagePanel->setNumMinorWidgets(1);
		
		imageViewer=new GLMotif::ScrolledImage("ImageViewer",imagePanel,image,imageResolution,false);
		imageViewer->setPreferredSize(GLMotif::Vector(image.getWidth()/imageResolution[0],image.getHeight()/imageResolution[1],0));
		imageViewer->manageChild();
		imageViewer->getImage()->setInterpolationMode(GL_LINEAR_MIPMAP_LINEAR);
		imageViewer->getImage()->setMipmapLevel(10);
		imageViewer->setDragScrolling(true);
		
		GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",imagePanel,false);
		buttonMargin->setAlignment(GLMotif::Alignment::HCENTER);
		
		GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
		buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
		buttonBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		buttonBox->setNumMinorWidgets(1);
		
		zoomOutButton=new GLMotif::Button("ZoomOutButton",buttonBox,"-");
		zoomOutButton->getSelectCallbacks().add(this,&ImageViewer::zoomOutCallback);
		zoomOutButton->setEnabled(false);
		
		zoomFactor=new GLMotif::TextFieldSlider("ZoomFactor",buttonBox,8,ss.fontHeight*10.0f);
		zoomFactor->getTextField()->setFieldWidth(7);
		zoomFactor->getTextField()->setPrecision(3);
		zoomFactor->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
		zoomFactor->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
		zoomFactor->setValueType(GLMotif::TextFieldSlider::FLOAT);
		zoomFactor->setValueRange(1.0,1.0e3,0.01);
		zoomFactor->setValue(1.0);
		zoomFactor->getValueChangedCallbacks().add(this,&ImageViewer::zoomFactorCallback);
		
		zoomInButton=new GLMotif::Button("ZoomInButton",buttonBox,"+");
		zoomInButton->getSelectCallbacks().add(this,&ImageViewer::zoomInCallback);
		
		buttonBox->manageChild();
		
		buttonMargin->manageChild();
		
		imagePanel->setRowWeight(0,1.0f);
		imagePanel->manageChild();
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("Vrui::ImageViewer: Unable to view image %s due to exception %s",arguments[0],err.what());
		}
	}

ImageViewer::~ImageViewer(void)
	{
	/* Destroy the image viewer dialog: */
	delete imageDialog;
	}

VisletFactory* ImageViewer::getFactory(void) const
	{
	return factory;
	}

void ImageViewer::enable(bool startup)
	{
	/* Show the image viewer dialog: */
	popupPrimaryWidget(imageDialog);
	
	/* Enable the vislet: */
	Vislet::enable(startup);
	}

void ImageViewer::disable(bool shutdown)
	{
	/* Hide the image viewer dialog: */
	popdownPrimaryWidget(imageDialog);
	
	/* Disable the vislet: */
	Vislet::disable(shutdown);
	}

}

}
