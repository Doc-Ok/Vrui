#!/bin/bash
########################################################################
# Script to run a Vrui application on an HTC Vive or HTC Vive Pro head-
# mounted display.
# Copyright (c) 2018-2020 Oliver Kreylos
# 
# This file is part of the Virtual Reality User Interface Library
# (Vrui).
# 
# The Virtual Reality User Interface Library is free software; you can
# redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# The Virtual Reality User Interface Library is distributed in the hope
# that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Virtual Reality User Interface Library; if not, write
# to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA
########################################################################

# Configure environment:
VRUIBINDIR=/usr/local/bin
USERCONFIGDIR=$HOME/.config/Vrui-5.0

# Output port to which the VR headset is connected:
VIVE_PORT=

# Type of connected VR headset (OG Vive or Vive Pro):
FINDVIVERESULT=
VIVE_TYPE=None

#
# Helper functions
#

# Commands to identify an OG Vive or a Vive Pro:
FIND_VIVE="$VRUIBINDIR/FindHMD -size 2160 1200 -rate 89.53"
FIND_VIVEPRO="$VRUIBINDIR/FindHMD -size 2880 1600 -rate 90.0409"

# Determine the type of the connected VR headset:
GetViveType()
	{
	# Check if an OG Vive is connected:
	VIVE_PORT=$($FIND_VIVE 2>/dev/null)
	FINDVIVERESULT=$?
	if (($FINDVIVERESULT != 0)); then
		# Check if a Vive Pro is connected:
		VIVE_PORT=$($FIND_VIVEPRO 2>/dev/null)
		FINDVIVERESULT=$?
		if (($FINDVIVERESULT == 0)); then
			VIVE_TYPE=VivePro
		fi
	else
		VIVE_TYPE=Vive
	fi
	}

#
# Main entry point
#

# Determine the type of connected VR headset:
GetViveType

# Check if a Vive or Vive Pro were found:
if (($FINDVIVERESULT == 1)); then
	echo No Vive or VivePro HMD found\; please connect your Vive or Vive Pro and try again
	exit 1
fi

# Check if the found VR headset is enabled:
if (($FINDVIVERESULT == 2)); then
	echo $VIVE_TYPE HMD on video port $VIVE_PORT is disabled; please enable it and try again
	exit 2
fi

# Extract the name of the VR application:
VRUIAPPNAME=$1
shift
echo Running $VRUIAPPNAME on $VIVE_TYPE HMD connected to video output port $VIVE_PORT

# Set up VR application options:
USE_CONTROLWINDOW=0 # Open a secondary window showing a 3rd-person perspective
SHOW_CONTROLLERS=1 # Show fancy 3D controller models
SUPERSAMPLING=1.0 # Adjust application rendering resolution

# Include the user configuration file if it exists:
[ -f $USERCONFIGDIR/OnVive.conf ] && source $USERCONFIGDIR/OnVive.conf

#
# Construct a command line to configure Vrui:
#

# Select the window section based on headset type and configure the display port:
VRUIWINSEC=HMDWindow$VIVE_TYPE
VRUI_OPTIONS=(-rootSection Vive -setConfig "windowNames=($VRUIWINSEC)")
VRUI_OPTIONS+=(-setConfig "$VRUIWINSEC/outputName=$VIVE_PORT")

# Configure pre-warp supersampling:
VRUI_OPTIONS+=(-setConfig "$VRUIWINSEC/LensCorrector/superSampling=$SUPERSAMPLING")

# Select the sound context section based on headset type:
VRUISNDSEC=SoundContext$VIVE_TYPE
VRUI_OPTIONS+=(-setConfig "soundContextName=$VRUISNDSEC")

# Add options to show a third-person control window:
if [ $USE_CONTROLWINDOW -ne 0 ]; then
	VRUI_OPTIONS+=(-mergeConfig ControlWindow)
fi

# Add options to show fancy 3D controller models:
if [ $SHOW_CONTROLLERS -ne 0 ]; then
	VRUI_OPTIONS+=(-vislet DeviceRenderer \;)
fi

#
# Run the Vrui application:
#

# Tell OpenGL to synchronize with the HMD's display:
export __GL_SYNC_DISPLAY_DEVICE=$VIVE_PORT

# Run the VR application with optional configuration in VR mode:
$VRUIAPPNAME "${VRUI_OPTIONS[@]}" "$@"
