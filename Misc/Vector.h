/***********************************************************************
Vector - Class to represent variable-sized arrays of arbitrary types.
A reduced re-implementation of std::vector providing a non-templatized
core representation to be usable as a DataType compound type.
Copyright (c) 2019-2020 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_VECTOR_INCLUDED
#define MISC_VECTOR_INCLUDED

#include <stddef.h>
#include <stdlib.h>
#include <utility>

namespace Misc {

class VectorBase // Non-templatized base class for Vectors
	{
	/* Elements: */
	protected:
	size_t allocSize; // Allocation size of the array of elements
	size_t numElements; // Number of elements currently in the array
	void* elements; // Pointer to array element storage
	
	/* Constructors and destructors: */
	public:
	VectorBase(void) // Creates an empty vector without array element storage
		:allocSize(0),numElements(0),elements(0)
		{
		}
	private:
	VectorBase(const VectorBase& source); // Prohibit copy constructor
	VectorBase& operator=(const VectorBase& source); // Prohibit assignment operator
	public:
	~VectorBase(void) // Destroys array element storage; obviously does not deconstruct array elements
		{
		free(elements);
		}
	
	/* Methods: */
	size_t capacity(void) const // Returns the allocation size of the array of elements
		{
		return allocSize;
		}
	bool empty(void) const // Returns true if there are no elements
		{
		return numElements==0;
		}
	size_t size(void) const // Returns the current number of elements
		{
		return numElements;
		}
	
	/* Low-level methods to access or manipulate untyped vectors: */
	void init(void) // Initializes a new vector in place
		{
		allocSize=0;
		numElements=0;
		elements=0;
		}
	void allocate(size_t newAllocSize,size_t elementSize) // Allocates array element storage
		{
		allocSize=newAllocSize;
		elements=malloc(allocSize*elementSize);
		}
	void reallocate(size_t newAllocSize,size_t elementSize) // Destroys and re-allocates array element storage
		{
		free(elements);
		allocSize=newAllocSize;
		elements=malloc(allocSize*elementSize);
		}
	void setSize(size_t newNumElements) // Sets the number of elements
		{
		numElements=newNumElements;
		}
	const void* getElements(void) const // Returns the untyped array element storage
		{
		return elements;
		}
	void* getElements(void) // Ditto
		{
		return elements;
		}
	};

