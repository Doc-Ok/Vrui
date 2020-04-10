/***********************************************************************
JsonSource - Class to retrieve JSON entities from JSON files.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the I/O Support Library (IO).

The I/O Support Library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The I/O Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the I/O Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <IO/JsonSource.h>

#include <IO/OpenFile.h>
#include <IO/JsonEntityTypes.h>

namespace IO {

/***************************
Methods of class JsonSource:
***************************/

JsonSource::JsonSource(const char* fileName)
	:file(openFile(fileName))
	{
	/* Set up the JSON file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,");
	file.setQuote('"',true);
	file.setEscape('\\');
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonSource::JsonSource(FilePtr sFile)
	:file(sFile)
	{
	/* Set up the JSON file syntax: */
	file.setWhitespace('\n',true);
	file.setWhitespace('\r',true);
	file.setPunctuation("{}[]:,");
	file.setQuote('"',true);
	file.setEscape('\\');
	
	/* Prepare for reading: */
	file.skipWs();
	}

JsonPointer JsonSource::parseEntity(void)
	{
	/* Determine the type of the next entity: */
	switch(file.peekc())
		{
		case '"': // String
			{
			/* Parse a string: */
			return new JsonString(file.readString());
			}
		
		case '[': // Array
			{
			/* Skip the opening bracket: */
			file.skipString();
			
			/* Create a new array entity: */
			JsonArray* array=new JsonArray;
			
			/* Parse array items until the closing bracket: */
			while(true)
				{
				/* Parse the next array item: */
				JsonPointer item=parseEntity();
				array->getArray().push_back(item);
				
				/* Check for comma or closing bracket: */
				if(file.peekc()==',')
					{
					/* Skip the comma: */
					file.skipString();
					}
				else if(file.peekc()==']')
					{
					/* Skip the closing bracket and end the array: */
					file.skipString();
					break;
					}
				else
					throw std::runtime_error("JsonSource::parseEntity: Illegal token in array");
				}
			
			return array;
			}
		
		case '{': // Object
			{
			/* Skip the opening brace: */
			file.skipString();
			
			/* Create a new object entity: */
			JsonObject* object=new JsonObject;
			
			/* Parse (name, value) pairs until the closing brace: */
			while(true)
				{
				/* Parse the next entity name: */
				if(file.peekc()!='"')
					throw std::runtime_error("JsonSource::parseEntity: No name in object item");
				std::string name=file.readString();
				
				/* Check for the colon: */
				if(!file.isLiteral(':'))
					throw std::runtime_error("JsonSource::parseEntity: Missing colon in object item");
				
				/* Parse the next entity: */
				JsonPointer entity=parseEntity();
				
				/* Store the association: */
				object->getMap()[name]=entity;
				
				/* Check for comma or closing brace: */
				if(file.peekc()==',')
					{
					/* Skip the comma: */
					file.skipString();
					}
				else if(file.peekc()=='}')
					{
					/* Skip the closing brace and end the object: */
					file.skipString();
					break;
					}
				else
					throw std::runtime_error("JsonSource::parseEntity: Illegal token in object");
				}
			
			return object;
			}
		
		case 'F': // Boolean literal
		case 'f':
		case 'T':
		case 't':
			{
			std::string value=file.readString();
			if(strcasecmp(value.c_str(),"true")==0)
				return new JsonBoolean(true);
			else if(strcasecmp(value.c_str(),"false")==0)
				return new JsonBoolean(false);
			else
				throw std::runtime_error("JsonSource::parseEntity: Illegal boolean literal");
			}
		
		case 'n': // NULL value
		case 'N':
			{
			std::string null=file.readString();
			if(strcasecmp(null.c_str(),"null")==0)
				return 0;
			else
				throw std::runtime_error("JsonSource::parseEntity: Illegal null value");
			}
		
		case '+': // Number
		case '-':
		case '.':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
			/* Parse a number: */
			double number=file.readNumber();
			return new JsonNumber(number);
			}
		
		default:
			throw std::runtime_error("JsonSource::parseEntity: Illegal token");
		}
	}

}
