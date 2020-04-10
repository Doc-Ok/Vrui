/***********************************************************************
IKAvatarDriver - Class to calculate the pose of an avatar using inverse
kinematics and the tracking data from 6-DOF input devices.
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

#ifndef VRUI_IKAVATARDRIVER_INCLUDED
#define VRUI_IKAVATARDRIVER_INCLUDED

#include <string>
#include <Geometry/Plane.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Vrui/Vrui.h>
#include <Vrui/Viewer.h>
#include <Vrui/IKAvatar.h>

/* Forward declarations: */
namespace Misc {
class CallbackData;
}
namespace Vrui {
class InputDevice;
}

namespace Vrui {

class IKAvatarDriver
	{
	/* Embedded classes: */
	private:
	struct Arm // Structure holding IK state for an arm
		{
		/* Elements: */
		public:
		
		/* IK skeleton configuration: */
		ONTransform handToDevice; // Transformation from hand space, with the wrist joint at the origin, to hand tracking device space
		Point claviclePos; // Position of sternoclavicular joint relative in neck space
		Point shoulderPos; // Position of shoulder joint in clavicle space
		Point neckShoulderPos; // Position of shoulder joint in neck space if sternoclavicular joint is in rest position
		Scalar upperLength,lowerLength; // Lengths of upper and lower arms, respectively
		Scalar length; // Total arm length from shoulder to wrist when fully extended
		Scalar upperLength2,lowerLength2,length2; // Squared arm lengths
		
		/* Current tracking and IK calculation state: */
		InputDevice* handDevice; // Pointer to input device tracking the hand
		ONTransform hand; // Hand device transformation in world space
		bool valid; // Flag whether the arm has a valid pose, i.e., the hand device is actually held in hand
		Scalar lastWristRotationAngle; // Wrist rotation angle from last frame
		};
	
	struct Leg // Structure holding IK state for a leg
		{
		/* Elements: */
		public:
		
		/* IK skeleton configuration: */
		Point hipPos; // Position of hip joint in pelvis space
		Scalar upperLength,lowerLength; // Lengths of upper and lower legs, respectively
		Scalar length; // Total leg length from hip to ankle when fully extended
		Scalar upperLength2,lowerLength2,length2; // Squared leg lengths
		Point toePos,heelPos; // Positions of the foot's toe and heel in foot space
		Point solePos; // Position of the foot sole's pivot point
		
		/* Current tracking and IK calculation state: */
		bool planted; // Flag whether the foot is currently on the floor
		ONTransform ikFootPose; // Current position and orientation of foot in normalized IK space
		double liftTime; // Application time at which the foot lifted off the floor
		};
	
	/* Elements: */
	
	/* Inverse kinematics calculation parameters: */
	Scalar maxArmExtension; // Maximum relative extension of arm to be considered valid for IK
	Scalar minWristDist; // Minimum horizontal distance from neck to wrist for regular torso yaw formula
	Scalar neckPitchOffset,neckPitchScale; // Linear blending function for neck pitch
	Scalar maxNeckYaw; // Maximum angle between head and shoulders
	Scalar clavicleYawScale,maxClavicleYaw; // Scale factor and upper limit for sternoclavicular joint's yaw angle
	Scalar clavicleRollScale,maxClavicleRoll; // Scale factor and upper limit for sternoclavicular joint's roll angle
	Scalar wristRelaxFactor; // Relaxation factor for wrist angle
	Scalar shoulderRotationAngle; // Additional shoulder rotation angle
	Scalar pelvisPitchFactor; // Factor by how much the pelvis rotates against the spine's pitch angle
	Scalar pelvisSway; // Scale factor of how much foot position influences pelvic joint rotation
	Scalar pelvisFlex; // Weight in [0, 1] of how "loose" the pelvic joint is w.r.t. the spine
	Scalar maxFootWrenchCos; // Cosine of maximum angle by which a foot can be rotated relative to the pelvis
	Scalar footSplay; // Angle by which feet are rotated outwards from the pelvis forward direction
	Scalar stepTime; // Time to complete a step in seconds
	Scalar stepHeight; // Maximum lift of foot during a step
	
