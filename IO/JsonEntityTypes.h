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

#ifndef IO_JSONENTITYTYPES_INCLUDED
#define IO_JSONENTITYTYPES_INCLUDED

#include <string>
#include <vector>
#include <stdexcept>
#include <Misc/StringHashFunctions.h>
#include <Misc/HashTable.h>
#include <IO/JsonEntity.h>

namespace IO {

class JsonBoolean:public JsonEntity // Class for boolean values
	{
	/* Elements: */
	private:
	bool value; // The represented boolean value
	
	/* Constructors and destructors: */
	public:
	JsonBoolean(bool sValue)
		:value(sValue)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const;
	virtual std::string getTypeName(void) const;
	virtual void print(std::ostream& os) const;
	
	/* New methods: */
	bool getBoolean(void) const // Returns the represented boolean value
		{
		return value;
		}
	};

class JsonNumber:public JsonEntity // Class for floating-point numerical values
	{
	/* Elements: */
	private:
	double number; // The represented number
	
	/* Constructors and destructors: */
	public:
	JsonNumber(double sNumber)
		:number(sNumber)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const;
	virtual std::string getTypeName(void) const;
	virtual void print(std::ostream& os) const;
	
	/* New methods: */
	double getNumber(void) const // Returns the represented number
		{
		return number;
		}
	};

class JsonString:public JsonEntity // Class for string values
	{
	/* Elements: */
	private:
	std::string string; // The represented string
	
	/* Constructors and destructors: */
	public:
	JsonString(const char* sString)
		:string(sString)
		{
		}
	JsonString(const std::string& sString)
		:string(sString)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const;
	virtual std::string getTypeName(void) const;
	virtual void print(std::ostream& os) const;
	
	/* New methods: */
	const std::string& getString(void) const // Returns the represented string
		{
		return string;
		}
	};

class JsonArray:public JsonEntity // Class representing ordered lists of entities
	{
	/* Embedded classes: */
	public:
	typedef std::vector<JsonPointer> Array; // Type for arrays of JSON entities
	
	/* Elements: */
	private:
	Array array; // The represented array
	
	/* Methods from class JsonEntity: */
	public:
	virtual EntityType getType(void) const;
	virtual std::string getTypeName(void) const;
	virtual void print(std::ostream& os) const;
	
	/* New methods: */
	const Array& getArray(void) const // Returns the represented array
		{
		return array;
		}
	Array& getArray(void) // Ditto
		{
		return array;
		}
	bool empty(void) const // Returns true if the array is empty
		{
		return array.empty();
		}
	size_t size(void) const // Returns the number of elements in the array
		{
		return array.size();
		}
	JsonPointer getItem(size_t index) // Returns the array item of the given index
		{
		return array[index];
		}
	};

class JsonObject:public JsonEntity // Class representing an unordered collection of name-value pairs
	{
	/* Embedded classes: */
	public:
	typedef Misc::HashTable<std::string,JsonPointer> Map; // Type for maps mapping strings to json entities
	
	/* Elements: */
	private:
	Map map; // The associative map representing the JSON object
	
	/* Constructors and destructors: */
	public:
	JsonObject(size_t initialMapSize =17) // Creates an empty JSON object
		:map(initialMapSize)
		{
		}
	
	/* Methods from class JsonEntity: */
	virtual EntityType getType(void) const;
	virtual std::string getTypeName(void) const;
	virtual void print(std::ostream& os) const;
	
	/* New methods: */
	const Map& getMap(void) const // Returns the represented map
		{
		return map;
		}
	Map& getMap(void) // Ditto
		{
		return map;
		}
	bool hasProperty(const std::string& name) const // Returns true if an entity is associated with the given name
		{
		return map.isEntry(name);
		}
	JsonPointer getProperty(const std::string& name) // Returns the json entity associated with the given name
		{
		/* Return the entity associated with the given name, throwing an exception if it is not there: */
		return map.getEntry(name).getDest();
		}
	};

typedef Misc::Autopointer<JsonBoolean> JsonBooleanPointer; // Type for pointers to JSON booleans
typedef Misc::Autopointer<JsonNumber> JsonNumberPointer; // Type for pointers to JSON numbers
typedef Misc::Autopointer<JsonString> JsonStringPointer; // Type for pointers to JSON strings
typedef Misc::Autopointer<JsonArray> JsonArrayPointer; // Type for pointers to JSON arrays
typedef Misc::Autopointer<JsonObject> JsonObjectPointer; // Type for pointers to JSON objects

/****************
Helper functions:
****************/

inline bool getBoolean(JsonPointer entity) // Returns the boolean value represented by the given JSON entity; throws exception if entity is not a boolean
	{
	JsonBooleanPointer boolean(entity);
	if(boolean==0)
		throw std::runtime_error("IO::getBoolean: JSON entity is not a boolean");
	
	return boolean->getBoolean();
	}

inline double getNumber(JsonPointer entity) // Returns the number represented by the given JSON entity; throws exception if entity is not a number
	{
	JsonNumberPointer number(entity);
	if(number==0)
		throw std::runtime_error("IO::getNumber: JSON entity is not a number");
	
	return number->getNumber();
	}

inline const std::string& getString(JsonPointer entity) // Returns the string represented by the given JSON entity; throws exception if entity is not a string
	{
	JsonStringPointer string(entity);
	if(string==0)
		throw std::runtime_error("IO::getString: JSON entity is not a string");
	
	return string->getString();
	}

inline const JsonArray::Array& getArray(JsonPointer entity) // Returns the array represented by the given JSON entity; throws exception if entity is not an array
	{
	JsonArrayPointer array(entity);
	if(array==0)
		throw std::runtime_error("IO::getArray: JSON entity is not an array");
	
	return array->getArray();
	}

inline const JsonObject::Map& getObject(JsonPointer entity) // Returns the associative map represented by the given JSON entity; throws exception if entity is not an object
	{
	JsonObjectPointer object(entity);
	if(object==0)
		throw std::runtime_error("IO::getObject: JSON entity is not an object");
	
	return object->getMap();
	}

inline JsonPointer getObjectProperty(JsonPointer entity,const std::string& name) // Returns the property of the given name of the object represented by the given JSON entity; throws exception if entity is not an object or property does not exist
	{
	JsonObjectPointer object(entity);
	if(object==0)
		throw std::runtime_error("IO::getObjectProperty: JSON entity is not an object");
	
	return object->getProperty(name);
	}

}

#endif
