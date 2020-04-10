/***********************************************************************
Small viewer for movies stored as image sequences.
Copyright (c) 2012-2018 Oliver Kreylos

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

#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <Misc/ThrowStdErr.h>
#include <Misc/PrintfTemplateTests.h>
#include <Threads/MutexCond.h>
#include <Threads/Thread.h>
#include <Threads/TripleBuffer.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <GL/gl.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBTextureNonPowerOfTwo.h>
#include <Images/BaseImage.h>
#include <Images/ReadImageFile.h>
#include <GLMotif/StyleSheet.h>
#include <GLMotif/WidgetManager.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <GLMotif/TextFieldSlider.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>

class ImageSequenceViewer:public Vrui::Application,public GLObject
	{
	/* Embedded classes: */
	private:
	struct DataItem:public GLObject::DataItem
		{
		/* Elements: */
		public:
		GLuint imageTextureId; // ID of image texture object
		bool haveNpotdt; // Flag whether the local OpenGL supports non-power-of-two-dimension textures
		GLfloat texMin[2],texMax[2]; // Texture coordinate rectangle to render the image texture
		unsigned int textureVersion; // Version number of the image currently stored in the texture
		
		/* Constructors and destructors: */
		DataItem(void);
		virtual ~DataItem(void);
		};
	
	/* Elements: */
	IO::DirectoryPtr frameDir; // Directory containing all frame image files
	std::string frameNameTemplate; // The name template for all frame image files
	int firstIndex; // The index of the first frame
	int lastIndex; // The index one past the last frame
	unsigned int frameSize[2]; // Size of all image frames
	double frameTime; // Movie frame interval time in seconds
	Threads::TripleBuffer<Images::BaseImage> images; // Triple buffer of images streaming from the background loader thread
	int currentIndex; // The index of the image frame in the currently locked triple buffer slot
	unsigned int imageVersion; // The version number of the image in the currently locked triple buffer slot
	Threads::MutexCond loadRequestCond; // Condition variable to signal image load requests to the background thread
	int nextImageIndex; // Index of the next image requested by the foreground thread
	Threads::Thread imageLoaderThread; // Background thread to load images during automatic playback
	bool playing; // Flag whether the movie is currently playing
	double frameDueTime; // Time at which the next frame must be displayed during playback
	GLMotif::PopupWindow* playbackDialog; // The playback control dialog
	GLMotif::TextFieldSlider* frameIndexSlider; // Slider to select image frames
	
	/* Private methods: */
	void readImage(int imageIndex); // Reads the frame image of the given index into the next image buffer slot
	void* imageLoaderThreadMethod(void); // Method loading image frames in the background during automatic playback
	GLMotif::PopupWindow* createPlaybackDialog(void); // Creates the playback control dialog
	void playToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData);
	void frameIndexSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	ImageSequenceViewer(int& argc,char**& argv);
	virtual ~ImageSequenceViewer(void);
	
	/* Methods from Vrui::Application: */
	virtual void frame(void);
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	
	/* Methods from GLObject: */
	virtual void initContext(GLContextData& contextData) const;
	};

/**********************************************
Methods of class ImageSequenceViewer::DataItem:
**********************************************/

ImageSequenceViewer::DataItem::DataItem(void)
	:imageTextureId(0),
	 haveNpotdt(GLARBTextureNonPowerOfTwo::isSupported()),
	 textureVersion(0)
	{
	if(haveNpotdt)
		GLARBTextureNonPowerOfTwo::initExtension();
	glGenTextures(1,&imageTextureId);
	}

ImageSequenceViewer::DataItem::~DataItem(void)
	{
	glDeleteTextures(1,&imageTextureId);
	}

/************************************
Methods of class ImageSequenceViewer:
************************************/

void ImageSequenceViewer::readImage(int imageIndex)
	{
	/* Assemble the name of the requested image: */
	char frameName[2048];
	snprintf(frameName,sizeof(frameName),frameNameTemplate.c_str(),imageIndex);
	
	/* Read the image into the next triple buffer slot: */
	Images::BaseImage& image=images.startNewValue();
	image=Images::readGenericImageFile(*frameDir,frameName);
	images.postNewValue();
	}

void* ImageSequenceViewer::imageLoaderThreadMethod(void)
	{
	/* Index of the last image frame loaded: */
	int loadImageIndex=-1;
	while(true)
		{
		/* Wait for a load request: */
		{
		Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
		while(nextImageIndex==loadImageIndex)
			loadRequestCond.wait(loadRequestLock);
		loadImageIndex=nextImageIndex;
		}
		
		/* Load the requested image: */
		readImage(loadImageIndex);
		
		if(!playing)
			{
			/* Wake up the foreground thread: */
			Vrui::requestUpdate();
			}
		}
	
	return 0;
	}

