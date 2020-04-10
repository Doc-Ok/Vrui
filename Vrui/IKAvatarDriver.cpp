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

#include <Vrui/IKAvatarDriver.h>

#include <utility>
#include <Misc/ThrowStdErr.h>
#include <Misc/MessageLogger.h>
#include <Misc/FileTests.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/GeometryValueCoders.h>
#include <Vrui/Internal/Config.h>
#include <Vrui/InputDevice.h>
#include <Vrui/InputGraphManager.h>

namespace Vrui {

/*******************************
Methods of class IKAvatarDriver:
*******************************/

void IKAvatarDriver::viewerConfigChangedCallback(Viewer::ConfigChangedCallbackData* cbData)
	{
	/* Check if the viewer's head tracking state changed: */
	if(cbData->changeReasons&Viewer::ConfigChangedCallbackData::HeadDevice)
		{
		/* Unregister the tracking callback from the main viewer's previous head device: */
		if(viewerHeadDevice!=0)
			viewerHeadDevice->getTrackingCallbacks().remove(this,&IKAvatarDriver::trackingCallback);
		
		/* Retrieve the viewer's new head device: */
		viewerHeadDevice=const_cast<InputDevice*>(cbData->viewer->getHeadDevice()); // This is a bit of API flaw in Vrui
		
		/* Register a tracking callback with the main viewer's head device, if the main viewer is head tracked: */
		if(viewerHeadDevice!=0)
			viewerHeadDevice->getTrackingCallbacks().add(this,&IKAvatarDriver::trackingCallback);
		
		/* Call for an update: */
		needUpdate=true;
		}
	}

void IKAvatarDriver::trackingCallback(Misc::CallbackData* cbData)
	{
	/* Call for an update: */
	needUpdate=true;
	}

void IKAvatarDriver::navigationTransformationChangedCallback(NavigationTransformationChangedCallbackData* cbData)
	{
	/* Bail out if the avatar's feet are not supposed to follow navigation space: */
	if(!lockFeetToNavSpace)
		return;
	
	/* Transform the IK-space poses of any planted feet according to the change in navigation transformation: */
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		if(leg.planted)
			{
			/* Convert the foot pose from IK space to navigation space using the old transformation: */
			ONTransform oldFoot=leg.ikFootPose;
			oldFoot*=ONTransform::translateToOriginFrom(leg.solePos);
			oldFoot.leftMultiply(ikToPhys);
			Vrui::NavTransform navFoot(oldFoot.getTranslation(),oldFoot.getRotation(),Scalar(1));
			navFoot.leftMultiply(cbData->oldInverseTransform);
			
			/* Convert the foot pose from navigation space back to IK space using the new transformation: */
			navFoot.leftMultiply(cbData->newTransform);
			leg.ikFootPose=ONTransform(navFoot.getTranslation(),navFoot.getRotation());
			leg.ikFootPose.leftMultiply(physToIk);
			leg.ikFootPose*=ONTransform::translateFromOriginTo(leg.solePos);
			leg.ikFootPose.renormalize();
			
			/* Call for an update: */
			needUpdate=true;
			}
		}
	}

IKAvatarDriver::IKAvatarDriver(void)
	:maxArmExtension(1.2),
	 minWristDist(0.3),
	 neckPitchOffset(Math::rad(-135.3)),
	 neckPitchScale(1.0/3.0),
	 maxNeckYaw(Math::rad(90.0)),
	 clavicleYawScale(Math::rad(30.0)),
	 maxClavicleYaw(Math::rad(33.0)),
	 clavicleRollScale(Math::rad(50.0)),
	 maxClavicleRoll(Math::rad(33.0)),
	 wristRelaxFactor(0.5),
	 shoulderRotationAngle(Math::rad(30.0)),
	 pelvisPitchFactor(0.667),pelvisSway(1),pelvisFlex(0.5),
	 maxFootWrenchCos(Math::cos(Math::rad(30.0))),
	 footSplay(Math::rad(10.0)),
	 stepTime(0.2),
	 stepHeight(0.05),
	 viewerHeadDevice(0),
	 lockFeetToNavSpace(false)
	{
	/* Calculate transformations between physical space and normalized IK space: */
	{
	Vector physx=getForwardDirection()^getUpDirection();
	Vector physy=getUpDirection()^physx;
	ikToPhys=ONTransform::rotate(Rotation::fromBaseVectors(physx,physy));
	physToIk=Geometry::invert(ikToPhys);
	}
	
	/* Transform the environment's floor plane to normalized IK space: */
	ikFloor=getFloorPlane();
	ikFloor.transform(physToIk);
	
	/* Calculate a transformations from IK space to viewer space: */
	{
	Vector viewy(Vrui::getMainViewer()->getDeviceViewDirection());
	Vector viewx=viewy^Vector(Vrui::getMainViewer()->getDeviceUpDirection());
	ikToViewer=ONTransform::rotate(Rotation::fromBaseVectors(viewx,viewy));
	}
	
	/*********************************************************************
	Create a default IK skeleton configuration:
	*********************************************************************/
	
	/* Combine the head transformation from neck joint space to normalized IK head space with the viewer transformation: */
	headToViewer=ONTransform::translate(Vector(0.0,-0.096,-0.067));
	headToViewer.leftMultiply(ikToViewer);
	headToViewer.renormalize();
	
	uprightNeckHeight=Scalar(1.69);
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		
		/* Initialize arm configuration: */
		arm.handToDevice=ONTransform::translate(Vector(armIndex==0?-0.023:0.023,-0.192,-0.1));
		arm.claviclePos=Point(armIndex==0?-0.02:0.02,0.015,-0.197);
		arm.shoulderPos=Point(armIndex==0?-0.145:0.145,0.0,0.0);
		arm.neckShoulderPos=arm.claviclePos+(arm.shoulderPos-Point::origin);
		arm.upperLength=Scalar(0.325);
		arm.upperLength2=Math::sqr(arm.upperLength);
		arm.lowerLength=Scalar(0.305);
		arm.lowerLength2=Math::sqr(arm.lowerLength);
		arm.length=arm.upperLength+arm.lowerLength;
		arm.length2=Math::sqr(arm.length);
		
		/* Invalidate the hand tracking device: */
		arm.handDevice=0;
		
		/* Initialize arm transient state: */
		arm.hand=ONTransform::identity;
		arm.valid=false;
		arm.lastWristRotationAngle=Scalar(0);
		}
	
	pelvisPos=Point(0.0,0.015,-0.7);
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		
		/* Initialize leg configuration: */
		leg.hipPos=Point(legIndex==0?-0.15:0.15,0,0);
		leg.upperLength=Scalar(0.458);
		leg.upperLength2=Math::sqr(leg.upperLength);
		leg.lowerLength=Scalar(0.432);
		leg.lowerLength2=Math::sqr(leg.lowerLength);
		leg.length=leg.upperLength+leg.lowerLength;
		leg.length2=Math::sqr(leg.length);
		leg.toePos=Point(0.0,0.23,-0.1);
		leg.heelPos=Point(0.0,-0.07,-0.1);
		leg.solePos=Point(0.0,0.1,-0.1);
		
		/* Initialize leg transient state: */
		leg.planted=false;
		leg.liftTime=-2.0; // Before step reset timeout
		}
	
	/* Initialize transient state: */
	lastStepLeg=0; // Won't matter because the leg's lift time will be before the reset timeout
	needUpdate=true;
	
	/* Register a configuration change callback with the main viewer: */
	getMainViewer()->getConfigChangedCallbacks().add(this,&IKAvatarDriver::viewerConfigChangedCallback);
	
	/* Register a tracking callback with the main viewer's head device, if the main viewer is head tracked: */
	viewerHeadDevice=const_cast<InputDevice*>(getMainViewer()->getHeadDevice()); // This is a bit of API flaw in Vrui
	if(viewerHeadDevice!=0)
		viewerHeadDevice->getTrackingCallbacks().add(this,&IKAvatarDriver::trackingCallback);
	
	if(lockFeetToNavSpace)
		{
		/* Register a callback for navigation transformation changes with Vrui: */
		getNavigationTransformationChangedCallbacks().add(this,&IKAvatarDriver::navigationTransformationChangedCallback);
		}
	}

