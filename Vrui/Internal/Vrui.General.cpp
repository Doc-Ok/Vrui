/***********************************************************************
Environment-independent part of Vrui virtual reality development
toolkit.
Copyright (c) 2000-2020 Oliver Kreylos

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

#define EVILHACK_LOCK_INPUTDEVICE_POS 1
#define EVILHACK_USE_BLINDERS 0

#define DELAY_NAVIGATIONTRANSFORMATION 0
#define RENDERFRAMETIMES 0
#define SAVESHAREDVRUISTATE 0

#include <Vrui/Internal/Vrui.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <Misc/SelfDestructPointer.h>
#include <Misc/ThrowStdErr.h>
#include <Misc/StringPrintf.h>
#include <Misc/FileNameExtensions.h>
#include <Misc/CreateNumberedFileName.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/CompoundValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Misc/Time.h>
#include <Misc/TimerEventScheduler.h>
#include <IO/File.h>
#include <IO/Directory.h>
#include <IO/OpenFile.h>
#include <Cluster/Multiplexer.h>
#include <Cluster/MulticastPipe.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLLightModelTemplates.h>
#include <GL/GLLightTracker.h>
#include <GL/GLClipPlaneTracker.h>
#include <GL/GLContextData.h>
#include <GL/GLFont.h>
#include <GL/GLValueCoders.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Event.h>
#include <GLMotif/Widget.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Container.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Pager.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <GLMotif/CascadeButton.h>
#include <GLMotif/QuikwritingTextEntryMethod.h>
#include <AL/Config.h>
#include <AL/ALContextData.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/Internal/ScreenSaverInhibitor.h>
#if VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS
#include <Vrui/Internal/Linux/ScreenSaverInhibitorDBus.h>
#endif
#include <Vrui/Internal/ScreenProtectorArea.h>
#include <Vrui/Internal/MessageLogger.h>
#include <Vrui/TransparentObject.h>
#include <Vrui/VirtualInputDevice.h>
#include <Vrui/InputGraphManager.h>
#include <Vrui/InputDeviceManager.h>
#include <Vrui/Internal/InputDeviceAdapterMouse.h>
#include <Vrui/Internal/KeyboardTextEntryMethod.h>
#include <Vrui/Internal/MultipipeDispatcher.h>
#include <Vrui/TextEventDispatcher.h>
#include <Vrui/Lightsource.h>
#include <Vrui/LightsourceManager.h>
#include <Vrui/ClipPlaneManager.h>
#include <Vrui/Viewer.h>
#include <Vrui/VRScreen.h>
#include <Vrui/VRWindow.h>
#include <Vrui/WindowProperties.h>
#include <Vrui/Listener.h>
#include <Vrui/MutexMenu.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/GUIInteractor.h>
#include <Vrui/Internal/UIManagerFree.h>
#include <Vrui/Internal/UIManagerPlanar.h>
#include <Vrui/Internal/UIManagerSpherical.h>
#include <Vrui/Tool.h>
#include <Vrui/ToolManager.h>
#include <Vrui/Internal/ToolKillZone.h>
#include <Vrui/VisletManager.h>
#include <Vrui/Internal/InputDeviceDataSaver.h>
#include <Vrui/Internal/ScaleBar.h>

// DEBUGGING
#include <iostream>

#if EVILHACK_LOCK_INPUTDEVICE_POS

Vrui::InputDevice* lockedDevice=0;
Vrui::Vector lockedTranslation;

#endif

namespace Misc {

/***********************************************************************
Helper class to read screen protector device values from a configuration
file:
***********************************************************************/

template <>
class ValueCoder<Vrui::VruiState::ScreenProtectorDevice>
	{
	/* Methods: */
	public:
	static std::string encode(const Vrui::VruiState::ScreenProtectorDevice& value)
		{
		std::string result;
		result.push_back('(');
		result.append(ValueCoder<std::string>::encode(value.inputDevice->getDeviceName()));
		result.push_back(',');
		result.push_back(' ');
		result.append(ValueCoder<Vrui::Point>::encode(value.center));
		result.push_back(',');
		result.push_back(' ');
		result.append(ValueCoder<Vrui::Scalar>::encode(value.radius));
		result.push_back(')');
		return result;
		}
	static Vrui::VruiState::ScreenProtectorDevice decode(const char* start,const char* end,const char** decodeEnd =0)
		{
		try
			{
			Vrui::VruiState::ScreenProtectorDevice result;
			
			/* Check for opening parenthesis: */
			const char* cPtr=start;
			if(cPtr==end||*cPtr!='(')
				throw DecodingError("missing opening parenthesis");
			++cPtr;
			cPtr=skipWhitespace(cPtr,end);
			
			/* Read input device name: */
			std::string inputDeviceName=ValueCoder<std::string>::decode(cPtr,end,&cPtr);
			cPtr=skipWhitespace(cPtr,end);
			result.inputDevice=Vrui::findInputDevice(inputDeviceName.c_str());
			if(result.inputDevice==0)
				Misc::throwStdErr("unknown input device \"%s\"",inputDeviceName.c_str());
			
			cPtr=checkSeparator(',',cPtr,end);
			
			result.center=ValueCoder<Vrui::Point>::decode(cPtr,end,&cPtr);
			cPtr=skipWhitespace(cPtr,end);
			
			cPtr=checkSeparator(',',cPtr,end);
			
			result.radius=ValueCoder<Vrui::Scalar>::decode(cPtr,end,&cPtr);
			cPtr=skipWhitespace(cPtr,end);
			
			if(cPtr==end||*cPtr!=')')
				throw DecodingError("missing closing parenthesis");
			++cPtr;
			
			if(decodeEnd!=0)
				*decodeEnd=cPtr;
			return result;
			}
		catch(const std::runtime_error& err)
			{
			throw DecodingError(std::string("Unable to convert \"")+std::string(start,end)+std::string("\" to ScreenProtectorDevice due to ")+err.what());
			}
		}
	};

}

namespace Vrui {

/************
Global state:
************/

VruiState* vruiState=0;
const char* vruiViewpointFileHeader="Vrui viewpoint file v1.0\n";

#if RENDERFRAMETIMES
const int numFrameTimes=800;
double frameTimes[numFrameTimes];
int frameTimeIndex=-1;
#endif
#if SAVESHAREDVRUISTATE
IO::File* vruiSharedStateFile=0;
#endif

/********************************************************
Methods of class VruiState::DisplayStateMapper::DataItem:
********************************************************/

VruiState::DisplayStateMapper::DataItem::DataItem(void)
	:screenProtectorDisplayListId(0)
	{
	}

VruiState::DisplayStateMapper::DataItem::~DataItem(void)
	{
	/* Delete the screen protector display list (if it was created in the first place): */
	if(screenProtectorDisplayListId!=0)
		glDeleteLists(screenProtectorDisplayListId,1);
	}

/**********************************************
Methods of class VruiState::DisplayStateMapper:
**********************************************/

void VruiState::DisplayStateMapper::initContext(GLContextData& contextData) const
	{
	}

/**********************
Private Vrui functions:
**********************/

GLMotif::PopupMenu* VruiState::buildDialogsMenu(void)
	{
	GLMotif::WidgetManager* wm=getWidgetManager();
	
	/* Create the dialogs submenu: */
	dialogsMenu=new GLMotif::PopupMenu("DialogsMenu",wm);
	
	/* Add menu buttons for all popped-up dialog boxes: */
	poppedDialogs.clear();
	for(GLMotif::WidgetManager::PoppedWidgetIterator wIt=wm->beginPrimaryWidgets();wIt!=wm->endPrimaryWidgets();++wIt)
		{
		GLMotif::PopupWindow* dialog=dynamic_cast<GLMotif::PopupWindow*>(*wIt);
		if(dialog!=0)
			{
			/* Add an entry to the dialogs submenu: */
			GLMotif::Button* button=dialogsMenu->addEntry(dialog->getTitleString());
			
			/* Add a callback to the button: */
			button->getSelectCallbacks().add(this,&VruiState::dialogsMenuCallback,dialog);
			
			/* Save a pointer to the dialog window: */
			poppedDialogs.push_back(dialog);
			}
		}
	
	dialogsMenu->manageMenu();
	return dialogsMenu;
	}

GLMotif::PopupMenu* VruiState::buildAlignViewMenu(void)
	{
	GLMotif::PopupMenu* alignViewMenu=new GLMotif::PopupMenu("AlignViewMenu",getWidgetManager());
	
	GLMotif::Button* alignXYButton=new GLMotif::Button("AlignXYButton",alignViewMenu,"X - Y");
	alignXYButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* alignXZButton=new GLMotif::Button("AlignXZButton",alignViewMenu,"X - Z");
	alignXZButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* alignYZButton=new GLMotif::Button("AlignYZButton",alignViewMenu,"Y - Z");
	alignYZButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	alignViewMenu->addSeparator();
	
	GLMotif::Button* alignXUpDownButton=new GLMotif::Button("AlignXUpDownButton",alignViewMenu,"X Up/Down");
	alignXUpDownButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* alignYUpDownButton=new GLMotif::Button("AlignYUpDownButton",alignViewMenu,"Y Up/Down");
	alignYUpDownButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* alignZUpDownButton=new GLMotif::Button("AlignZUpDownButton",alignViewMenu,"Z Up/Down");
	alignZUpDownButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	alignViewMenu->addSeparator();
	
	GLMotif::Button* flipHButton=new GLMotif::Button("FlipHButton",alignViewMenu,"Flip H");
	flipHButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* flipVButton=new GLMotif::Button("FlipVButton",alignViewMenu,"Flip V");
	flipVButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	alignViewMenu->addSeparator();
	
	GLMotif::Button* rotateCCWButton=new GLMotif::Button("RotateCCWButton",alignViewMenu,"Rotate CCW");
	rotateCCWButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	GLMotif::Button* rotateCWButton=new GLMotif::Button("RotateCWButton",alignViewMenu,"Rotate CW");
	rotateCWButton->getSelectCallbacks().add(this,&VruiState::alignViewCallback);
	
	alignViewMenu->manageMenu();
	
	return alignViewMenu;
	}

GLMotif::PopupMenu* VruiState::buildViewMenu(void)
	{
	GLMotif::PopupMenu* viewMenu=new GLMotif::PopupMenu("ViewMenu",getWidgetManager());
	
	GLMotif::Button* resetViewButton=new GLMotif::Button("ResetViewButton",viewMenu,"Reset View");
	resetViewButton->getSelectCallbacks().add(this,&VruiState::resetViewCallback);
	
	/* Create the align view submenu: */
	GLMotif::CascadeButton* alignViewMenuCascade=new GLMotif::CascadeButton("AlignViewMenuCascade",viewMenu,"Align View");
	alignViewMenuCascade->setPopup(buildAlignViewMenu());
	
	viewMenu->addSeparator();
	
	undoViewButton=new GLMotif::Button("UndoViewButton",viewMenu,"Undo View");
	undoViewButton->getSelectCallbacks().add(this,&VruiState::undoViewCallback);
	undoViewButton->setEnabled(false);
	
	redoViewButton=new GLMotif::Button("RedoViewButton",viewMenu,"Redo View");
	redoViewButton->getSelectCallbacks().add(this,&VruiState::redoViewCallback);
	redoViewButton->setEnabled(false);
	
	viewMenu->addSeparator();
	
	GLMotif::Button* loadViewButton=new GLMotif::Button("LoadViewButton",viewMenu,"Load View...");
	viewSelectionHelper.addLoadCallback(loadViewButton,this,&VruiState::loadViewCallback);
	
	GLMotif::Button* saveViewButton=new GLMotif::Button("LoadViewButton",viewMenu,"Save View...");
	viewSelectionHelper.addSaveCallback(saveViewButton,this,&VruiState::saveViewCallback);
	
	viewMenu->manageMenu();
	
	return viewMenu;
	}

GLMotif::PopupMenu* VruiState::buildDevicesMenu(void)
	{
	GLMotif::PopupMenu* devicesMenu=new GLMotif::PopupMenu("DevicesMenu",getWidgetManager());
	
	/* Create buttons to create or destroy virtual input device: */
	GLMotif::Button* createOneButtonDeviceButton=new GLMotif::Button("CreateOneButtonDeviceButton",devicesMenu,"Create One-Button Device");
	createOneButtonDeviceButton->getSelectCallbacks().add(this,&VruiState::createInputDeviceCallback,1);

	GLMotif::Button* createTwoButtonDeviceButton=new GLMotif::Button("CreateTwoButtonDeviceButton",devicesMenu,"Create Two-Button Device");
	createTwoButtonDeviceButton->getSelectCallbacks().add(this,&VruiState::createInputDeviceCallback,2);
	
	devicesMenu->addSeparator();
	
	GLMotif::Button* destroyDeviceButton=new GLMotif::Button("DestroyDeviceButton",devicesMenu,"Destroy Oldest Device");
	destroyDeviceButton->getSelectCallbacks().add(this,&VruiState::destroyInputDeviceCallback);
	
	devicesMenu->addSeparator();
	
	GLMotif::Button* loadInputGraphButton=new GLMotif::Button("LoadInputGraphButton",devicesMenu,"Load Input Graph...");
	inputGraphSelectionHelper.addLoadCallback(loadInputGraphButton,this,&VruiState::loadInputGraphCallback);
	
	GLMotif::Button* saveInputGraphButton=new GLMotif::Button("SaveInputGraphButton",devicesMenu,"Save Input Graph...");
	inputGraphSelectionHelper.addSaveCallback(saveInputGraphButton,this,&VruiState::saveInputGraphCallback);
	
	devicesMenu->addSeparator();
	
	GLMotif::ToggleButton* toolKillZoneActiveToggle=new GLMotif::ToggleButton("ToolKillZoneActiveToggle",devicesMenu,"Tool Kill Zone Active");
	toolKillZoneActiveToggle->setToggle(getToolManager()->getToolKillZone()->isActive());
	toolKillZoneActiveToggle->getValueChangedCallbacks().add(this,&VruiState::toolKillZoneActiveCallback);
	
	GLMotif::ToggleButton* showToolKillZoneToggle=new GLMotif::ToggleButton("ShowToolKillZoneToggle",devicesMenu,"Show Tool Kill Zone");
	showToolKillZoneToggle->setToggle(getToolManager()->getToolKillZone()->getRender());
	showToolKillZoneToggle->getValueChangedCallbacks().add(this,&VruiState::showToolKillZoneCallback);
	
	if(protectScreens)
		{
		GLMotif::ToggleButton* protectScreensToggle=new GLMotif::ToggleButton("ProtectScreensToggle",devicesMenu,"Protect Screens");
		protectScreensToggle->setToggle(true);
		protectScreensToggle->getValueChangedCallbacks().add(this,&VruiState::protectScreensCallback);
		
		GLMotif::ToggleButton* alwaysProtectScreensToggle=new GLMotif::ToggleButton("AlwaysProtectScreensToggle",devicesMenu,"Show Protection Grids");
		alwaysProtectScreensToggle->track(alwaysRenderProtection);
		}
	
	devicesMenu->manageMenu();
	return devicesMenu;
	}

void VruiState::buildSystemMenu(GLMotif::Container* parent)
	{
	/* Create the dialogs submenu: */
	dialogsMenuCascade=new GLMotif::CascadeButton("DialogsMenuCascade",parent,"Dialogs");
	dialogsMenuCascade->setPopup(buildDialogsMenu());
	if(dialogsMenu->getNumEntries()==0)
		dialogsMenuCascade->setEnabled(false);
	
	/* Create the view submenu: */
	GLMotif::CascadeButton* viewMenuCascade=new GLMotif::CascadeButton("ViewMenuCascade",parent,"View");
	viewMenuCascade->setPopup(buildViewMenu());
	
	/* Create the devices submenu: */
	GLMotif::CascadeButton* devicesMenuCascade=new GLMotif::CascadeButton("DevicesMenuCascade",parent,"Devices");
	devicesMenuCascade->setPopup(buildDevicesMenu());
	
	if(visletManager->getNumVislets()>0)
		{
		/* Create the vislet submenu: */
		GLMotif::CascadeButton* visletMenuCascade=new GLMotif::CascadeButton("VisletMenuCascade",parent,"Vislets");
		visletMenuCascade->setPopup(visletManager->buildVisletMenu());
		}
	
	/* Create a button to show the scale bar: */
	GLMotif::ToggleButton* showScaleBarToggle=new GLMotif::ToggleButton("ShowScaleBarToggle",parent,"Show Scale Bar");
	showScaleBarToggle->getValueChangedCallbacks().add(this,&VruiState::showScaleBarToggleCallback);
	
	/* Create a button to show the settings dialog: */
	GLMotif::Button* showSettingsDialogButton=new GLMotif::Button("ShowSettingsDialogButton",parent,"Show Vrui Settings");
	showSettingsDialogButton->getSelectCallbacks().add(this,&VruiState::showSettingsDialogCallback);
	
	quitSeparator=new GLMotif::Separator("QuitSeparator",parent,GLMotif::Separator::HORIZONTAL,0.0f,GLMotif::Separator::LOWERED);
	
	/* Create a button to quit the current application: */
	GLMotif::Button* quitButton=new GLMotif::Button("QuitButton",parent,"Quit Program");
	quitButton->getSelectCallbacks().add(this,&VruiState::quitCallback);
	}

void VruiState::pushNavigationTransformation(void)
	{
	/* Check if the navigation transformation is different from the current undo buffer slot: */
	if(navigationUndoCurrent!=navigationUndoBuffer.end()&&*navigationUndoCurrent!=navigationTransformation)
		{
		/* Discard all stored navigation transformations after the current: */
		++navigationUndoCurrent;
		while(navigationUndoBuffer.end()!=navigationUndoCurrent)
			navigationUndoBuffer.pop_back();
		
		/* Make room if the undo buffer is full: */
		if(navigationUndoBuffer.full())
			navigationUndoBuffer.pop_front();
		
		/* Push the new navigation transformation: */
		navigationUndoBuffer.push_back(navigationTransformation);
		
		/* Enable the undo button; disable the redo button: */
		undoViewButton->setEnabled(true);
		redoViewButton->setEnabled(false);
		}
	}

void VruiState::updateNavigationTransformation(const NavTransform& newTransform)
	{
	/* Calculate the new inverse transformation: */
	NavTransform newInverseTransform=Geometry::invert(newTransform);
	
	/* Call all navigation changed callbacks: */
	NavigationTransformationChangedCallbackData cbData(navigationTransformation,inverseNavigationTransformation,newTransform,newInverseTransform);
	navigationTransformationChangedCallbacks.call(&cbData);
	
	/* Set the navigation transformation: */
	navigationTransformation=newTransform;
	inverseNavigationTransformation=newInverseTransform;
	
	/* Push the new navigation transformation into the navigation undo buffer if there is no active navigation tool: */
	if(activeNavigationTool==0)
		pushNavigationTransformation();
	}

void VruiState::loadViewpointFile(IO::Directory& directory,const char* viewpointFileName)
	{
	/* Open the viewpoint file: */
	IO::FilePtr viewpointFile=directory.openFile(viewpointFileName);
	viewpointFile->setEndianness(Misc::LittleEndian);
	
	/* Check the header: */
	char header[80];
	viewpointFile->read(header,strlen(vruiViewpointFileHeader));
	header[strlen(vruiViewpointFileHeader)]='\0';
	if(strcmp(header,vruiViewpointFileHeader)==0)
		{
		/* Read the environment's center point in navigational coordinates: */
		Point center;
		viewpointFile->read<Scalar>(center.getComponents(),3);
		
		/* Read the environment's size in navigational coordinates: */
		Scalar size=viewpointFile->read<Scalar>();
		
		/* Read the environment's forward direction in navigational coordinates: */
		Vector forward;
		viewpointFile->read<Scalar>(forward.getComponents(),3);
		
		/* Read the environment's up direction in navigational coordinates: */
		Vector up;
		viewpointFile->read<Scalar>(up.getComponents(),3);
		
		/* Construct the navigation transformation: */
		NavTransform nav=NavTransform::identity;
		nav*=NavTransform::translateFromOriginTo(getDisplayCenter());
		nav*=NavTransform::rotate(Rotation::fromBaseVectors(getForwardDirection()^getUpDirection(),getForwardDirection()));
		nav*=NavTransform::scale(getDisplaySize()/size);
		nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(forward^up,forward)));
		nav*=NavTransform::translateToOriginFrom(center);
		setNavigationTransformation(nav);
		}
	else
		Misc::throwStdErr("File %s is not a Vrui viewpoint file",viewpointFileName);
	}