GLMotif::PopupWindow* ImageSequenceViewer::createPlaybackDialog(void)
	{
	/* Create a popup shell to hold the playback control dialog: */
	GLMotif::PopupWindow* playbackDialogPopup=new GLMotif::PopupWindow("PlaybackDialogPopup",Vrui::getWidgetManager(),"Playback Control");
	playbackDialogPopup->setResizableFlags(true,false);
	
	/* Create a rowcolumn to hold the playback controls: */
	GLMotif::RowColumn* playbackDialog=new GLMotif::RowColumn("PlaybackDialog",playbackDialogPopup,false);
	playbackDialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	playbackDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	playbackDialog->setNumMinorWidgets(1);
	
	/* Create the playback toggle: */
	GLMotif::ToggleButton* playToggle=new GLMotif::ToggleButton("PlayToggle",playbackDialog,"Play");
	playToggle->getValueChangedCallbacks().add(this,&ImageSequenceViewer::playToggleCallback);
	
	/* Create the frame index slider: */
	frameIndexSlider=new GLMotif::TextFieldSlider("FrameIndexSlider",playbackDialog,6,Vrui::getWidgetManager()->getStyleSheet()->fontHeight*20.0f);
	frameIndexSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	frameIndexSlider->setValueType(GLMotif::TextFieldSlider::INT);
	frameIndexSlider->setValueRange(double(firstIndex),double(lastIndex-1),1.0);
	frameIndexSlider->setValue(double(firstIndex));
	frameIndexSlider->getValueChangedCallbacks().add(this,&ImageSequenceViewer::frameIndexSliderCallback);
	
	playbackDialog->setColumnWeight(1,1.0f);
	playbackDialog->manageChild();
	
	return playbackDialogPopup;
	}

void ImageSequenceViewer::playToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	if(cbData->set)
		{
		/* Start playback: */
		playing=true;
		
		/* Request the next image: */
		{
		Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
		if(nextImageIndex<lastIndex-1)
			{
			++nextImageIndex;
			loadRequestCond.signal();
			}
		else
			playing=false;
		}
		
		frameDueTime=Vrui::getApplicationTime()+frameTime;
		Vrui::scheduleUpdate(frameDueTime);
		}
	else
		{
		playing=false;
		}
	}

void ImageSequenceViewer::frameIndexSliderCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Request the newly selected image frame: */
	Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
	nextImageIndex=int(Math::floor(cbData->value+0.5));
	loadRequestCond.signal();
	}

ImageSequenceViewer::ImageSequenceViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 firstIndex(0),lastIndex(0),
	 frameTime(1.0/30.0),
	 playing(false),
	 frameDueTime(0.0),currentIndex(-1),imageVersion(0),
	 playbackDialog(0)
	{
	/* Parse the command line: */
	bool autoPlay=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"r")==0)
				{
				++i;
				frameTime=1.0/atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"p")==0)
				autoPlay=true;
			}
		else if(frameNameTemplate.empty())
			frameNameTemplate=argv[i];
		}
	
	/* Check if the frame name template is a valid integer printf template: */
	unsigned int indexStart,indexLength;
	indexLength=indexStart=0;
	if(!Misc::isValidTemplate(frameNameTemplate,'d',2048,&indexStart,&indexLength))
		Misc::throwStdErr("ImageSequenceViewer: Invalid frame name template \"%s\"",frameNameTemplate.c_str());
	
	/* Split the frame name template into directory and file name: */
	std::string::iterator lastSlashIt;
	for(std::string::iterator fntIt=frameNameTemplate.begin();fntIt!=frameNameTemplate.end();++fntIt)
		if(*fntIt=='/')
			lastSlashIt=fntIt;
	std::string frameDirName(frameNameTemplate.begin(),lastSlashIt);
	frameNameTemplate=std::string(lastSlashIt+1,frameNameTemplate.end());
	
	/* Ensure that the index conversion is in the file name, and not in the path: */
	if(indexStart<frameDirName.length()+1)
		Misc::throwStdErr("ImageSequenceViewer: Frame name template \"%s\" has %%d conversion in path name",frameNameTemplate.c_str());
	indexStart-=frameDirName.length()+1;
	
	/* Open the directory containing all frame images: */
	frameDir=IO::openDirectory(frameDirName.c_str());
	
	/* Determine the index range of the frame sequence: */
	firstIndex=Math::Constants<int>::max;
	lastIndex=Math::Constants<int>::min;
	frameDir->rewind();
	while(frameDir->readNextEntry())
		{
		/* Check if the current file is a frame file: */
		const char* cPtr=frameDir->getEntryName();
		if(strncmp(cPtr,frameNameTemplate.c_str(),indexStart)==0)
			{
			/* Extract the current file's index: */
			cPtr+=indexStart;
			int index=0;
			while(*cPtr>='0'&&*cPtr<='9')
				{
				index=index*10+int(*cPtr-'0');
				++cPtr;
				}
			if(strcmp(cPtr,frameNameTemplate.c_str()+indexStart+indexLength)==0)
				{
				/* Update the index range: */
				if(firstIndex>index)
					firstIndex=index;
				if(lastIndex<index+1)
					lastIndex=index+1;
				}
			}
		}
	if(firstIndex>=lastIndex)
		Misc::throwStdErr("No frame images found");
	std::cout<<"Reading frame sequence from index "<<firstIndex<<" to "<<lastIndex-1<<std::endl;
	
	/* Read the first image immediately: */
	readImage(firstIndex);
	images.lockNewValue();
	++imageVersion;
	
	/* Remember the first image's size for the rest of the sequence: */
	frameSize[0]=images.getLockedValue().getSize(0);
	frameSize[1]=images.getLockedValue().getSize(1);
	
	/* Start the image loader thread and request the next image frame: */
	nextImageIndex=firstIndex+1;
	imageLoaderThread.start(this,&ImageSequenceViewer::imageLoaderThreadMethod);
	
	/* Create the user interface: */
	playbackDialog=createPlaybackDialog();
	Vrui::popupPrimaryWidget(playbackDialog);
	
	if(autoPlay)
		{
		/* Start playing from the first frame: */
		playing=true;
		frameDueTime=Vrui::getApplicationTime()+frameTime;
		Vrui::scheduleUpdate(frameDueTime);
		}
	}

