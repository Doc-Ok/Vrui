/***********************************************************************
RGB - Class to represent colors in the RGB color space.
Copyright (c) 2020 Oliver Kreylos

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

#ifndef MISC_RGB_INCLUDED
#define MISC_RGB_INCLUDED

namespace Misc {

template <class ScalarParam>
class RGB
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Type for RGB color components
	static const int numComponents=3; // Number of color components
	
	/* Elements: */
	private:
	Scalar components[3]; // Array of RGB color components
	
	/* Private methods: */
	template <class SourceScalarParam>
	void convertAndCopy(const SourceScalarParam* sComponents); // Converts and copies an array of color components
	
	/* Constructors and destructors: */
	public:
	RGB(void) // Dummy constructor
		{
		}
	RGB(Scalar sRed,Scalar sGreen,Scalar sBlue) // Component-wise initialization
		{
		/* Set color components: */
		components[0]=sRed;
		components[1]=sGreen;
		components[2]=sBlue;
		}
	RGB(const RGB& source) // Copy constructor
		{
		/* Copy color components from the source: */
		for(int i=0;i<3;++i)
			components[i]=source.components[i];
		}
	template <class SourceScalarParam>
	RGB(const RGB<SourceScalarParam>& source) // Copy constructor with type conversion
		{
		/* Convert and copy color components from the source: */
		convertAndCopy(source.getComponents());
		}
	RGB& operator=(const RGB& source) // Assignment operator
		{
		/* Copy color components from the source (no need to check for aliasing): */
		for(int i=0;i<3;++i)
			components[i]=source.components[i];
		
		return *this;
		}
	template <class SourceScalarParam>
	RGB& operator=(const RGB<SourceScalarParam>& source) // Assignment operator with type conversion
		{
		/* Convert and copy color components from the source (no need to check for aliasing): */
		convertAndCopy(source.getComponents());
		
		return *this;
		}
	
	/* Methods: */
	const Scalar* getComponents(void) const // Returns the array of color components
		{
		return components;
		}
	Scalar* getComponents(void) // Ditto
		{
		return components;
		}
	Scalar operator[](int index) const // Returns one color component as an rvalue
		{
		return components[index];
		}
	Scalar& operator[](int index) // Returns one color component as an lvalue
		{
		return components[index];
		}
	};

}

#endif