	/*********************************************************************
	Coordinate transformations between physical space and normalized
	inverse kinematics space. For simplicity, IK space has the x axis
	pointing "right," the y axis pointing "forward," the z axis pointing
	up, and uses meters as measurement unit.
	*********************************************************************/
	
	ONTransform ikToPhys,physToIk; // Rotations between physical space and normalized IK space
	Plane ikFloor; // Environment's floor plane in normalized IK space
	ONTransform ikToViewer; // Rotation from normalized IK space to viewer space
	
	/* Avatar configuration: */
	std::string avatarModelFileName; // Name of the VRML file from which to load the avatar's graphical representation
	
	/* IK skeleton configuration: */
	InputDevice* viewerHeadDevice; // Pointer to the main viewer's current head tracking device, or null if viewer is static
	ONTransform headToViewer; // Transformation from head space, with neck joint at origin, to viewer space
	Scalar uprightNeckHeight; // Height of neck joint above ground when user stands upright
	Arm arms[2]; // State of the left and right arms
	Point pelvisPos; // Position of pelvic joint in neck space
	Leg legs[2]; // State of the left and right legs
	bool lockFeetToNavSpace; // Flag whether the feet should be locked to their navigation-space positions when planted
	
	/* IK calculation state: */
	int lastStepLeg; // Index of the last leg that stepped due to balance or rotation
	bool needUpdate; // Flag whether the avatar state must be updated due to pose changes
	
	/* Private methods: */
	Scalar calcHeight(const Point& p) const // Calculates the height of a point in normalized IK space above the environment's floor
		{
		/* Intersect a ray along the z direction with the floor plane: */
		return (p*ikFloor.getNormal()-ikFloor.getOffset())/ikFloor.getNormal()[2];
		}
	Point projectToFloor(const Point& p) const // Projects a point in normalized IK space to the environment's floor
		{
		/* Intersect a ray along the z direction with the floor plane: */
		Scalar lambda=(ikFloor.getOffset()-p*ikFloor.getNormal())/ikFloor.getNormal()[2];
		return Point(p[0],p[1],p[2]+lambda);
		}
	void viewerConfigChangedCallback(Viewer::ConfigChangedCallbackData* cbData); // Callback called when the main viewer changes configuration
	void trackingCallback(Misc::CallbackData* cbData); // Callback called when one of the tracking devices changes pose
	void navigationTransformationChangedCallback(NavigationTransformationChangedCallbackData* cbData); // Callback called when Vrui's navigation transformation changes
	
	/* Constructors and destructors: */
	public:
	IKAvatarDriver(void); // Creates an uninitialized IK avatar driver with default IK parameters
	~IKAvatarDriver(void);
	
	/* Methods: */
	void configure(const char* configName =0); // Configures the driver from a configuration sub-section of the given name, or from the default section if configName is null
	void configureFromTPose(void); // Configures the driver on-the-fly from a standard T-pose
	const std::string& getAvatarModelFileName(void) const // Returns the configured avatar VRML file name
		{
		return avatarModelFileName;
		}
	IKAvatar::Configuration getConfiguration(void) const; // Returns an IK avatar configuration structure
	void setHandDevice(int armIndex,InputDevice* newHandDevice); // Sets the left or right arm's hand tracking device
	bool getLockFeetToNavSpace(void) const // Returns true if the poses of planted feet are locked to navigational space
		{
		return lockFeetToNavSpace;
		}
	void setLockFeetToNavSpace(bool newLockFeetToNavSpace); // Sets whether feet should follow changes in the navigation transformation when they are planted on the ground
	void scaleAvatar(Scalar scale); // Applies a scale factor to the IK avatar driver's parameters and IK configuration to account for different units of measurement
	bool needsUpdate(void) const // Returns true if the IK avatar state needs to be recalculated due to pose changes
		{
		return needUpdate;
		}
	bool calculateState(IKAvatar::State& state); // Calculates a new IK avatar state based on current head and hand tracking data and writes it into the provided structure; returns true if the avatar is currently undergoing animation and requires further updates
	};

}

#endif