IKAvatarDriver::~IKAvatarDriver(void)
	{
	/* Unregister the configuration change callback with the main viewer: */
	getMainViewer()->getConfigChangedCallbacks().remove(this,&IKAvatarDriver::viewerConfigChangedCallback);
	
	/* Unregister the tracking callback from the main viewer's current head device: */
	if(viewerHeadDevice!=0)
		viewerHeadDevice->getTrackingCallbacks().remove(this,&IKAvatarDriver::trackingCallback);
	
	/* Unregister tracking callbacks from both hand tracking devices: */
	for(int armIndex=0;armIndex<2;++armIndex)
		if(arms[armIndex].handDevice!=0)
			arms[armIndex].handDevice->getTrackingCallbacks().remove(this,&IKAvatarDriver::trackingCallback);
	
	if(lockFeetToNavSpace)
		{
		/* Unregister the callback for navigation transformation changes with Vrui: */
		getNavigationTransformationChangedCallbacks().remove(this,&IKAvatarDriver::navigationTransformationChangedCallback);
		}
	}

void IKAvatarDriver::configure(const char* configName)
	{
	/* Open the system-wide IK avatar driver configuration file: */
	Misc::ConfigurationFile avatarConfiguration(VRUI_INTERNAL_CONFIG_SYSCONFIGDIR "/IKAvatar" VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
	
	#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
	
	/* Merge the per-user IK avatar driver configuration file if it exists: */
	const char* home=getenv("HOME");
	if(home!=0&&home[0]!='\0')
		{
		/* Construct the configuration file's name: */
		std::string userConfigFileName=home;
		userConfigFileName.push_back('/');
		userConfigFileName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
		userConfigFileName.append("/IKAvatar");
		userConfigFileName.append(VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX);
		
		/* Check if the configuration file exists: */
		if(Misc::isPathFile(userConfigFileName.c_str()))
			{
			/* Merge the configuration file: */
			avatarConfiguration.merge(userConfigFileName.c_str());
			}
		}
	
	#endif
	
	/* Open the requested configuration file section: */
	Misc::ConfigurationFileSection cfg;
	if(configName!=0&&configName[0]!='\0')
		{
		/* Find the requested configuration section: */
		cfg=avatarConfiguration.getSection(configName);
		if(!cfg.isValid())
			Misc::throwStdErr("Vrui::IKAvatar::configure: Configuration %s not found",configName);
		}
	else
		{
		/* Find the first configuration sub-section: */
		cfg=Misc::ConfigurationFileSection(avatarConfiguration.beginSubsections());
		if(!cfg.isValid())
			throw std::runtime_error("Vrui::IKAvatar::configure: No default configuration found");
		}
	
	/* Configure inverse kinematics calculation parameters: */
	maxArmExtension=cfg.retrieveValue<Scalar>("./maxArmExtension",maxArmExtension);
	minWristDist=cfg.retrieveValue<Scalar>("./minWristDist",minWristDist);
	neckPitchOffset=Math::rad(cfg.retrieveValue<Scalar>("./neckPitchOffset",Math::deg(neckPitchOffset)));
	neckPitchScale=cfg.retrieveValue<Scalar>("./neckPitchScale",neckPitchScale);
	maxNeckYaw=Math::rad(cfg.retrieveValue<Scalar>("./maxNeckYaw",Math::deg(maxNeckYaw)));
	clavicleYawScale=Math::rad(cfg.retrieveValue<Scalar>("./clavicleYawScale",Math::deg(clavicleYawScale)));
	maxClavicleYaw=Math::rad(cfg.retrieveValue<Scalar>("./maxClavicleYaw",Math::deg(maxClavicleYaw)));
	clavicleRollScale=Math::rad(cfg.retrieveValue<Scalar>("./clavicleRollScale",Math::deg(clavicleRollScale)));
	maxClavicleRoll=Math::rad(cfg.retrieveValue<Scalar>("./maxClavicleRoll",Math::deg(maxClavicleRoll)));
	wristRelaxFactor=cfg.retrieveValue<Scalar>("./wristRelaxFactor",wristRelaxFactor);
	shoulderRotationAngle=Math::rad(cfg.retrieveValue<Scalar>("./shoulderRotationAngle",Math::deg(shoulderRotationAngle)));
	pelvisPitchFactor=cfg.retrieveValue<Scalar>("./pelvisPitchFactor",pelvisPitchFactor);
	pelvisSway=cfg.retrieveValue<Scalar>("./pelvisSway",pelvisSway);
	pelvisFlex=Math::rad(cfg.retrieveValue<Scalar>("./pelvisFlex",Math::deg(pelvisFlex)));
	maxFootWrenchCos=Math::cos(Math::rad(cfg.retrieveValue<Scalar>("./maxFootWrench",Math::deg(Math::acos(maxFootWrenchCos)))));
	footSplay=Math::rad(cfg.retrieveValue<Scalar>("./footSplay",Math::deg(footSplay)));
	stepTime=cfg.retrieveValue<double>("./stepTime",stepTime);
	stepHeight=cfg.retrieveValue<Scalar>("./stepHeight",stepHeight);
	
	/* Configure the matching avatar file name: */
	avatarModelFileName=cfg.retrieveString("./avatarModelFileName");
	
	/* Configure the head-to-viewer transformation: */
	headToViewer=cfg.retrieveValue<ONTransform>("./headToViewer",Geometry::invert(ikToViewer)*headToViewer);
	headToViewer.leftMultiply(ikToViewer);
	headToViewer.renormalize();
	
	/* Configure the height of the neck joint above the floor when standing erect: */
	uprightNeckHeight=cfg.retrieveValue<Scalar>("./uprightNeckHeight",uprightNeckHeight);
	
	/* Configure both arms: */
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		Misc::ConfigurationFileSection armCfg=cfg.getSection(armIndex==0?"LeftArm":"RightArm");
		
		/* Configure the arm's skeleton: */
		arm.handToDevice=armCfg.retrieveValue<ONTransform>("./handToDevice",arm.handToDevice);
		arm.claviclePos=armCfg.retrieveValue<Point>("./claviclePos",arm.claviclePos);
		arm.shoulderPos=armCfg.retrieveValue<Point>("./shoulderPos",arm.shoulderPos);
		arm.neckShoulderPos=arm.claviclePos+(arm.shoulderPos-Point::origin);
		arm.upperLength=armCfg.retrieveValue<Scalar>("./upperLength",arm.upperLength);
		arm.upperLength2=Math::sqr(arm.upperLength);
		arm.lowerLength=armCfg.retrieveValue<Scalar>("./lowerLength",arm.lowerLength);
		arm.lowerLength2=Math::sqr(arm.lowerLength);
		arm.length=arm.upperLength+arm.lowerLength;
		arm.length2=Math::sqr(arm.length);
		
		/* Retrieve the hand tracking device: */
		setHandDevice(armIndex,findInputDevice(armCfg.retrieveString("./handDeviceName",std::string()).c_str()));
		
		/* Initialize arm transient state: */
		arm.hand=ONTransform::identity;
		arm.valid=false;
		arm.lastWristRotationAngle=Scalar(0);
		}
	
	/* Configure the pelvis position: */
	pelvisPos=cfg.retrieveValue<Point>("./pelvisPos",pelvisPos);
	
	/* Configure both legs: */
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		Misc::ConfigurationFileSection legCfg=cfg.getSection(legIndex==0?"LeftLeg":"RightLeg");
		
		/* Initialize leg configuration: */
		leg.hipPos=legCfg.retrieveValue<Point>("./hipPos",leg.hipPos);
		leg.upperLength=legCfg.retrieveValue<Scalar>("./upperLength",leg.upperLength);
		leg.upperLength2=Math::sqr(leg.upperLength);
		leg.lowerLength=legCfg.retrieveValue<Scalar>("./lowerLength",leg.lowerLength);
		leg.lowerLength2=Math::sqr(leg.lowerLength);
		leg.length=leg.upperLength+leg.lowerLength;
		leg.length2=Math::sqr(leg.length);
		leg.toePos=legCfg.retrieveValue<Point>("./toePos",leg.toePos);
		leg.heelPos=legCfg.retrieveValue<Point>("./heelPos",leg.heelPos);
		leg.solePos=legCfg.retrieveValue<Point>("./solePos",leg.solePos);
		
		/* Initialize leg transient state: */
		leg.planted=false;
		leg.liftTime=-2.0; // Before step reset timeout
		}
	
	/* Check whether to lock the poses of planted feet to navigational space: */
	setLockFeetToNavSpace(cfg.retrieveValue<bool>("./lockFeetToNavSpace",lockFeetToNavSpace));
	
	/* Call for an update: */
	needUpdate=true;
	}

