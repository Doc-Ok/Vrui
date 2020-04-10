/***********************************************************************
Config - Internal configuration header file for the Vrui Library.
Copyright (c) 2014-2020 Oliver Kreylos

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

#ifndef VRUI_INTERNAL_CONFIG_INCLUDED
#define VRUI_INTERNAL_CONFIG_INCLUDED

#define VRUI_INTERNAL_CONFIG_HAVE_XRANDR 1
#define VRUI_INTERNAL_CONFIG_HAVE_XINPUT2 1
#define VRUI_INTERNAL_CONFIG_HAVE_LIBDBUS 1

#define VRUI_INTERNAL_CONFIG_VRWINDOW_USE_SWAPGROUPS 0

#define VRUI_INTERNAL_CONFIG_INPUT_H_HAS_STRUCTS 1

#define VRUI_INTERNAL_CONFIG_LIBDIR_DEBUG "/home/okreylos/lib64"
#define VRUI_INTERNAL_CONFIG_LIBDIR_RELEASE "/home/okreylos/lib64"
#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_DEBUG "/home/okreylos/Libraries/exe"
#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_RELEASE "/home/okreylos/Libraries/exe"
#define VRUI_INTERNAL_CONFIG_PLUGINDIR_DEBUG "/home/okreylos/Share"
#define VRUI_INTERNAL_CONFIG_PLUGINDIR_RELEASE "/home/okreylos/Share"

#ifdef DEBUG
	#define VRUI_INTERNAL_CONFIG_LIBDIR VRUI_INTERNAL_CONFIG_LIBDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_PLUGINDIR VRUI_INTERNAL_CONFIG_PLUGINDIR_DEBUG
#else
	#define VRUI_INTERNAL_CONFIG_LIBDIR VRUI_INTERNAL_CONFIG_LIBDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_EXECUTABLEDIR VRUI_INTERNAL_CONFIG_EXECUTABLEDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_PLUGINDIR VRUI_INTERNAL_CONFIG_PLUGINDIR_RELEASE
#endif

#define VRUI_INTERNAL_CONFIG_TOOLDIR_DEBUG "/home/okreylos/Share/VRTools-2.0/lib64/debug"
#define VRUI_INTERNAL_CONFIG_TOOLDIR_RELEASE "/home/okreylos/Share/VRTools-2.0/lib64"
#define VRUI_INTERNAL_CONFIG_VISLETDIR_DEBUG "/home/okreylos/Share/VRVislets-2.0/lib64/debug"
#define VRUI_INTERNAL_CONFIG_VISLETDIR_RELEASE "/home/okreylos/Share/VRVislets-2.0/lib64"
#ifdef DEBUG
	#define VRUI_INTERNAL_CONFIG_TOOLDIR VRUI_INTERNAL_CONFIG_TOOLDIR_DEBUG
	#define VRUI_INTERNAL_CONFIG_VISLETDIR VRUI_INTERNAL_CONFIG_VISLETDIR_DEBUG
#else
	#define VRUI_INTERNAL_CONFIG_TOOLDIR VRUI_INTERNAL_CONFIG_TOOLDIR_RELEASE
	#define VRUI_INTERNAL_CONFIG_VISLETDIR VRUI_INTERNAL_CONFIG_VISLETDIR_RELEASE
#endif

#define VRUI_INTERNAL_CONFIG_TOOLNAMETEMPLATE "lib%s.so"
#define VRUI_INTERNAL_CONFIG_VISLETNAMETEMPLATE "lib%s.so"

#define VRUI_INTERNAL_CONFIG_CONFIGFILENAME "Vrui"
#define VRUI_INTERNAL_CONFIG_CONFIGFILESUFFIX ".cfg"
#define VRUI_INTERNAL_CONFIG_SYSCONFIGDIR "/home/okreylos/Share"
#define VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE 1
#define VRUI_INTERNAL_CONFIG_USERCONFIGDIR ".config/Vrui-devel"
#define VRUI_INTERNAL_CONFIG_APPCONFIGDIR "Applications"
#define VRUI_INTERNAL_CONFIG_DEFAULTROOTSECTION "Desktop"

#define VRUI_INTERNAL_CONFIG_VERSION 4005003
#define VRUI_INTERNAL_CONFIG_ETCDIR "/home/okreylos/Share"
#define VRUI_INTERNAL_CONFIG_SHAREDIR "/home/okreylos/Share"

#endif
