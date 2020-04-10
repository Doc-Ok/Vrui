#!/bin/bash
########################################################################
# Script to set up an environment to run VRDeviceDaemon's OpenVRHost
# device driver module, to receive tracking data for an HTC Vive using
# OpenVR's Lighthouse low-level driver module.
# Copyright (c) 2016-2019 Oliver Kreylos
#
# This file is part of the Vrui VR Device Driver Daemon
# (VRDeviceDaemon).
# 
# The Vrui VR Device Driver Daemon is free software; you can
# redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# The Vrui VR Device Driver Daemon is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Vrui VR Device Driver Daemon; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Configure environment:
STEAMDIR=$HOME/.steam/steam
RUNTIMEDIR1=$STEAMDIR/ubuntu12_32/steam-runtime/amd64/lib/x86_64-linux-gnu
RUNTIMEDIR2=$STEAMDIR/ubuntu12_32/steam-runtime/amd64/usr/lib/x86_64-linux-gnu
STEAMVRDIR=$STEAMDIR/SteamApps/common/SteamVR
DRIVERDIR=$STEAMVRDIR/drivers/lighthouse/bin/linux64
STEAMLDPATH=$LD_LIBRARY_PATH:$RUNTIMEDIR1:$RUNTIMEDIR2:$DRIVERDIR
VRUIBINDIR=/usr/local/bin

# Output port to which the VR headset is connected:
VIVE_PORT=

# Type of connected VR headset (OG Vive or Vive Pro):
VIVE_TYPE=None

# Commands to identify an OG Vive or a Vive Pro:
FIND_VIVE="$VRUIBINDIR/FindHMD -size 2160 1200 -rate 89.53"
FIND_VIVEPRO="$VRUIBINDIR/FindHMD -size 2880 1600 -rate 90.0409"

#
# Helper functions
#

# Determine the type of the connected VR headset:
GetViveType()
	{
	# Check if an OG Vive is connected:
	VIVE_PORT=$($FIND_VIVE 2>/dev/null)
	RESULT=$?
	if (( $RESULT == 1 )); then
		# Check if a Vive Pro is connected:
		VIVE_PORT=$($FIND_VIVEPRO 2>/dev/null)
		RESULT=$?
		if (( $RESULT != 1 )); then
			VIVE_TYPE=VivePro
		fi
	else
		VIVE_TYPE=Vive
	fi
	}

# State for startup/shutdown procedure:
XBACKGROUND_PID=0

# Enable Vive's display and display a black window:
ViveOn ()
	{
	if [[ "$VIVE_TYPE" == "Vive" ]]; then
		echo "Enabling Vive's display"
		xrandr $($FIND_VIVE -enableCmd)
		
		echo "Starting XBackground process"
		$VRUIBINDIR/XBackground -geometry $($FIND_VIVE -printGeometry) -nd -type 4 &
		XBACKGROUND_PID=$!
	else
		echo "Enabling Vive Pro's display"
		xrandr $($FIND_VIVEPRO -enableCmd)
		
		echo "Starting XBackground process"
		$VRUIBINDIR/XBackground -geometry $($FIND_VIVEPRO -printGeometry) -nd -type 4 &
		XBACKGROUND_PID=$!
	fi
	}

# Disable Vive's display:
ViveOff ()
	{
	echo "Terminating XBackground process"
	kill -TERM $XBACKGROUND_PID
	
	if [[ "$VIVE_TYPE" == "Vive" ]]; then
		echo "Disabling Vive's display"
		xrandr $($FIND_VIVE -disableCmd)
	else
		echo "Disabling Vive Pro's display"
		xrandr $($FIND_VIVEPRO -disableCmd)
	fi
	
	exit
	}

#
# Main entry point
#

# Trap signals to turn off the Vive's display on termination:
trap 'ViveOff' HUP
trap 'ViveOff' INT
trap 'ViveOff' TERM

# Determine the type of connected VR headset:
GetViveType
if [[ "$VIVE_TYPE" == "None" ]]; then
	echo No Vive or Vive Pro headsets found; exiting
	exit
fi

# Enable the Vive's display:
ViveOn

# Run VRDeviceDaemon:
# LD_LIBRARY_PATH=$STEAMLDPATH $VRUIBINDIR/VRDeviceDaemon -rootSection Vive

# Use the following command if things are working, and you don't want to
# see all those "Broken pipe" messages:
( LD_LIBRARY_PATH=$STEAMLDPATH $VRUIBINDIR/VRDeviceDaemon -rootSection Vive 2>&1 ) | fgrep -v ": Broken pipe"

# Disable the Vive's display:
ViveOff