void IKAvatarDriver::configureFromTPose(void)
	{
	/* Ensure that both hand devices have valid tracking data: */
	if(!getInputGraphManager()->isEnabled(arms[0].handDevice)||!getInputGraphManager()->isEnabled(arms[1].handDevice))
		{
		Misc::userError("IKAvatarDriver: Both hand tracking devices require valid tracking data for T-pose calibration");
		return;
		}
	
	/* Transform the viewer transformation to normalized IK space: */
	ONTransform headT=getMainViewer()->getHeadTransformation();
	headT*=headToViewer;
	headT.leftMultiply(physToIk);
	headT.renormalize();
	
	/* Get the position of both wrist joints in normalized IK space: */
	Point wristPos[2];
	for(int i=0;i<2;++i)
		{
		ONTransform hand=arms[i].handDevice->getTransformation();
		hand*=arms[i].handToDevice;
		hand.leftMultiply(physToIk);
		hand.renormalize();
		wristPos[i]=hand.getOrigin();
		}
	
	/* Check if the hands are on the correct sides of the body: */
	if(((wristPos[1]-wristPos[0])^headT.getDirection(1))[2]<Scalar(0))
		{
		/* Flip the left and right hand tracking devices and wrist positions: */
		std::swap(arms[0].handDevice,arms[1].handDevice);
		std::swap(wristPos[0],wristPos[1]);
		}
	
	/* Estimate avatar parameters based on T-pose, this all being vastly ad-hoc: */
	uprightNeckHeight=calcHeight(headT.getOrigin());
	Scalar shoulderWidth=Geometry::dist(arms[0].neckShoulderPos,arms[1].neckShoulderPos);
	Scalar wristDist=Geometry::dist(wristPos[0],wristPos[1]);
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		arm.length=Math::div2(wristDist-shoulderWidth);
		arm.length*=Scalar(1.01); // Small fudge factor
		arm.length2=Math::sqr(arm.length);
		arm.upperLength=arm.length*Scalar(0.52);
		arm.upperLength2=Math::sqr(arm.upperLength);
		arm.lowerLength=arm.length*Scalar(0.48);
		arm.lowerLength2=Math::sqr(arm.lowerLength);
		}
	Scalar torsoLegLength=uprightNeckHeight-Scalar(0.1);
	Point neckPos=Geometry::mid(arms[0].claviclePos,arms[1].claviclePos);
	pelvisPos=Point(0,neckPos[1],-torsoLegLength*Scalar(0.45));
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		leg.upperLength=torsoLegLength*Scalar(0.28);
		leg.upperLength*=Scalar(1.01); // Small fudge factor
		leg.upperLength2=Math::sqr(leg.upperLength);
		leg.lowerLength=torsoLegLength*Scalar(0.27);
		leg.lowerLength*=Scalar(1.01); // Small fudge factor
		leg.lowerLength2=Math::sqr(leg.lowerLength);
		leg.length=leg.upperLength+leg.lowerLength;
		leg.length2=Math::sqr(leg.length);
		}
	
	/* Call for an update: */
	needUpdate=true;
	}

