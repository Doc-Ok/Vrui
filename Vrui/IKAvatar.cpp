/***********************************************************************
IKAvatar - Class to represent a VR user as an inverse kinematics-
controlled 3D geometry avatar.
Copyright (c) 2020 Oliver Kreylos

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

#include <Vrui/IKAvatar.h>

#include <Misc/MessageLogger.h>
#include <IO/OpenFile.h>
#include <SceneGraph/NodeCreator.h>
#include <SceneGraph/VRMLFile.h>
#include <Vrui/Internal/Config.h>

namespace Vrui {

/*************************
Methods of class IKAvatar:
*************************/

void IKAvatar::linkAvatar(SceneGraph::VRMLFile& avatarFile)
	{
	/* Retrieve the neck joint node: */
	neckNode=JointPointer(avatarFile.getNode("NT"));
	valid=neckNode!=0;
	
	/* Initialize the left and right arms: */
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		/* Retrieve the arm joint nodes: */
		Arm& arm=arms[armIndex];
		arm.clavicleNode=JointPointer(avatarFile.getNode(armIndex==0?"LCT":"RCT"));
		arm.shoulderNode=JointPointer(avatarFile.getNode(armIndex==0?"LST":"RST"));
		arm.elbowNode=JointPointer(avatarFile.getNode(armIndex==0?"LET":"RET"));
		arm.wristNode=JointPointer(avatarFile.getNode(armIndex==0?"LWT":"RWT"));
		valid=valid&&arm.clavicleNode!=0&&arm.shoulderNode!=0&&arm.elbowNode!=0&&arm.wristNode!=0;
		}
	
	/* Retrieve the pelvis joint node: */
	pelvisNode=JointPointer(avatarFile.getNode("PT"));
	valid=valid&&pelvisNode!=0;
	
	/* Initialize the left and right legs: */
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		/* Retrieve the leg joint nodes: */
		Leg& leg=legs[legIndex];
		leg.hipNode=JointPointer(avatarFile.getNode(legIndex==0?"LHT":"RHT"));
		leg.kneeNode=JointPointer(avatarFile.getNode(legIndex==0?"LKT":"RKT"));
		leg.ankleNode=JointPointer(avatarFile.getNode(legIndex==0?"LAT":"RAT"));
		valid=valid&&leg.hipNode!=0&&leg.kneeNode!=0&&leg.ankleNode!=0;
		}
	
	/* Mark the avatar's forward kinematics state as invalid: */
	stateValid=false;
	}

IKAvatar::IKAvatar(void)
	:headNode(new SceneGraph::TransformNode),
	 valid(false),stateValid(false)
	{
	}

void IKAvatar::loadAvatar(const char* avatarFileName)
	{
	/* Remove the current avatar: */
	headNode->children.clearValues();
	
	/* Load and parse the VRML file: */
	SceneGraph::NodeCreator nodeCreator;
	SceneGraph::VRMLFile avatarFile(*IO::openDirectory(VRUI_INTERNAL_CONFIG_SHAREDIR),avatarFileName,nodeCreator);
	avatarFile.parse(headNode);
	
	/* Link the avatar's joint nodes and check for errors: */
	linkAvatar(avatarFile);
	if(!valid)
		Misc::formattedUserError("Vrui::IKAvatar::loadAvatar: Invalid avatar in VRML file %s",avatarFileName);
	}

void IKAvatar::loadAvatar(IO::Directory& directory,const char* avatarFileName)
	{
	/* Remove the current avatar: */
	headNode->children.clearValues();
	
	/* Load and parse the VRML file: */
	SceneGraph::NodeCreator nodeCreator;
	SceneGraph::VRMLFile avatarFile(directory,avatarFileName,nodeCreator);
	avatarFile.parse(headNode);
	
	/* Link the avatar's joint nodes and check for errors: */
	linkAvatar(avatarFile);
	if(!valid)
		Misc::formattedUserError("Vrui::IKAvatar::loadAvatar: Invalid avatar in VRML file %s",avatarFileName);
	}

