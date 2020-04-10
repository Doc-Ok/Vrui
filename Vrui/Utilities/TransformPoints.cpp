/***********************************************************************
TransformPoints - Transforms a set of points from/into a coordinate
system defined by a point transformation node read from a VRML scene
graph file.
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

#include <iostream>
#include <SceneGraph/GroupNode.h>
#include <SceneGraph/PointTransformNode.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	const char* sceneGraphFileName=0;
	const char* transformNodeName=0;
	bool inverseTransform=false;
	for(int argi=1;argi<argc;++argi)
		{
		if(argv[argi][0]=='-')
			{
			if(strcasecmp(argv[argi]+1,"inverse")==0||strcasecmp(argv[argi]+1,"i")==0)
				inverseTransform=true;
			else
				std::cerr<<"Ignoring command line option "<<argv[argi]<<std::endl;
			}
		else if(sceneGraphFileName==0)
			sceneGraphFileName=argv[argi];
		else if(transformNodeName==0)
			transformNodeName=argv[argi];
		else
			std::cerr<<"Ignoring command line argument "<<argv[argi]<<std::endl;
		}
	if(sceneGraphFileName==0)
		{
		std::cerr<<"No scene graph file name provided"<<std::endl;
		return 1;
		}
	if(transformNodeName==0)
		{
		std::cerr<<"No point transformation node name provided"<<std::endl;
		return 1;
		}
	
	/* Load the scene graph: */
	SceneGraph::NodePointer ptNode;
	try
		{
		SceneGraph::NodeCreator nodeCreator;
		SceneGraph::GroupNodePointer root=new SceneGraph::GroupNode;
		
		/* Load and parse the VRML file: */
		SceneGraph::VRMLFile vrmlFile(sceneGraphFileName,nodeCreator);
		vrmlFile.parse(root);
		
		/* Retrieve the point transformation node: */
		ptNode=vrmlFile.getNode(transformNodeName);
		}
	catch(const std::runtime_error& err)
		{
		std::cerr<<"Caught exception "<<err.what()<<" while loading scene graph file "<<sceneGraphFileName<<std::endl;
		return 1;
		}
	
	if(ptNode==0)
		{
		std::cerr<<"Node "<<transformNodeName<<" not found in scene graph file "<<sceneGraphFileName<<std::endl;
		return 1;
		}
	SceneGraph::PointTransformNodePointer pointTransformNode(ptNode);
	if(pointTransformNode==0)
		{
		std::cerr<<"Node "<<transformNodeName<<" in scene graph file "<<sceneGraphFileName<<" is not a point transformation node"<<std::endl;
		return 1;
		}
	
	/* Read points from standard input and print their transformed or inverse-transformed results: */
	while(true)
		{
		/* Read the next point: */
		SceneGraph::PointTransformNode::TPoint p;
		for(int i=0;i<3;++i)
			std::cin>>p[i];
		
		/* Check for end-of-file: */
		if(std::cin.eof())
			break;
		
		/* Transform or inverse-transform the point: */
		if(inverseTransform)
			p=pointTransformNode->inverseTransformPoint(p);
		else
			p=pointTransformNode->transformPoint(p);
		
		/* Print the result point: */
		std::cout<<p[0]<<' '<<p[1]<<' '<<p[2]<<std::endl;
		}
	
	return 0;
	}