IKAvatar::Configuration IKAvatarDriver::getConfiguration(void) const
	{
	IKAvatar::Configuration result;
	
	/*********************************************************************
	Copy configuration data from this object into the configuration
	structure:
	*********************************************************************/
	
	result.headToDevice=headToViewer;
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		IKAvatar::Configuration::Arm& rArm=result.arms[armIndex];
		const Arm& arm=arms[armIndex];
		
		rArm.claviclePos=arm.claviclePos;
		rArm.shoulderPos=arm.shoulderPos;
		rArm.upperLength=arm.upperLength;
		rArm.lowerLength=arm.lowerLength;
		}
	
	result.pelvisPos=pelvisPos;
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		IKAvatar::Configuration::Leg& rLeg=result.legs[legIndex];
		const Leg& leg=legs[legIndex];
		
		rLeg.hipPos=leg.hipPos;
		rLeg.upperLength=leg.upperLength;
		rLeg.lowerLength=leg.lowerLength;
		}
	
	return result;
	}

void IKAvatarDriver::setHandDevice(int armIndex,InputDevice* newHandDevice)
	{
	/* Bail out if the new device is the same as the current: */
	Arm& arm=arms[armIndex];
	if(arm.handDevice==newHandDevice)
		return;
	
	/* Check if the arm's hand device is currently valid: */
	if(arm.handDevice!=0)
		{
		/* Remove the tracking callback from the current hand device: */
		arm.handDevice->getTrackingCallbacks().remove(this,&IKAvatarDriver::trackingCallback);
		}
	
	/* Set the given arm's hand tracking device: */
	arm.handDevice=newHandDevice;
	
	/* Check if the arm's new hand device is valid: */
	if(arm.handDevice!=0)
		{
		/* Add the tracking callback to the new hand device: */
		arm.handDevice->getTrackingCallbacks().add(this,&IKAvatarDriver::trackingCallback);
		}
	
	/* Call for an update: */
	needUpdate=true;
	}

void IKAvatarDriver::setLockFeetToNavSpace(bool newLockFeetToNavSpace)
	{
	/* Bail out if the new setting is the same as the current one: */
	if(lockFeetToNavSpace==newLockFeetToNavSpace)
		return;
	
	/* Update the locking flag and install/uninstall the navigation change callback: */
	lockFeetToNavSpace=newLockFeetToNavSpace;
	if(lockFeetToNavSpace)
		getNavigationTransformationChangedCallbacks().add(this,&IKAvatarDriver::navigationTransformationChangedCallback);
	else
		getNavigationTransformationChangedCallbacks().remove(this,&IKAvatarDriver::navigationTransformationChangedCallback);
	}

void IKAvatarDriver::scaleAvatar(Scalar scale)
	{
	/* Scale all scale-affected configuration parameters: */
	minWristDist*=scale;
	pelvisSway/=scale;
	stepHeight*=scale;
	
	headToViewer.leftMultiply(Geometry::invert(ikToViewer));
	headToViewer=ONTransform(headToViewer.getTranslation()*scale,headToViewer.getRotation());
	headToViewer.leftMultiply(ikToViewer);
	headToViewer.renormalize();
	
	uprightNeckHeight*=scale;
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		
		arm.handToDevice=ONTransform(arm.handToDevice.getTranslation()*scale,arm.handToDevice.getRotation());
		arm.claviclePos=Point::origin+(arm.claviclePos-Point::origin)*scale;
		arm.shoulderPos=Point::origin+(arm.shoulderPos-Point::origin)*scale;
		arm.neckShoulderPos=Point::origin+(arm.neckShoulderPos-Point::origin)*scale;
		arm.upperLength*=scale;
		arm.lowerLength*=scale;
		arm.length*=scale;
		arm.upperLength2*=scale*scale;
		arm.lowerLength2*=scale*scale;
		arm.length2*=scale*scale;
		}
	
	pelvisPos=Point::origin+(pelvisPos-Point::origin)*scale;;
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		
		leg.hipPos=Point::origin+(leg.hipPos-Point::origin)*scale;
		leg.upperLength*=scale;
		leg.lowerLength*=scale;
		leg.length*=scale;
		leg.upperLength2*=scale*scale;
		leg.lowerLength2*=scale*scale;
		leg.length2*=scale*scale;
		leg.toePos=Point::origin+(leg.toePos-Point::origin)*scale;
		leg.heelPos=Point::origin+(leg.heelPos-Point::origin)*scale;
		leg.solePos=Point::origin+(leg.solePos-Point::origin)*scale;
		}
	
	/* Call for an update: */
	needUpdate=true;
	}

namespace {

/****************
Helper functions:
****************/

inline Scalar safeAcos(Scalar v) // Calculate arc-cosine safe from range errors due to numerical inaccuracies
	{
	if(v>=Scalar(1))
		return Scalar(0);
	else if(v<=Scalar(-1))
		return Math::Constants<Scalar>::pi;
	else
		return Math::acos(v);
	}

inline Vector calcForwardDirectionAndPitch(const Rotation& r,Scalar& pitchAngle) // Calculates a forward-facing vector and a pitch angle for a rotation that might look straight up or down or slightly backwards
	{
	/* Project the rotation's y direction to the horizontal (x, y) plane and calculate its new length: */
	Vector y=r.getDirection(1);
	Vector hy(y[0],y[1],Scalar(0));
	Scalar hyLen=Math::sqrt(hy[0]*hy[0]+hy[1]*hy[1]);
	
	/* Project the rotation's x direction to the horizontal (x, y) plane, rotate it around +z by 90 degrees to point forward, and calculate its length: */
	Vector hx=r.getDirection(0);
	hx=Vector(-hx[1],hx[0],Scalar(0));
	Scalar hxLen=Math::sqrt(hx[0]*hx[0]+hx[1]*hx[1]);
	
	/* Calculate a normalized forward direction by blending the two vectors weighted by their length: */
	Vector forward(hx*hxLen+hy*hyLen);
	/* ^^^ Weighing isn't really necessary. Investigate! */
	forward/=Math::sqrt(Math::sqr(forward[0])+Math::sqr(forward[1]));
	
	/* Calculate the pitch angle: */
	pitchAngle=safeAcos(y*forward);
	if(y[2]<Scalar(0))
		pitchAngle=-pitchAngle;
	
	return forward;
	}

bool isPointInsideTriangle(const Point& p,const Point& t0,const Point& t1,const Point& t2)
	{
	Vector n=(t1-t0)^(t2-t1);
	return (p-t0)*(n^(t1-t0))>=Scalar(0)
	       &&(p-t1)*(n^(t2-t1))>=Scalar(0)
	       &&(p-t2)*(n^(t0-t2))>=Scalar(0);
	}

}