VruiState::VruiState(Cluster::Multiplexer* sMultiplexer,Cluster::MulticastPipe* sPipe)
	:screenSaverInhibitor(0),
	 multiplexer(sMultiplexer),
	 master(multiplexer==0||multiplexer->isMaster()),
	 pipe(sPipe),
	 randomSeed(0),
	 inchScale(1.0),
	 meterScale(1000.0/25.4),
	 displayCenter(0.0,0.0,0.0),displaySize(1.0),
	 forwardDirection(0.0,1.0,0.0),
	 upDirection(0.0,0.0,1.0),
	 floorPlane(Vector(0.0,0.0,1.0),0.0),
	 glyphRenderer(0),
	 newInputDevicePosition(0.0,0.0,0.0),
	 virtualInputDevice(0),
	 inputGraphManager(0),
	 inputGraphSelectionHelper(0,"SavedInputGraph.inputgraph",".inputgraph",0),
	 loadInputGraph(false),
	 textEventDispatcher(0),
	 inputDeviceManager(0),
	 inputDeviceDataSaver(0),
	 multipipeDispatcher(0),
	 lightsourceManager(0),sunLightsource(0),sunAzimuth(0.0f),sunElevation(60.0f),sunIntensity(1.0f),
	 clipPlaneManager(0),
	 numViewers(0),viewers(0),mainViewer(0),
	 numScreens(0),screens(0),mainScreen(0),
	 numProtectorAreas(0),protectorAreas(0),numProtectorDevices(0),protectorDevices(0),
	 protectScreens(false),alwaysRenderProtection(false),renderProtection(0),protectorGridColor(0.0f,1.0f,0.0f),protectorGridSpacing(12),
	 numHapticDevices(0),hapticDevices(0),
	 numListeners(0),listeners(0),mainListener(0),
	 frontplaneDist(1.0),
	 backplaneDist(1000.0),
	 backgroundColor(Color(0.0f,0.0f,0.0f,1.0f)),
	 foregroundColor(Color(1.0f,1.0f,1.0f,1.0f)),
	 ambientLightColor(Color(0.2f,0.2f,0.2f)),
	 useSound(false),
	 widgetMaterial(GLMaterial::Color(1.0f,1.0f,1.0f),GLMaterial::Color(0.5f,0.5f,0.5f),25.0f),
	 timerEventScheduler(0),
	 widgetManager(0),uiManager(0),
	 dialogsMenu(0),
	 systemMenu(0),systemMenuTopLevel(false),dialogsMenuCascade(0),
	 mainMenu(0),
	 viewSelectionHelper(0,"SavedViewpoint.view",".view",0),
	 settingsDialog(0),settingsPager(0),
	 userMessagesToConsole(false),
	 navigationTransformationEnabled(false),
	 delayNavigationTransformation(false),
	 navigationTransformationChangedMask(0x0),
	 navigationTransformation(NavTransform::identity),inverseNavigationTransformation(NavTransform::identity),
	 navigationUndoBuffer(32), // Ought to be more than enough
	 navigationUndoCurrent(navigationUndoBuffer.begin()),
	 coordinateManager(0),scaleBar(0),
	 toolManager(0),
	 visletManager(0),
	 prepareMainLoopFunction(0),prepareMainLoopFunctionData(0),
	 frameFunction(0),frameFunctionData(0),
	 displayFunction(0),displayFunctionData(0),
	 soundFunction(0),soundFunctionData(0),
	 resetNavigationFunction(0),resetNavigationFunctionData(0),
	 finishMainLoopFunction(0),finishMainLoopFunctionData(0),
	 minimumFrameTime(0.0),lastFrame(0.0),nextFrameTime(0.0),
	 synchFrameTime(0.0),synchWait(false),
	 numRecentFrameTimes(0),recentFrameTimes(0),nextFrameTimeIndex(0),sortedFrameTimes(0),
	 animationFrameInterval(1.0/125.0),
	 activeNavigationTool(0),
	 updateContinuously(false),
	 predictVsync(false),vsyncInterval(0,0),numVsyncs(0),nextVsync(0,0),postVsyncDisplayDelay(0.0)
	{
	#if SAVESHAREDVRUISTATE
	vruiSharedStateFile=IO::openFile("/tmp/VruiSharedState.dat",IO::File::WriteOnly);
	vruiSharedStateFile->setEndianness(IO::File::LittleEndian);
	#endif
	}

VruiState::~VruiState(void)
	{
	#if SAVESHAREDVRUISTATE
	delete vruiSharedStateFile;
	#endif
	
	/* Delete time management: */
	delete[] recentFrameTimes;
	delete[] sortedFrameTimes;
	
	/* Deregister the popup callback: */
	widgetManager->getWidgetPopCallbacks().remove(this,&VruiState::widgetPopCallback);
	
	/* Delete an optional input device data saver: */
	delete inputDeviceDataSaver;
	
	/* Destroy the input graph: */
	inputGraphManager->clear();
	
	/* Delete tool management: */
	delete toolManager;
	
	/* Delete vislet management: */
	delete visletManager;
	
	/* Delete coordinate manager: */
	delete scaleBar;
	delete coordinateManager;
	
	/* Delete widget management: */
	if(systemMenuTopLevel)
		delete systemMenu;
	delete mainMenu;
	delete settingsDialog;
	viewSelectionHelper.closeDialogs();
	inputGraphSelectionHelper.closeDialogs();
	delete uiStyleSheet.font;
	delete widgetManager;
	delete timerEventScheduler;
	
	/* Delete listeners: */
	delete[] listeners;
	
	/* Delete screen protection management: */
	delete[] protectorAreas;
	delete[] protectorDevices;
	
	/* Delete kill zone tracking of haptic input devices: */
	delete[] hapticDevices;
	
	/* Delete screen management: */
	delete[] screens;
	
	/* Delete viewer management: */
	delete[] viewers;
	
	/* Delete clipping plane management: */
	delete clipPlaneManager;
	
	/* Delete light source management: */
	delete lightsourceManager;
	
	/* Delete input device management: */
	delete multipipeDispatcher;
	delete inputDeviceManager;
	delete textEventDispatcher;
	
	/* Delete input graph management: */
	delete inputGraphManager;
	delete virtualInputDevice;
	
	/* Delete glyph management: */
	delete glyphRenderer;
	
	/* Uninhibit the screen saver: */
	delete screenSaverInhibitor;
	
	/* Reset the current directory: */
	IO::Directory::setCurrent(0);
	}

