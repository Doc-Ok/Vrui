/***********************************************************************
ObjectLoader - Light-weight class to load objects from dynamic shared
object files (DSOs).
Copyright (c) 2009-2019 Oliver Kreylos

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

#ifndef PLUGINS_OBJECTLOADER_INCLUDED
#define PLUGINS_OBJECTLOADER_INCLUDED

#include <string>
#include <stdexcept>
#include <Misc/HashTable.h>
#include <Misc/FileLocator.h>

namespace Plugins {

class ObjectLoaderBase // Base class for object loader classes to factor out non-template code
	{
	/* Embedded classes: */
	public:
	struct Error:public std::runtime_error
		{
		/* Constructors and destructors: */
		Error(const std::string& cause)
			:std::runtime_error(cause)
			{
			}
		};
	
	struct DsoError:public Error // Error type thrown if something goes wrong while processing a DSO
		{
		/* Constructors and destructors: */
		DsoError(const std::string& cause) // Creates DsoError object from error string returned by dl_* calls
			:Error(std::string("Object loader DSO error: ")+cause)
			{
			}
		};
	
	/* Elements: */
	protected:
	std::string dsoNameTemplate; // printf-style format string to create DSO names from class names with exactly one %s conversion and zero or one %u conversions
	std::string::iterator classNameStart; // Position of class name %s conversion in DSO name template
	std::string::iterator versionStart; // Position of version number %u conversion in DSO name template, or dsoNameTemplate.end()
	Misc::FileLocator dsoLocator; // File locator to find DSO files
	
	/* Protected methods: */
	void* loadDso(const char* className); // Loads a DSO for the given file name and returns low-level DSO pointer
	void* loadDso(const char* className,unsigned int version); // Ditto, with specific version number
	
	/* Constructors and destructors: */
	public:
	ObjectLoaderBase(const char* sDsoNameTemplate); // Initializes DSO locator search path to template's base directory
	
	/* Methods: */
	const Misc::FileLocator& getDsoLocator(void) const // Returns reference to the DSO file locator
		{
		return dsoLocator;
		}
	Misc::FileLocator& getDsoLocator(void) // Ditto
		{
		return dsoLocator;
		}
	};

template <class ManagedClassParam>
class ObjectLoader:public ObjectLoaderBase
	{
	/* Embedded classes: */
	public:
	typedef ManagedClassParam ManagedClass; // Base class of objects that can be loaded
	
	private:
	typedef void (*DestroyObjectFunction)(ManagedClass*); // Type of object destruction function stored in DSOs
	
	struct DsoState // Structure retaining information about loaded DSOs
		{
		/* Elements: */
		public:
		void* dsoHandle; // Handle of DSO containing class code
		DestroyObjectFunction destroyObjectFunction; // Pointer to function to destroy objects of this class
		
		/* Constructors and destructors: */
		DsoState(void* sDsoHandle);
		};
	
	typedef Misc::HashTable<ManagedClass*,DsoState> DsoStateHasher; // Type for hash tables to find the DSO state of an object
	
	/* Elements: */
	DsoStateHasher dsoStates; // Hash table of DSO states of created objects
	
	/* Constructors and destructors: */
	public:
	ObjectLoader(const char* sDsoNameTemplate); // Creates "empty" manager; initializes DSO locator search path to template's base directory
	~ObjectLoader(void); // Releases all loaded object classes and DSOs
	
	/* Methods: */
	ManagedClass* createObject(const char* className); // Creates an object of the given class name by searching for a matching DSO
	ManagedClass* createObject(const char* className,unsigned int version); // Ditto, using an additional version number; requires a %u conversion somewhere in the DSO name template
	template <class CreateObjectFunctionArgumentParam>
	ManagedClass* createObject(const char* className,CreateObjectFunctionArgumentParam argument); // Creates an object of the given class name by searching for a matching DSO; passes additional argument to object creation function
	template <class CreateObjectFunctionArgumentParam>
	ManagedClass* createObject(const char* className,unsigned int version,CreateObjectFunctionArgumentParam argument); // Ditto, using an additional version number; requires a %u conversion somewhere in the DSO name template
	bool isManaged(ManagedClass* object) const // Returns true if the given object is managed by the object loader
		{
		return dsoStates.isEntry(object);
		}
	void destroyObject(ManagedClass* object); // Destroys the object and releases the DSO from which it was loaded
	};

}

#ifndef PLUGINS_OBJECTLOADER_IMPLEMENTATION
#include <Plugins/ObjectLoader.icpp>
#endif

#endif
