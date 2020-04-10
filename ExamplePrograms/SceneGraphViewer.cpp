/***********************************************************************
SceneGraphViewer - Viewer for one or more scene graphs loaded from VRML
2.0 files.
Copyright (c) 2010-2020 Oliver Kreylos

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

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Geometry/ComponentArray.h>
#include <Geometry/Point.h>
#include <Geometry/OrthogonalTransformation.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/Button.h>
#include <GLMotif/ToggleButton.h>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/SceneGraphSupport.h>

class SceneGraphViewer:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	struct SGItem // Structure for scene graph list items
		{
		/* Elements: */
		public:
		std::string fileName; // Name of file from which the scene graph was loaded
		SceneGraph::GroupNodePointer root; // Pointer to the scene graph's root node
		bool navigational; // Flag whether the scene graph is in navigational coordinates
		bool enabled; // Flag whether the scene graph is currently enabled
		};
	
	/* Elements: */
	private:
	std::vector<SGItem> sceneGraphs; // List of loaded scene graphs
	GLMotif::PopupMenu* mainMenu;
	
	/* Private methods: */
	void goToPhysicalSpaceCallback(Misc::CallbackData* cbData);
	void sceneGraphToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& index);
	void reloadAllSceneGraphsCallback(Misc::CallbackData* cbData);
	
	/* Constructors and destructors: */
	public:
	SceneGraphViewer(int& argc,char**& argv);
	~SceneGraphViewer(void);
	
	/* Methods: */
	virtual void display(GLContextData& contextData) const;
	virtual void resetNavigation(void);
	};

/*********************************
Methods of class SceneGraphViewer:
*********************************/

void SceneGraphViewer::goToPhysicalSpaceCallback(Misc::CallbackData* cbData)
	{
	Vrui::setNavigationTransformation(Vrui::NavTransform::identity);
	}

void SceneGraphViewer::sceneGraphToggleCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData,const int& index)
	{
	/* Enable or disable the selected scene graph: */
	sceneGraphs[index].enabled=cbData->set;
	}

void SceneGraphViewer::reloadAllSceneGraphsCallback(Misc::CallbackData* cbData)
	{
	/* Create a node creator to parse the VRML files: */
	SceneGraph::NodeCreator nodeCreator;
	
	int index=0;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt,++index)
		{
		/* Delete the current scene graph: */
		bool enableOnLoad=sgIt->root==0||sgIt->enabled;
		sgIt->root=0;
		sgIt->enabled=false;
		
		/* Try reloading the scene graph: */
		try
			{
			/* Create the new scene graph's root node: */
			SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
			
			/* Load and parse the VRML file: */
			SceneGraph::VRMLFile vrmlFile(sgIt->fileName.c_str(),nodeCreator);
			vrmlFile.parse(root);
			
			/* Add the new scene graph to the list: */
			sgIt->root=root;
			sgIt->enabled=enableOnLoad;
			}
		catch(std::runtime_error err)
			{
			/* Print an error message and try the next file: */
			Misc::formattedUserWarning("Scene Graph Viewer: Ignoring file %s due to exception %s",sgIt->fileName.c_str(),err.what());
			}
		
		/* Update the state of the menu button associated with this scene graph: */
		GLMotif::ToggleButton* sceneGraphToggle=dynamic_cast<GLMotif::ToggleButton*>(mainMenu->getEntry(index));
		if(sceneGraphToggle!=0)
			sceneGraphToggle->setToggle(sgIt->enabled);
		}
	}