void VruiState::initialize(const Misc::ConfigurationFileSection& configFileSection)
	{
	typedef std::vector<std::string> StringList;
	
	/* Install pipe command callbacks: */
	commandDispatcher.addCommandCallback("showMessage",&VruiState::showMessageCommandCallback,this,"<message text>","Shows a text message to the user");
	commandDispatcher.addCommandCallback("resetView",&VruiState::resetViewCommandCallback,this,0,"Resets the view");
	commandDispatcher.addCommandCallback("loadView",&VruiState::loadViewCommandCallback,this,"<viewpoint file name>","Loads a viewpoint file");
	commandDispatcher.addCommandCallback("loadInputGraph",&VruiState::loadInputGraphCommandCallback,this,"<input graph file name>","Loads an input graph file");
	commandDispatcher.addCommandCallback("saveScreenshot",&VruiState::saveScreenshotCommandCallback,this,"<screenshot file name> [<window index>]","Saves a screenshot from the window of the given index to an image file of the given name");
	commandDispatcher.addCommandCallback("quit",&VruiState::quitCommandCallback,this,0,"Exits from the application");
	
	/* Check whether the screen saver should be inhibited: */
	if(configFileSection.retrieveValue<bool>("./inhibitScreenSaver",false))
		inhibitScreenSaver();
	
	if(multiplexer!=0)
		{
		/* Set the multiplexer's timeout values: */
		multiplexer->setConnectionWaitTimeout(configFileSection.retrieveValue<double>("./multipipeConnectionWaitTimeout",0.1));
		multiplexer->setPingTimeout(configFileSection.retrieveValue<double>("./multipipePingTimeout",10.0),configFileSection.retrieveValue<int>("./multipipePingRetries",3));
		multiplexer->setReceiveWaitTimeout(configFileSection.retrieveValue<double>("./multipipeReceiveWaitTimeout",0.01));
		multiplexer->setBarrierWaitTimeout(configFileSection.retrieveValue<double>("./multipipeBarrierWaitTimeout",0.01));
		}
	
	/* Create a Vrui-specific message logger: */
	Misc::MessageLogger::setMessageLogger(new Vrui::MessageLogger);
	
	/* Set the current directory of the IO sub-library: */
	IO::Directory::setCurrent(IO::openDirectory("."));
	
	/* Initialize random number and time management: */
	if(master)
		{
		randomSeed=(unsigned int)time(0);
		lastFrame=appTime.peekTime();
		}
	
	/* Read the conversion factors from Vrui physical coordinate units to inches and meters: */
	inchScale=configFileSection.retrieveValue<Scalar>("./inchScale",inchScale);
	Scalar readMeterScale=configFileSection.retrieveValue<Scalar>("./meterScale",Scalar(0));
	if(readMeterScale>Scalar(0))
		{
		/* Update meter scale, and calculate inch scale: */
		meterScale=readMeterScale;
		inchScale=meterScale*0.0254;
		}
	else
		{
		/* Calculate meter scale: */
		meterScale=inchScale/0.0254;
		}
	
	/* Initialize environment dimensions: */
	displayCenter=configFileSection.retrieveValue<Point>("./displayCenter");
	displaySize=configFileSection.retrieveValue<Scalar>("./displaySize");
	forwardDirection=configFileSection.retrieveValue<Vector>("./forwardDirection",forwardDirection);
	forwardDirection.normalize();
	upDirection=configFileSection.retrieveValue<Vector>("./upDirection",upDirection);
	upDirection.normalize();
	floorPlane=configFileSection.retrieveValue<Plane>("./floorPlane",floorPlane);
	floorPlane.normalize();
	
	/* Create Vrui's default widget style sheet: */
	GLFont* font=loadFont(configFileSection.retrieveString("./uiFontName","CenturySchoolbookBoldItalic").c_str());
	font->setTextHeight(configFileSection.retrieveValue<double>("./uiFontTextHeight",1.0*inchScale));
	font->setAntialiasing(configFileSection.retrieveValue<bool>("./uiFontAntialiasing",true));
	uiStyleSheet.setFont(font);
	uiStyleSheet.setSize(configFileSection.retrieveValue<float>("./uiSize",uiStyleSheet.size));
	uiStyleSheet.borderColor=uiStyleSheet.bgColor=configFileSection.retrieveValue<Color>("./uiBgColor",uiStyleSheet.bgColor);
	uiStyleSheet.fgColor=configFileSection.retrieveValue<Color>("./uiFgColor",uiStyleSheet.fgColor);
	uiStyleSheet.textfieldBgColor=configFileSection.retrieveValue<Color>("./uiTextFieldBgColor",uiStyleSheet.textfieldBgColor);
	uiStyleSheet.textfieldFgColor=configFileSection.retrieveValue<Color>("./uiTextFieldFgColor",uiStyleSheet.textfieldFgColor);
	uiStyleSheet.selectionBgColor=configFileSection.retrieveValue<Color>("./uiSelectionBgColor",uiStyleSheet.selectionBgColor);
	uiStyleSheet.selectionFgColor=configFileSection.retrieveValue<Color>("./uiSelectionFgColor",uiStyleSheet.selectionFgColor);
	uiStyleSheet.titlebarBgColor=configFileSection.retrieveValue<Color>("./uiTitleBarBgColor",uiStyleSheet.titlebarBgColor);
	uiStyleSheet.titlebarFgColor=configFileSection.retrieveValue<Color>("./uiTitleBarFgColor",uiStyleSheet.titlebarFgColor);
	uiStyleSheet.sliderHandleWidth=configFileSection.retrieveValue<double>("./uiSliderWidth",uiStyleSheet.sliderHandleWidth);
	uiStyleSheet.sliderHandleColor=configFileSection.retrieveValue<Color>("./uiSliderHandleColor",uiStyleSheet.sliderHandleColor);
	uiStyleSheet.sliderShaftColor=configFileSection.retrieveValue<Color>("./uiSliderShaftColor",uiStyleSheet.sliderShaftColor);
	
	/* Initialize widget management: */
	timerEventScheduler=new Misc::TimerEventScheduler;
	widgetManager=new GLMotif::WidgetManager;
	widgetManager->setStyleSheet(&uiStyleSheet);
	widgetManager->setTimerEventScheduler(timerEventScheduler);
	widgetManager->setDrawOverlayWidgets(configFileSection.retrieveValue<bool>("./drawOverlayWidgets",widgetManager->getDrawOverlayWidgets()));
	widgetManager->getWidgetPopCallbacks().add(this,&VruiState::widgetPopCallback);
	
	/* Create a UI manager: */
	Misc::ConfigurationFileSection uiManagerSection=configFileSection.getSection(configFileSection.retrieveString("./uiManager").c_str());
	std::string uiManagerType=uiManagerSection.retrieveString("./type","Free");
	if(uiManagerType=="Free")
		uiManager=new UIManagerFree(uiManagerSection);
	else if(uiManagerType=="Planar")
		uiManager=new UIManagerPlanar(uiManagerSection);
	else if(uiManagerType=="Spherical")
		uiManager=new UIManagerSpherical(uiManagerSection);
	else
		Misc::throwStdErr("Vrui::initialize: Unknown UI manager type \"%s\"",uiManagerType.c_str());
	widgetManager->setArranger(uiManager); // Widget manager now owns uiManager object
	
	/* Remember whether to route user messages to the console: */
	userMessagesToConsole=configFileSection.retrieveValue<bool>("./userMessagesToConsole",userMessagesToConsole);
	
	/* Create a text event dispatcher to manage GLMotif text and text control events in a cluster-transparent manner: */
	textEventDispatcher=new TextEventDispatcher(master);
	
	/* Dispatch any early text events: */
	textEventDispatcher->dispatchEvents(*widgetManager);
	
	/* Initialize the glyph renderer: */
	GLfloat glyphRendererGlyphSize=configFileSection.retrieveValue<GLfloat>("./glyphSize",GLfloat(inchScale));
	std::string glyphRendererCursorImageFileName=VRUI_INTERNAL_CONFIG_SHAREDIR;
	glyphRendererCursorImageFileName.append("/Textures/Cursor.Xcur");
	glyphRendererCursorImageFileName=configFileSection.retrieveString("./glyphCursorFileName",glyphRendererCursorImageFileName.c_str());
	unsigned int glyphRendererCursorNominalSize=configFileSection.retrieveValue<unsigned int>("./glyphCursorNominalSize",24);
	glyphRenderer=new GlyphRenderer(glyphRendererGlyphSize,glyphRendererCursorImageFileName,glyphRendererCursorNominalSize);
	
	/* Initialize rendering parameters: */
	frontplaneDist=configFileSection.retrieveValue<Scalar>("./frontplaneDist",frontplaneDist);
	backplaneDist=configFileSection.retrieveValue<Scalar>("./backplaneDist",backplaneDist);
	backgroundColor=configFileSection.retrieveValue<Color>("./backgroundColor",backgroundColor);
	for(int i=0;i<3;++i)
		foregroundColor[i]=1.0f-backgroundColor[i];
	foregroundColor[3]=1.0f;
	foregroundColor=configFileSection.retrieveValue<Color>("./foregroundColor",foregroundColor);
	ambientLightColor=configFileSection.retrieveValue<Color>("./ambientLightColor",ambientLightColor);
	widgetMaterial=configFileSection.retrieveValue<GLMaterial>("./widgetMaterial",widgetMaterial);
	
	/* Initialize input graph manager: */
	newInputDevicePosition=configFileSection.retrieveValue<Point>("./newInputDevicePosition",displayCenter);
	virtualInputDevice=new VirtualInputDevice(glyphRenderer,configFileSection);
	inputGraphManager=new InputGraphManager(glyphRenderer,virtualInputDevice);
	
	/* Initialize input device manager: */
	inputDeviceManager=new InputDeviceManager(inputGraphManager,textEventDispatcher);
	if(master)
		inputDeviceManager->initialize(configFileSection);
	
	/* Check if there is a mouse input device adapter: */
	InputDeviceAdapterMouse* mouseAdapter=0;
	for(int i=0;i<inputDeviceManager->getNumInputDeviceAdapters()&&mouseAdapter==0;++i)
		mouseAdapter=dynamic_cast<InputDeviceAdapterMouse*>(inputDeviceManager->getInputDeviceAdapter(i));
	std::string textEntryMethod=mouseAdapter!=0?"Keyboard":"Quikwriting";
	textEntryMethod=configFileSection.retrieveString("./textEntryMethod",textEntryMethod);
	if(textEntryMethod=="Keyboard")
		{
		/* Check if there is a mouse adapter: */
		if(mouseAdapter!=0)
			widgetManager->setTextEntryMethod(new KeyboardTextEntryMethod(mouseAdapter));
		else
			{
			Misc::userWarning("Vrui::initialize: No mouse input device adapter; falling back to Quikwriting text entry method");
			widgetManager->setTextEntryMethod(new GLMotif::QuikwritingTextEntryMethod(widgetManager));
			}
		}
	else if(textEntryMethod=="Quikwriting")
		widgetManager->setTextEntryMethod(new GLMotif::QuikwritingTextEntryMethod(widgetManager));
	else
		Misc::throwStdErr("Vrui::initialize: Unknown text entry method \"%s\"",textEntryMethod.c_str());
	
	/* Distribute the random seed and initial application time: */
	if(multiplexer!=0)
		{
		pipe->broadcast<unsigned int>(randomSeed);
		pipe->broadcast<double>(lastFrame);
		pipe->flush();
		}
	srand(randomSeed);
	lastFrameDelta=0.0;
	
	/* If in cluster mode, create a dispatcher to send input device states to the slaves: */
	if(multiplexer!=0)
		{
		multipipeDispatcher=new MultipipeDispatcher(inputDeviceManager,pipe);
		if(!master)
			{
			/* On slaves, multipipe dispatcher is owned by input device manager: */
			multipipeDispatcher=0;
			}
		}
	
	/* Update all physical input devices to get initial positions and orientations: */
	if(master)
		{
		inputDeviceManager->updateInputDevices();
		
		#if EVILHACK_LOCK_INPUTDEVICE_POS
		if(lockedDevice!=0)
			lockedDevice->setTransformation(Vrui::TrackerState(lockedTranslation,lockedDevice->getOrientation()));
		#endif
		
		if(multiplexer!=0)
			{
			multipipeDispatcher->updateInputDevices();
			textEventDispatcher->writeEventQueues(*pipe);
			pipe->flush();
			}
		}
	else
		{
		inputDeviceManager->updateInputDevices();
		textEventDispatcher->readEventQueues(*pipe);
		}
	
	if(master)
		{
		/* Check if the user wants to save input device data: */
		std::string iddsSectionName=configFileSection.retrieveString("./inputDeviceDataSaver","");
		if(!iddsSectionName.empty())
			{
			/* Go to input device data saver's section: */
			Misc::ConfigurationFileSection iddsSection=configFileSection.getSection(iddsSectionName.c_str());
			
			/* Initialize the input device data saver: */
			inputDeviceDataSaver=new InputDeviceDataSaver(iddsSection,*inputDeviceManager,textEventDispatcher,randomSeed);
			
			/* Save initial input device state: */
			inputDeviceDataSaver->saveCurrentState(lastFrame);
			}
		}
	
	/* Initialize the update regime: */
	if(master)
		updateContinuously=configFileSection.retrieveValue<bool>("./updateContinuously",updateContinuously);
	else
		updateContinuously=true; // Slave nodes always run in continuous mode; they will block on updates from the master
	
	/* Initialize the light source manager: */
	lightsourceManager=new LightsourceManager;
	
	/* Initialize the clipping plane manager: */
	clipPlaneManager=new ClipPlaneManager;
	
	/* Initialize the viewers: */
	StringList viewerNames=configFileSection.retrieveValue<StringList>("./viewerNames");
	numViewers=viewerNames.size();
	viewers=new Viewer[numViewers];
	for(int i=0;i<numViewers;++i)
		{
		/* Go to viewer's section: */
		Misc::ConfigurationFileSection viewerSection=configFileSection.getSection(viewerNames[i].c_str());
		
		/* Initialize viewer: */
		viewers[i].initialize(viewerSection);
		}
	mainViewer=&viewers[0];
	
	/* Initialize the screens: */
	StringList screenNames=configFileSection.retrieveValue<StringList>("./screenNames");
	numScreens=screenNames.size();
	screens=new VRScreen[numScreens];
	for(int i=0;i<numScreens;++i)
		{
		/* Go to screen's section: */
		Misc::ConfigurationFileSection screenSection=configFileSection.getSection(screenNames[i].c_str());
		
		/* Initialize screen: */
		screens[i].initialize(screenSection);
		}
	mainScreen=&screens[0];
	
	/* Initialize screen protection areas: */
	if(configFileSection.hasTag("./screenProtectorAreas"))
		{
		/* Read an explicit list of screen protection areas from the configuration file section: */
		typedef std::vector<ScreenProtectorArea> ScreenProtectorAreaList;
		ScreenProtectorAreaList pas=configFileSection.retrieveValue<ScreenProtectorAreaList>("./screenProtectorAreas");
		numProtectorAreas=int(pas.size());
		protectorAreas=new ScreenProtectorArea[numProtectorAreas+numScreens]; // Leave some extra room in case some actual screens are protected
		for(int i=0;i<numProtectorAreas;++i)
			protectorAreas[i]=pas[i];
		}
	else
		{
		/* Prepare a protector array for protected screens: */
		numProtectorAreas=0;
		protectorAreas=new ScreenProtectorArea[numScreens];
		}
	
	/* Create a list of screen protection areas from configured display screens: */
	for(int i=0;i<numScreens;++i)
		{
		/* Go to screen's section: */
		Misc::ConfigurationFileSection screenSection=configFileSection.getSection(screenNames[i].c_str());
		if(screenSection.retrieveValue("./protectScreen",false))
			{
			protectorAreas[numProtectorAreas]=ScreenProtectorArea(screens[i]);
			++numProtectorAreas;
			}
		}
	
	/* Initialize screen protection devices: */
	typedef std::vector<ScreenProtectorDevice> ScreenProtectorDeviceList;
	ScreenProtectorDeviceList spdl=configFileSection.retrieveValue<ScreenProtectorDeviceList>("./screenProtectorDevices",ScreenProtectorDeviceList());
	numProtectorDevices=int(spdl.size());
	protectorDevices=new ScreenProtectorDevice[numProtectorDevices];
	for(int i=0;i<numProtectorDevices;++i)
		protectorDevices[i]=spdl[i];
	
	/* Create a list of input devices that have haptic features: */
	for(int i=0;i<inputDeviceManager->getNumInputDevices();++i)
		if(inputDeviceManager->hasHapticFeature(inputDeviceManager->getInputDevice(i)))
			++numHapticDevices;
	hapticDevices=new HapticDevice[numHapticDevices];
	HapticDevice* hPtr=hapticDevices;
	for(int i=0;i<inputDeviceManager->getNumInputDevices();++i)
		{
		InputDevice* device=inputDeviceManager->getInputDevice(i);
		if(inputDeviceManager->hasHapticFeature(device))
			{
			hPtr->inputDevice=device;
			hPtr->inKillZone=false;
			++hPtr;
			}
		}
	
	/* Check whether screen protection is used: */
	protectScreens=numProtectorAreas>0&&numProtectorDevices>0;
	
	/* Read protector grid color and spacing: */
	protectorGridColor=configFileSection.retrieveValue<Color>("./screenProtectorGridColor",protectorGridColor);
	protectorGridSpacing=configFileSection.retrieveValue<Scalar>("./screenProtectorGridSpacing",getInchFactor()*Scalar(12));
	
	/* Initialize the listeners: */
	StringList listenerNames=configFileSection.retrieveValue<StringList>("./listenerNames",StringList());
	numListeners=listenerNames.size();
	listeners=new Listener[numListeners];
	for(int i=0;i<numListeners;++i)
		{
		/* Go to listener's section: */
		Misc::ConfigurationFileSection listenerSection=configFileSection.getSection(listenerNames[i].c_str());
		
		/* Initialize listener: */
		listeners[i].initialize(listenerSection);
		}
	mainListener=&listeners[0];
	
	/* Initialize the directories used to load files: */
	viewSelectionHelper.setWidgetManager(widgetManager);
	viewSelectionHelper.setCurrentDirectory(IO::Directory::getCurrent());
	inputGraphSelectionHelper.setWidgetManager(widgetManager);
	inputGraphSelectionHelper.setCurrentDirectory(IO::Directory::getCurrent());
	
	/* Initialize 3D picking: */
	pointPickDistance=Scalar(uiStyleSheet.size*2.0f);
	pointPickDistance=configFileSection.retrieveValue<Scalar>("./pointPickDistance",pointPickDistance);
	Scalar rayPickAngle=Math::deg(Math::atan(pointPickDistance/mainScreen->getScreenTransformation().inverseTransform(mainViewer->getHeadPosition())[2]));
	rayPickAngle=configFileSection.retrieveValue<Scalar>("./rayPickAngle",rayPickAngle);
	if(rayPickAngle<Scalar(0))
		rayPickAngle=Scalar(0);
	if(rayPickAngle>Scalar(90))
		rayPickAngle=Scalar(90);
	rayPickCosine=Math::cos(Math::rad(rayPickAngle));
	
	/* Create the coordinate manager: */
	coordinateManager=new CoordinateManager;
	
	/* Go to tool manager's section: */
	Misc::ConfigurationFileSection toolSection=configFileSection.getSection(configFileSection.retrieveString("./tools").c_str());
	
	/* Initialize tool manager: */
	toolManager=new ToolManager(inputDeviceManager,toolSection);
	
	try
		{
		/* Go to vislet manager's section: */
		Misc::ConfigurationFileSection visletSection=configFileSection.getSection(configFileSection.retrieveString("./vislets").c_str());
		
		/* Initialize vislet manager: */
		visletManager=new VisletManager(visletSection);
		}
	catch(const std::runtime_error&)
		{
		/* Ignore error and continue... */
		}
	
	/* Check if there is a frame rate limit: */
	double maxFrameRate=configFileSection.retrieveValue<double>("./maximumFrameRate",0.0);
	if(maxFrameRate>0.0)
		{
		/* Calculate the minimum frame time: */
		minimumFrameTime=1.0/maxFrameRate;
		}
	
	/* Set the current application time in the timer event scheduler: */
	timerEventScheduler->triggerEvents(lastFrame);
	
	/* Initialize the frame time calculator: */
	numRecentFrameTimes=5;
	recentFrameTimes=new double[numRecentFrameTimes];
	for(int i=0;i<numRecentFrameTimes;++i)
		recentFrameTimes[i]=1.0;
	nextFrameTimeIndex=0;
	sortedFrameTimes=new double[numRecentFrameTimes];
	currentFrameTime=1.0;
	
	/* Initialize the suggested animation frame interval: */
	animationFrameInterval=configFileSection.retrieveValue<double>("./animationFrameInterval",animationFrameInterval);
	
	/* Initialize latency mitigation: */
	predictVsync=configFileSection.retrieveValue<bool>("./predictVsync",predictVsync);
	if(predictVsync)
		{
		/* Read the synchronized display's frame duration in ms: */
		vsyncInterval=Realtime::TimeVector(configFileSection.retrieveValue<double>("./vsyncInterval")/1000.0);
		
		/* Read the synchronized display's post-vsync delay in ms: */
		postVsyncDisplayDelay=configFileSection.retrieveValue<double>("./postVsyncDisplayDelay")/1000.0;
		
		/* Initialize the next vsync time to a very large value, to be corrected during the first frame: */
		nextVsync.set();
		nextVsync+=Realtime::TimeVector(100000,0);
		}
	}

void VruiState::createSystemMenu(void)
	{
	/* Create the Vrui system menu and install it as the main menu: */
	systemMenu=new GLMotif::PopupMenu("VruiSystemMenu",widgetManager);
	systemMenu->setTitle("Vrui System");
	buildSystemMenu(systemMenu);
	systemMenu->manageMenu();
	systemMenuTopLevel=true;
	mainMenu=new MutexMenu(systemMenu);
	}

