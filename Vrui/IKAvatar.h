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

#ifndef VRUI_IKAVATAR_INCLUDED
#define VRUI_IKAVATAR_INCLUDED

#include <Geometry/Point.h>
#include <Geometry/Rotation.h>
#include <Geometry/OrthonormalTransformation.h>
#include <SceneGraph/TransformNode.h>
#include <Vrui/Geometry.h>

/* Forward declarations: */
namespace IO {
class Directory;
}
namespace SceneGraph {
class VRMLFile;
}

namespace Vrui {

class IKAvatar
	{
	/* Embedded classes: */
	public:
	struct Configuration // Structure defining the user-fitting parameters of an IK avatar's skeleton
		{
		/* Embedded classes: */
		public:
		struct Arm // Structure defining an arm
			{
			/* Elements: */
			public:
			Point claviclePos; // Position of sternoclavicular joint in neck space
			Point shoulderPos; // Position of shoulder joint in clavicle space
			Scalar upperLength; // Length of upper arm
			Scalar lowerLength; // Length of lower arm
			};
		
		struct Leg // Structure defining a leg
			{
			/* Elements: */
			public:
			Point hipPos; // Position of hip joint in pelvis space
			Scalar upperLength; // Length of upper leg
			Scalar lowerLength; // Length of lower leg
			};
		
		/* Elements: */
		public:
		ONTransform headToDevice; // Transformation from head space, with neck joint at origin, to head tracking device space
		Arm arms[2]; // Configuration of the left and right arms
		Point pelvisPos; // Position of pelvis joint in neck space
		Leg legs[2]; // Configuration of the left and right legs
		};
	
	struct State // Structure defining the forward kinematics state of an IK avatar's skeleton
		{
		/* Embedded classes: */
		public:
		struct Arm // Forward kinematics state of an arm
			{
			/* Elements: */
			public:
			Rotation clavicle; // Rotation from neck to clavicle
			Rotation shoulder; // Rotation from clavicle to upper arm
			Rotation elbow; // Rotation from upper arm to lower arm
			Rotation wrist; // Rotation from lower arm to hand
			};
		
		struct Leg // Forward kinematics state of a leg
			{
			/* Elements: */
			public:
			Rotation hip; // Rotation from pelvis to upper leg
			Rotation knee; // Rotation from upper leg to lower leg
			Rotation ankle; // Rotation from lower leg to foot
			};
		
		/* Elements: */
		public:
		Rotation neck; // Rotation from head to neck
		Arm arms[2]; // Forward kinematics states of the left and right arm
		Rotation pelvis; // Rotation from spine to pelvis
		Leg legs[2]; // Forward kinematics states of the left and right legs
		};
	
	private:
	typedef SceneGraph::TransformNodePointer JointPointer; // Type for pointers to scene graph nodes representing forward kinematics joints
	
	struct Arm // Structure representing an arm
		{
		/* Elements: */
		public:
		JointPointer clavicleNode; // Scene graph node representing the sternoclavicular joint
		JointPointer shoulderNode; // Scene graph node representing the shoulder joint
		JointPointer elbowNode; // Scene graph node representing the elbow joint
		JointPointer wristNode; // Scene graph node representing the wrist joint
		};
	
	struct Leg // Structure representing a leg
		{
		/* Elements: */
		public:
		JointPointer hipNode; // Scene graph node representing the hip joint
		JointPointer kneeNode; // Scene graph node representing the knee joint
		JointPointer ankleNode; // Scene graph node representing the ankle joint
		};
	
	/* Elements: */
	ONTransform headToDevice; // Transformation from head space, with neck joint at origin, to head tracking device space
	JointPointer headNode; // Pointer to the root node of the avatar representation
	JointPointer neckNode; // Scene graph node representing the neck joint
	Arm arms[2]; // Scene graph nodes representing the joints of the left and right arms
	JointPointer pelvisNode; // Scene graph node representing the pelvic joint
	Leg legs[2]; // Scene graph nodes representing the joints of the left and right legs
	bool valid; // Flag whether the avatar's representation is valid
	bool stateValid; // Flag whether a valid skeleton state has been applied to the avatar representation
	
	/* Private methods: */
	void linkAvatar(SceneGraph::VRMLFile& avatarFile); // Links the avatar to a newly-loaded scene graph's joint nodes
	
	/* Constructors and destructors: */
	public:
	IKAvatar(void); // Creates an uninitialized avatar with a persistent scene graph root node
	
	/* Methods: */
	void loadAvatar(const char* avatarFileName); // Creates an avatar representation by reading a VRML file of the given name relative to Vrui's resource directory
	void loadAvatar(IO::Directory& directory,const char* avatarFileName); // Creates an avatar representation by reading a VRML file of the given name relative to the given directory
	void configureAvatar(const Configuration& configuration); // Configures the avatar representation
	void scaleAvatar(Scalar scale); // Applies a scaling factor to the entire avatar to account for different units of measurement
	void invalidateState(void); // Marks the avatar's skeleton state as invalid
	void updateState(const State& newState); // Updates the avatar's representation with the given forward kinematics state
	bool isValid(void) const // Returns true if the avatar and its forward-kinematics skeleton state are both valid
		{
		return stateValid;
		}
	void setRootTransform(const ONTransform& newRootTransform); // Sets the avatar's root transformation, i.e., attaches the avatar to a head tracking device's current pose
	const SceneGraph::TransformNode* getSceneGraph(void) const // Returns the avatar scene graph's root node
		{
		return headNode.getPointer();
		}
	SceneGraph::TransformNode* getSceneGraph(void) // Ditto
		{
		return headNode.getPointer();
		}
	};

}

#endif
