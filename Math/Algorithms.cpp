/***********************************************************************
Algorithms - Implementations of common numerical algorithms.
Copyright (c) 2015-2019 Oliver Kreylos

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

#include <Math/Algorithms.h>

#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Math/Math.h>
#include <Math/Constants.h>

namespace Math {

namespace {

static const double rootOffset=2.0*3.14159265358979323846/3.0; // 2/3 pi

}

unsigned int solveCubicEquation(const double coefficients[4],double solutions[3])
	{
	/* Normalize the cubic equation: */
	double nc[3];
	nc[0]=coefficients[1]/coefficients[0];
	nc[1]=coefficients[2]/coefficients[0];
	nc[2]=coefficients[3]/coefficients[0];
	
	unsigned int numRoots;
	
	double q=(Math::sqr(nc[0])-3.0*nc[1])/9.0;
	double q3=Math::sqr(q)*q;
	double r=((2.0*Math::sqr(nc[0])-9.0*nc[1])*nc[0]+27.0*nc[2])/54.0;
	if(Math::sqr(r)<q3)
		{
		/* There are three real roots: */
		double thetaThird=Math::acos(r/Math::sqrt(q3))/3.0;
		double factor=-2.0*Math::sqrt(q);
		solutions[0]=factor*Math::cos(thetaThird)-nc[0]/3.0;
		solutions[1]=factor*Math::cos(thetaThird+rootOffset)-nc[0]/3.0;
		solutions[2]=factor*Math::cos(thetaThird-rootOffset)-nc[0]/3.0;
		
		numRoots=3;
		}
	else
		{
		/* There is only one real root: */
		double a=Math::pow(Math::abs(r)+Math::sqrt(Math::sqr(r)-q3),1.0/3.0);
		if(r>0.0)
			a=-a;
		double b=a==0.0?0.0:q/a;
		solutions[0]=a+b-nc[0]/3.0;
		
		numRoots=1;
		}
	
	/* Use Newton iteration to clean up the roots: */
	for(unsigned int i=0;i<numRoots;++i)
		for(int j=0;j<2;++j)
			{
			double f=((solutions[i]+nc[0])*solutions[i]+nc[1])*solutions[i]+nc[2];
			double fp=(3.0*solutions[i]+2.0*nc[0])*solutions[i]+nc[1];
			double s=f/fp;
			solutions[i]-=s;
			}
	
	return numRoots;
	}

namespace {

/**************
Helper classes:
**************/

template <class FloatParam>
union FloatAndInt // Union to access the binary representation of a floating-point number
	{
	/* Elements: */
	public:
	FloatParam f; // Floating-point value
	unsigned char i[sizeof(FloatParam)]; // Array of bytes of same size as floating-point value
	};

template <>
union FloatAndInt<Misc::Float32>
	{
	/* Elements: */
	public:
	Misc::Float32 f; // Floating-point value
	Misc::SInt32 i; // Integer value of same size
	};

template <>
union FloatAndInt<Misc::Float64>
	{
	/* Elements: */
	public:
	Misc::Float64 f; // Floating-point value
	Misc::SInt64 i; // Integer value of same size
	};

/****************
Helper functions:
****************/

template <class ScalarParam>
inline ScalarParam nudgeUpInt(ScalarParam value) // Nudges up any integer type
	{
	/* Check against the maximum representable value: */
	if(value<Math::Constants<ScalarParam>::max)
		return value+ScalarParam(1);
	else
		throw std::runtime_error("Math::nudgeUp: Value cannot be nudged up");
	}

template <class ScalarParam>
inline ScalarParam nudgeUpFloat(ScalarParam value) // Nudges up any floating-point type
	{
	/* Check for finiteness: */
	if(isFinite(value))
		{
		FloatAndInt<ScalarParam> fandi;
		fandi.f=value;
		if(value>=Misc::Float32(0))
			++(fandi.i);
		else
			--(fandi.i);
		return fandi.f;
		}
	else
		throw std::runtime_error("Math::nudgeUp: Value cannot be nudged up");
	}

template <class ScalarParam>
inline ScalarParam nudgeDownInt(ScalarParam value) // Nudges down any integer type
	{
	/* Check against the minimum representable value: */
	if(value>Math::Constants<ScalarParam>::min)
		return value-ScalarParam(1);
	else
		throw std::runtime_error("Math::nudgeDown: Value cannot be nudged down");
	}

template <class ScalarParam>
inline ScalarParam nudgeDownFloat(ScalarParam value) // Nudges down any floating-point type
	{
	/* Check for finiteness: */
	if(isFinite(value))
		{
		FloatAndInt<ScalarParam> fandi;
		fandi.f=value;
		if(value<=Misc::Float32(0))
			{
			++(fandi.i);
			if(value==Misc::Float32(0))
				fandi.f=-fandi.f;
			}
		else
			--(fandi.i);
		return fandi.f;
		}
	else
		throw std::runtime_error("Math::nudgeDown: Value cannot be nudged down");
	}

}

template <class ScalarParam>
ScalarParam nudgeUp(ScalarParam value)
	{
	throw std::runtime_error("Math::nudgeUp: Value cannot be nudged up");
	}

template <>
signed char nudgeUp(signed char value)
	{
	return nudgeUpInt(value);
	}

template <>
unsigned char nudgeUp(unsigned char value)
	{
	return nudgeUpInt(value);
	}

template <>
char nudgeUp(char value)
	{
	return nudgeUpInt(value);
	}

template <>
short nudgeUp(short value)
	{
	return nudgeUpInt(value);
	}

template <>
unsigned short nudgeUp(unsigned short value)
	{
	return nudgeUpInt(value);
	}

template <>
int nudgeUp(int value)
	{
	return nudgeUpInt(value);
	}

template <>
unsigned int nudgeUp(unsigned int value)
	{
	return nudgeUpInt(value);
	}

template <>
long nudgeUp(long value)
	{
	return nudgeUpInt(value);
	}

template <>
unsigned long nudgeUp(unsigned long value)
	{
	return nudgeUpInt(value);
	}

template <>
float nudgeUp(float value)
	{
	return nudgeUpFloat(value);
	}

template <>
double nudgeUp(double value)
	{
	return nudgeUpFloat(value);
	}

template <class ScalarParam>
ScalarParam nudgeDown(ScalarParam value)
	{
	throw std::runtime_error("Math::nudgeDown: Value cannot be nudged down");
	}

template <>
signed char nudgeDown(signed char value)
	{
	return nudgeDownInt(value);
	}

template <>
unsigned char nudgeDown(unsigned char value)
	{
	return nudgeDownInt(value);
	}

template <>
char nudgeDown(char value)
	{
	return nudgeDownInt(value);
	}

template <>
short nudgeDown(short value)
	{
	return nudgeDownInt(value);
	}

template <>
unsigned short nudgeDown(unsigned short value)
	{
	return nudgeDownInt(value);
	}

template <>
int nudgeDown(int value)
	{
	return nudgeDownInt(value);
	}

template <>
unsigned int nudgeDown(unsigned int value)
	{
	return nudgeDownInt(value);
	}

template <>
long nudgeDown(long value)
	{
	return nudgeDownInt(value);
	}

template <>
unsigned long nudgeDown(unsigned long value)
	{
	return nudgeDownInt(value);
	}

template <>
float nudgeDown(float value)
	{
	return nudgeDownFloat(value);
	}

template <>
double nudgeDown(double value)
	{
	return nudgeDownFloat(value);
	}

}
