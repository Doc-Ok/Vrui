/***********************************************************************
JsonEntityTypes - Classes for concrete entities parsed from JSON
(JavaScript Object Notation) files.
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

#include <IO/JsonEntityTypes.h>

namespace IO {

/****************************
Methods of class JsonBoolean:
****************************/

JsonEntity::EntityType JsonBoolean::getType(void) const
	{
	return BOOLEAN;
	}

std::string JsonBoolean::getTypeName(void) const
	{
	return "Boolean";
	}

void JsonBoolean::print(std::ostream& os) const
	{
	os<<(value?"true":"false");
	}

/***************************
Methods of class JsonNumber:
***************************/

JsonEntity::EntityType JsonNumber::getType(void) const
	{
	return NUMBER;
	}

std::string JsonNumber::getTypeName(void) const
	{
	return "Number";
	}

void JsonNumber::print(std::ostream& os) const
	{
	os<<number;
	}

/***************************
Methods of class JsonString:
***************************/

JsonEntity::EntityType JsonString::getType(void) const
	{
	return STRING;
	}

std::string JsonString::getTypeName(void) const
	{
	return "String";
	}

void JsonString::print(std::ostream& os) const
	{
	os<<'"'<<string<<'"';
	}

/**************************
Methods of class JsonArray:
**************************/

JsonEntity::EntityType JsonArray::getType(void) const
	{
	return ARRAY;
	}

std::string JsonArray::getTypeName(void) const
	{
	return "Array";
	}

void JsonArray::print(std::ostream& os) const
	{
	os<<'[';
	Array::const_iterator aIt=array.begin();
	if(aIt!=array.end())
		{
		(*aIt)->print(os);
		++aIt;
		}
	for(;aIt!=array.end();++aIt)
		{
		os<<',';
		(*aIt)->print(os);
		}
	os<<']';
	}

/**************************
Methods of class JsonObject:
**************************/

JsonEntity::EntityType JsonObject::getType(void) const
	{
	return OBJECT;
	}

std::string JsonObject::getTypeName(void) const
	{
	return "Object";
	}

void JsonObject::print(std::ostream& os) const
	{
	os<<'{';
	Map::ConstIterator mIt=map.begin();
	if(!mIt.isFinished())
		{
		os<<'"'<<mIt->getSource()<<'"'<<':';
		mIt->getDest()->print(os);
		++mIt;
		}
	for(;!mIt.isFinished();++mIt)
		{
		os<<',';
		os<<'"'<<mIt->getSource()<<'"'<<':';
		mIt->getDest()->print(os);
		}
	os<<'}';
	}

}
