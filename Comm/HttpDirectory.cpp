/***********************************************************************
HttpDirectory - Class to access remote directories and files over the
HTTP/1.1 protocol.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Portable Communications Library (Comm).

The Portable Communications Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Portable Communications Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Portable Communications Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <Comm/HttpDirectory.h>

#include <string.h>
#include <Misc/PrintInteger.h>
#include <Misc/ThrowStdErr.h>
#include <Comm/HttpFile.h>

namespace Comm {

/******************************
Methods of class HttpDirectory:
******************************/

void HttpDirectory::init(HttpFile::URLParts& urlParts)
	{
	/* Re-assemble the absolute URL: */
	if(urlParts.https)
		url="https://";
	else
		url="http://";
	url+=urlParts.serverName;
	if(urlParts.portNumber!=80)
		{
		url.push_back(':');
		char portBuffer[6];
		url.append(Misc::print(urlParts.portNumber,portBuffer+5));
		}
	
	/* Remember the length of the URL prefix: */
	prefixLength=(unsigned int)(url.length());
	
	/* Normalize the resource path: */
	normalizePath(urlParts.resourcePath,1);
	
	/* Append the resource path if it is not the root: */
	if(urlParts.resourcePath.length()>1)
		url.append(urlParts.resourcePath);
	}

HttpDirectory::HttpDirectory(const char* sUrl)
	{
	/* Split the given URL into its components: */
	HttpFile::URLParts urlParts=HttpFile::splitUrl(sUrl);
	
	/* Check the URL for validity: */
	if(urlParts.serverName.empty()||urlParts.portNumber<0||urlParts.portNumber>65535)
		throw OpenError(sUrl);
	
	/* Initialize the object: */
	init(urlParts);
	}

HttpDirectory::HttpDirectory(const char* sUrlBegin,const char* sUrlEnd)
	{
	/* Split the given URL into its components: */
	HttpFile::URLParts urlParts=HttpFile::splitUrl(sUrlBegin,sUrlEnd);
	
	/* Check the URL for validity: */
	if(urlParts.serverName.empty()||urlParts.portNumber<0||urlParts.portNumber>65535)
		throw OpenError(std::string(sUrlBegin,sUrlEnd).c_str());
	
	/* Initialize the object: */
	init(urlParts);
	}

HttpDirectory::HttpDirectory(const char* sUrl,unsigned int sPrefixLength)
	:url(sUrl),prefixLength(sPrefixLength)
	{
	}

HttpDirectory::~HttpDirectory(void)
	{
	}

std::string HttpDirectory::getName(void) const
	{
	/* If this is the server's root directory, return the full URL including the prefix, otherwise return the last resource path component: */
	if(url.length()==prefixLength)
		return url;
	else
		return std::string(getLastComponent(url,prefixLength),url.end());
	}

std::string HttpDirectory::getPath(void) const
	{
	/* Return the full URL including the prefix to be able to open an HTTP file with it: */
	return url;
	}

std::string HttpDirectory::getPath(const char* relativePath) const
	{
	/* Check if the given path is absolute: */
	std::string absPath;
	if(relativePath[0]=='/')
		{
		/* Use the relative path: */
		absPath=relativePath;
		}
	else
		{
		/* Append the relative path to the absolute resource path: */
		absPath=std::string(url.begin()+prefixLength,url.end());
		absPath.push_back('/');
		absPath.append(relativePath);
		}
	
	/* Normalize the absolute path and append it to the URL prefix: */
	normalizePath(absPath,1);
	std::string result(url.begin(),url.begin()+prefixLength);
	result.append(absPath);
	
	return result;
	}

bool HttpDirectory::hasParent(void) const
	{
	return url.length()>prefixLength;
	}

IO::DirectoryPtr HttpDirectory::getParent(void) const
	{
	/* Check for the special case of the root directory: */
	if(url.length()==prefixLength)
		return 0;
	else
		{
		/* Find the last component in the absolute path name: */
		std::string::const_iterator lastCompIt=getLastComponent(url,prefixLength);
		
		/* Strip off the last slash unless it's the prefix: */
		if(lastCompIt-url.begin()>prefixLength)
			--lastCompIt;
		
		/* Open and return the directory corresponding to the path name prefix before the last slash: */
		return new HttpDirectory(std::string(url.begin(),lastCompIt).c_str(),prefixLength);
		}
	}

void HttpDirectory::rewind(void)
	{
	/* Do nothing */
	}

bool HttpDirectory::readNextEntry(void)
	{
	/* Always fail: */
	return false;
	}

const char* HttpDirectory::getEntryName(void) const
	{
	/* Always fail: */
	return 0;
	}

Misc::PathType HttpDirectory::getEntryType(void) const
	{
	/* Always fail: */
	return Misc::PATHTYPE_DOES_NOT_EXIST;
	}

Misc::PathType HttpDirectory::getPathType(const char* relativePath) const
	{
	/* We can only guess at this point, so pretend it's a file: */
	return Misc::PATHTYPE_FILE;
	}

IO::FilePtr HttpDirectory::openFile(const char* fileName,IO::File::AccessMode accessMode) const
	{
	/* Check the requested access mode: */
	if(accessMode==IO::File::WriteOnly||accessMode==IO::File::ReadWrite)
		Misc::throwStdErr("Comm::HttpDirectory::openFile: Write access to HTTP files not supported");
	
	/* Assemble the file's absolute URL: */
	std::string fileUrl;
	
	/* Check if the file URL is absolute: */
	if(fileName[0]=='/')
		{
		/* Use the absolute resource path: */
		fileUrl=std::string(url.begin(),url.begin()+prefixLength);
		fileUrl.append(fileName);
		}
	else
		{
		/* Assemble the absolute resource path: */
		fileUrl=url;
		fileUrl.push_back('/');
		fileUrl.append(fileName);
		}
		
	/* Open and return the file: */
	return new HttpFile(fileUrl.c_str());
	}

IO::DirectoryPtr HttpDirectory::openDirectory(const char* directoryName) const
	{
	/* Assemble the directory's absolute URL: */
	std::string directoryUrl;
	
	/* Check if the directory URL is absolute: */
	if(directoryName[0]=='/')
		{
		/* Use the absolute resource path: */
		directoryUrl=std::string(url.begin(),url.begin()+prefixLength);
		directoryUrl.append(directoryName);
		}
	else
		{
		/* Assemble the absolute resource path: */
		directoryUrl=url;
		directoryUrl.push_back('/');
		directoryUrl.append(directoryName);
		}
	
	/* Return a new directory: */
	return new HttpDirectory(directoryUrl.c_str());
	}

}