void VruiState::createSettingsDialog(void)
	{
	/* Create the settings dialog window pop-up: */
	settingsDialog=new GLMotif::PopupWindow("VruiSettingsDialog",Vrui::getWidgetManager(),"Vrui System Settings");
	settingsDialog->setHideButton(true);
	settingsDialog->setCloseButton(true);
	settingsDialog->setResizableFlags(true,true);
	
	/* Create a pager to hold independent sets of settings: */
	settingsPager=new GLMotif::Pager("SettingsPager",settingsDialog,false);
	settingsPager->setMarginWidth(uiStyleSheet.size*0.5f);
	
	/* Create a page for environment settings: */
	settingsPager->setNextPageName("Environment");
	
	GLMotif::Margin* environmentSettingsMargin=new GLMotif::Margin("EnvironmentSettingsMargin",settingsPager,false);
	environmentSettingsMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
	
	GLMotif::RowColumn* environmentSettings=new GLMotif::RowColumn("EnvironmentSettings",environmentSettingsMargin,false);
	environmentSettings->setOrientation(GLMotif::RowColumn::VERTICAL);
	environmentSettings->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	environmentSettings->setNumMinorWidgets(2);
	
	new GLMotif::Label("NavigationUnitLabel",environmentSettings,"Nav. Space Unit");
	
	GLMotif::RowColumn* navigationUnitBox=new GLMotif::RowColumn("NavigationUnitBox",environmentSettings,false);
	navigationUnitBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	navigationUnitBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	navigationUnitBox->setNumMinorWidgets(1);
	
	GLMotif::TextField* navigationUnitScale=new GLMotif::TextField("NavigationUnitScale",navigationUnitBox,8);
	navigationUnitScale->setValueType(GLMotif::TextField::FLOAT);
	navigationUnitScale->setFloatFormat(GLMotif::TextField::SMART);
	navigationUnitScale->setEditable(true);
	navigationUnitScale->setValue(coordinateManager->getUnit().factor);
	navigationUnitScale->getValueChangedCallbacks().add(this,&VruiState::navigationUnitScaleValueChangedCallback);
	
	GLMotif::DropdownBox* navigationUnit=new GLMotif::DropdownBox("NavigationUnit",navigationUnitBox);
	navigationUnit->addItem("<undefined>");
	for(int i=1;i<Geometry::LinearUnit::NUM_UNITS;++i)
		{
		/* Create a unit to query its name (poor API): */
		Geometry::LinearUnit unit(Geometry::LinearUnit::Unit(i),1);
		
		/* Add the unit to the drop-down box: */
		navigationUnit->addItem(unit.getName());
		}
	navigationUnit->setSelectedItem(coordinateManager->getUnit().unit);
	navigationUnit->getValueChangedCallbacks().add(this,&VruiState::navigationUnitValueChangedCallback);
	
	navigationUnitBox->setColumnWeight(0,1.0f);
	navigationUnitBox->setColumnWeight(1,1.0f);
	navigationUnitBox->manageChild();
	
	environmentSettings->manageChild();
	
	environmentSettingsMargin->manageChild();
	
	/* Create a page for lighting settings: */
	settingsPager->setNextPageName("Lights");
	
	GLMotif::Margin* lightSettingsMargin=new GLMotif::Margin("LightSettingsMargin",settingsPager,false);
	lightSettingsMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
	
	GLMotif::RowColumn* lightSettings=new GLMotif::RowColumn("LightSettings",lightSettingsMargin,false);
	lightSettings->setOrientation(GLMotif::RowColumn::VERTICAL);
	lightSettings->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	lightSettings->setNumMinorWidgets(2);
	
	/* Create a slider to set ambient light intensity: */
	new GLMotif::Label("AmbientLabel",lightSettings,"Ambient Intensity");
	
	GLMotif::TextFieldSlider* ambientIntensitySlider=new GLMotif::TextFieldSlider("AmbientIntensitySlider",lightSettings,5,uiStyleSheet.fontHeight*5.0f);
	ambientIntensitySlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	ambientIntensitySlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	ambientIntensitySlider->setValueRange(0.0,1.0,0.005);
	float ambientIntensity=(ambientLightColor[0]+ambientLightColor[1]+ambientLightColor[2])/3.0f;
	ambientIntensitySlider->setValue(ambientIntensity);
	ambientIntensitySlider->getValueChangedCallbacks().add(this,&VruiState::ambientIntensityValueChangedCallback);
	
	/* Create a row of buttons to toggle viewer's headlights: */
	new GLMotif::Label("HeadlightsLabel",lightSettings,"Headlights");
	
	GLMotif::RowColumn* headlightsBox=new GLMotif::RowColumn("HeadlightsBox",lightSettings,false);
	headlightsBox->setAlignment(GLMotif::Alignment::LEFT);
	headlightsBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	headlightsBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	headlightsBox->setNumMinorWidgets(1);
	
	for(int i=0;i<numViewers;++i)
		{
		/* Create a toggle button for the viewer: */
		GLMotif::ToggleButton* viewerToggle=new GLMotif::ToggleButton(viewers[i].getName(),headlightsBox,viewers[i].getName());
		viewerToggle->setBorderType(GLMotif::Widget::PLAIN);
		viewerToggle->setBorderWidth(0.0f);
		viewerToggle->setToggle(viewers[i].getHeadlight().isEnabled());
		viewerToggle->getValueChangedCallbacks().add(this,&VruiState::viewerHeadlightValueChangedCallback,i);
		}
	
	headlightsBox->manageChild();
	
	/* Create a toggle and sliders to create a directional Sun light source: */
	GLMotif::Margin* sunToggleMargin=new GLMotif::Margin("SunToggleMargin",lightSettings,false);
	sunToggleMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::LEFT,GLMotif::Alignment::VCENTER));
	
	GLMotif::ToggleButton* sunToggle=new GLMotif::ToggleButton("SunToggle",sunToggleMargin,"Sun");
	sunToggle->setBorderType(GLMotif::Widget::PLAIN);
	sunToggle->setBorderWidth(0.0f);
	sunToggle->setToggle(false);
	sunToggle->getValueChangedCallbacks().add(this,&VruiState::sunValueChangedCallback);
	
	sunToggleMargin->manageChild();
	
	/* Create text field sliders to set the Sun's azimuth (relative to forward direction) and elevation (relative to up): */
	GLMotif::RowColumn* sunBox=new GLMotif::RowColumn("SunBox",lightSettings,false);
	sunBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	sunBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	sunBox->setNumMinorWidgets(2);
	
	new GLMotif::Label("AzimuthLabel",sunBox,"Azimuth");
	
	sunAzimuthSlider=new GLMotif::TextFieldSlider("SunAzimuthSlider",sunBox,5,uiStyleSheet.fontHeight*5.0f);
	sunAzimuthSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	sunAzimuthSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	sunAzimuthSlider->setValueRange(-180.0,180.0,1.0);
	sunAzimuthSlider->getSlider()->addNotch(0.0);
	sunAzimuthSlider->setValue(sunAzimuth);
	sunAzimuthSlider->getValueChangedCallbacks().add(this,&VruiState::sunAzimuthValueChangedCallback);
	sunAzimuthSlider->setEnabled(false);
	
	new GLMotif::Label("ElevationLabel",sunBox,"Elevation");
	
	sunElevationSlider=new GLMotif::TextFieldSlider("SunElevationSlider",sunBox,5,uiStyleSheet.fontHeight*5.0f);
	sunElevationSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	sunElevationSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	sunElevationSlider->setValueRange(0.0,90.0,1.0);
	sunElevationSlider->setValue(sunElevation);
	sunElevationSlider->getValueChangedCallbacks().add(this,&VruiState::sunElevationValueChangedCallback);
	sunElevationSlider->setEnabled(false);
	
	new GLMotif::Label("IntensityLabel",sunBox,"Intensity");
	
	sunIntensitySlider=new GLMotif::TextFieldSlider("SunIntensitySlider",sunBox,5,uiStyleSheet.fontHeight*5.0f);
	sunIntensitySlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
	sunIntensitySlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	sunIntensitySlider->setValueRange(0.0,1.0,0.005);
	sunIntensitySlider->setValue(sunIntensity);
	sunIntensitySlider->getValueChangedCallbacks().add(this,&VruiState::sunIntensityValueChangedCallback);
	sunIntensitySlider->setEnabled(false);
	
	sunBox->manageChild();
	
	lightSettings->manageChild();
	
	lightSettingsMargin->manageChild();
	
	/* Create a page for graphics settings: */
	settingsPager->setNextPageName("Graphics");
	
	GLMotif::Margin* graphicsSettingsMargin=new GLMotif::Margin("GraphicsSettingsMargin",settingsPager,false);
	graphicsSettingsMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
	
	GLMotif::RowColumn* graphicsSettings=new GLMotif::RowColumn("GraphicsSettings",graphicsSettingsMargin,false);
	graphicsSettings->setOrientation(GLMotif::RowColumn::VERTICAL);
	graphicsSettings->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	graphicsSettings->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* colorBox=new GLMotif::RowColumn("ColorBox",graphicsSettings,false);
	colorBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	colorBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	colorBox->setNumMinorWidgets(1);
	
	new GLMotif::Label("BackgroundColorLabel",colorBox,"Background");
	
	GLMotif::Margin* backgroundColorMargin=new GLMotif::Margin("BackgroundColorMargin",colorBox,false);
	backgroundColorMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER));
	
	GLMotif::HSVColorSelector* backgroundColorSelector=new GLMotif::HSVColorSelector("BackgroundColorSelector",backgroundColorMargin);
	backgroundColorSelector->setCurrentColor(Vrui::getBackgroundColor());
	backgroundColorSelector->getValueChangedCallbacks().add(this,&VruiState::backgroundColorValueChangedCallback);
	
	backgroundColorMargin->manageChild();
	
	new GLMotif::Label("ForegroundColorLabel",colorBox,"Foreground");
	
	GLMotif::Margin* foregroundColorMargin=new GLMotif::Margin("ForegroundColorMargin",colorBox,false);
	foregroundColorMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HCENTER));
	
	GLMotif::HSVColorSelector* foregroundColorSelector=new GLMotif::HSVColorSelector("ForegroundColorSelector",foregroundColorMargin);
	foregroundColorSelector->setCurrentColor(Vrui::getForegroundColor());
	foregroundColorSelector->getValueChangedCallbacks().add(this,&VruiState::foregroundColorValueChangedCallback);
	
	foregroundColorMargin->manageChild();
	
	colorBox->setColumnWeight(1,1.0f);
	colorBox->setColumnWeight(3,1.0f);
	colorBox->manageChild();
	
	GLMotif::RowColumn* planesBox=new GLMotif::RowColumn("ColorBox",graphicsSettings,false);
	planesBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	planesBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	planesBox->setNumMinorWidgets(2);
	
	new GLMotif::Label("BackplaneLabel",planesBox,"Backplane");
	
	GLMotif::TextFieldSlider* backplaneSlider=new GLMotif::TextFieldSlider("BackplaneSlider",planesBox,8,uiStyleSheet.fontHeight*10.0f);
	backplaneSlider->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
	backplaneSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	backplaneSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	backplaneSlider->setValueRange(getBackplaneDist()/100.0,getBackplaneDist()*100.0,0.0);
	backplaneSlider->getSlider()->addNotch(Math::log10(getBackplaneDist()));
	backplaneSlider->setValue(getBackplaneDist());
	backplaneSlider->getValueChangedCallbacks().add(this,&VruiState::backplaneValueChangedCallback);
	
	new GLMotif::Label("FrontplaneLabel",planesBox,"Frontplane");
	
	GLMotif::TextFieldSlider* frontplaneSlider=new GLMotif::TextFieldSlider("FrontplaneSlider",planesBox,8,uiStyleSheet.fontHeight*10.0f);
	frontplaneSlider->setSliderMapping(GLMotif::TextFieldSlider::EXP10);
	frontplaneSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
	frontplaneSlider->getTextField()->setFloatFormat(GLMotif::TextField::SMART);
	frontplaneSlider->setValueRange(getFrontplaneDist()/100.0,getFrontplaneDist()*100.0,0.0);
	frontplaneSlider->getSlider()->addNotch(Math::log10(getFrontplaneDist()));
	frontplaneSlider->setValue(getFrontplaneDist());
	frontplaneSlider->getValueChangedCallbacks().add(this,&VruiState::frontplaneValueChangedCallback);
	
	planesBox->setColumnWeight(1,1.0f);
	planesBox->manageChild();
	
	graphicsSettings->manageChild();
	
	graphicsSettingsMargin->manageChild();
	
	if(useSound)
		{
		/* Create a page for sound settings: */
		settingsPager->setNextPageName("Sound");
		
		GLMotif::Margin* soundSettingsMargin=new GLMotif::Margin("SoundSettingsMargin",settingsPager,false);
		soundSettingsMargin->setAlignment(GLMotif::Alignment(GLMotif::Alignment::HFILL,GLMotif::Alignment::TOP));
		
		GLMotif::RowColumn* soundSettings=new GLMotif::RowColumn("SoundSettings",soundSettingsMargin,false);
		soundSettings->setOrientation(GLMotif::RowColumn::VERTICAL);
		soundSettings->setPacking(GLMotif::RowColumn::PACK_TIGHT);
		soundSettings->setNumMinorWidgets(2);
		
		new GLMotif::Label("GlobalGainLabel",soundSettings,"Global Gain (dB)");
		
		GLMotif::TextFieldSlider* globalGainSlider=new GLMotif::TextFieldSlider("GlobalGainSlider",soundSettings,6,uiStyleSheet.fontHeight*10.0f);
		globalGainSlider->setSliderMapping(GLMotif::TextFieldSlider::LINEAR);
		globalGainSlider->setValueType(GLMotif::TextFieldSlider::FLOAT);
		globalGainSlider->getTextField()->setFloatFormat(GLMotif::TextField::FIXED);
		globalGainSlider->getTextField()->setPrecision(1);
		globalGainSlider->setValueRange(-30.0,10.0,0.1);
		globalGainSlider->getSlider()->addNotch(0.0);
		globalGainSlider->setValue(getMainListener()->getGain());
		globalGainSlider->getValueChangedCallbacks().add(this,&VruiState::globalGainValueChangedCallback);
		
		soundSettings->manageChild();
		
		soundSettingsMargin->manageChild();
		}
	
	settingsPager->setCurrentChildIndex(0);
	settingsPager->manageChild();
	}

DisplayState* VruiState::registerContext(GLContextData& contextData) const
	{
	/* Try retrieving an already existing display state mapper data item: */
	DisplayStateMapper::DataItem* dataItem=contextData.retrieveDataItem<DisplayStateMapper::DataItem>(&displayStateMapper);
	if(dataItem==0)
		{
		/* Create a new display state mapper data item: */
		dataItem=new DisplayStateMapper::DataItem;
		
		/* Associate it with the OpenGL context: */
		contextData.addDataItem(&displayStateMapper,dataItem);
		
		if(protectScreens)
			{
			/* Create a display list to render the screen protector grids: */
			dataItem->screenProtectorDisplayListId=glGenLists(1);
			glNewList(dataItem->screenProtectorDisplayListId,GL_COMPILE);
			for(int area=0;area<numProtectorAreas;++area)
				protectorAreas[area].glRenderAction(protectorGridSpacing);
			glEndList();
			}
		}
	
	/* Return a pointer to the display state structure: */
	return &dataItem->displayState;
	}

void VruiState::prepareMainLoop(void)
	{
	/* From now on, display all user messages as GLMotif dialogs unless told otherwise: */
	dynamic_cast<MessageLogger*>(Misc::MessageLogger::getMessageLogger().getPointer())->setUserToConsole(userMessagesToConsole);
	
	/* Create the system menu if the application didn't install one: */
	if(mainMenu==0)
		createSystemMenu();
	
	/* Create the settings dialog: */
	createSettingsDialog();
	
	/* Check if the user gave a viewpoint file on the command line: */
	if(!viewpointFileName.empty())
		{
		/* Split the given name into directory and file name: */
		const char* vfn=viewpointFileName.c_str();
		const char* fileName=Misc::getFileName(vfn);
		std::string dirName(vfn,fileName);
		
		/* Override the navigation transformation: */
		try
			{
			viewSelectionHelper.setCurrentDirectory(IO::openDirectory(dirName.c_str()));
			loadViewpointFile(*viewSelectionHelper.getCurrentDirectory(),fileName);
			}
		catch(const std::runtime_error& err)
			{
			/* Log an error message and continue: */
			Misc::formattedUserError("Unable to load viewpoint file %s due to exception %s",viewpointFileName.c_str(),err.what());
			}
		}
	
	/* Push the initial navigation transformation into the undo buffer: */
	navigationUndoBuffer.push_back(navigationTransformation);
	
	#if DELAY_NAVIGATIONTRANSFORMATION
	/* Start delaying the navigation transformation at this point: */
	delayNavigationTransformation=true;
	#endif
	
	if(loadInputGraph)
		{
		/* Load the requested input graph: */
		inputGraphManager->loadInputGraph(*inputGraphSelectionHelper.getCurrentDirectory(),inputGraphFileName.c_str(),"InputGraph");
		loadInputGraph=false;
		}
	else
		{
		/* Create default tool assignment: */
		toolManager->loadDefaultTools();
		}
	
	/* Tell the tool manager that from now on it has to call newly-created tools' frame methods: */
	toolManager->enterMainLoop();
	
	/* Tell all input device adapters that main loop is about to start: */
	inputDeviceManager->prepareMainLoop();
	
	/* Enable all vislets for the first time: */
	visletManager->enable();
	
	if(inputDeviceDataSaver!=0)
		{
		/* Tell the input device data saver to get going: */
		inputDeviceDataSaver->prepareMainLoop();
		}
	
	/* Call main loop preparation function: */
	if(prepareMainLoopFunction!=0)
		prepareMainLoopFunction(prepareMainLoopFunctionData);
	
	/* Update the application time so that the first frame's frame time is exactly zero: */
	if(master)
		{
		/* Check if there is a synchronization request for the first frame: */
		if(synchFrameTime>0.0)
			{
			/* Check if the frame needs to be delayed: */
			if(synchWait&&lastFrame<synchFrameTime)
				{
				/* Sleep for a while to reach the synchronized frame time: */
				vruiDelay(synchFrameTime-lastFrame);
				}
			
			/* Override the free-running timer: */
			lastFrame=synchFrameTime;
			}
		else
			{
			/* Take an application timer snapshot: */
			lastFrame=appTime.peekTime();
			
			/* Synchronize the first frame to the new application time: */
			synchFrameTime=lastFrame;
			synchWait=false;
			}
		}
	}

