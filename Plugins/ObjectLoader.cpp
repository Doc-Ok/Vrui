/***********************************************************************
ObjectLoader - Light-weight class to load objects from dynamic shared
object files (DSOs).
Copyright (c) 2019 Oliver Kreylos

This file is part of the Plugin Handling Library (Plugins).

The Plugin Handling Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Plugin Handling Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Plugin Handling Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <Plugins/ObjectLoader.h>

#include <Misc/PrintInteger.h>

namespace Plugins {

/*********************************
Methods of class ObjectLoaderBase:
*********************************/

void* ObjectLoaderBase::loadDso(const char* className)
	{
	/* Assemble a DSO name from the class name and the template, leaving an optional %u conversion as-is: */
	std::string dsoName(dsoNameTemplate.begin(),classNameStart);
	dsoName.append(className);
	dsoName.append(classNameStart+2,dsoNameTemplate.end());
	
	/* Locate the DSO: */
	std::string fullDsoName;
	try
		{
		/* Check if the DSO name template contains a version number: */
		if(versionStart!=dsoNameTemplate.end())
			{
			/* Find the highest-version DSO that matches the class name: */
			fullDsoName=dsoLocator.locateNumberedFile(dsoName.c_str());
			}
		else
			{
			/* Find the DSO name as constructed: */
			fullDsoName=dsoLocator.locateFile(dsoName.c_str());
			}
		}
	catch(const std::runtime_error& err)
		{
		/* Re-throw the error as an object loader error: */
		throw Error(err.what());
		}
	
	/* Open the DSO: */
	void* dsoHandle=dlopen(fullDsoName.c_str(),RTLD_LAZY|RTLD_GLOBAL);
	if(dsoHandle==0)
		throw DsoError(dlerror());
	
	return dsoHandle;
	}

void* ObjectLoaderBase::loadDso(const char* className,unsigned int version)
	{
	/* Assemble a DSO name from the class name and version number and the template: */
	std::string dsoName;
	if(classNameStart<versionStart)
		{
		/* Insert class name first, then version number: */
		dsoName.append(dsoNameTemplate.begin(),classNameStart);
		dsoName.append(className);
		dsoName.append(classNameStart+2,versionStart);
		char versionBuffer[16];
		dsoName.append(Misc::print(version,versionBuffer+15));
		dsoName.append(versionStart+2,dsoNameTemplate.end());
		}
	else
		{
		/* Insert version number first, then class name: */
		dsoName.append(dsoNameTemplate.begin(),versionStart);
		char versionBuffer[16];
		dsoName.append(Misc::print(version,versionBuffer+15));
		dsoName.append(versionStart+2,classNameStart);
		dsoName.append(className);
		dsoName.append(classNameStart+2,dsoNameTemplate.end());
		}
	
	/* Locate the DSO: */
	std::string fullDsoName;
	try
		{
		fullDsoName=dsoLocator.locateFile(dsoName.c_str());
		}
	catch(const std::runtime_error& err)
		{
		/* Re-throw the error as an object loader error: */
		throw Error(err.what());
		}
	
	/* Open the DSO: */
	void* dsoHandle=dlopen(fullDsoName.c_str(),RTLD_LAZY|RTLD_GLOBAL);
	if(dsoHandle==0)
		throw DsoError(dlerror());
	
	return dsoHandle;
	}

ObjectLoaderBase::ObjectLoaderBase(const char* sDsoNameTemplate)
	{
	/* Split the DSO name template into base directory and file name and check it for validity: */
	const char* templateStart=sDsoNameTemplate;
	const char* cStart=0;
	const char* vStart=0;
	for(const char* tPtr=templateStart;*tPtr!='\0';++tPtr)
		{
		if(*tPtr=='/'&&cStart==0&&vStart==0) // Find the last slash before the %s and potential %u conversion
			templateStart=tPtr+1;
		else if(*tPtr=='%'&&tPtr[1]!='%')
			{
			if(tPtr[1]=='s'&&cStart==0)
				cStart=tPtr;
			else if(tPtr[1]=='u'&&vStart==0)
				vStart=tPtr;
			else
				Misc::throwStdErr("ObjectLoader::ObjectLoader: Invalid DSO name template %s",sDsoNameTemplate);
			}
		}
	if(cStart==0)
		Misc::throwStdErr("ObjectLoader::ObjectLoader: Invalid DSO name template %s",sDsoNameTemplate);
	
	/* Store the DSO name template and the conversion iterators: */
	dsoNameTemplate=templateStart;
	classNameStart=dsoNameTemplate.begin()+(cStart-templateStart);
	if(vStart!=0)
		versionStart=dsoNameTemplate.begin()+(vStart-templateStart);
	else
		versionStart=dsoNameTemplate.end();
	
	/* Store the DSO name template's directory prefix in the file locator: */
	if(templateStart!=sDsoNameTemplate)
		dsoLocator.addPath(std::string(sDsoNameTemplate,templateStart));
	}

}