ImageSequenceViewer::~ImageSequenceViewer(void)
	{
	/* Stop the image loader thread: */
	imageLoaderThread.cancel();
	imageLoaderThread.join();
	
	delete playbackDialog;
	}

void ImageSequenceViewer::frame(void)
	{
	if(playing)
		{
		/* Check if it's time to show the next image: */
		if(Vrui::getApplicationTime()>=frameDueTime)
			{
			/* Show the pending image: */
			if(images.lockNewValue())
				++imageVersion;
			
			/* Update the frame index slider: */
			frameIndexSlider->setValue(nextImageIndex);
			
			/* Request the next image: */
			{
			Threads::MutexCond::Lock loadRequestLock(loadRequestCond);
			if(nextImageIndex<lastIndex-1)
				{
				++nextImageIndex;
				loadRequestCond.signal();
				}
			else
				playing=false;
			}
			
			frameDueTime+=frameTime;
			}
		
		/* Schedule the next update: */
		Vrui::scheduleUpdate(frameDueTime);
		}
	else
		{
		/* Check if a new image has been loaded: */
		if(images.lockNewValue())
			{
			/* Show the new image: */
			++imageVersion;
			}
		}
	}

void ImageSequenceViewer::display(GLContextData& contextData) const
	{
	/* Get the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->imageTextureId);
	
	/* Check if the texture object is up-to-date: */
	if(dataItem->textureVersion!=imageVersion)
		{
		/* Upload the new image into the texture: */
		images.getLockedValue().glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,!dataItem->haveNpotdt);
		
		dataItem->textureVersion=imageVersion;
		}
	
	/* Draw the image: */
	glBegin(GL_QUADS);
	glTexCoord2f(dataItem->texMin[0],dataItem->texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(dataItem->texMax[0],dataItem->texMin[1]);
	glVertex2i(frameSize[0],0);
	glTexCoord2f(dataItem->texMax[0],dataItem->texMax[1]);
	glVertex2i(frameSize[0],frameSize[1]);
	glTexCoord2f(dataItem->texMin[0],dataItem->texMax[1]);
	glVertex2i(0,frameSize[1]);
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Draw the image's backside: */
	glDisable(GL_TEXTURE_2D);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.7f,0.7f,0.7f)));
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex2i(0,0);
	glVertex2i(0,frameSize[1]);
	glVertex2i(frameSize[0],frameSize[1]);
	glVertex2i(frameSize[0],0);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void ImageSequenceViewer::resetNavigation(void)
	{
	/* Reset the Vrui navigation transformation: */
	Vrui::Scalar w=Vrui::Scalar(frameSize[0]);
	Vrui::Scalar h=Vrui::Scalar(frameSize[1]);
	Vrui::Point center(Math::div2(w),Math::div2(h),Vrui::Scalar(0.01));
	Vrui::Scalar size=Math::sqrt(Math::sqr(w)+Math::sqr(h));
	Vrui::setNavigationTransformation(center,size,Vrui::Vector(0,1,0));
	}

void ImageSequenceViewer::initContext(GLContextData& contextData) const
	{
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Calculate the texture coordinate rectangle: */
	unsigned int texSize[2];
	if(dataItem->haveNpotdt)
		{
		for(int i=0;i<2;++i)
			texSize[i]=frameSize[i];
		}
	else
		{
		for(int i=0;i<2;++i)
			for(texSize[i]=1U;texSize[i]<frameSize[i];texSize[i]<<=1)
				;
		}
	for(int i=0;i<2;++i)
		{
		dataItem->texMin[i]=0.0f;
		dataItem->texMax[i]=GLfloat(frameSize[i])/GLfloat(texSize[i]);
		}
	
	/* Bind the texture object: */
	glBindTexture(GL_TEXTURE_2D,dataItem->imageTextureId);
	
	/* Initialize basic texture settings: */
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(ImageSequenceViewer)
