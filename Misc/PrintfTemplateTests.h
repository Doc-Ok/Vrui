/***********************************************************************
PrintfTemplateTests - Helper functions to test the well-formedness of
strings to be used as templates for printf functions.
Copyright (c) 2018 Oliver Kreylos

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

#ifndef MISC_PRINTFTEMPLATETESTS_INCLUDED
#define MISC_PRINTFTEMPLATETESTS_INCLUDED

#include <string>

namespace Misc {

bool isValidTemplate(const char* templateString,char conversion,unsigned int maxLength,unsigned int* conversionStart =0,unsigned int* conversionLength =0,unsigned int* maxReplacementLength =0); // Returns true if the given template contains exactly one % conversion of the given type and does not overrun the given maximum string length, NUL included; returns conversion start character, conversion length, and maximum length of replacement text if respective pointers are non-null
bool isValidTemplate(const std::string& templateString,char conversion,unsigned int maxLength,unsigned int* conversionStart =0,unsigned int* conversionLength =0,unsigned int* maxReplacementLength =0); // Ditto, for C++ string

}

#endif