template <class ElementParam>
class Vector:public VectorBase // Templatized concrete Vector class
	{
	/* Embedded classes: */
	public:
	typedef ElementParam Element; // Type of array elements
	
	class const_iterator;
	
	class iterator // Class to iterate through mutable vectors
		{
		friend class Vector;
		friend class const_iterator;
		
		/* Elements: */
		private:
		Element* element; // Pointer to the iterated-to element
		
		/* Constructors and destructors: */
		public:
		iterator(void) // Creates an invalid iterator
			:element(0)
			{
			}
		private:
		iterator(Element* sElement) // Creates an iterator to an array element; called by Vector methods
			:element(sElement)
			{
			}
		
		/* Methods: */
		public:
		friend bool operator==(const iterator& it1,const iterator& it2) // Equality operator
			{
			return it1.element==it2.element;
			}
		friend bool operator!=(const iterator& it1,const iterator& it2) // Inequality operator
			{
			return it1.element!=it2.element;
			}
		
		/* Indirection methods: */
		Element& operator*(void) const // Returns the iterated-to element
			{
			return *element;
			}
		Element* operator->(void) const // Returns the iterated-to element for structure element selection
			{
			return element;
			}
		
		/* Iteration methods: */
		iterator& operator++(void) // Pre-increment
			{
			++element;
			return *this;
			}
		iterator operator++(int) // Post-increment
			{
			Element* result=element;
			++element;
			return iterator(result);
			}
		iterator& operator--(void) // Pre-decrement
			{
			--element;
			return *this;
			}
		iterator operator--(int) // Post-decrement
			{
			Element* result=element;
			--element;
			return iterator(result);
			}
		iterator& operator+=(ptrdiff_t offset) // Addition assignment operator
			{
			element+=offset;
			return *this;
			}
		iterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
			{
			element-=offset;
			return *this;
			}
		iterator operator+(ptrdiff_t offset) const // Addition operator
			{
			return iterator(element+offset);
			}
		iterator operator-(ptrdiff_t offset) const // Subtraction operator
			{
			return iterator(element-offset);
			}
		friend ptrdiff_t operator-(const iterator& it1,const iterator& it2) // Difference operator
			{
			return it1.element-it2.element;
			}
		};
	
	class const_iterator // Class to iterate through immutable vectors
		{
		friend class Vector;
		
		/* Elements: */
		private:
		const Element* element; // Pointer to the iterated-to element
		
		/* Constructors and destructors: */
		public:
		const_iterator(void) // Creates an invalid iterator
			:element(0)
			{
			}
		private:
		const_iterator(const Element* sElement) // Creates an iterator to an array element; called by Vector methods
			:element(sElement)
			{
			}
		public:
		const_iterator(const iterator& source) // Converts a mutable iterator to a const iterator
			:element(source.element)
			{
			}
		
		/* Methods: */
		public:
		friend bool operator==(const const_iterator& it1,const const_iterator& it2) // Equality operator
			{
			return it1.element==it2.element;
			}
		friend bool operator!=(const const_iterator& it1,const const_iterator& it2) // Inequality operator
			{
			return it1.element!=it2.element;
			}
		
		/* Indirection methods: */
		const Element& operator*(void) const // Returns the iterated-to element
			{
			return *element;
			}
		const Element* operator->(void) const // Returns the iterated-to element for structure element selection
			{
			return element;
			}
		
		/* Iteration methods: */
		const_iterator& operator++(void) // Pre-increment
			{
			++element;
			return *this;
			}
		const_iterator operator++(int) // Post-increment
			{
			const Element* result=element;
			++element;
			return const_iterator(result);
			}
		const_iterator& operator--(void) // Pre-decrement
			{
			--element;
			return *this;
			}
		const_iterator operator--(int) // Post-decrement
			{
			const Element* result=element;
			--element;
			return const_iterator(result);
			}
		const_iterator& operator+=(ptrdiff_t offset) // Addition assignment operator
			{
			element+=offset;
			return *this;
			}
		const_iterator& operator-=(ptrdiff_t offset) // Subtraction assignment operator
			{
			element-=offset;
			return *this;
			}
		const_iterator operator+(ptrdiff_t offset) const // Addition operator
			{
			return const_iterator(element+offset);
			}
		const_iterator operator-(ptrdiff_t offset) const // Subtraction operator
			{
			return const_iterator(element-offset);
			}
		friend ptrdiff_t operator-(const const_iterator& it1,const const_iterator& it2) // Difference operator
			{
			return it1.element-it2.element;
			}
		};
	
	/* Private methods: */
	static void destroyElements(size_t numElements,void* elements) // Destroys the given array of elements
		{
		/* Call the Element class's destructor on all array elements: */
		Element* ePtr=static_cast<Element*>(elements);
		Element* eEnd=ePtr+numElements;
		while(ePtr!=eEnd)
			(ePtr++)->~Element();
		}
	static void copyElements(void* destElements,size_t numElements,const void* sourceElements) // Copies the given array of elements
		{
		/* Call the Element class's copy constructor via placement new on all array elements: */
		Element* dePtr=static_cast<Element*>(destElements);
		const Element* sePtr=static_cast<const Element*>(sourceElements);
		const Element* seEnd=sePtr+numElements;
		while(sePtr!=seEnd)
			new (dePtr++) Element(*(sePtr++));
		}
	static void moveElements(void* destElements,size_t numElements,void* sourceElements) // Moves the given array of elements
		{
		/* Call the Element class's copy constructor via placement new on all destination array elements, and then call the Element class's destructor on all source array elements: */
		Element* dePtr=static_cast<Element*>(destElements);
		Element* sePtr=static_cast<Element*>(sourceElements);
		Element* seEnd=sePtr+numElements;
		while(sePtr!=seEnd)
			{
			new (dePtr++) Element(*sePtr);
			(sePtr++)->~Element();
			}
		}
	void grow(size_t newAllocSize) // Increases the size of the array element storage to the given number of elements
		{
		/* Save the current array element storage: */
		void* oldElements=elements;
		
		/* Allocate a larger array element storage: */
		allocate(newAllocSize,sizeof(Element));
		
		/* Move the previous array elements into the new array element storage: */
		moveElements(elements,numElements,oldElements);
		
		/* Release the previous array element storage: */
		free(oldElements);
		}
	
	/* Constructors and destructors: */
	public:
	Vector(void) // Constructs empty array with no array element storage
		{
		/* Base class constructor does all the work */
		}
	Vector(const Vector& source) // Copy constructor
		{
		/* Allocate array storage: */
		allocate(source.numElements,sizeof(Element));
		
		/* Copy the source's element array: */
		copyElements(elements,source.numElements,source.elements);
		numElements=source.numElements;
		}
	Vector& operator=(const Vector& source) // Assignment operator
		{
		/* Check for aliasing: */
		if(this!=&source)
			{
			/* Check if there is enough room in the existing array element storage: */
			if(allocSize>=source.numElements)
				{
				/* Assign elements common to both arrays: */
				const Element* sPtr=static_cast<const Element*>(source.elements);
				const Element* sEndPtr=sPtr+source.numElements;
				Element* dPtr=static_cast<Element*>(elements);
				Element* dEndPtr=dPtr+numElements;
				const Element* sCommonEndPtr=sPtr+(numElements<source.numElements?numElements:source.numElements);
				for(;sPtr!=sCommonEndPtr;++sPtr,++dPtr)
					*dPtr=*sPtr;
				
				if(numElements<source.numElements)
					{
					/* Copy leftover elements from the source array: */
					for(;sPtr!=sEndPtr;++sPtr,++dPtr)
						new (dPtr) Element(*sPtr);
					}
				
				if(numElements>source.numElements)
					{
					/* Destroy leftover elements in this array: */
					for(;dPtr!=dEndPtr;++dPtr)
						dPtr->~Element();
					}
				}
			else
				{
				/* Destroy all elements: */
				destroyElements(numElements,elements);
				
				/* Re-allocate the array element storage: */
				reallocate(source.numElements,sizeof(Element));
				
				/* Copy the source's element array: */
				copyElements(elements,source.numElements,source.elements);
				}
			
			/* Set the array's new number of elements: */
			numElements=source.numElements;
			}
		
		return *this;
		}
	~Vector(void)
		{
		/* Destroy all elements and then let the base class destructor free the array: */
		destroyElements(numElements,elements);
		}
	
	/* Methods: */
	
	/* Element access methods: */
	const Element* data(void) const // Accesses the array of elements
		{
		return static_cast<const Element*>(elements);
		}
	Element* data(void) // Ditto
		{
		return static_cast<Element*>(elements);
		}
	const Element& front(void) const // Accesses the first array element; assumes array is not empty
		{
		return static_cast<const Element*>(elements)[0];
		}
	Element& front(void) // Ditto
		{
		return static_cast<Element*>(elements)[0];
		}
	const Element& back(void) const // Accesses the last array element; assumes array is not empty
		{
		return static_cast<const Element*>(elements)[numElements-1];
		}
	Element& back(void) // Ditto
		{
		return static_cast<Element*>(elements)[numElements-1];
		}
	const Element& operator[](size_t index) const // Returns the array element of the given index; assumes index is in bounds
		{
		return static_cast<const Element*>(elements)[index];
		}
	Element& operator[](size_t index) // Ditto
		{
		return static_cast<Element*>(elements)[index];
		}
	
	/* Iteration methods: */
	const_iterator begin(void) const // Returns an iterator to the first array element
		{
		return const_iterator(static_cast<const Element*>(elements));
		}
	iterator begin(void) // Ditto
		{
		return iterator(static_cast<Element*>(elements));
		}
	const_iterator end(void) const // Returns an iterator behind the last array element
		{
		return const_iterator(static_cast<const Element*>(elements)+numElements);
		}
	iterator end(void) // Ditto
		{
		return iterator(static_cast<Element*>(elements)+numElements);
		}
	
	/* Array manipulation methods: */
	void reserve(size_t newAllocSize) // Creates room in the array element storage for at least the given number of elements
		{
		/* Grow the array element storage if the requested size is bigger than the current size: */
		if(newAllocSize>allocSize)
			grow(newAllocSize);
		}
	void clear(void) // Removes all array elements
		{
		/* Destroy all elements and reset the element counter; do not destroy array element storage: */
		destroyElements(numElements,elements);
		numElements=0;
		}
	void push_back(const Element& newElement) // Adds a new element to the end of the array
		{
		/* Grow the array element storage if it is full: */
		if(numElements==allocSize)
			grow((allocSize*5)/4+2); // No particular reason for this formula
		
		/* Add the new element via placement new copy construction: */
		new (static_cast<Element*>(elements)+numElements) Element(newElement);
		++numElements;
		}
	void pop_back(void) // Removes the last element of the array; assumes array is not empty
		{
		/* Destroy the last element by calling the Element class's destructor: */
		--numElements;
		static_cast<Element*>(elements)[numElements].~Element();
		}
	friend void swap(Vector& v1,Vector& v2) // Swaps two vectors by exchanging their array element storages
		{
		using std::swap;
		swap(v1.allocSize,v2.allocSize);
		swap(v1.numElements,v2.numElements);
		swap(v1.elements,v2.elements);
		}
	};

}

#endif
