/***********************************************************************
OBJValueSource - Helper class to parse files in Wavefront OBJ format.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef SCENEGRAPH_INTERNAL_OBJVALUESOURCE_INCLUDED
#define SCENEGRAPH_INTERNAL_OBJVALUESOURCE_INCLUDED

#include <ctype.h>
#include <string>
#include <Misc/StringPrintf.h>
#include <Misc/ThrowStdErr.h>
#include <IO/Directory.h>
#include <IO/ValueSource.h>

namespace SceneGraph {

class OBJValueSource:public IO::ValueSource
	{
	/* Embedded classes: */
	private:
	typedef IO::ValueSource Base; // Base class type
	
	/* Elements: */
	private:
	std::string fileName; // Name of source file
	unsigned int lineNumber; // Current line number
	
	/* Private methods: */
	void skipContinuations(void)
		{
		while(!Base::eof()&&Base::peekc()=='\\')
			{
			/* Skip the rest of the line: */
			Base::skipLine();
			++lineNumber;
			Base::skipWs();
			}
		}
	
	/* Constructors and destructors: */
	public:
	OBJValueSource(const IO::Directory& directory,const std::string& sFileName)
		:Base(directory.openFile(sFileName.c_str())),fileName(sFileName),lineNumber(1)
		{
		/* Set default punctuation characters: */
		Base::setPunctuation("/#\\\n");
		skipWs();
		skipComments();
		}
	
	/* Overloaded methods from IO::ValueSource: */
	void skipWs(void)
		{
		Base::skipWs();
		skipContinuations();
		}
	void skipLine(void)
		{
		while(!Base::eof()&&Base::peekc()!='\n')
			{
			/* Check for line continuation characters: */
			if(Base::peekc()=='\\')
				{
				/* Skip the continued line end: */
				Base::skipLine();
				++lineNumber;
				}
			else
				{
				/* Skip the next character: */
				Base::getChar();
				}
			}
		}
	int readChar(void)
		{
		int result=Base::readChar();
		if(result=='\n')
			++lineNumber;
		skipContinuations();
		return result;
		}
	std::string readString(void)
		{
		std::string result=Base::readString();
		skipContinuations();
		return result;
		}
	std::string readLine(void)
		{
		std::string result;
		skipWs();
		while(!Base::eof()&&Base::peekc()!='\n')
			{
			/* Check for line continuation characters: */
			if(Base::peekc()=='\\')
				{
				/* Skip the continued line end: */
				Base::skipLine();
				++lineNumber;
				}
			else
				{
				/* Read and store the next character: */
				result.push_back(Base::getChar());
				}
			}
		
		/* Trim whitespace from the end of the read string: */
		while(isspace(result.back()))
			result.pop_back();
		
		return result;
		}
	int readInteger(void)
		{
		int result=0;
		try
			{
			result=Base::readInteger();
			skipContinuations();
			}
		catch(const Base::NumberError& err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%u",fileName.c_str(),lineNumber);
			}
		return result;
		}
	unsigned int readUnsignedInteger(void)
		{
		unsigned int result=0;
		try
			{
			result=Base::readUnsignedInteger();
			skipContinuations();
			}
		catch(const Base::NumberError& err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%u",fileName.c_str(),lineNumber);
			}
		return result;
		}
	double readNumber(void)
		{
		double result=0.0;
		try
			{
			result=Base::readNumber();
			skipContinuations();
			}
		catch(const Base::NumberError& err)
			{
			Misc::throwStdErr("OBJValueSource: Number format error at %s:%u",fileName.c_str(),lineNumber);
			}
		return result;
		}
	
	/* New methods: */
	bool eol(void) const
		{
		return Base::eof()||Base::peekc()=='\n';
		}
	void skipComments(void)
		{
		while(!Base::eof()&&(Base::peekc()=='\n'||Base::peekc()=='#'))
			{
			skipLine();
			readChar();
			}
		}
	void finishLine(void)
		{
		skipLine();
		readChar();
		skipComments();
		}
	std::string where(void) const // Returns a string with the current file:line location
		{
		return Misc::stringPrintf("%s:%u",fileName.c_str(),lineNumber);
		}
	Color readColor(void) // Returns an RGB color
		{
		Color result;
		for(int i=0;i<3;++i)
			result[i]=GLfloat(readNumber());
		return result;
		}
	};

}

#endif
