/***********************************************************************
MathMarshallers - Marshaller classes for math objects.
Copyright (c) 2010-2020 Oliver Kreylos

This file is part of the Templatized Math Library (Math).

The Templatized Math Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Math Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Math Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#ifndef MATH_MATHMARSHALLERS_INCLUDED
#define MATH_MATHMARSHALLERS_INCLUDED

#include <Misc/Marshaller.h>
#include <Math/Complex.h>
#include <Math/BrokenLine.h>

namespace Misc {

template <class ScalarParam>
class Marshaller<Math::Complex<ScalarParam> >
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type for complex numbers
	typedef Math::Complex<ScalarParam> Value; // Value type
	
	/* Methods: */
	public:
	static size_t getSize(const Value& value);
		{
		return Marshaller<Scalar>::getSize(value.getReal())+Marshaller<Scalar>::getSize(value.getImag());
		}
	template <class DataSinkParam>
	static void write(const Value& sink)
		{
		Marshaller<Scalar>::write(value.getReal(),sink);
		Marshaller<Scalar>::write(value.getImag(),sink);
		}
	template <class DataSourceParam>
	static Value& read(DataSourceParam& source,Value& value)
		{
		Scalar real=Marshaller<Scalar>::read(source);
		Scalar imag=Marshaller<Scalar>::read(source);
		value=Value(real,imag);
		return value;
		}
	template <class DataSourceParam>
	static Value read(DataSourceParam& source)
		{
		Scalar real=Marshaller<Scalar>::read(source);
		Scalar imag=Marshaller<Scalar>::read(source);
		return Value(real,imag);
		}
	};

template <class ScalarParam>
class Marshaller<Math::BrokenLine<ScalarParam> >
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type for broken line
	typedef Math::BrokenLine<ScalarParam> Value; // Value type
	
	/* Methods: */
	public:
	static size_t getSize(const Value& value);
		{
		return Marshaller<Scalar>::getSize(value.min)+Marshaller<Scalar>::getSize(value.deadMin)+Marshaller<Scalar>::getSize(value.deadMax)+Marshaller<Scalar>::getSize(value.max);
		}
	template <class DataSinkParam>
	static void write(const Value& sink)
		{
		Marshaller<Scalar>::write(value.min,sink);
		Marshaller<Scalar>::write(value.deadMin,sink);
		Marshaller<Scalar>::write(value.deadMax,sink);
		Marshaller<Scalar>::write(value.max,sink);
		}
	template <class DataSourceParam>
	static Value& read(DataSourceParam& source,Value& value)
		{
		Marshaller<Scalar>::read(source,value.min);
		Marshaller<Scalar>::read(source,value.deadMin);
		Marshaller<Scalar>::read(source,value.deadMax);
		Marshaller<Scalar>::read(source,value.max);
		return value;
		}
	template <class DataSourceParam>
	static Value read(DataSourceParam& source)
		{
		Value result;
		Marshaller<Scalar>::read(source,result.min);
		Marshaller<Scalar>::read(source,result.deadMin);
		Marshaller<Scalar>::read(source,result.deadMax);
		Marshaller<Scalar>::read(source,result.max);
		return result;
		}
	};

}

#endif