void VruiState::update(void)
	{
	/*********************************************************************
	Update the application time and all related state:
	*********************************************************************/
	
	double lastLastFrame=lastFrame;
	if(master)
		{
		/* Take an application timer snapshot: */
		lastFrame=appTime.peekTime();
		if(synchFrameTime>0.0)
			{
			/* Check if the frame needs to be delayed: */
			if(synchWait&&lastFrame<synchFrameTime)
				{
				/* Sleep for a while to reach the synchronized frame time: */
				vruiDelay(synchFrameTime-lastFrame);
				}
			
			/* Override the free-running timer: */
			lastFrame=synchFrameTime;
			synchFrameTime=0.0;
			synchWait=false;
			}
		else if(minimumFrameTime>0.0)
			{
			/* Check if the time for the last frame was less than the allowed minimum: */
			if(lastFrame-lastLastFrame<minimumFrameTime)
				{
				/* Sleep for a while to reach the minimum frame time: */
				vruiDelay(minimumFrameTime-(lastFrame-lastLastFrame));
				
				/* Take another application timer snapshot: */
				lastFrame=appTime.peekTime();
				}
			}
		if(multiplexer!=0)
			pipe->write<double>(lastFrame);
		
		/* Update the Vrui application timer and the frame time history: */
		recentFrameTimes[nextFrameTimeIndex]=lastFrame-lastLastFrame;
		++nextFrameTimeIndex;
		if(nextFrameTimeIndex==numRecentFrameTimes)
			nextFrameTimeIndex=0;
		
		/* Calculate current median frame time: */
		for(int i=0;i<numRecentFrameTimes;++i)
			{
			int j;
			for(j=i-1;j>=0&&sortedFrameTimes[j]>recentFrameTimes[i];--j)
				sortedFrameTimes[j+1]=sortedFrameTimes[j];
			sortedFrameTimes[j+1]=recentFrameTimes[i];
			}
		currentFrameTime=sortedFrameTimes[numRecentFrameTimes/2];
		if(multiplexer!=0)
			pipe->write<double>(currentFrameTime);
		}
	else
		{
		/* Receive application time and current median frame time: */
		pipe->read<double>(lastFrame);
		pipe->read<double>(currentFrameTime);
		}
	
	/* Calculate the current frame time delta: */
	lastFrameDelta=lastFrame-lastLastFrame;
	
	#if RENDERFRAMETIMES
	/* Update the frame time graph: */
	++frameTimeIndex;
	if(frameTimeIndex==numFrameTimes)
		frameTimeIndex=0;
	frameTimes[frameTimeIndex]=lastFrame-lastLastFrame;
	#endif
	
	/* Reset the next scheduled frame time: */
	nextFrameTime=0.0;
	
	/*********************************************************************
	Update input device state and distribute all shared state:
	*********************************************************************/
	
	int navBroadcastMask=navigationTransformationChangedMask;
	if(master)
		{
		/* Check if device state prediction is enabled and at least a few vsync periods have passed: */
		if(predictVsync&&numVsyncs>=10)
			{
			/* Calculate the presentation time of the synched display based on the predicted vsync time and post-vsync delay: */
			Realtime::TimePointMonotonic predictTime=nextVsync;
			predictTime+=postVsyncDisplayDelay;
			
			/* Set the prediction time for the current frame in the input device manager: */
			inputDeviceManager->setPredictionTime(predictTime);
			}
		
		/* Update all physical input devices: */
		inputDeviceManager->updateInputDevices();
		
		#if EVILHACK_LOCK_INPUTDEVICE_POS
		if(lockedDevice!=0)
			lockedDevice->setTransformation(Vrui::TrackerState(lockedTranslation,lockedDevice->getOrientation()));
		#endif
		
		if(multiplexer!=0)
			{
			/* Write input device states and text events to all slaves: */
			multipipeDispatcher->updateInputDevices();
			textEventDispatcher->writeEventQueues(*pipe);
			}
		
		/* Save input device states to data file if requested: */
		if(inputDeviceDataSaver!=0)
			inputDeviceDataSaver->saveCurrentState(lastFrame);
		
		#if DELAY_NAVIGATIONTRANSFORMATION
		if(navigationTransformationEnabled&&(navigationTransformationChangedMask&0x1))
			{
			/* Update the navigation transformation: */
			updateNavigationTransformation(newNavigationTransformation);
			navigationTransformationChangedMask=0x0;
			}
		#endif
		}
	else
		{
		/* Receive input device states and text events from the master: */
		inputDeviceManager->updateInputDevices();
		textEventDispatcher->readEventQueues(*pipe);
		}
	
	if(multiplexer!=0)
		{
		/* Broadcast the current navigation transformation and/or display center/size: */
		pipe->broadcast<int>(navBroadcastMask);
		if(navBroadcastMask&0x1)
			{
			if(master)
				{
				/* Send the new navigation transformation: */
				pipe->write<Scalar>(navigationTransformation.getTranslation().getComponents(),3);
				pipe->write<Scalar>(navigationTransformation.getRotation().getQuaternion(),4);
				pipe->write<Scalar>(navigationTransformation.getScaling());
				}
			else
				{
				/* Receive the new navigation transformation: */
				Vector translation;
				pipe->read<Scalar>(translation.getComponents(),3);
				Scalar rotationQuaternion[4];
				pipe->read<Scalar>(rotationQuaternion,4);
				Scalar scaling=pipe->read<Scalar>();
				
				/* Update the navigation transformation: */
				navigationTransformationEnabled=true;
				updateNavigationTransformation(NavTransform(translation,Rotation(rotationQuaternion),scaling));
				}
			}
		if(navBroadcastMask&0x2)
			{
			/* Broadcast the new display center and size: */
			pipe->broadcast<Scalar>(displayCenter.getComponents(),3);
			pipe->broadcast<Scalar>(displaySize);
			}
		if(navBroadcastMask&0x4)
			{
			if(master)
				{
				/* Send the tool kill zone's new center: */
				pipe->write<Scalar>(toolManager->getToolKillZone()->getCenter().getComponents(),3);
				}
			else
				{
				/* Receive the tool kill zone' new center: */
				Point newCenter;
				pipe->read<Scalar>(newCenter.getComponents(),3);
				toolManager->getToolKillZone()->setCenter(newCenter);
				}
			}
		
		pipe->flush();
		}
	
	#if SAVESHAREDVRUISTATE
	/* Save shared state to a local file for post-mortem analysis purposes: */
	vruiSharedStateFile->write<double>(lastFrame);
	vruiSharedStateFile->write<double>(currentFrameTime);
	int numInputDevices=inputDeviceManager->getNumInputDevices();
	vruiSharedStateFile->write<int>(numInputDevices);
	for(int i=0;i<numInputDevices;++i)
		{
		InputDevice* id=inputDeviceManager->getInputDevice(i);
		vruiSharedStateFile->write<Scalar>(id->getPosition().getComponents(),3);
		vruiSharedStateFile->write<Scalar>(id->getOrientation().getQuaternion(),4);
		}
	#endif
	
	/*********************************************************************
	Update all managers:
	*********************************************************************/
	
	/* Set the widget manager's time: */
	widgetManager->setTime(lastFrame);
	
	/* Trigger all due timer events: */
	timerEventScheduler->triggerEvents(lastFrame);
	
	/* Dispatch all text events: */
	textEventDispatcher->dispatchEvents(*widgetManager);
	
	/* Update the input graph: */
	inputGraphManager->update();
	
	/* Update the tool manager: */
	toolManager->update();
	
	/* Check if a new input graph needs to be loaded: */
	if(loadInputGraph)
		{
		try
			{
			/* Load the input graph from the selected configuration file: */
			inputGraphManager->clear();
			inputGraphManager->loadInputGraph(*inputGraphSelectionHelper.getCurrentDirectory(),inputGraphFileName.c_str(),"InputGraph");
			}
		catch(const std::runtime_error& err)
			{
			/* Show an error message: */
			Misc::formattedUserError("Vrui::loadInputGraph: Could not load input graph from file \"%s\" due to exception %s",inputGraphFileName.c_str(),err.what());
			}
		
		loadInputGraph=false;
		}
	
	/* Update viewer states: */
	for(int i=0;i<numViewers;++i)
		viewers[i].update();
	
	/* Check for screen protection: */
	if(protectScreens)
		{
		/* Check all protected devices against all protection areas: */
		renderProtection=Scalar(0);
		for(int device=0;device<numProtectorDevices;++device)
			if(inputGraphManager->isEnabled(protectorDevices[device].inputDevice))
				{
				/* Calculate this protector's sphere center: */
				Point center=protectorDevices[device].inputDevice->getTransformation().transform(protectorDevices[device].center);
				
				/* Check the device against all protection areas: */
				for(int area=0;area<numProtectorAreas;++area)
					{
					Scalar penetration=protectorAreas[area].calcPenetrationDepth(center,protectorDevices[device].radius);
					if(renderProtection<penetration)
						renderProtection=penetration;
					}
				}
		}
	for(int hapticDeviceIndex=0;hapticDeviceIndex<numHapticDevices;++hapticDeviceIndex)
		{
		HapticDevice& hd=hapticDevices[hapticDeviceIndex];
		bool inKillZone=toolManager->getToolKillZone()->isDeviceIn(hd.inputDevice);
		if(inKillZone!=hd.inKillZone)
			{
			/* Request a haptic tick on the device: */
			inputDeviceManager->hapticTick(hd.inputDevice,10,200,255);
			}
		hd.inKillZone=inKillZone;
		}
	
	/* Update listener states: */
	for(int i=0;i<numListeners;++i)
		listeners[i].update();
	
	/* Call frame functions of all loaded vislets: */
	if(visletManager!=0)
		visletManager->frame();
	
	/* Call all additional frame callbacks: */
	{
	Threads::Mutex::Lock frameCallbacksLock(frameCallbacksMutex);
	for(std::vector<FrameCallbackSlot>::iterator fcIt=frameCallbacks.begin();fcIt!=frameCallbacks.end();++fcIt)
		{
		/* Call the callback and check if it wants to be removed: */
		if(fcIt->callback(fcIt->userData))
			{
			/* Remove the callback from the list: */
			*fcIt=frameCallbacks.back();
			frameCallbacks.pop_back();
			--fcIt;
			}
		}
	}
	
	/* Call frame function: */
	frameFunction(frameFunctionData);
	
	/* Finish any pending messages on the main pipe, in case an application didn't clean up: */
	if(multiplexer!=0)
		pipe->flush();
	}

#if EVILHACK_USE_BLINDERS

namespace {

void setClipPlane(GLClipPlaneTracker* cpt,const Plane& clipPlane,GLint clipPlaneIndex)
	{
	/* Set the clipping plane in the clipping plane tracker and OpenGL: */
	GLClipPlaneTracker::Plane plane;
	for(int i=0;i<3;++i)
		plane[i]=clipPlane.getNormal()[i];
	plane[3]=-clipPlane.getOffset();
	cpt->enableClipPlane(clipPlaneIndex,plane);
	}

}

#endif

void VruiState::display(DisplayState* displayState,GLContextData& contextData) const
	{
	/* Initialize lighting state through the display state's light tracker: */
	GLLightTracker* lt=contextData.getLightTracker();
	lt->setLightingEnabled(true);
	lt->setSpecularColorSeparate(false);
	lt->setLightingTwoSided(false);
	lt->setColorMaterials(false);
	lt->setColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	lt->setNormalScalingMode(GLLightTracker::NormalScalingNormalize);
	
	/* Enable ambient light source: */
	glLightModelAmbient(ambientLightColor);
	
	/* Go to physical coordinates: */
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrix(displayState->modelviewPhysical);
	
	/* Set light sources: */
	lightsourceManager->setLightsources(navigationTransformationEnabled,displayState,contextData);
	
	#if EVILHACK_USE_BLINDERS
	
	Scalar horzFov2=Math::rad(Math::div2(Scalar(30)));
	// Scalar vertFov2=Math::rad(Math::div2(Scalar(100)));
	Scalar vertFov2=horzFov2*Scalar(9)/Scalar(16);
	Scalar frontFov=Scalar(24)*inchScale;
	
	/* Attach blinders to the main viewer: */
	const TrackerState& vts=mainViewer->getHeadTransformation();
	Point eyePos=mainViewer->getDeviceEyePosition(Viewer::MONO);
	
	Plane cpLeft(Vector(Math::cos(horzFov2),Math::sin(horzFov2),0),eyePos);
	cpLeft.transform(vts);
	setClipPlane(contextData.getClipPlaneTracker(),cpLeft,0);
	
	Plane cpRight(Vector(-Math::cos(horzFov2),Math::sin(horzFov2),0),eyePos);
	cpRight.transform(vts);
	setClipPlane(contextData.getClipPlaneTracker(),cpRight,1);
	
	Plane cpBottom(Vector(0,Math::sin(vertFov2),Math::cos(vertFov2)),eyePos);
	cpBottom.transform(vts);
	setClipPlane(contextData.getClipPlaneTracker(),cpBottom,2);
	
	Plane cpTop(Vector(0,Math::sin(vertFov2),-Math::cos(vertFov2)),eyePos);
	cpTop.transform(vts);
	setClipPlane(contextData.getClipPlaneTracker(),cpTop,3);
	
	Plane cpFront(Vector(0,1,0),eyePos+Vector(0,frontFov,0));
	cpFront.transform(vts);
	setClipPlane(contextData.getClipPlaneTracker(),cpFront,4);
	
	#endif
	
	/* Render input device manager's state: */
	inputDeviceManager->glRenderAction(contextData);
	
	/* Render input graph devices: */
	inputGraphManager->glRenderDevices(contextData);
	
	/* Display any realized widgets: */
	glMaterial(GLMaterialEnums::FRONT,widgetMaterial);
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
	widgetManager->draw(contextData);
	glDisable(GL_COLOR_MATERIAL);
	
	#if !EVILHACK_USE_BLINDERS
	
	/* Set clipping planes: */
	clipPlaneManager->setClipPlanes(navigationTransformationEnabled,displayState,contextData);
	
	#endif
	
	/* Render tool manager's state: */
	toolManager->glRenderAction(contextData);
	
	/* Render input graph tools: */
	inputGraphManager->glRenderTools(contextData);
	
	#if EVILHACK_USE_BLINDERS
	contextData.getClipPlaneTracker()->pause();
	#endif
	
	/* Display all loaded vislets: */
	if(visletManager!=0)
		visletManager->display(contextData);
	
	#if EVILHACK_USE_BLINDERS
	contextData.getClipPlaneTracker()->resume();
	#endif
	
	/* Call the user display function: */
	if(displayFunction!=0)
		{
		if(navigationTransformationEnabled)
			{
			/* Go to navigational coordinates: */
			glLoadMatrix(displayState->modelviewNavigational);
			}
		displayFunction(contextData,displayFunctionData);
		if(navigationTransformationEnabled)
			{
			/* Go back to physical coordinates: */
			glLoadMatrix(displayState->modelviewPhysical);
			}
		}
	
	/* Execute the transparency rendering pass: */
	if(TransparentObject::needRenderPass())
		{
		/* Set up OpenGL state for transparency: */
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		
		/* Execute transparent rendering pass: */
		TransparentObject::transparencyPass(contextData);
		
		/* Return to standard OpenGL state: */
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		}
	
	/* Temporarily disable all clipping planes, bypassing the OpenGL context's clipping plane tracker: */
	contextData.getClipPlaneTracker()->pause();
	
	/* Render screen protectors if necessary: */
	if(displayState->window->protectScreens&&(alwaysRenderProtection||renderProtection>Scalar(0)))
		{
		/* Set up OpenGL state to render the screen protection grids: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glLineWidth(1.0f);
		
		/* Access the display state mapper's context data item: */
		DisplayStateMapper::DataItem* dsmDataItem=contextData.retrieveDataItem<DisplayStateMapper::DataItem>(&displayStateMapper);
		
		if(alwaysRenderProtection)
			{
			/* Draw the screen protection grids with normal depth embedding: */
			glColor4f(protectorGridColor[0],protectorGridColor[1],protectorGridColor[2],0.333f);
			glCallList(dsmDataItem->screenProtectorDisplayListId);
			}
		
		if(renderProtection>Scalar(0))
			{
			/* Draw the screen protection grids overlaying any other geometry: */
			glDisable(GL_DEPTH_TEST);
			
			/* Execute the screen protector display list: */
			glColor4f(protectorGridColor[0],protectorGridColor[1],protectorGridColor[2],float(renderProtection));
			glCallList(contextData.retrieveDataItem<DisplayStateMapper::DataItem>(&displayStateMapper)->screenProtectorDisplayListId);
			}
		
		/* Restore OpenGL state: */
		glDisable(GL_BLEND);
		glPopAttrib();
		}
	}

void VruiState::sound(ALContextData& contextData) const
	{
	#if ALSUPPORT_CONFIG_HAVE_OPENAL
	/* Display all loaded vislets: */
	if(visletManager!=0)
		visletManager->sound(contextData);
	
	/* Call the user sound function: */
	if(soundFunction!=0)
		{
		if(navigationTransformationEnabled)
			{
			/* Go to navigational coordinates: */
			contextData.pushMatrix();
			contextData.multMatrix(navigationTransformation);
			}
		soundFunction(contextData,soundFunctionData);
		if(navigationTransformationEnabled)
			{
			/* Go back to physical coordinates: */
			contextData.popMatrix();
			}
		}
	#endif
	}

void VruiState::finishMainLoop(void)
	{
	/* Call main loop shutdown function: */
	if(finishMainLoopFunction!=0)
		finishMainLoopFunction(finishMainLoopFunctionData);
	
	/* Destroy all tools: */
	toolManager->destroyTools();
	
	/* Disable all vislets for the last time: */
	visletManager->disable();
	
	/* Deregister the popup callback: */
	widgetManager->getWidgetPopCallbacks().remove(this,&VruiState::widgetPopCallback);
	}

void VruiState::showMessageCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	/* Show an "error" message: */
	showErrorMessage("Message",std::string(argumentBegin,argumentEnd).c_str(),"Jolly Good!");
	}

void VruiState::resetViewCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VruiState* thisPtr=static_cast<VruiState*>(userData);
	
	/* Call the application-supplied navigation reset function if no navigation tools are active: */
	if(thisPtr->activeNavigationTool==0&&thisPtr->resetNavigationFunction!=0)
		(*thisPtr->resetNavigationFunction)(thisPtr->resetNavigationFunctionData);
	else
		{
		/* Print an error message: */
		std::cout<<"loadView: Unable to reset view because navigation transformation is locked"<<std::endl;
		}
	}

void VruiState::loadViewCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VruiState* thisPtr=static_cast<VruiState*>(userData);
	
	/* Load the requested viewpoint file only if there are no active navigation tools: */
	std::string viewFileName(argumentBegin,argumentEnd);
	if(thisPtr->activeNavigationTool==0)
		{
		try
			{
			/* Load the requested viewpoint file: */
			thisPtr->loadViewpointFile(*IO::openDirectory("."),viewFileName.c_str());
			}
		catch(const std::runtime_error& err)
			{
			/* Print an error message: */
			std::cout<<"loadView: Unable to load view file "<<viewFileName<<" due to exception "<<err.what()<<std::endl;
			}
		}
	else
		{
		/* Print an error message: */
		std::cout<<"loadView: Unable to load view file "<<viewFileName<<" because navigation transformation is locked"<<std::endl;
		}
	}

void VruiState::loadInputGraphCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	VruiState* thisPtr=static_cast<VruiState*>(userData);
	
	/* Remember to load the requested input graph file at the next opportune time: */
	thisPtr->loadInputGraph=true;
	thisPtr->inputGraphFileName=std::string(argumentBegin,argumentEnd);
	}

