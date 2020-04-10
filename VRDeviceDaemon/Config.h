/***********************************************************************
Config - Daemon for distributed VR device driver architecture.
Copyright (c) 2014-2020 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRDEVICEDAEMON_CONFIG_INCLUDED
#define VRDEVICEDAEMON_CONFIG_INCLUDED

#define VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_DEBUG "/home/okreylos/Share/VRDevices/lib64/debug"
#define VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_RELEASE "/home/okreylos/Share/VRDevices/lib64"
#define VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_DEBUG "/home/okreylos/Share/VRCalibrators/lib64/debug"
#define VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_RELEASE "/home/okreylos/Share/VRCalibrators/lib64"
#ifdef DEBUG
	#define VRDEVICEDAEMON_CONFIG_VRDEVICESDIR VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_DEBUG
	#define VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_DEBUG
#else
	#define VRDEVICEDAEMON_CONFIG_VRDEVICESDIR VRDEVICEDAEMON_CONFIG_VRDEVICESDIR_RELEASE
	#define VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR VRDEVICEDAEMON_CONFIG_VRCALIBRATORSDIR_RELEASE
#endif

#define VRDEVICEDAEMON_CONFIG_DSONAMETEMPLATE "lib%s.so"

#define VRDEVICEDAEMON_CONFIG_CONFIGFILENAME "/home/okreylos/Share/VRDevices.cfg"
#define VRDEVICEDAEMON_CONFIG_CONFIGDIR "/home/okreylos/Projects/VRDeviceDaemon/Config"

#define VRDEVICEDAEMON_CONFIG_INPUT_H_HAS_STRUCTS 1

#endif
