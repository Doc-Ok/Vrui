/***********************************************************************
FindHMD - Utility to find a connected HMD based on its preferred video
mode, using the X11 Xrandr extension.
Copyright (c) 2018-2020 Oliver Kreylos

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

#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define VERBOSE 0

int xrandrErrorBase;
bool hadError=false;

int errorHandler(Display* display,XErrorEvent* err)
	{
	if(err->error_code==BadValue)
		std::cout<<"X error: bad value"<<std::endl;
	else
		std::cout<<"X error: unknown error"<<std::endl;
	
	hadError=true;
	return 0;
	}

XRRModeInfo* findMode(XRRScreenResources* screenResources,RRMode modeId)
	{
	/* Find the mode ID in the screen resource's modes: */
	for(int i=0;i<screenResources->nmode;++i)
		if(screenResources->modes[i].id==modeId)
			return &screenResources->modes[i];
	
	return 0;
	}

void printMode(std::ostream& os,XRRScreenResources* screenResources,RRMode modeId)
	{
	XRRModeInfo* mode=findMode(screenResources,modeId);
	if(mode!=0)
		os<<mode->width<<'x'<<mode->height<<'@'<<double(mode->dotClock)/(double(mode->vTotal)*double(mode->hTotal));
	else
		os<<"<not found>";
	}

int findHmd(Display* display,const unsigned int size[2],double rate,double rateFuzz,bool printGeometry)
	{
	/* Find the HMD's output name, whether it is enabled, and its geometry: */
	std::string hmdOutputName;
	bool hmdEnabled=false;
	int hmdGeometry[4]={0,0,0,0};
	
	/* Iterate through all of the display's screens: */
	for(int screen=0;screen<XScreenCount(display)&&hmdOutputName.empty();++screen)
		{
		/* Get the screen's resources: */
		XRRScreenResources* screenResources=XRRGetScreenResources(display,RootWindow(display,screen));
		
		/* Check all outputs for a connected display with a preferred mode matching the query: */
		for(int output=0;output<screenResources->noutput&&hmdOutputName.empty();++output)
			{
			/* Get the output descriptor and check if there is a display connected: */
			XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,screenResources->outputs[output]);
			if(outputInfo->connection==RR_Connected&&outputInfo->nmode>0&&outputInfo->npreferred>0&&outputInfo->npreferred<=outputInfo->nmode)
				{
				#if VERBOSE
				std::cout<<"FindHMD: Output "<<std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen)<<" modes:";
				for(int i=0;i<outputInfo->nmode;++i)
					{
					std::cout<<' ';
					printMode(std::cout,screenResources,outputInfo->modes[i]);
					}
				std::cout<<std::endl;
				std::cout<<"\tpreferred mode: ";
				printMode(std::cout,screenResources,outputInfo->modes[outputInfo->npreferred-1]);
				std::cout<<std::endl;
				#endif
				
				/* Retrieve a mode descriptor for the connected display's preferred mode: */
				XRRModeInfo* preferredMode=findMode(screenResources,outputInfo->modes[outputInfo->npreferred-1]);
				if(preferredMode!=0)
					{
					#if VERBOSE
					std::cout<<"FindHMD: Output "<<std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen);
					std::cout<<" preferred mode is ";
					printMode(std::cout,screenResources,outputInfo->modes[outputInfo->npreferred-1]);
					std::cout<<std::endl;
					#endif
					
					/* Check if the connected display's preferred mode matches the query: */
					if(preferredMode->width==size[0]&&preferredMode->height==size[1])
						{
						/* Calculate the mode's refresh rate and check if it matches the query: */
						double modeRate=double(preferredMode->dotClock)/(double(preferredMode->vTotal)*double(preferredMode->hTotal));
						if(modeRate>=rate/(rateFuzz+1.0)&&modeRate<=rate*(rateFuzz+1.0))
							{
							/* Remember the video output port to which the HMD is connected: */
							hmdOutputName=std::string(outputInfo->name,outputInfo->name+outputInfo->nameLen);
							
							/* Check if the HMD's display is enabled: */
							hmdEnabled=outputInfo->crtc!=None;
							if(hmdEnabled)
								{
								/* Find the HMD's crtc position: */
								XRRCrtcInfo* crtcInfo=XRRGetCrtcInfo(display,screenResources,outputInfo->crtc);
								hmdGeometry[0]=crtcInfo->width;
								hmdGeometry[1]=crtcInfo->height;
								hmdGeometry[2]=crtcInfo->x;
								hmdGeometry[3]=crtcInfo->y;
								
								/* Clean up: */
								XRRFreeCrtcInfo(crtcInfo);
								}
							}
						}
					}
				}
			
			/* Clean up: */
			XRRFreeOutputInfo(outputInfo);
			}
		
		/* Clean up: */
		XRRFreeScreenResources(screenResources);
		}
	
	if(hmdOutputName.empty())
		{
		std::cerr<<"FindHMD: No HMD matching display specifications "<<size[0]<<'x'<<size[1]<<'@'<<rate<<" found"<<std::endl;
		return 1;
		}
	else if(hmdEnabled)
		{
		/* Print the output's name or geometry: */
		if(printGeometry)
			std::cout<<hmdGeometry[0]<<'x'<<hmdGeometry[1]<<'+'<<hmdGeometry[2]<<'+'<<hmdGeometry[3]<<std::endl;
		else
			std::cout<<hmdOutputName<<std::endl;
		return 0;
		}
	else
		{
		/* Print the output's name, but throw an error: */
		std::cout<<hmdOutputName<<std::endl;
		std::cerr<<"FindHMD: HMD found on video output port "<<hmdOutputName<<", but is not enabled"<<std::endl;
		return 2;
		}
	}