void VruiState::saveScreenshotCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	try
		{
		/* Parse the screenshot file name: */
		const char* cPtr=argumentBegin;
		std::string screenshotFileName=Misc::ValueCoder<std::string>::decode(cPtr,argumentEnd,&cPtr);
		
		/* Check for the optional window index: */
		int windowIndex=0;
		cPtr=Misc::skipWhitespace(cPtr,argumentEnd);
		if(cPtr!=argumentEnd)
			{
			/* Parse the window index: */
			windowIndex=Misc::ValueCoder<int>::decode(cPtr,argumentEnd,&cPtr);
			if(windowIndex<0||windowIndex>=getNumWindows())
				throw std::runtime_error("window index out of bounds");
			}
		
		/* Check if the window index is valid on this node: */
		if(getWindow(windowIndex)!=0)
			{
			/* Request a screenshot from the window: */
			getWindow(windowIndex)->requestScreenshot(screenshotFileName.c_str());
			}
		}
	catch(const std::runtime_error& err)
		{
		std::cout<<"saveScreenshot: Unable to save screenshot due to exception "<<err.what()<<std::endl;
		}
	}

void VruiState::quitCommandCallback(const char* argumentBegin,const char* argumentEnd,void* userData)
	{
	/* Request Vrui to shut down cleanly: */
	shutdown();
	}

void VruiState::dialogsMenuCallback(GLMotif::Button::SelectCallbackData* cbData,GLMotif::PopupWindow* const& dialog)
	{
	/* Check if the dialog is visible or hidden: */
	GLMotif::WidgetManager* wm=getWidgetManager();
	if(wm->isVisible(dialog))
		{
		/* Initialize the pop-up position: */
		Point hotSpot=uiManager->getHotSpot();
		
		/* Move the dialog window to the hot spot position: */
		ONTransform transform=uiManager->calcUITransform(hotSpot);
		transform*=ONTransform::translate(-Vector(dialog->calcHotSpot().getXyzw()));
		wm->setPrimaryWidgetTransformation(dialog,transform);
		}
	else
		{
		/* Show the hidden dialog window at its previous position: */
		wm->show(dialog);
		}
	}

void VruiState::widgetPopCallback(GLMotif::WidgetManager::WidgetPopCallbackData* cbData)
	{
	/* Don't do anything if there is no dialogs menu yet: */
	if(dialogsMenu==0)
		return;
	
	/* Check if the widget is a dialog: */
	GLMotif::PopupWindow* dialog=dynamic_cast<GLMotif::PopupWindow*>(cbData->topLevelWidget);
	if(dialog==0)
		return;
	
	if(cbData->popup)
		{
		/* Append the newly popped-up dialog to the dialogs menu: */
		GLMotif::Button* button=dialogsMenu->addEntry(dialog->getTitleString());
		button->getSelectCallbacks().add(this,&VruiState::dialogsMenuCallback,dialog);
		poppedDialogs.push_back(dialog);
		
		/* Enable the dialogs menu if it has become non-empty: */
		if(dialogsMenu->getNumEntries()==1)
			{
			/* Enable the "Dialogs" cascade button: */
			dialogsMenuCascade->setEnabled(true);
			}
		}
	else
		{
		/* Find the popped-down dialog in the dialogs menu: */
		int menuIndex=0;
		std::vector<GLMotif::PopupWindow*>::iterator dIt;
		for(dIt=poppedDialogs.begin();dIt!=poppedDialogs.end()&&*dIt!=dialog;++dIt,++menuIndex)
			;
		if(dIt!=poppedDialogs.end())
			{
			/* Remove the popped-down dialog from the dialogs menu and delete the button widget: */
			poppedDialogs.erase(dIt);
			delete dialogsMenu->removeEntry(menuIndex);
			
			/* Disable the dialogs menu if it has become empty: */
			if(dialogsMenu->getNumEntries()==0)
				{
				/* Disable the "Dialogs" cascade button: */
				dialogsMenuCascade->setEnabled(false);
				}
			}
		}
	}

void VruiState::loadViewCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Load the selected file only if there are no active navigation tools: */
	if(activeNavigationTool==0)
		{
		/* Load the selected viewpoint file: */
		loadViewpointFile(*cbData->selectedDirectory,cbData->selectedFileName);
		}
	}

void VruiState::saveViewCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Write the viewpoint file: */
	IO::FilePtr viewpointFile(cbData->selectedDirectory->openFile(cbData->selectedFileName,IO::File::WriteOnly));
	viewpointFile->setEndianness(Misc::LittleEndian);
	
	/* Write a header identifying this as an environment-independent viewpoint file: */
	viewpointFile->write<char>(vruiViewpointFileHeader,strlen(vruiViewpointFileHeader));
	
	/* Write the environment's center point in navigational coordinates: */
	Point center=getInverseNavigationTransformation().transform(getDisplayCenter());
	viewpointFile->write<Scalar>(center.getComponents(),3);
	
	/* Write the environment's size in navigational coordinates: */
	Scalar size=getDisplaySize()*getInverseNavigationTransformation().getScaling();
	viewpointFile->write<Scalar>(size);
	
	/* Write the environment's forward direction in navigational coordinates: */
	Vector forward=getInverseNavigationTransformation().transform(getForwardDirection());
	viewpointFile->write<Scalar>(forward.getComponents(),3);
	
	/* Write the environment's up direction in navigational coordinates: */
	Vector up=getInverseNavigationTransformation().transform(getUpDirection());
	viewpointFile->write<Scalar>(up.getComponents(),3);
	}

void VruiState::resetViewCallback(Misc::CallbackData* cbData)
	{
	/* Call the application-supplied navigation reset function if no navigation tools are active: */
	if(activeNavigationTool==0&&resetNavigationFunction!=0)
		(*resetNavigationFunction)(resetNavigationFunctionData);
	}

void VruiState::alignViewCallback(Misc::CallbackData* cbData)
	{
	/* Only align if no navigation tools are active: */
	if(activeNavigationTool==0)
		{
		/* Convert the callback to the correct type: */
		GLMotif::Button::SelectCallbackData* myCbData=static_cast<GLMotif::Button::SelectCallbackData*>(cbData);
		
		/* Get a pointer to the popup menu containing the button: */
		GLMotif::PopupMenu* menu=dynamic_cast<GLMotif::PopupMenu*>(myCbData->button->getParent()->getParent());
		if(menu!=0)
			{
			/* Get the position of the display center in navigational coordinates and the current navigational scale: */
			Point navCenter=inverseNavigationTransformation.transform(displayCenter);
			Scalar navScale=navigationTransformation.getScaling();
			
			/* Get the environment's horizontal and vertical axes in physical and navigational space: */
			Vector h=forwardDirection^upDirection;
			Vector hNav=inverseNavigationTransformation.transform(h);
			Vector v=upDirection;
			Vector vNav=inverseNavigationTransformation.transform(v);
			
			/* Calculate a rotation from (x, y) to (h, v): */
			Rotation baseRot=Rotation::fromBaseVectors(h,v);
			
			/* Navigate according to the index of the button in the sub-menu: */
			NavTransform nav;
			int entryIndex=menu->getEntryIndex(myCbData->button);
			switch(entryIndex)
				{
				case 0: // X - Y
					nav=NavTransform::translateFromOriginTo(displayCenter);
					nav*=NavTransform::scale(navScale);
					nav*=NavTransform::rotate(baseRot);
					nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Vector(1,0,0),Vector(0,1,0))));
					nav*=NavTransform::translateToOriginFrom(navCenter);
					break;
				
				case 1: // X - Z
					nav=NavTransform::translateFromOriginTo(displayCenter);
					nav*=NavTransform::scale(navScale);
					nav*=NavTransform::rotate(baseRot);
					nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Vector(1,0,0),Vector(0,0,1))));
					nav*=NavTransform::translateToOriginFrom(navCenter);
					break;
				
				case 2: // Y - Z
					nav=NavTransform::translateFromOriginTo(displayCenter);
					nav*=NavTransform::scale(navScale);
					nav*=NavTransform::rotate(baseRot);
					nav*=NavTransform::rotate(Geometry::invert(Rotation::fromBaseVectors(Vector(0,1,0),Vector(0,0,1))));
					nav*=NavTransform::translateToOriginFrom(navCenter);
					break;
				
				case 3: // X Up/Down
				case 4: // Y Up/Down
				case 5: // Z Up/Down
					{
					/* Set up the direction vector that is supposed to align with "up": */
					Vector navUp=Vector::zero;
					navUp[entryIndex-3]=Scalar(1);
					
					/* Choose the direction that's more closely aligned with the current up direction: */
					if(navUp*vNav<Scalar(0))
						navUp=-navUp;
					
					/* Rotate around the display center to align the "up" direction with the up direction: */
					nav=navigationTransformation;
					nav*=NavTransform::rotateAround(navCenter,Rotation::rotateFromTo(navUp,vNav));
					break;
					}
				
				case 6: // Flip H
					/* Rotate 180 degrees around the vertical axis: */
					nav=navigationTransformation;
					nav*=NavTransform::rotateAround(navCenter,Rotation::rotateAxis(vNav,Math::rad(Scalar(180))));
					break;
				
				case 7: // Flip V
					/* Rotate 180 degrees around the horizontal axis: */
					nav=navigationTransformation;
					nav*=NavTransform::rotateAround(navCenter,Rotation::rotateAxis(hNav,Math::rad(Scalar(180))));
					break;
				
				case 8: // Rotate CCW
					/* Rotate 90 degrees around the h^v axis: */
					nav=navigationTransformation;
					nav*=NavTransform::rotateAround(navCenter,Rotation::rotateAxis(hNav^vNav,Math::rad(Scalar(90))));
					break;
				
				case 9: // Rotate CW
					/* Rotate -90 degrees around the h^v axis: */
					nav=navigationTransformation;
					nav*=NavTransform::rotateAround(navCenter,Rotation::rotateAxis(hNav^vNav,Math::rad(Scalar(-90))));
					break;
				}
			
			/* Set the new navigation transformation: */
			setNavigationTransformation(nav);
			}
		}
	}

void VruiState::undoViewCallback(Misc::CallbackData* cbData)
	{
	/* Don't undo navigation if there is an active navigation tool: */
	if(activeNavigationTool==0)
		{
		/* Move the current undo buffer slot to the previous navigation transformation: */
		--navigationUndoCurrent;
		
		/* Set the navigation transformation: */
		setNavigationTransformation(*navigationUndoCurrent);
		
		/* Disable the undo button if there are no more undos and enable the redo button: */
		undoViewButton->setEnabled(navigationUndoCurrent!=navigationUndoBuffer.begin());
		redoViewButton->setEnabled(true);
		}
	}

void VruiState::redoViewCallback(Misc::CallbackData* cbData)
	{
	/* Don't redo navigation if there is an active navigation tool: */
	if(activeNavigationTool==0)
		{
		/* Move the current undo buffer slot to the next navigation transformation: */
		++navigationUndoCurrent;
		
		/* Set the navigation transformation: */
		setNavigationTransformation(*navigationUndoCurrent);
		
		/* Enable the undo button and disable the redo button if there are no more redos: */
		Misc::RingBuffer<NavTransform>::iterator lastIt=navigationUndoBuffer.end();
		--lastIt;
		undoViewButton->setEnabled(true);
		redoViewButton->setEnabled(navigationUndoCurrent!=lastIt);
		}
	}

void VruiState::createInputDeviceCallback(Misc::CallbackData* cbData,const int& numButtons)
	{
	/* Create a new one-button virtual input device: */
	createdVirtualInputDevices.push_back(addVirtualInputDevice("VirtualInputDevice",numButtons,0));
	}

void VruiState::destroyInputDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Destroy the oldest virtual input device: */
	if(!createdVirtualInputDevices.empty())
		{
		inputDeviceManager->destroyInputDevice(createdVirtualInputDevices.front());
		createdVirtualInputDevices.pop_front();
		}
	}

void VruiState::loadInputGraphCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Remember to load the given input graph file at the next opportune time: */
	loadInputGraph=true;
	inputGraphFileName=cbData->selectedFileName;
	}

void VruiState::saveInputGraphCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Save the input graph: */
	inputGraphManager->saveInputGraph(*cbData->selectedDirectory,cbData->selectedFileName,"InputGraph");
	}

void VruiState::toolKillZoneActiveCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Set the tool kill zone's active flag: */
	getToolManager()->getToolKillZone()->setActive(cbData->set);
	}

void VruiState::showToolKillZoneCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Set the tool kill zone's render flag: */
	getToolManager()->getToolKillZone()->setRender(cbData->set);
	}

void VruiState::protectScreensCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Toggle screen protection: */
	protectScreens=cbData->set;
	if(!protectScreens)
		renderProtection=Scalar(0);
	}

void VruiState::showSettingsDialogCallback(Misc::CallbackData* cbData)
	{
	/* Pop up the settings dialog: */
	popupPrimaryWidget(settingsDialog);
	}

void VruiState::showScaleBarToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	if(cbData->set)
		{
		/* Create a new scale bar: */
		scaleBar=new ScaleBar("VruiScaleBar",getWidgetManager());
		popupPrimaryWidget(scaleBar);
		}
	else
		{
		/* Destroy the scale bar: */
		delete scaleBar;
		scaleBar=0;
		}
	}

void VruiState::quitCallback(Misc::CallbackData* cbData)
	{
	/* Request Vrui to shut down cleanly: */
	shutdown();
	}

void VruiState::navigationUnitScaleValueChangedCallback(GLMotif::TextField::ValueChangedCallbackData* cbData)
	{
	/* Create a new linear unit and set it in the coordinate manager: */
	Geometry::LinearUnit::Scalar factor(atof(cbData->value));
	if(factor>0)
		{
		Geometry::LinearUnit newUnit(coordinateManager->getUnit().unit,factor);
		coordinateManager->setUnit(newUnit);
		}
	else
		{
		/* Bad entry; reset the text field's value: */
		cbData->textField->setValue(coordinateManager->getUnit().factor);
		}
	}

void VruiState::navigationUnitValueChangedCallback(GLMotif::DropdownBox::ValueChangedCallbackData* cbData)
	{
	/* Create a new linear unit and set it in the coordinate manager: */
	Geometry::LinearUnit newUnit(Geometry::LinearUnit::Unit(cbData->newSelectedItem),coordinateManager->getUnit().factor);
	coordinateManager->setUnit(newUnit);
	}

void VruiState::ambientIntensityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the ambient light color: */
	for(int i=0;i<3;++i)
		ambientLightColor[i]=float(cbData->value);
	}

void VruiState::viewerHeadlightValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& viewerIndex)
	{
	/* Enable or disable the viewer's headlight: */
	viewers[viewerIndex].setHeadlightState(cbData->set);
	}

void VruiState::updateSunLightsource(void)
	{
	/* Set the Sun lightsources's parameters: */
	GLLight::Color sunColor(sunIntensity,sunIntensity,sunIntensity,1.0f);
	sunLightsource->getLight().diffuse=sunColor;
	sunLightsource->getLight().specular=sunColor;
	
	/* Calculate the Sun's direction vector: */
	Vector x=forwardDirection^upDirection;
	x.normalize();
	Vector y=upDirection^x;
	y.normalize();
	Scalar sa=Math::sin(Math::rad(sunAzimuth));
	Scalar ca=Math::cos(Math::rad(sunAzimuth));
	Scalar se=Math::sin(Math::rad(sunElevation));
	Scalar ce=Math::cos(Math::rad(sunElevation));
	Vector sunDir=x*(sa*ce)+y*(-ca*ce)+upDirection*se;
	sunLightsource->getLight().position=GLLight::Position(sunDir[0],sunDir[1],sunDir[2],0);
	}

void VruiState::sunValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	if(cbData->set)
		{
		/* Create a new light source: */
		sunLightsource=lightsourceManager->createLightsource(true);
		updateSunLightsource();
		sunLightsource->enable();
		}
	else
		{
		/* Destroy the Sun light source: */
		lightsourceManager->destroyLightsource(sunLightsource);
		sunLightsource=0;
		}
	
	/* Enable or disable the Sun lightsource controls: */
	sunAzimuthSlider->setEnabled(cbData->set);
	sunElevationSlider->setEnabled(cbData->set);
	sunIntensitySlider->setEnabled(cbData->set);
	}

void VruiState::sunAzimuthValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the Sun's azimuth angle: */
	sunAzimuth=float(cbData->value);
	updateSunLightsource();
	}

void VruiState::sunElevationValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the Sun's elevation angle: */
	sunElevation=float(cbData->value);
	updateSunLightsource();
	}

void VruiState::sunIntensityValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the Sun's intensity: */
	sunIntensity=float(cbData->value);
	updateSunLightsource();
	}

void VruiState::backgroundColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	/* Set the background color: */
	setBackgroundColor(cbData->newColor);
	}

void VruiState::foregroundColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	/* Set the foreground color: */
	setForegroundColor(cbData->newColor);
	}

void VruiState::backplaneValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Check if the new backplane distance is larger than the frontplane distance: */
	if(cbData->value>getFrontplaneDist())
		{
		/* Set the backplane distance: */
		setBackplaneDist(cbData->value);
		}
	else
		{
		/* Reset the slider to the current value: */
		cbData->slider->setValue(getBackplaneDist());
		}
	}

void VruiState::frontplaneValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Check if the new frontplane distance is smaller than the backplane distance: */
	if(cbData->value<getBackplaneDist())
		{
		/* Set the frontplane distance: */
		setFrontplaneDist(cbData->value);
		}
	else
		{
		/* Reset the slider to the current value: */
		cbData->slider->setValue(getFrontplaneDist());
		}
	}