bool IKAvatarDriver::calculateState(IKAvatar::State& state)
	{
	/* Transform the viewer transformation to normalized IK space: */
	ONTransform headT=getMainViewer()->getHeadTransformation();
	headT*=headToViewer;
	headT.leftMultiply(physToIk);
	headT.renormalize();
	
	/*********************************************************************
	Calculate the torso yaw and pitch angle, i.e., the rotation between
	the head and the neck:
	*********************************************************************/
	
	/* Extract a horizontal forward direction and the pitch angle from the head transformation: */
	Scalar headPitch;
	Vector headForward=calcForwardDirectionAndPitch(headT.getRotation(),headPitch);
	
	/* Calculate neck pitch by blending head pitch and a fixed value based on neck height above ground: */
	Scalar neckHeight=calcHeight(headT.getOrigin());
	Scalar neckPitch=Math::min((uprightNeckHeight-neckHeight)*(neckPitchOffset+neckPitchScale*headPitch)/uprightNeckHeight,Scalar(0));
	Rotation neckPitchRot=Rotation::rotateX(neckPitch);
	
	/* Calculate an initial unpitched neck pose based on the head forward direction: */
	Scalar neckYawAngle=Math::atan2(-headForward[0],headForward[1]);
	Rotation neckYawRot=Rotation::rotateZ(neckYawAngle);
	ONTransform neckYawT(headT.getTranslation(),neckYawRot);
	
	/* Calculate rotations around the vertical axis to align each shoulder with its respective wrist: */
	bool armValids[2]; // Flags if the arms can reach the hand tracking devices
	Scalar shoulderAngles[2]; // Angles pointing from the neck towards each hand relative to the initial neck yaw
	Scalar shoulderTolerances[2]; // Range of angles around shoulder angles where the hand tracking device is within reach
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		
		/* Initialize the arm's state: */
		armValids[armIndex]=false;
		shoulderAngles[armIndex]=Scalar(0);
		shoulderTolerances[armIndex]=Math::Constants<Scalar>::pi;
		
		/* Check if the arm's hand has valid tracking data: */
		if(arm.handDevice!=0&&getInputGraphManager()->isEnabled(arm.handDevice))
			{
			/* Transform the hand transformation to IK space: */
			arm.hand=arm.handDevice->getTransformation();
			arm.hand*=arm.handToDevice;
			arm.hand.leftMultiply(physToIk);
			arm.hand.renormalize();
			
			/* Calculate the position of the wrist in initial unpitched neck space: */
			Point wrist=neckYawT.inverseTransform(arm.hand.getOrigin());
			
			/* Slightly raise the shoulder resting position to account for potential clavicle roll: */
			Point shoulder=arm.neckShoulderPos;
			shoulder[2]+=Math::sin(Math::div2(maxClavicleRoll))*Math::abs(arm.shoulderPos[0]);
			
			/* Calculate the raised shoulder position in initial unpitched neck space: */
			shoulder=neckPitchRot.transform(shoulder);
			
			/* Calculate the horizontal and full distances from neck to wrist: */
			Scalar hw2=Math::sqr(wrist[0])+Math::sqr(wrist[1]);
			Scalar w2=hw2+Math::sqr(wrist[2]);
			
			/* Calculate the horizontal and full distances from neck to shoulder: */
			Scalar hs2=Math::sqr(shoulder[0])+Math::sqr(shoulder[1]);
			Scalar s2=hs2+Math::sqr(shoulder[2]);
			
			/* Calculate the best-case distance between the shoulder and wrist: */
			Scalar hwhs=Math::sqrt(hw2*hs2);
			Scalar sw2=w2+s2-Scalar(2)*(hwhs+wrist[2]*shoulder[2]);
			
			/* Check if the arm is within range of the shoulder: */
			if(sw2<Math::sqr(arm.length*maxArmExtension))
				{
				armValids[armIndex]=true;
				
				/* Calculate the horizontal distance from neck to wrist: */
				if(hw2>Scalar(0))
					{
					/* Calculate a rotation angle to align the shoulder and wrist: */
					Scalar alpha=safeAcos((wrist[0]*shoulder[0]+wrist[1]*shoulder[1])/hwhs);
					if(shoulder[0]*wrist[1]<shoulder[1]*wrist[0])
						alpha=-alpha;
					
					/* Reduce the rotation angle if the wrist is close to the neck horizontally: */
					if(hw2<Math::sqr(minWristDist))
						alpha*=Math::sqrt(hw2)/minWristDist;
					
					shoulderAngles[armIndex]=alpha;
					
					/* Calculate the angle interval in which the arm is not overextended: */
					Scalar ar2=Math::max(arm.length2-Math::sqr(wrist[2]-shoulder[2]),Scalar(0));
					shoulderTolerances[armIndex]=safeAcos((hw2+hs2-ar2)/(Scalar(2)*hwhs));
					}
				}
			else
				{
				/* Reset the previous wrist rotation angle: */
				arm.lastWristRotationAngle=Scalar(0);
				}
			}
		}
	
	/* Calculate the initial final neck yaw angle as the average between the two shoulder angles: */
	Scalar neckYaw=Math::mid(shoulderAngles[0],shoulderAngles[1]);
	
	/* Check if the neck yaw angle can be adjusted to be within both arms' tolerance interval: */
	bool arm0InInterval=Math::abs(neckYaw-shoulderAngles[0])<=shoulderTolerances[0];
	bool arm1InInterval=Math::abs(neckYaw-shoulderAngles[1])<=shoulderTolerances[1];
	if(arm0InInterval&&!arm1InInterval)
		{
		/* Move the neck yaw angle towards the right arm's valid interval: */
		if(neckYaw<shoulderAngles[1])
			{
			/* Increase the neck yaw angle: */
			neckYaw=Math::min(shoulderAngles[1]-shoulderTolerances[1],shoulderAngles[0]+shoulderTolerances[0]);
			}
		else
			{
			/* Decrease the neck yaw angle: */
			neckYaw=Math::max(shoulderAngles[1]+shoulderTolerances[1],shoulderAngles[0]-shoulderTolerances[0]);
			}
		}
	if(!arm0InInterval&&arm1InInterval)
		{
		/* Move the neck yaw angle towards the left arm's valid interval: */
		if(neckYaw<shoulderAngles[0])
			{
			/* Increase the neck yaw angle: */
			neckYaw=Math::min(shoulderAngles[0]-shoulderTolerances[0],shoulderAngles[1]+shoulderTolerances[1]);
			}
		else
			{
			/* Decrease the neck yaw angle: */
			neckYaw=Math::max(shoulderAngles[0]+shoulderTolerances[0],shoulderAngles[1]-shoulderTolerances[1]);
			}
		}
	
	/* Calculate the final neck pose: */
	neckYawRot=Rotation::rotateZ(neckYawAngle+Math::clamp(neckYaw,-maxNeckYaw,maxNeckYaw));
	Rotation neckRot=neckYawRot;
	neckRot*=neckPitchRot;
	neckRot.renormalize();
	ONTransform neck(neckYawT.getTranslation(),neckRot);
	
	/* Update the avatar state with the neck rotation in head space: */
	state.neck=Geometry::invert(headT.getRotation());
	state.neck*=neckRot;
	state.neck.renormalize();
	
	/*********************************************************************
	Calculate inverse kinematics poses for each of the arms:
	*********************************************************************/
	
	for(int armIndex=0;armIndex<2;++armIndex)
		{
		Arm& arm=arms[armIndex];
		
		/* Initialize the arm transformations: */
		ONTransform clavicle=ONTransform::translateFromOriginTo(arm.claviclePos);
		ONTransform shoulder;
		ONTransform elbow=ONTransform::translate(Vector(0,arm.upperLength,0));
		
		/* Initialize the wrist pose: */
		Vector wrist(0,arm.lowerLength,0);
		Rotation wristUpperArm;
		
		/* Check if the arm has valid IK state: */
		if(armValids[armIndex])
			{
			/* Calculate the position of the wrist in neck space: */
			Point neckWrist=neck.inverseTransform(arm.hand.getOrigin());
			
			/* Calculate yaw and roll angles for the sternoclavicular joint to accommodate over-extension: */
			Vector shoulderToWrist=neckWrist-arm.neckShoulderPos;
			Scalar clavicleYaw=(shoulderToWrist[1])/arm.length-Scalar(0.5);
			Scalar stwx=shoulderToWrist[0];
			if(armIndex==1)
				stwx=-stwx;
			if(stwx>Scalar(0))
				clavicleYaw+=stwx/arm.length;
			clavicleYaw=Math::clamp(clavicleYaw*clavicleYawScale,Scalar(0),maxClavicleYaw);
			if(armIndex==0)
				clavicleYaw=-clavicleYaw;
			Scalar clavicleRoll=(shoulderToWrist[2])/arm.length-Scalar(0.5);
			clavicleRoll=Math::clamp(clavicleRoll*clavicleRollScale,Scalar(0),maxClavicleRoll);
			if(armIndex==1)
				clavicleRoll=-clavicleRoll;
			
			/* Calculate the sternoclavicular joint's pose in neck space: */
			clavicle*=ONTransform::rotate(Rotation::rotateZ(clavicleYaw));
			clavicle*=ONTransform::rotate(Rotation::rotateY(clavicleRoll));
			clavicle.renormalize();
			
			/* Calculate the wrist position in sternoclavicular joint space: */
			shoulderToWrist=clavicle.inverseTransform(neckWrist)-arm.shoulderPos;
			Scalar stwLen2=Geometry::sqr(shoulderToWrist);
			Scalar stwLen=Math::sqrt(stwLen2);
			
			/* Calculate a differential rotation between lower arm and wrist in shoulder space: */
			wristUpperArm=arm.hand.getRotation();
			wristUpperArm.leftMultiply(Geometry::invert(neck.getRotation()));
			wristUpperArm.leftMultiply(Geometry::invert(clavicle.getRotation()));
			
			/* Calculate a minimum-arc rotation to point the upper arm at the wrist: */
			shoulder=ONTransform::rotate(Rotation::rotateFromTo(Vector(0,1,0),shoulderToWrist));
			
			/* Position the elbow: */
			if(stwLen2<arm.length2)
				{
				/* Calculate the elbow angle: */
				Scalar alpha=Math::acos((stwLen2+arm.upperLength2-arm.lowerLength2)/(Scalar(2)*stwLen*arm.upperLength));
				shoulder*=ONTransform::rotate(Rotation::rotateX(-alpha));
				
				/* Calculate the elbow pose: */
				Scalar omega=Math::acos((arm.upperLength2+arm.lowerLength2-stwLen2)/(Scalar(2)*arm.upperLength*arm.lowerLength));
				elbow*=ONTransform::rotate(Rotation::rotateX(Math::Constants<Scalar>::pi-omega));
				}
			else
				{
				/* Arm is over-extended; stretch the arm to make up for it somehow: */
				Scalar extension=stwLen-arm.length;
				shoulder*=ONTransform::translate(Vector(0,0,-extension/Scalar(3)));
				elbow*=ONTransform::translate(Vector(0,0,-extension/Scalar(3)));
				wrist+=Vector(0,0,-extension/Scalar(3));
				}
			
			/* Calculate the wrist orientation in elbow space: */
			Rotation wristElbow=wristUpperArm;
			wristElbow.leftMultiply(Geometry::invert(shoulder.getRotation()));
			wristElbow.leftMultiply(Geometry::invert(elbow.getRotation()));
			Vector wristRotAxis=shoulder.transform(elbow.transform(wristElbow.getScaledAxis()));
			
			/* Project the wrist rotation axis to the vector from shoulder to wrist to rotate the elbow in-place: */
			Vector projWristRotAxis=shoulderToWrist*((wristRotAxis*shoulderToWrist)/stwLen2);
			Scalar wristRotationAngle=(projWristRotAxis*shoulderToWrist)/stwLen;
			if(armIndex==0)
				wristRotationAngle=-wristRotationAngle;
			
			/* Check if the wrist rotation angle changed too much since the last frame: */
			if(Math::abs(wristRotationAngle-arm.lastWristRotationAngle)>Math::rad(Scalar(120)))
				{
				/* Mirror the axis from the [0, 180] degree range to the [-360, -180] degree range: */
				Scalar angle=Geometry::mag(wristRotAxis);
				Scalar newAngle=angle-Scalar(2)*Math::Constants<Scalar>::pi;
				projWristRotAxis*=newAngle/angle;
				wristRotationAngle*=newAngle/angle;
				}
			arm.lastWristRotationAngle=wristRotationAngle;
			
			/* Scale the wrist rotation axis by the relaxation factor: */
			projWristRotAxis*=wristRelaxFactor;
			shoulder.leftMultiply(ONTransform::rotate(Rotation(projWristRotAxis)));
			
			/* Rotate by a small additional angle: */
			shoulder.leftMultiply(ONTransform::rotate(Rotation::rotateAxis(shoulderToWrist,armIndex==0?shoulderRotationAngle:-shoulderRotationAngle)));
			
			/* Finalize the shoulder transformation: */
			shoulder.leftMultiply(ONTransform::translateFromOriginTo(arm.shoulderPos));
			shoulder.renormalize();
			
			/* Calculate the final wrist transformation: */
			wristUpperArm.leftMultiply(Geometry::invert(shoulder.getRotation()));
			wristUpperArm.leftMultiply(Geometry::invert(elbow.getRotation()));
			wristUpperArm.renormalize();
			}
		else
			{
			/* Controller is not in hand; reset arm to resting pose: */
			shoulder=ONTransform::translateFromOriginTo(arm.shoulderPos);
			shoulder*=ONTransform::rotate(Rotation::rotateX(Math::rad(Scalar(-100))));
			
			elbow*=ONTransform::rotate(Rotation::rotateX(Math::rad(Scalar(40))));
			
			wristUpperArm=Rotation::identity;
			}
		
		/* Update the avatar state: */
		IKAvatar::State::Arm& sArm=state.arms[armIndex];
		sArm.clavicle=clavicle.getRotation();
		sArm.shoulder=shoulder.getRotation();
		sArm.elbow=elbow.getRotation();
		sArm.wrist=wristUpperArm;
		}
	
	/*********************************************************************
	Orient the pelvis such that it averages between the orientation of the
	neck and the average orientation of the feet.
	*********************************************************************/
	
	/* Initialize the pelvis transform pitched partway between spine and vertical: */
	ONTransform pelvis(pelvisPos-Point::origin,Rotation::rotateX(-neckPitch*pelvisPitchFactor));
	
	/* Estimate the body's center of gravity in neck space: */
	Point bodyCog=Geometry::mid(Point::origin,pelvisPos);
	
	/* Transform the center of gravity to normalized IK space and project it onto the floor: */
	Point floorCog=projectToFloor(neck.transform(bodyCog));
	
	/* Calculate the poses of any lifted feet in normalized IK space: */
	ONTransform ikFootPoses[2];
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		
		if(leg.planted)
			{
			/* Keep the foot pose from the last frame: */
			ikFootPoses[legIndex]=leg.ikFootPose;
			}
		else
			{
			/* Calculate the foot's final travel pose to place the foot on the ground, underneath the body's center of gravity: */
			Rotation finalRot=neckYawRot;
			finalRot*=Rotation::rotateZ(legIndex==0?footSplay:-footSplay);
			finalRot.renormalize();
			Vector finalTrans=projectToFloor(neck.transform(bodyCog+(leg.hipPos-Point::origin)))-Point::origin;
			finalTrans+=finalRot.transform(Point::origin-leg.solePos);
			
			/* Calculate the foot's travel pose: */
			Scalar weight=Scalar(Vrui::getApplicationTime()-leg.liftTime)/stepTime;
			if(weight<Scalar(1))
				{
				/* Blend the initial and final foot travel poses: */
				Vector t=leg.ikFootPose.getTranslation()*(Scalar(1)-weight)+finalTrans*weight;
				t[2]+=Scalar(4)*(weight-Math::sqr(weight))*stepHeight;
				
				Vector dr=(finalRot/leg.ikFootPose.getRotation()).getScaledAxis();
				dr*=weight;
				Rotation r=leg.ikFootPose.getRotation();
				r*=Rotation(dr);
				r.renormalize();
				ikFootPoses[legIndex]=ONTransform(t,r);
				}
			else
				{
				/* Move the foot to the final pose: */
				ikFootPoses[legIndex]=ONTransform(finalTrans,finalRot);
				}
			}
		}
	
	/* Calculate a desired yaw rotation for the pelvic joint: */
	Vector fldir=ikFootPoses[0].getDirection(1);
	fldir/=Math::sqrt(Math::sqr(fldir[0])+Math::sqr(fldir[1]));
	Vector frdir=ikFootPoses[1].getDirection(1);
	frdir/=Math::sqrt(Math::sqr(frdir[0])+Math::sqr(frdir[1]));
	Vector fd=ikFootPoses[1].transform(legs[1].solePos)-ikFootPoses[0].transform(legs[0].solePos);
	Vector pYaw=fldir+frdir;
	pYaw[0]-=fd[1]*pelvisSway;
	pYaw[1]+=fd[0]*pelvisSway;
	pYaw[2]=Scalar(0);
	Scalar pYaw2=Math::sqr(pYaw[0])+Math::sqr(pYaw[1]);
	Vector nYaw=neckYawRot.getDirection(1);
	Scalar pYawAngle=safeAcos((nYaw*pYaw)/Math::sqrt(pYaw2));
	if(nYaw[0]*pYaw[1]<nYaw[1]*pYaw[0])
		pYawAngle=-pYawAngle;
	pYawAngle*=pelvisFlex;
	
	/* Calculate the final pelvis transform in normalized IK space: */
	ONTransform ikPelvis=pelvis;
	ikPelvis.leftMultiply(neck);
	ikPelvis=ONTransform(ikPelvis.getTranslation(),Rotation::rotateZ(pYawAngle)*ikPelvis.getRotation());
	ikPelvis.renormalize();
	
	/* Calculate a forward-facing direction for the pelvis in normalized IK space: */
	Scalar pelvisPitch;
	Vector pelvisForward=calcForwardDirectionAndPitch(ikPelvis.getRotation(),pelvisPitch);
	
	/* Invert the pelvis transformation: */
	ONTransform invIkPelvis=ikPelvis;
	invIkPelvis.doInvert();
	
	/* Transform the final pelvis transform back to neck space: */
	pelvis=ikPelvis;
	pelvis.leftMultiply(Geometry::invert(neck));
	pelvis.renormalize();
	
	/* Update the avatar state: */
	state.pelvis=pelvis.getRotation();
	
	/*********************************************************************
	Calculate inverse kinematics poses for each of the legs:
	*********************************************************************/
	
	for(int legIndex=0;legIndex<2;++legIndex)
		{
		Leg& leg=legs[legIndex];
		IKAvatar::State::Leg& sLeg=state.legs[legIndex];
		
		/* Calculate the foot pose in pelvis space: */
		ONTransform foot=ikFootPoses[legIndex];
		foot.leftMultiply(invIkPelvis);
		foot.renormalize();
		
		/* Calculate the direction from hip to ankle in pelvic space: */
		Point anklePos=foot.getOrigin();
		Scalar htaLen2=Geometry::sqrDist(leg.hipPos,anklePos);
		
		/* Check if the desired ankle position is within range: */
		if(htaLen2<leg.length2)
			{
			/* Calculate the base hip pose: */
			Vector hipToAnkle=anklePos-leg.hipPos;
			sLeg.hip=Rotation::rotateFromTo(Vector(0,0,-1),hipToAnkle);
			
			/* Calculate the knee angle: */
			Scalar htaLen=Math::sqrt(htaLen2);
			Scalar alpha=Math::acos((htaLen2+leg.upperLength2-leg.lowerLength2)/(Scalar(2)*htaLen*leg.upperLength));
			sLeg.hip*=Rotation::rotateX(alpha);
			
			/* Calculate the angles from the plane containing the upper and lower leg to the pelvis's and foot's forward directions: */
			Vector kneeDir=sLeg.hip.getDirection(1);
			Scalar pelvisAngle=Math::asin(((invIkPelvis.transform(pelvisForward)^hipToAnkle)*kneeDir)/htaLen);
			Scalar footAngle=Math::asin(((foot.getDirection(1)^hipToAnkle)*kneeDir)/htaLen);
			
			/* Rotate the entire leg to split the angle between the hip's and ankle's forward directions: */
			sLeg.hip.leftMultiply(Rotation::rotateAxis(hipToAnkle,Math::div2(pelvisAngle+footAngle)));
			sLeg.hip.renormalize();
			
			/* Calculate the knee pose: */
			Scalar omega=Math::acos((leg.upperLength2+leg.lowerLength2-htaLen2)/(Scalar(2)*leg.upperLength*leg.lowerLength));
			sLeg.knee=Rotation::rotateX(omega-Math::Constants<Scalar>::pi);
			
			/* Calculate the ankle pose: */
			sLeg.ankle=foot.getRotation();
			sLeg.ankle.leftMultiply(Geometry::invert(sLeg.hip));
			sLeg.ankle.leftMultiply(Geometry::invert(sLeg.knee));
			sLeg.ankle.renormalize();
			
			/* Plant the foot unless it is traveling: */
			if(Scalar(Vrui::getApplicationTime()-leg.liftTime)>=stepTime)
				{
				leg.planted=true;
				leg.ikFootPose=ikFootPoses[legIndex];
				}
			}
		else
			{
			/* Straighten the knee: */
			sLeg.knee=Rotation::identity;
			
			/* Rotate the foot up from the toe to extend the leg's reach (i.e., stand on tip-toes): */
			Point toePos=foot.transform(leg.toePos);
			Vector x=anklePos-toePos;
			Vector y=x^foot.getDirection(0);
			Vector th=toePos-leg.hipPos;
			Scalar hx=th*x;
			Scalar hx2=hx*hx;
			Scalar hy=th*y;
			Scalar b=Math::div2(x.sqr()+th.sqr()-leg.length2);
			Scalar denom=hx2+hy*hy;
			Scalar nph=(b*hy)/denom;
			Scalar q=(b*b-hx2)/denom;
			Scalar det=nph*nph-q;
			if(det>=Scalar(0))
				{
				det=Math::sqrt(det);
				Scalar a=nph-det;
				if(a<Scalar(0))
					a=nph+det;
				Scalar footPitch=Math::asin(a);
				anklePos=toePos+x*Math::cos(footPitch)+y*a;
				
				/* Calculate the hip pose: */
				Vector hipToAnkle=anklePos-leg.hipPos;
				sLeg.hip=Rotation::rotateFromTo(Vector(0,0,-1),hipToAnkle);
				
				/* Calculate the ankle pose: */
				sLeg.ankle=foot.getRotation();
				sLeg.ankle*=Rotation::rotateX(Math::asin(a));
				sLeg.ankle.leftMultiply(Geometry::invert(sLeg.hip));
				sLeg.ankle.leftMultiply(Geometry::invert(sLeg.knee));
				sLeg.ankle.renormalize();
				
				/* Plant the foot unless it is traveling: */
				if(Scalar(Vrui::getApplicationTime()-leg.liftTime)>=stepTime)
					{
					leg.planted=true;
					leg.ikFootPose=ikFootPoses[legIndex];
					}
				}
			else if(leg.planted)
				{
				/* Lift the foot: */
				leg.planted=false;
				leg.liftTime=Vrui::getApplicationTime();
				}
			}
		}
	
	/* Check if the body's center of gravity is outside of the feet's convex hull if both feet are planted: */
	if(legs[0].planted&&legs[1].planted)
		{
		Point l0=ikFootPoses[0].transform(legs[0].heelPos);
		Point l1=ikFootPoses[0].transform(legs[0].toePos);
		Point r0=ikFootPoses[1].transform(legs[1].heelPos);
		Point r1=ikFootPoses[1].transform(legs[1].toePos);
		if(!(isPointInsideTriangle(floorCog,l0,r1,l1)||isPointInsideTriangle(floorCog,r0,r1,l0)))
			{
			/* Determine the weight-supporting foot, i.e., the one closer to the center of gravity: */
			Scalar ld2=Geometry::sqrDist(ikFootPoses[0].transform(legs[0].solePos),floorCog);
			Scalar rd2=Geometry::sqrDist(ikFootPoses[1].transform(legs[1].solePos),floorCog);
			int liftIndex=ld2<=rd2?1:0;
			if(liftIndex==lastStepLeg&&legs[lastStepLeg].liftTime>=Vrui::getApplicationTime()-stepTime*2.0)
				liftIndex=1-liftIndex;
			lastStepLeg=liftIndex;
			legs[liftIndex].planted=false;
			legs[liftIndex].liftTime=Vrui::getApplicationTime();
			}
		}
	
	/* Check if either foot is out of line with the pelvis if both feet are planted: */
	if(legs[0].planted&&legs[1].planted)
		{
		/* Calculate a forward-facing direction for the pelvis, coping with inversion: */
		Scalar lcos=fldir[0]*pelvisForward[0]+fldir[1]*pelvisForward[1];
		Scalar rcos=frdir[0]*pelvisForward[0]+frdir[1]*pelvisForward[1];
		if(lcos<maxFootWrenchCos||rcos<maxFootWrenchCos)
			{
			int liftIndex=lcos<=rcos?0:1;
			if(liftIndex==lastStepLeg&&legs[lastStepLeg].liftTime>=Vrui::getApplicationTime()-stepTime*2.0)
				liftIndex=1-liftIndex;
			lastStepLeg=liftIndex;
			legs[liftIndex].planted=false;
			legs[liftIndex].liftTime=Vrui::getApplicationTime();
			}
		}
	
	/* Request further updates if either foot is traveling: */
	needUpdate=!(legs[0].planted&&legs[1].planted);
	
	/* Return true if further updates are needed: */
	return needUpdate;
	}

}