int createXrandrCommand(Display* display,const unsigned int size[2],double rate,double rateFuzz,bool enableHmd)
	{
	/* Collect the name of the HMD's output and its best-fitting mode: */
	std::string hmdOutputName;
	RRMode hmdMode;
	
	/* Calculate the bounding box of all connected and enabled displays: */
	int nonHmdBox[4]={32768,32768,-32768,-32768};
	
	/* Keep track of the primary monitor/output: */
	bool foundPrimary=false;
	
	/* Iterate through all of the display's screens: */
	for(int screen=0;screen<XScreenCount(display);++screen)
		{
		/* Get the screen's resources: */
		XRRScreenResources* screenResources=XRRGetScreenResources(display,RootWindow(display,screen));
		
		/* Find the primary monitor: */
		int numMonitors=0;
		XRRMonitorInfo* monitors=XRRGetMonitors(display,RootWindow(display,screen),True,&numMonitors);
		XRRMonitorInfo* primaryMonitor=0;
		for(int monitor=0;monitor<numMonitors;++monitor)
			if(monitors[monitor].primary)
				primaryMonitor=&monitors[monitor];
		
		/* Iterate through the screen's outputs: */
		for(int output=0;output<screenResources->noutput;++output)
			{
			/* Get the output descriptor and check if there is a display connected: */
			XRROutputInfo* outputInfo=XRRGetOutputInfo(display,screenResources,screenResources->outputs[output]);
			std::string outputName(outputInfo->name,outputInfo->name+outputInfo->nameLen);
			if(outputInfo->connection==RR_Connected&&outputInfo->nmode>0&&outputInfo->npreferred>0&&outputInfo->npreferred<=outputInfo->nmode)
				{
				#if VERBOSE
				std::cout<<"FindHMD: Output "<<outputName<<" modes:";
				for(int i=0;i<outputInfo->nmode;++i)
					{
					std::cout<<' ';
					printMode(std::cout,screenResources,outputInfo->modes[i]);
					}
				std::cout<<std::endl;
				std::cout<<"\tpreferred mode: ";
				printMode(std::cout,screenResources,outputInfo->modes[outputInfo->npreferred-1]);
				std::cout<<std::endl;
				#endif
				
				/* Check if the connected display has a mode that matches the query: */
				bool isHmd=false;
				for(int mode=0;mode<outputInfo->nmode&&!isHmd;++mode)
					{
					XRRModeInfo* modeInfo=findMode(screenResources,outputInfo->modes[mode]);
					double modeRate=double(modeInfo->dotClock)/(double(modeInfo->vTotal)*double(modeInfo->hTotal));
					isHmd=modeInfo->width==size[0]&&modeInfo->height==size[1]&&modeRate>=rate/(rateFuzz+1.0)&&modeRate<=rate*(rateFuzz+1.0);
					if(isHmd)
						{
						hmdOutputName=outputName;
						hmdMode=modeInfo->id;
						}
					}
				
				if(!isHmd)
					{
					/* Configure the non-HMD output: */
					std::cout<<" --output "<<outputName;
					
					/* Check if the connected display is enabled: */
					if(outputInfo->crtc!=None)
						{
						/* Set the connected display to its current mode: */
						XRRCrtcInfo* crtcInfo=XRRGetCrtcInfo(display,screenResources,outputInfo->crtc);
						XRRModeInfo* modeInfo=findMode(screenResources,crtcInfo->mode);
						std::cout<<" --mode 0x"<<std::hex<<modeInfo->id<<std::dec<<" --pos "<<crtcInfo->x<<'x'<<crtcInfo->y;
						
						/* Add the display to the non-HMD bounding box: */
						if(nonHmdBox[0]>crtcInfo->x)
							nonHmdBox[0]=crtcInfo->x;
						if(nonHmdBox[1]>crtcInfo->y)
							nonHmdBox[1]=crtcInfo->y;
						if(nonHmdBox[2]<crtcInfo->x+int(crtcInfo->width))
							nonHmdBox[2]=crtcInfo->x+int(crtcInfo->width);
						if(nonHmdBox[3]<crtcInfo->y+int(crtcInfo->height))
							nonHmdBox[3]=crtcInfo->y+int(crtcInfo->height);
						
						/* Clean up: */
						XRRFreeCrtcInfo(crtcInfo);
						
						/* Check if this output should be the primary: */
						if(primaryMonitor!=0)
							{
							for(int moutput=0;moutput<primaryMonitor->noutput&&!foundPrimary;++moutput)
								if(primaryMonitor->outputs[moutput]==screenResources->outputs[output])
									{
									std::cout<<" --primary";
									foundPrimary=true;
									}
							}
						}
					else
						{
						/* Keep the connected display disabled: */
						std::cout<<" --off";
						}
					}
				}
			
			/* Clean up: */
			XRRFreeOutputInfo(outputInfo);
			}
		
		/* Clean up: */
		XRRFreeMonitors(monitors);
		XRRFreeScreenResources(screenResources);
		}
	
	/* Check if a matching HMD was found: */
	if(!hmdOutputName.empty())
		{
		if(!foundPrimary&&enableHmd)
			{
			/* Make the last non-HMD output the primary: */
			std::cout<<" --primary";
			}
		
		/* Add commands to enable or disable the HMD: */
		std::cout<<" --output "<<hmdOutputName;
		if(enableHmd)
			{
			std::cout<<" --mode 0x"<<std::hex<<hmdMode<<std::dec;
			
			/* Position the HMD to the right of all other displays: */
			std::cout<<" --pos "<<nonHmdBox[2]<<'x'<<nonHmdBox[1];
			}
		else
			std::cout<<" --off";
		
		std::cout<<std::endl;
		return 0;
		}
	else
		{
		std::cout<<std::endl;
		std::cerr<<"FindHMD: No HMD matching display specifications "<<size[0]<<'x'<<size[1]<<'@'<<rate<<" found"<<std::endl;
		return 1;
		}
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	const char* displayName=getenv("DISPLAY");
	unsigned int size[2]={2160,1200};
	double rate=89.5273;
	double rateFuzz=0.01;
	int mode=0;
	bool printGeometry=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"h")==0)
				{
				std::cout<<"Usage: "<<argv[0]<<" [-display <display name>] [-size <width> <height>] [-rate <rate>] [-rateFuzz <rate fuzz>] [-enableCmd] [-disableCmd] [-printGeometry]"<<std::endl;
				std::cout<<"\t-display <display name> : Connect to the X display of the given name; defaults to standard display"<<std::endl;
				std::cout<<"\t-size <width> <height>  : Size of the desired HMD's screen in pixels; defaults to 2160x1200"<<std::endl;
				std::cout<<"\t-rate <rate>            : Refresh rate of the desired HMD's screen in Hz; defaults to 89.5273"<<std::endl;
				std::cout<<"\t-rateFuzz <rate fuzz>   : Fuzz factor for refresh rate comparisons; defaults to 0.01"<<std::endl;
				std::cout<<"\t-enableCmd              : Print an xrandr option list to enable the desired HMD"<<std::endl;
				std::cout<<"\t-disableCmd             : Print an xrandr option list to disable the desired HMD"<<std::endl;
				std::cout<<"\t-printGeometry          : Print the position and size of the HMD's screen in virtual screen coordinates"<<std::endl;
				
				return 0;
				}
			else if(strcasecmp(argv[i]+1,"display")==0)
				{
				++i;
				displayName=argv[i];
				}
			else if(strcasecmp(argv[i]+1,"size")==0)
				{
				for(int j=0;j<2;++j)
					{
					++i;
					size[j]=atoi(argv[i]);
					}
				}
			else if(strcasecmp(argv[i]+1,"rate")==0)
				{
				++i;
				rate=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"rateFuzz")==0)
				{
				++i;
				rateFuzz=atof(argv[i]);
				}
			else if(strcasecmp(argv[i]+1,"enableCmd")==0)
				mode=1;
			else if(strcasecmp(argv[i]+1,"disableCmd")==0)
				mode=2;
			else if(strcasecmp(argv[i]+1,"printGeometry")==0)
				printGeometry=true;
			else
				std::cerr<<"Ignoring unrecognized option "<<argv[i]<<std::endl;
			}
		}
	
	if(displayName==0)
		{
		std::cerr<<"FindHMD: No display name provided"<<std::endl;
		return 1;
		}
	
	/* Open a connection to the X display: */
	Display* display=XOpenDisplay(displayName);
	if(display==0)
		{
		std::cerr<<"FindHMD: Unable to connect to display "<<displayName<<std::endl;
		return 1;
		}
	
	/* Set the error handler: */
	XSetErrorHandler(errorHandler);
	
	/* Query the Xrandr extension: */
	int xrandrEventBase;
	int xrandrMajor,xrandrMinor;
	if(!XRRQueryExtension(display,&xrandrEventBase,&xrandrErrorBase)||XRRQueryVersion(display,&xrandrMajor,&xrandrMinor)==0)
		{
		std::cerr<<"FindHMD: Display "<<displayName<<" does not support RANDR extension"<<std::endl;
		XCloseDisplay(display);
		return 1;
		}
	
	#if VERBOSE
	std::cout<<"FindHMD: Found RANDR extension version "<<xrandrMajor<<'.'<<xrandrMinor<<std::endl;
	#endif
	
	/* Do the thing: */
	int result=0;
	switch(mode)
		{
		case 0: // Find the HMD's output port name and/or screen geometry: */
			result=findHmd(display,size,rate,rateFuzz,printGeometry);
			break;
		
		case 1: // Construct an xrandr command line to enable the HMD's display: */
			result=createXrandrCommand(display,size,rate,rateFuzz,true);
			break;
		
		case 2: // Construct an xrandr command line to disable the HMD's display: */
			result=createXrandrCommand(display,size,rate,rateFuzz,false);
			break;
		}
	
	/* Clean up and return: */
	XCloseDisplay(display);
	return result;
	}