SceneGraphViewer::SceneGraphViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 mainMenu(0)
	{
	/* Create a node creator to parse the VRML files: */
	SceneGraph::NodeCreator nodeCreator;
	
	/* Parse the command line: */
	bool navigational=true;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"navigational")==0||strcasecmp(argv[i]+1,"n")==0)
				navigational=true;
			else if(strcasecmp(argv[i]+1,"physical")==0||strcasecmp(argv[i]+1,"p")==0)
				navigational=false;
			}
		else
			{
			/* Create a scene graph item for the given file name: */
			sceneGraphs.push_back(SGItem());
			SGItem& sg=sceneGraphs.back();
			sg.fileName=argv[i];
			sg.navigational=navigational;
			sg.enabled=false;
			
			/* Try loading the scene graph: */
			try
				{
				/* Create the new scene graph's root node: */
				SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
				
				/* Load and parse the VRML file: */
				SceneGraph::VRMLFile vrmlFile(sg.fileName.c_str(),nodeCreator);
				vrmlFile.parse(root);
				
				/* Add the new scene graph to the list: */
				sg.root=root;
				sg.enabled=true;
				}
			catch(std::runtime_error err)
				{
				/* Print an error message and try the next file: */
				Misc::formattedUserWarning("Scene Graph Viewer: Ignoring file %s due to exception %s",sg.fileName.c_str(),err.what());
				
				/* Remove the tentatively-added scene graph again: */
				sceneGraphs.pop_back();
				}
			}
		}
	
	/* Create the main menu shell: */
	mainMenu=new GLMotif::PopupMenu("MainMenu",Vrui::getWidgetManager());
	mainMenu->setTitle("Scene Graph Viewer");
	
	/* Add a button to go to physical space: */
	GLMotif::Button* goToPhysicalSpaceButton=new GLMotif::Button("GoToPhysicalSpaceButton",mainMenu,"Go To Physical Space");
	goToPhysicalSpaceButton->getSelectCallbacks().add(this,&SceneGraphViewer::goToPhysicalSpaceCallback);
	
	/* Add a toggle button for each scene graph: */
	mainMenu->addSeparator();
	int index=0;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt,++index)
		{
		/* Generate a widget name: */
		char toggleName[40];
		snprintf(toggleName,sizeof(toggleName),"SceneGraphToggle%d",index);
		
		/* Extract a short name from the scene graph file name: */
		std::string::iterator start=sgIt->fileName.begin();
		std::string::iterator end=sgIt->fileName.end();
		for(std::string::iterator fnIt=sgIt->fileName.begin();fnIt!=sgIt->fileName.end();++fnIt)
			{
			if(*fnIt=='/')
				{
				start=fnIt+1;
				end=sgIt->fileName.end();
				}
			else if(*fnIt=='.')
				end=fnIt;
			}
		std::string name(start,end);
		
		/* Create a toggle button: */
		GLMotif::ToggleButton* sceneGraphToggle=new GLMotif::ToggleButton(toggleName,mainMenu,name.c_str());
		sceneGraphToggle->setToggle(sgIt->enabled);
		sceneGraphToggle->getValueChangedCallbacks().add(this,&SceneGraphViewer::sceneGraphToggleCallback,index);
		}
	mainMenu->addSeparator();
	
	/* Add a button to force a reload on all scene graphs: */
	GLMotif::Button* reloadAllSceneGraphsButton=new GLMotif::Button("ReloadAllSceneGraphsButton",mainMenu,"Reload All");
	reloadAllSceneGraphsButton->getSelectCallbacks().add(this,&SceneGraphViewer::reloadAllSceneGraphsCallback);
	
	/* Finish and install the main menu: */
	mainMenu->manageMenu();
	Vrui::setMainMenu(mainMenu);
	}

SceneGraphViewer::~SceneGraphViewer(void)
	{
	delete mainMenu;
	}

void SceneGraphViewer::display(GLContextData& contextData) const
	{
	/* Save OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);
	
	/* Render all scene graphs: */
	for(std::vector<SGItem>::const_iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		if(sgIt->enabled)
			Vrui::renderSceneGraph(sgIt->root.getPointer(),sgIt->navigational,contextData);
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void SceneGraphViewer::resetNavigation(void)
	{
	/* Set the navigation transformation: */
	SceneGraph::Box bbox=SceneGraph::Box::empty;
	for(std::vector<SGItem>::iterator sgIt=sceneGraphs.begin();sgIt!=sceneGraphs.end();++sgIt)
		if(sgIt->navigational)
			bbox.addBox(sgIt->root->calcBoundingBox());
	Vrui::setNavigationTransformation(Geometry::mid(bbox.min,bbox.max),Math::div2(Geometry::dist(bbox.min,bbox.max)));
	}

VRUI_APPLICATION_RUN(SceneGraphViewer)