void VruiState::globalGainValueChangedCallback(GLMotif::TextFieldSlider::ValueChangedCallbackData* cbData)
	{
	/* Set the main listener's gain using a decibel scale with a cut-off to muted at -30dB: */
	getMainListener()->setGain(cbData->value>-30.0?Math::pow(10.0,cbData->value/10.0):0.0);
	}

/********************************
Global Vrui kernel API functions:
********************************/

void setRandomSeed(unsigned int newRandomSeed)
	{
	vruiState->randomSeed=newRandomSeed;
	}

void vruiDelay(double interval)
	{
	#ifdef __SGI_IRIX__
	long intervalCount=(long)(interval*(double)CLK_TCK+0.5);
	while(intervalCount>0)
		intervalCount=sginap(intervalCount);
	#else
	int seconds=int(floor(interval));
	interval-=double(seconds);
	int microseconds=int(floor(interval*1000000.0+0.5));
	struct timeval tv;
	tv.tv_sec=seconds;
	tv.tv_usec=microseconds;
	select(0,0,0,0,&tv);
	#endif
	}

double peekApplicationTime(void)
	{
	/* Take an application timer snapshot: */
	double result=vruiState->appTime.peekTime();
	
	/* Check if the next frame will be delayed due to playback synchronization: */
	if(result<vruiState->synchFrameTime)
		result=vruiState->synchFrameTime;
	
	/* Check if the next frame will be delayed due to frame rate cap: */
	if(result<vruiState->lastFrame+vruiState->minimumFrameTime)
		result=vruiState->lastFrame+vruiState->minimumFrameTime;
	
	return result;
	}

void synchronize(double firstFrameTime)
	{
	vruiState->lastFrame=firstFrameTime;
	}

void synchronize(double nextFrameTime,bool wait)
	{
	vruiState->synchFrameTime=nextFrameTime;
	vruiState->synchWait=wait;
	}

void resetNavigation(void)
	{
	/* Call the application-provided reset function: */
	if(vruiState->resetNavigationFunction!=0)
		{
		(*vruiState->resetNavigationFunction)(vruiState->resetNavigationFunctionData);
		}
	}

void setDisplayCenter(const Point& newDisplayCenter,Scalar newDisplaySize)
	{
	/* Update the display center: */
	vruiState->displayCenter=newDisplayCenter;
	vruiState->displaySize=newDisplaySize;
	vruiState->navigationTransformationChangedMask|=0x2;
	
	/* Call the environment definition changed callbacks: */
	{
	EnvironmentDefinitionChangedCallbackData cbData(EnvironmentDefinitionChangedCallbackData::DisplayCenter|EnvironmentDefinitionChangedCallbackData::DisplaySize);
	vruiState->environmentDefinitionChangedCallbacks.call(&cbData);
	}
	}

void vsync(void)
	{
	/* Check if vsync prediction is enabled: */
	if(vruiState->predictVsync)
		{
		/* Get the current time: */
		Realtime::TimePointMonotonic now;
		
		/* Check if vsync occurred earlier than predicted: */
		if(vruiState->nextVsync>=now)
			{
			/* Correct the prediction: */
			vruiState->nextVsync=now;
			++vruiState->numVsyncs;
			vruiState->nextVsync+=vruiState->vsyncInterval;
			}
		else
			{
			/* Advance the prediction in frame increments until the predicted time is later than the current time: */
			while(vruiState->nextVsync<now)
				{
				++vruiState->numVsyncs;
				vruiState->nextVsync+=vruiState->vsyncInterval;
				}
			}
		}
	}

/**********************************
Call-in functions for user program:
**********************************/

void setPrepareMainLoopFunction(PrepareMainLoopFunctionType prepareMainLoopFunction,void* userData)
	{
	vruiState->prepareMainLoopFunction=prepareMainLoopFunction;
	vruiState->prepareMainLoopFunctionData=userData;
	}

void setFrameFunction(FrameFunctionType frameFunction,void* userData)
	{
	vruiState->frameFunction=frameFunction;
	vruiState->frameFunctionData=userData;
	}

void setDisplayFunction(DisplayFunctionType displayFunction,void* userData)
	{
	vruiState->displayFunction=displayFunction;
	vruiState->displayFunctionData=userData;
	}

void setSoundFunction(SoundFunctionType soundFunction,void* userData)
	{
	vruiState->soundFunction=soundFunction;
	vruiState->soundFunctionData=userData;
	}

void setResetNavigationFunction(ResetNavigationFunctionType resetNavigationFunction,void* userData)
	{
	vruiState->resetNavigationFunction=resetNavigationFunction;
	vruiState->resetNavigationFunctionData=userData;
	}

void setFinishMainLoopFunction(FinishMainLoopFunctionType finishMainLoopFunction,void* userData)
	{
	vruiState->finishMainLoopFunction=finishMainLoopFunction;
	vruiState->finishMainLoopFunctionData=userData;
	}

Cluster::Multiplexer* getClusterMultiplexer(void)
	{
	return vruiState->multiplexer;
	}

bool isMaster(void)
	{
	return vruiState->master;
	}

int getNodeIndex(void)
	{
	if(vruiState->multiplexer!=0)
		return vruiState->multiplexer->getNodeIndex();
	else
		return 0;
	}

int getNumNodes(void)
	{
	if(vruiState->multiplexer!=0)
		return vruiState->multiplexer->getNumNodes();
	else
		return 1;
	}

Cluster::MulticastPipe* getMainPipe(void)
	{
	return vruiState->pipe;
	}

Cluster::MulticastPipe* openPipe(void)
	{
	if(vruiState->multiplexer!=0)
		return new Cluster::MulticastPipe(vruiState->multiplexer);
	else
		return 0;
	}

GlyphRenderer* getGlyphRenderer(void)
	{
	return vruiState->glyphRenderer;
	}

void renderGlyph(const Glyph& glyph,const OGTransform& transformation,GLContextData& contextData)
	{
	vruiState->glyphRenderer->renderGlyph(glyph,transformation,vruiState->glyphRenderer->getContextDataItem(contextData));
	}

VirtualInputDevice* getVirtualInputDevice(void)
	{
	return vruiState->virtualInputDevice;
	}

InputGraphManager* getInputGraphManager(void)
	{
	return vruiState->inputGraphManager;
	}

InputDeviceManager* getInputDeviceManager(void)
	{
	return vruiState->inputDeviceManager;
	}

int getNumInputDevices(void)
	{
	return vruiState->inputDeviceManager->getNumInputDevices();
	}

InputDevice* getInputDevice(int index)
	{
	return vruiState->inputDeviceManager->getInputDevice(index);
	}

InputDevice* findInputDevice(const char* name)
	{
	return vruiState->inputDeviceManager->findInputDevice(name);
	}

InputDevice* addVirtualInputDevice(const char* name,int numButtons,int numValuators)
	{
	InputDevice* newDevice=vruiState->inputDeviceManager->createInputDevice(name,InputDevice::TRACK_POS|InputDevice::TRACK_DIR|InputDevice::TRACK_ORIENT,numButtons,numValuators);
	newDevice->setTransformation(TrackerState::translateFromOriginTo(vruiState->newInputDevicePosition));
	vruiState->inputGraphManager->getInputDeviceGlyph(newDevice).enable(Glyph::BOX,vruiState->widgetMaterial);
	return newDevice;
	}

LightsourceManager* getLightsourceManager(void)
	{
	return vruiState->lightsourceManager;
	}

ClipPlaneManager* getClipPlaneManager(void)
	{
	return vruiState->clipPlaneManager;
	}

Viewer* getMainViewer(void)
	{
	return vruiState->mainViewer;
	}

int getNumViewers(void)
	{
	return vruiState->numViewers;
	}

Viewer* getViewer(int index)
	{
	return &vruiState->viewers[index];
	}

Viewer* findViewer(const char* name)
	{
	Viewer* result=0;
	for(int i=0;i<vruiState->numViewers;++i)
		if(strcmp(name,vruiState->viewers[i].getName())==0)
			{
			result=&vruiState->viewers[i];
			break;
			}
	return result;
	}

VRScreen* getMainScreen(void)
	{
	return vruiState->mainScreen;
	}

int getNumScreens(void)
	{
	return vruiState->numScreens;
	}

VRScreen* getScreen(int index)
	{
	return vruiState->screens+index;
	}

VRScreen* findScreen(const char* name)
	{
	VRScreen* result=0;
	for(int i=0;i<vruiState->numScreens;++i)
		if(strcmp(name,vruiState->screens[i].getName())==0)
			{
			result=&vruiState->screens[i];
			break;
			}
	return result;
	}

std::pair<VRScreen*,Scalar> findScreen(const Ray& ray)
	{
	/* Find the closest intersection with any screen: */
	VRScreen* closestScreen=0;
	Scalar closestLambda=Math::Constants<Scalar>::max;
	for(int screenIndex=0;screenIndex<vruiState->numScreens;++screenIndex)
		if(vruiState->screens[screenIndex].isIntersect())
			{
			VRScreen* screen=&vruiState->screens[screenIndex];
			
			/* Calculate screen plane: */
			ONTransform t=screen->getScreenTransformation();
			Vector screenNormal=t.getDirection(2);
			Scalar screenOffset=screenNormal*t.getOrigin();
			
			/* Intersect selection ray with screen plane: */
			Scalar divisor=screenNormal*ray.getDirection();
			if(divisor!=Scalar(0))
				{
				Scalar lambda=(screenOffset-screenNormal*ray.getOrigin())/divisor;
				if(lambda>=Scalar(0)&&lambda<closestLambda)
					{
					/* Check if the ray intersects the screen: */
					Point screenPos=t.inverseTransform(ray.getOrigin()+ray.getDirection()*lambda);
					if(screen->isOffAxis())
						{
						/* Check the intersection point against the projected screen quadrilateral: */
						VRScreen::PTransform2::Point sp(screenPos[0],screenPos[1]);
						sp=screen->getScreenHomography().inverseTransform(sp);
						if(sp[0]>=Scalar(0)&&sp[0]<=screen->getWidth()&&sp[1]>=Scalar(0)&&sp[1]<=screen->getHeight())
							{
							/* Save the intersection: */
							closestScreen=screen;
							closestLambda=lambda;
							}
						}
					else
						{
						/* Check the intersection point against the upright screen rectangle: */
						if(screenPos[0]>=Scalar(0)&&screenPos[0]<=screen->getWidth()&&screenPos[1]>=Scalar(0)&&screenPos[1]<=screen->getHeight())
							{
							/* Save the intersection: */
							closestScreen=screen;
							closestLambda=lambda;
							}
						}
					}
				}
			}
	
	return std::pair<VRScreen*,Scalar>(closestScreen,closestLambda);
	}

void requestWindowProperties(const WindowProperties& properties)
	{
	/* Merge the given properties with the accumulated properties: */
	vruiState->windowProperties.merge(properties);
	}

Listener* getMainListener(void)
	{
	return vruiState->mainListener;
	}

int getNumListeners(void)
	{
	return vruiState->numListeners;
	}

Listener* getListener(int index)
	{
	return &vruiState->listeners[index];
	}

Listener* findListener(const char* name)
	{
	Listener* result=0;
	for(int i=0;i<vruiState->numListeners;++i)
		if(strcmp(name,vruiState->listeners[i].getName())==0)
			{
			result=&vruiState->listeners[i];
			break;
			}
	return result;
	}

void requestSound(void)
	{
	vruiState->useSound=true;
	}

Scalar getInchFactor(void)
	{
	return vruiState->inchScale;
	}

Scalar getMeterFactor(void)
	{
	return vruiState->meterScale;
	}

Scalar getDisplaySize(void)
	{
	return vruiState->displaySize;
	}

const Point& getDisplayCenter(void)
	{
	return vruiState->displayCenter;
	}

const Vector& getForwardDirection(void)
	{
	return vruiState->forwardDirection;
	}

const Vector& getUpDirection(void)
	{
	return vruiState->upDirection;
	}

const Plane& getFloorPlane(void)
	{
	return vruiState->floorPlane;
	}

Misc::CallbackList& getEnvironmentDefinitionChangedCallbacks(void)
	{
	return vruiState->environmentDefinitionChangedCallbacks;
	}

void setFrontplaneDist(Scalar newFrontplaneDist)
	{
	vruiState->frontplaneDist=newFrontplaneDist;
	}

Scalar getFrontplaneDist(void)
	{
	return vruiState->frontplaneDist;
	}

void setBackplaneDist(Scalar newBackplaneDist)
	{
	vruiState->backplaneDist=newBackplaneDist;
	}

Scalar getBackplaneDist(void)
	{
	return vruiState->backplaneDist;
	}

void setBackgroundColor(const Color& newBackgroundColor)
	{
	vruiState->backgroundColor=newBackgroundColor;
	
	/* Calculate a new contrasting foreground color: */
	for(int i=0;i<3;++i)
		vruiState->foregroundColor[i]=1.0f-newBackgroundColor[i];
	vruiState->foregroundColor[3]=1.0f;
	}

void setForegroundColor(const Color& newForegroundColor)
	{
	vruiState->foregroundColor=newForegroundColor;
	}

const Color& getBackgroundColor(void)
	{
	return vruiState->backgroundColor;
	}

const Color& getForegroundColor(void)
	{
	return vruiState->foregroundColor;
	}

GLFont* loadFont(const char* fontName)
	{
	return new GLFont(fontName);
	}

const GLMotif::StyleSheet* getUiStyleSheet(void)
	{
	return &vruiState->uiStyleSheet;
	}

float getUiSize(void)
	{
	return vruiState->uiStyleSheet.size;
	}

const Color& getUiBgColor(void)
	{
	return vruiState->uiStyleSheet.bgColor;
	}

const Color& getUiFgColor(void)
	{
	return vruiState->uiStyleSheet.fgColor;
	}

const Color& getUiTextFieldBgColor(void)
	{
	return vruiState->uiStyleSheet.textfieldBgColor;
	}

const Color& getUiTextFieldFgColor(void)
	{
	return vruiState->uiStyleSheet.textfieldFgColor;
	}

GLFont* getUiFont(void)
	{
	return vruiState->uiStyleSheet.font;
	}

void setWidgetMaterial(const GLMaterial& newWidgetMaterial)
	{
	vruiState->widgetMaterial=newWidgetMaterial;
	}

const GLMaterial& getWidgetMaterial(void)
	{
	return vruiState->widgetMaterial;
	}

void setMainMenu(GLMotif::PopupMenu* newMainMenu)
	{
	/* Delete old main menu shell and system menu popup: */
	delete vruiState->mainMenu;
	delete vruiState->systemMenu;
	vruiState->systemMenu=0;
	
	/* Add the Vrui system menu to the end of the given main menu: */
	if(newMainMenu->getMenu()!=0)
		{
		/* Create the Vrui system menu as a dependent pop-up: */
		vruiState->systemMenu=new GLMotif::PopupMenu("VruiSystemMenu",vruiState->widgetManager);
		vruiState->buildSystemMenu(vruiState->systemMenu);
		vruiState->systemMenu->manageMenu();
		vruiState->systemMenuTopLevel=false;
		
		/* Create a cascade button at the end of the main menu: */
		newMainMenu->addSeparator();
		
		GLMotif::CascadeButton* systemMenuCascade=new GLMotif::CascadeButton("VruiSystemMenuCascade",newMainMenu,"Vrui System");
		systemMenuCascade->setPopup(vruiState->systemMenu);
		}
	
	/* Create new main menu shell: */
	vruiState->mainMenu=new MutexMenu(newMainMenu);
	}

MutexMenu* getMainMenu(void)
	{
	return vruiState->mainMenu;
	}

GLMotif::Pager* getSettingsPager(void)
	{
	/* Return the settings pager: */
	return vruiState->settingsPager;
	}

GLMotif::Button* addShowSettingsDialogButton(const char* buttonLabel)
	{
	/* Find the quit button separator in the system menu: */
	GLMotif::RowColumn* menu=vruiState->systemMenu->getMenu();
	GLint separatorIndex=menu->getChildIndex(vruiState->quitSeparator);
	if(separatorIndex>=0)
		{
		/* Insert a new button at the index of the quit button separator: */
		menu->setNextChildIndex(separatorIndex);
		return vruiState->systemMenu->addEntry(buttonLabel);
		}
	else
		return 0;
	}

void removeShowSettingsDialogButton(GLMotif::Button* button)
	{
	/* Remove the button from the system menu and delete it: */
	vruiState->systemMenu->removeEntry(button);
	delete button;
	}

Misc::TimerEventScheduler* getTimerEventScheduler(void)
	{
	return vruiState->timerEventScheduler;
	}

TextEventDispatcher* getTextEventDispatcher(void)
	{
	return vruiState->textEventDispatcher;
	}

GLMotif::WidgetManager* getWidgetManager(void)
	{
	return vruiState->widgetManager;
	}

UIManager* getUiManager(void)
	{
	return vruiState->uiManager;
	}

void popupPrimaryWidget(GLMotif::Widget* topLevel)
	{
	/* Check if the widget is already popped up: */
	GLMotif::WidgetManager* wm=getWidgetManager();
	if(wm->isManaged(topLevel))
		{
		/* Check if the widget is visible or hidden: */
		if(wm->isVisible(topLevel))
			{
			/* Initialize the pop-up position: */
			Point hotSpot=vruiState->uiManager->getHotSpot();
			
			/* Move the widget to the hot spot position: */
			ONTransform transform=vruiState->uiManager->calcUITransform(hotSpot);
			transform*=ONTransform::translate(-Vector(topLevel->calcHotSpot().getXyzw()));
			wm->setPrimaryWidgetTransformation(topLevel,transform);
			}
		else
			{
			/* Show the hidden widget at its previous position: */
			wm->show(topLevel);
			}
		}
	else
		{
		/* Forward call to the widget manager: */
		wm->popupPrimaryWidget(topLevel);
		}
	}