namespace {

/****************
Helper functions:
****************/

void setPosition(SceneGraph::TransformNode& node,const Point& position,Scalar scale) // Sets a transform node's translation based on an origin position
	{
	/* Set the transform node's translation: */
	node.translation.setValue(SceneGraph::Vector((position-Point::origin)/scale));
	node.update();
	}

void setRotation(SceneGraph::TransformNode& node,const Rotation& rotation) // Sets a transform node's rotation
	{
	/* Set the transform node's rotation: */
	node.rotation.setValue(SceneGraph::Rotation(rotation));
	node.update();
	}

}

void IKAvatar::configureAvatar(const Configuration& configuration)
	{
	/* Bail out if the avatar is not valid: */
	if(!valid)
		return;
	
	/* Retrieve the avatar's scale factor: */
	Scalar scale(Math::pow(double(headNode->scale.getValue()[0])*double(headNode->scale.getValue()[1])*double(headNode->scale.getValue()[2]),1.0/3.0));
	
	/* Store the head transformation: */
	// headToDevice=ONTransform(configuration.headToDevice.getTranslation()/scale,configuration.headToDevice.getRotation());
	headToDevice=configuration.headToDevice;
	
	/* Configure the joint positions: */
	setPosition(*neckNode,Point::origin,scale);
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		setPosition(*arms[armIndex].clavicleNode,configuration.arms[armIndex].claviclePos,scale);
		setPosition(*arms[armIndex].shoulderNode,configuration.arms[armIndex].shoulderPos,scale);
		setPosition(*arms[armIndex].elbowNode,Point(0,configuration.arms[armIndex].upperLength,0),scale);
		setPosition(*arms[armIndex].wristNode,Point(0,configuration.arms[armIndex].lowerLength,0),scale);
		}
	
	setPosition(*pelvisNode,configuration.pelvisPos,scale);
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		setPosition(*legs[legIndex].hipNode,configuration.legs[legIndex].hipPos,scale);
		setPosition(*legs[legIndex].kneeNode,Point(0,0,-configuration.legs[legIndex].upperLength),scale);
		setPosition(*legs[legIndex].ankleNode,Point(0,0,-configuration.legs[legIndex].lowerLength),scale);
		}
	
	/* Mark the avatar's forward kinematics state as invalid: */
	stateValid=false;
	}

void IKAvatar::scaleAvatar(Scalar scale)
	{
	/* Scale the root node's translation: */
	headNode->translation.setValue(headNode->translation.getValue()*SceneGraph::Scalar(scale));
	
	/* Set the root node's scaling factor: */
	headNode->scale.setValue(SceneGraph::Size(scale,scale,scale));
	
	headNode->update();
	}

void IKAvatar::invalidateState(void)
	{
	/* Mark the forward kinematics state as invalid: */
	stateValid=false;
	}

void IKAvatar::updateState(const IKAvatar::State& newState)
	{
	/* Bail out if the avatar is not valid: */
	if(!valid)
		return;
	
	/* Apply the joint rotations from the given forward kinematics state to the avatar representation: */
	setRotation(*neckNode,newState.neck);
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		const State::Arm& sArm=newState.arms[armIndex];
		setRotation(*arm.clavicleNode,sArm.clavicle);
		setRotation(*arm.shoulderNode,sArm.shoulder);
		setRotation(*arm.elbowNode,sArm.elbow);
		setRotation(*arm.wristNode,sArm.wrist);
		}
	
	setRotation(*pelvisNode,newState.pelvis);
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		const State::Leg& sLeg=newState.legs[legIndex];
		setRotation(*leg.hipNode,sLeg.hip);
		setRotation(*leg.kneeNode,sLeg.knee);
		setRotation(*leg.ankleNode,sLeg.ankle);
		}
	
	/* Mark the avatar state as valid: */
	stateValid=true;
	}

void IKAvatar::setRootTransform(const ONTransform& newRootTransform)
	{
	/* Bail out if the avatar is not valid: */
	if(!valid)
		return;
	
	/* Apply the head to device transformation: */
	ONTransform root=newRootTransform;
	root*=headToDevice;
	root.renormalize();
	
	/* Set the root node's transformation: */
	headNode->translation.setValue(SceneGraph::Vector(root.getTranslation()));
	headNode->rotation.setValue(SceneGraph::Rotation(root.getRotation()));
	headNode->update();
	}

}
