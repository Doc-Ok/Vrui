/***********************************************************************
TransformCalculator - Calculates an orthogonal transformation
(translation, rotation, uniform scaling) from a sequence of elementary
transformations and prints the result in a variety of formats.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <ctype.h>
#include <string.h>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <Misc/ThrowStdErr.h>
#include <Misc/ValueCoder.h>
#include <Misc/StandardValueCoders.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/GeometryValueCoders.h>

const char* parseTuple(const char* begin,const char* end,int numElements,double values[])
	{
	/* Parse tuple's components: */
	for(int i=0;i<numElements;++i)
		{
		/* Skip whitespace: */
		while(begin!=end&&isspace(*begin))
			++begin;
		
		/* Decode a floating-point number: */
		values[i]=Misc::ValueCoder<double>::decode(begin,end,&begin);
		}
	
	return begin;
	}

int main(int argc,char* argv[])
	{
	/* Parse the command line: */
	typedef Geometry::OrthogonalTransformation<double,3> Transform;
	enum PrintFormat
		{
		TRANSFORM,VRML
		};
	Transform transform=Transform::identity;
	PrintFormat printFormat=TRANSFORM;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"vrml")==0)
				printFormat=VRML;
			else if(strcasecmp(argv[i]+1,"transform")==0)
				printFormat=TRANSFORM;
			else if(strcasecmp(argv[i]+1,"invert")==0)
				{
				/* Invert the current transformation: */
				transform=Geometry::invert(transform);
				}
			else
				std::cerr<<"Ignoring unrecognized option "<<argv[i]<<std::endl;
			}
		else if(printFormat==TRANSFORM)
			{
			try
				{
				/* Parse a transformation and append it to the current transformation: */
				transform*=Misc::ValueCoder<Transform>::decode(argv[i],argv[i]+strlen(argv[i]));
				transform.renormalize();
				}
			catch(const std::runtime_error& err)
				{
				std::cerr<<"Ignoring argument "<<argv[i]<<" due to exception "<<err.what()<<std::endl;
				}
			}
		else
			{
			try
				{
				/* Parse a transformation in VRML format: */
				const char* tokBegin=argv[i];
				const char* argEnd;
				for(argEnd=argv[i];*argEnd!='\0';++argEnd)
					;
				Transform::Vector translation=Transform::Vector::zero;
				Transform::Point center=Transform::Point::origin;
				Transform::Scalar scale(1);
				Transform::Rotation rotation=Transform::Rotation::identity;
				while(tokBegin!=argEnd)
					{
					/* Find the next field name: */
					const char* tokEnd;
					for(tokEnd=tokBegin;tokEnd!=argEnd&&!isspace(*tokEnd);++tokEnd)
						;
					
					/* Parse the field name: */
					if(tokEnd-tokBegin==11&&strncasecmp(tokBegin,"translation",11)==0)
						{
						/* Parse a translation vector: */
						tokEnd=parseTuple(tokEnd,argEnd,3,translation.getComponents());
						}
					else if(tokEnd-tokBegin==6&&strncasecmp(tokBegin,"center",6)==0)
						{
						/* Parse a rotation/scaling center point: */
						tokEnd=parseTuple(tokEnd,argEnd,3,center.getComponents());
						}
					else if(tokEnd-tokBegin==8&&strncasecmp(tokBegin,"rotation",8)==0)
						{
						/* Parse a rotation axis and angle: */
						double rot[4];
						tokEnd=parseTuple(tokEnd,argEnd,4,rot);
						
						/* Convert to rotation: */
						rotation=Transform::Rotation::rotateAxis(Transform::Vector(rot[0],rot[1],rot[2]),rot[3]);
						}
					else if(tokEnd-tokBegin==5&&strncasecmp(tokBegin,"scale",5)==0)
						{
						/* Parse a non-uniform scale: */
						double sc[3];
						tokEnd=parseTuple(tokEnd,argEnd,3,sc);
						
						/* Check if scale is uniform: */
						if(sc[0]==sc[1]&&sc[1]==sc[2])
							scale=sc[0];
						else
							throw std::runtime_error("Non-uniform scaling in VRML transformation");
						}
					else if(tokEnd-tokBegin==16&&strncasecmp(tokBegin,"scaleOrientation",16)==0)
						{
						/* Parse a rotation axis and angle and ignore them: */
						double rot[4];
						tokEnd=parseTuple(tokEnd,argEnd,4,rot);
						}
					else
						Misc::throwStdErr("Unrecognized VRML transform token %s",std::string(tokBegin,tokEnd).c_str());
					
					/* Skip whitespace: */
					while(tokEnd!=argEnd&&isspace(*tokEnd))
						++tokEnd;
					
					/* Parse the next token: */
					tokBegin=tokEnd;
					}
				
				/* Append the transformation to the current transformation: */
				transform*=Transform::translate(translation);
				if(center!=Transform::Point::origin)
					transform*=Transform::translateFromOriginTo(center);
				transform*=Transform::rotate(rotation);
				transform*=Transform::scale(scale);
				if(center!=Transform::Point::origin)
					transform*=Transform::translateToOriginFrom(center);
				transform.renormalize();
				}
			catch(const std::runtime_error& err)
				{
				std::cerr<<"Ignoring argument "<<argv[i]<<" due to exception "<<err.what()<<std::endl;
				}
			}
		}
	
	/* Print the final transformation in the current format: */
	switch(printFormat)
		{
		case TRANSFORM:
			std::cout<<Misc::ValueCoder<Transform>::encode(transform)<<std::endl;
			break;
		
		case VRML:
			{
			const Transform::Vector& t=transform.getTranslation();
			const Transform::Rotation& r=transform.getRotation();
			Transform::Vector axis=r.getAxis();
			Transform::Scalar angle=r.getAngle();
			Transform::Scalar s=transform.getScaling();
			std::cout.precision(12);
			std::cout<<"translation "<<t[0]<<' '<<t[1]<<' '<<t[2]<<std::endl;
			std::cout<<"rotation "<<axis[0]<<' '<<axis[1]<<' '<<axis[2]<<' '<<angle<<std::endl;
			if(s!=Transform::Scalar(1))
				std::cout<<"scale "<<s<<' '<<s<<' '<<s<<std::endl;
			
			break;
			}
		}
	
	return 0;
	}