void popupPrimaryWidget(GLMotif::Widget* topLevel,const Point& hotSpot,bool navigational)
	{
	/* Calculate the hot spot in physical coordinates: */
	Point globalHotSpot=hotSpot;
	if(navigational&&vruiState->navigationTransformationEnabled)
		globalHotSpot=vruiState->inverseNavigationTransformation.transform(globalHotSpot);
	
	/* Forward call to widget manager: */
	vruiState->widgetManager->popupPrimaryWidget(topLevel,globalHotSpot);
	}

void popupPrimaryScreenWidget(GLMotif::Widget* topLevel,Scalar x,Scalar y)
	{
	typedef GLMotif::WidgetManager::Transformation WTransform;
	typedef WTransform::Vector WVector;
	
	/* Calculate a transformation moving the widget to its given position on the screen: */
	Scalar screenX=x*(vruiState->mainScreen->getWidth()-topLevel->getExterior().size[0]);
	Scalar screenY=y*(vruiState->mainScreen->getHeight()-topLevel->getExterior().size[1]);
	WTransform widgetTransformation=vruiState->mainScreen->getTransform();
	widgetTransformation*=WTransform::translate(WVector(screenX,screenY,vruiState->inchScale));
	
	/* Pop up the widget: */
	vruiState->widgetManager->popupPrimaryWidget(topLevel,widgetTransformation);
	}

void popdownPrimaryWidget(GLMotif::Widget* topLevel)
	{
	/* Pop down the widget: */
	vruiState->widgetManager->popdownWidget(topLevel);
	}

namespace {

/**************************************
Helper function to close error dialogs:
**************************************/

void closeWindowCallback(Misc::CallbackData* cbData,void*)
	{
	/* Check if the callback came from a button: */
	GLMotif::Button::CallbackData* buttonCbData=dynamic_cast<GLMotif::Button::CallbackData*>(cbData);
	if(buttonCbData!=0)
		{
		/* Close the top-level widget to which the button belongs: */
		getWidgetManager()->deleteWidget(buttonCbData->button->getRoot());
		}
	
	/* Check if the callback came from a popup window: */
	GLMotif::PopupWindow::CallbackData* windowCbData=dynamic_cast<GLMotif::PopupWindow::CallbackData*>(cbData);
	if(windowCbData!=0)
		{
		/* Close the popup window: */
		getWidgetManager()->deleteWidget(windowCbData->popupWindow);
		}
	}

}

void showErrorMessage(const char* title,const char* message,const char* buttonLabel)
	{
	/* Create a popup window: */
	GLMotif::PopupWindow* errorDialog=new GLMotif::PopupWindow("VruiErrorMessage",getWidgetManager(),title);
	errorDialog->setResizableFlags(false,false);
	errorDialog->setHideButton(false);
	
	GLMotif::RowColumn* error=new GLMotif::RowColumn("Error",errorDialog,false);
	error->setOrientation(GLMotif::RowColumn::VERTICAL);
	error->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	/* Skip initial whitespace in the error message: */
	const char* linePtr=message;
	while(isspace(*linePtr)&&*linePtr!='\0')
		++linePtr;
	
	/* Break the error message into multiple lines: */
	while(*linePtr!='\0')
		{
		/* Find potential line break points: */
		const char* breakPtr=0;
		const char* cPtr=linePtr;
		do
			{
			/* Find the end of the current word: */
			while(!isspace(*cPtr)&&*cPtr!='-'&&*cPtr!='/'&&*cPtr!='\0')
				++cPtr;
			
			/* Skip past dashes and slashes: */
			while(*cPtr=='-'||*cPtr=='/')
				++cPtr;
			
			/* If the line is already too long, and there is a previous break point, break there: */
			if(cPtr-linePtr>=40&&breakPtr!=0)
				break;
			
			/* Mark the break point: */
			breakPtr=cPtr;
			
			/* Skip whitespace: */
			while(isspace(*cPtr)&&*cPtr!='\0')
				++cPtr;
			}
		while(cPtr-linePtr<40&&*breakPtr!='\n'&&*breakPtr!='\0');
		
		/* Add the current line: */
		new GLMotif::Label("ErrorLine",error,linePtr,breakPtr);
		
		/* Go to the beginning of the next line: */
		linePtr=breakPtr;
		while(isspace(*linePtr)&&*linePtr!='\0')
			++linePtr;
		}
	
	/* Add an acknowledgment button: */
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",error,false);
	buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
	GLMotif::Button* okButton=new GLMotif::Button("OkButton",buttonMargin,buttonLabel!=0?buttonLabel:"Too Sad!");
	okButton->getSelectCallbacks().add(closeWindowCallback,0);
	
	buttonMargin->manageChild();
	
	error->manageChild();
	
	/* Show the popup window: */
	popupPrimaryWidget(errorDialog);
	}

Scalar getPointPickDistance(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->pointPickDistance*vruiState->inverseNavigationTransformation.getScaling();
	else
		return vruiState->pointPickDistance;
	}

Scalar getRayPickCosine(void)
	{
	return vruiState->rayPickCosine;
	}

void setNavigationTransformation(const NavTransform& newNavigationTransformation)
	{
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=newNavigationTransformation;
		vruiState->newNavigationTransformation.renormalize();
		if(!vruiState->navigationTransformationEnabled||vruiState->newNavigationTransformation!=vruiState->navigationTransformation)
			{
			vruiState->navigationTransformationChangedMask|=0x1;
			requestUpdate();	
			}
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->updateNavigationTransformation(newNavigationTransformation);
		}
	#else
	/* Change the navigation transformation right away: */
	vruiState->updateNavigationTransformation(newNavigationTransformation);
	#endif
	
	vruiState->navigationTransformationEnabled=true;
	}

void setNavigationTransformation(const Point& center,Scalar radius)
	{
	/* Assemble the new navigation transformation: */
	NavTransform t=NavTransform::translateFromOriginTo(vruiState->displayCenter);
	t*=NavTransform::scale(vruiState->displaySize/radius);
	t*=NavTransform::translateToOriginFrom(center);
	
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=t;
		if(!vruiState->navigationTransformationEnabled||vruiState->newNavigationTransformation!=vruiState->navigationTransformation)
			{
			vruiState->navigationTransformationChangedMask|=0x1;
			requestUpdate();	
			}
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->updateNavigationTransformation(t);
		}
	#else
	/* Change the navigation transformation right away: */
	vruiState->updateNavigationTransformation(t);
	#endif
	
	vruiState->navigationTransformationEnabled=true;
	}

void setNavigationTransformation(const Point& center,Scalar radius,const Vector& up)
	{
	/* Assemble the new navigation transformation: */
	NavTransform t=NavTransform::translateFromOriginTo(vruiState->displayCenter);
	t*=NavTransform::scale(vruiState->displaySize/radius);
	t*=NavTransform::rotate(NavTransform::Rotation::rotateFromTo(up,vruiState->upDirection));
	t*=NavTransform::translateToOriginFrom(center);
	
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		vruiState->newNavigationTransformation=t;
		if(!vruiState->navigationTransformationEnabled||vruiState->newNavigationTransformation!=vruiState->navigationTransformation)
			{
			vruiState->navigationTransformationChangedMask|=0x1;
			requestUpdate();	
			}
		}
	else
		{
		/* Change the navigation transformation right away: */
		vruiState->updateNavigationTransformation(t);
		}
	#else
	/* Change the navigation transformation right away: */
	vruiState->updateNavigationTransformation(t);
	#endif
	
	vruiState->navigationTransformationEnabled=true;
	}

void concatenateNavigationTransformation(const NavTransform& t)
	{
	/* Bail out if the incremental transformation is the identity transformation: */
	if(t==NavTransform::identity)
		return;
	
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		if((vruiState->navigationTransformationChangedMask&0x1)==0)
			vruiState->newNavigationTransformation=vruiState->navigationTransformation;
		vruiState->newNavigationTransformation*=t;
		vruiState->newNavigationTransformation.renormalize();
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		NavTransform newTransform=vruiState->navigationTransformation;
		newTransform*=t;
		newTransform.renormalize();
		vruiState->updateNavigationTransformation(newTransform);
		}
	#else
	/* Change the navigation transformation right away: */
	NavTransform newTransform=vruiState->navigationTransformation;
	newTransform*=t;
	newTransform.renormalize();
	vruiState->updateNavigationTransformation(newTransform);
	#endif
	}

void concatenateNavigationTransformationLeft(const NavTransform& t)
	{
	/* Bail out if the incremental transformation is the identity transformation: */
	if(t==NavTransform::identity)
		return;
	
	#if DELAY_NAVIGATIONTRANSFORMATION
	if(vruiState->delayNavigationTransformation)
		{
		/* Schedule a change in navigation transformation for the next frame: */
		if((vruiState->navigationTransformationChangedMask&0x1)==0)
			vruiState->newNavigationTransformation=vruiState->navigationTransformation;
		vruiState->newNavigationTransformation.leftMultiply(t);
		vruiState->newNavigationTransformation.renormalize();
		vruiState->navigationTransformationChangedMask|=0x1;
		requestUpdate();
		}
	else
		{
		/* Change the navigation transformation right away: */
		NavTransform newTransform=vruiState->navigationTransformation;
		newTransform.leftMultiply(t);
		newTransform.renormalize();
		vruiState->updateNavigationTransformation(newTransform);
		}
	#else
	/* Change the navigation transformation right away: */
	NavTransform newTransform=vruiState->navigationTransformation;
	newTransform.leftMultiply(t);
	newTransform.renormalize();
	vruiState->updateNavigationTransformation(newTransform);
	#endif
	}

const NavTransform& getNavigationTransformation(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->navigationTransformation;
	else
		return NavTransform::identity;
	}

const NavTransform& getInverseNavigationTransformation(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation;
	else
		return NavTransform::identity;
	}

void disableNavigationTransformation(void)
	{
	/* Disable and reset the navigation transformation: */
	vruiState->navigationTransformationEnabled=false;
	vruiState->updateNavigationTransformation(NavTransform::identity);
	}

Point getHeadPosition(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(vruiState->mainViewer->getHeadPosition());
	else
		return vruiState->mainViewer->getHeadPosition();
	}

Vector getViewDirection(void)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(vruiState->mainViewer->getViewDirection());
	else
		return vruiState->mainViewer->getViewDirection();
	}

Point getDevicePosition(InputDevice* device)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation.transform(device->getPosition());
	else
		return device->getPosition();
	}

NavTrackerState getDeviceTransformation(InputDevice* device)
	{
	if(vruiState->navigationTransformationEnabled)
		return vruiState->inverseNavigationTransformation*NavTransform(device->getTransformation());
	else
		return device->getTransformation();
	}

Misc::CallbackList& getNavigationTransformationChangedCallbacks(void)
	{
	return vruiState->navigationTransformationChangedCallbacks;
	}

CoordinateManager* getCoordinateManager(void)
	{
	return vruiState->coordinateManager;
	}

ToolManager* getToolManager(void)
	{
	return vruiState->toolManager;
	}

Misc::CallbackList& getNavigationToolActivationCallbacks(void)
	{
	return vruiState->navigationToolActivationCallbacks;
	}

bool activateNavigationTool(const Tool* tool)
	{
	/* Can not activate the given tool if navigation is disabled: */
	if(!vruiState->navigationTransformationEnabled)
		return false;
	
	/* Can not activate the given tool if another navigation tool is already active: */
	if(vruiState->activeNavigationTool!=0&&vruiState->activeNavigationTool!=tool)
		return false;
	
	if(tool!=0&&vruiState->activeNavigationTool==0)
		{
		/* Call the navigation tool activation callbacks: */
		NavigationToolActivationCallbackData cbData(true);
		vruiState->navigationToolActivationCallbacks.call(&cbData);
		}
	
	/* Activate the given tool: */
	vruiState->activeNavigationTool=tool;
	return true;
	}

void deactivateNavigationTool(const Tool* tool)
	{
	/* Check if the given tool is currently active: */
	if(vruiState->activeNavigationTool==tool)
		{
		if(vruiState->activeNavigationTool!=0)
			{
			/* Call the navigation tool activation callbacks: */
			NavigationToolActivationCallbackData cbData(false);
			vruiState->navigationToolActivationCallbacks.call(&cbData);
			
			/* Push the current navigation transformation into the navigation undo buffer: */
			vruiState->pushNavigationTransformation();
			}
		
		/* Deactivate the given tool: */
		vruiState->activeNavigationTool=0;
		}
	}

VisletManager* getVisletManager(void)
	{
	return vruiState->visletManager;
	}

Misc::Time getTimeOfDay(void)
	{
	Misc::Time result;
	
	if(vruiState->master)
		{
		/* Query the system's wall clock time: */
		result=Misc::Time::now();
		
		if(vruiState->multiplexer!=0)
			{
			/* Send the time value to the slaves: */
			vruiState->pipe->write(result.tv_sec);
			vruiState->pipe->write(result.tv_nsec);
			vruiState->pipe->flush();
			}
		}
	else
		{
		/* Receive the time value from the master: */
		vruiState->pipe->read(result.tv_sec);
		vruiState->pipe->read(result.tv_nsec);
		}
	
	return result;
	}

double getApplicationTime(void)
	{
	return vruiState->lastFrame;
	}

double getFrameTime(void)
	{
	return vruiState->lastFrameDelta;
	}

double getCurrentFrameTime(void)
	{
	return vruiState->currentFrameTime;
	}

double getNextAnimationTime(void)
	{
	return vruiState->lastFrame+vruiState->animationFrameInterval;
	}

void addFrameCallback(FrameCallback newFrameCallback,void* newFrameCallbackUserData)
	{
	Threads::Mutex::Lock frameCallbacksLock(vruiState->frameCallbacksMutex);
	
	/* Check if the callback is already in the list: */
	for(std::vector<VruiState::FrameCallbackSlot>::iterator fcIt=vruiState->frameCallbacks.begin();fcIt!=vruiState->frameCallbacks.end();++fcIt)
		if(fcIt->callback==newFrameCallback&&fcIt->userData==newFrameCallbackUserData)
			{
			/* Callback already exists; bail out: */
			return;
			}
	
	/* Add the callback to the list: */
	VruiState::FrameCallbackSlot fcs;
	fcs.callback=newFrameCallback;
	fcs.userData=newFrameCallbackUserData;
	vruiState->frameCallbacks.push_back(fcs);
	}

Misc::CommandDispatcher& getCommandDispatcher(void)
	{
	return vruiState->commandDispatcher;
	}

void updateContinuously(void)
	{
	vruiState->updateContinuously=true;
	}

void scheduleUpdate(double nextFrameTime)
	{
	if(vruiState->nextFrameTime==0.0||vruiState->nextFrameTime>nextFrameTime)
		vruiState->nextFrameTime=nextFrameTime;
	}

const DisplayState& getDisplayState(GLContextData& contextData)
	{
	/* Retrieve the display state mapper's data item from the OpenGL context: */
	VruiState::DisplayStateMapper::DataItem* dataItem=contextData.retrieveDataItem<VruiState::DisplayStateMapper::DataItem>(&vruiState->displayStateMapper);
	
	/* Return the embedded display state object: */
	return dataItem->displayState;
	}

void goToNavigationalSpace(GLContextData& contextData)
	{
	/* Push the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	/* Retrieve the display state mapper's data item from the OpenGL context: */
	VruiState::DisplayStateMapper::DataItem* dataItem=contextData.retrieveDataItem<VruiState::DisplayStateMapper::DataItem>(&vruiState->displayStateMapper);
	
	/* Load the navigational-space modelview matrix: */
	glLoadMatrix(dataItem->displayState.modelviewNavigational);
	}

void goToPhysicalSpace(GLContextData& contextData)
	{
	/* Push the modelview matrix: */
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	/* Retrieve the display state mapper's data item from the OpenGL context: */
	VruiState::DisplayStateMapper::DataItem* dataItem=contextData.retrieveDataItem<VruiState::DisplayStateMapper::DataItem>(&vruiState->displayStateMapper);
	
	/* Load the physical-space modelview matrix: */
	glLoadMatrix(dataItem->displayState.modelviewPhysical);
	}

void inhibitScreenSaver(void)
	{
	if(vruiState->screenSaverInhibitor==0)
		{
		#if VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS
		try
			{
			vruiState->screenSaverInhibitor=new ScreenSaverInhibitorDBus;
			}
		catch(const std::runtime_error& err)
			{
			Misc::formattedConsoleWarning("Vrui: Unable to inhibit screen saver due to exception %s",err.what());
			}
		#else
		Misc::consoleWarning("Vrui: Screen saver inhibition not supported");
		#endif
		}
	}

void uninhibitScreenSaver(void)
	{
	if(vruiState->screenSaverInhibitor!=0)
		{
		delete vruiState->screenSaverInhibitor;
		vruiState->screenSaverInhibitor=0;
		}
	}

#if EVILHACK_LOCK_INPUTDEVICE_POS

void lockDevice(Vrui::InputDevice* device)
	{
	lockedDevice=device;
	if(lockedDevice!=0)
		lockedTranslation=lockedDevice->getTransformation().getTranslation();
	}
	
#endif

}
