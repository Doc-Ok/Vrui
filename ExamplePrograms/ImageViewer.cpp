/***********************************************************************
Small image viewer using Vrui.
Copyright (c) 2011-2019 Oliver Kreylos

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "ImageViewer.h"

#include <string.h>
#include <stdexcept>
#include <Misc/MessageLogger.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Math/Matrix.h>
#include <Math/SimplexMinimizer.h>
#include <Geometry/OrthogonalTransformation.h>
#include <Geometry/ProjectiveTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLMaterial.h>
#include <GL/GLObject.h>
#include <GL/GLContextData.h>
#include <GL/GLGeometryWrappers.h>
#include <Images/RGBImage.h>
#include <Images/ReadImageFile.h>
#include <Images/WriteImageFile.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>

#if 0
// DEBUGGING
#include <iostream>
#include <Geometry/OutputOperators.h>
#endif

/*************************************************
Static elements of class ImageViewer::PipetteTool:
*************************************************/

ImageViewer::PipetteToolFactory* ImageViewer::PipetteTool::factory=0;

/*****************************************
Methods of class ImageViewer::PipetteTool:
*****************************************/

void ImageViewer::PipetteTool::setPixelPos(void)
	{
	/* Get the first button slot's device ray: */
	Vrui::Ray ray=getButtonDeviceRay(0);
	
	/* Transform the ray to navigational space: */
	ray.transform(Vrui::getInverseNavigationTransformation());
	
	/* Intersect the ray with the z=0 plane: */
	if(ray.getOrigin()[2]*ray.getDirection()[2]<Vrui::Scalar(0))
		{
		Vrui::Scalar lambda=-ray.getOrigin()[2]/ray.getDirection()[2];
		Vrui::Point intersection=ray(lambda);
		x=int(Math::floor(intersection[0]));
		y=int(Math::floor(intersection[1]));
		}
	else
		x=y=0;
	}

void ImageViewer::PipetteTool::initClass(void)
	{
	/* Create a factory object for the pipette tool class: */
	factory=new PipetteToolFactory("PipetteTool","Pick Color Value",0,*Vrui::getToolManager());
	
	/* Set the pipette tool class' input layout: */
	factory->setNumButtons(1);
	factory->setButtonFunction(0,"Pick Color");
	
	/* Register the pipette tool class with the Vrui tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

ImageViewer::PipetteTool::PipetteTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 dragging(false)
	{
	}

const Vrui::ToolFactory* ImageViewer::PipetteTool::getFactory(void) const
	{
	return factory;
	}

namespace {

template <class ScalarParam>
class RectangleAverager
	{
	/* Methods: */
	public:
	static GLColor<GLfloat,4> averageRect(const Images::BaseImage& image,int xmin,int xmax,int ymin,int ymax)
		{
		const ScalarParam* imgPtr=static_cast<const ScalarParam*>(image.getPixels());
		ptrdiff_t stride=image.getRowStride()/sizeof(ScalarParam);
		
		/* Accumulate the given rectangle: */
		GLColor<GLfloat,4> result(0.0f,0.0f,0.0f,0.0f);
		const ScalarParam* rowPtr=imgPtr+ymin*stride;
		for(int y=ymin;y<ymax;++y,rowPtr+=stride)
			{
			const ScalarParam* pPtr=rowPtr+xmin*image.getNumChannels();
			for(int x=xmin;x<xmax;++x)
				for(int channel=0;channel<image.getNumChannels();++channel,++pPtr)
					result[channel]+=GLfloat(*pPtr);
			}
		
		/* Normalize the result color: */
		for(int channel=0;channel<4;++channel)
			result[channel]/=GLfloat((ymax-ymin)*(xmax-xmin));
		
		/* Swizzle the result color to better represent the image's intent: */
		switch(image.getFormat())
			{
			case GL_LUMINANCE:
				result[2]=result[1]=result[0];
				result[3]=1.0f;
				break;
			
			case GL_LUMINANCE_ALPHA:
				result[3]=result[1];
				result[2]=result[1]=result[0];
				break;
			
			case GL_RGB:
				result[3]=1.0f;
				break;
			}
		
		return result;
		}
	};

}

void ImageViewer::PipetteTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		/* Start a new dragging operation: */
		dragging=true;
		setPixelPos();
		x0=x;
		y0=y;
		}
	else
		{
		/* Stop dragging: */
		dragging=false;
		setPixelPos();
		
		/* Access the displayed image: */
		const Images::BaseImage& image(application->textures.getTexture(0U).getImage());
		
		/* Calculate the average pixel value inside the selection rectangle: */
		int xmin=Math::max(Math::min(x0,x),0);
		int xmax=Math::min(Math::max(x0,x),int(image.getSize(0)));
		int ymin=Math::max(Math::min(y0,y),0);
		int ymax=Math::min(Math::max(y0,y),int(image.getSize(1)));
		
		if(xmax>xmin&&ymax>ymin)
			{
			/* Sample the image: */
			GLColor<GLfloat,4> average(0.0f,0.0f,0.0f,0.0f);
			switch(image.getScalarType())
				{
				case GL_UNSIGNED_BYTE:
					average=RectangleAverager<GLubyte>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_UNSIGNED_SHORT:
					average=RectangleAverager<GLushort>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_SHORT:
					average=RectangleAverager<GLshort>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_UNSIGNED_INT:
					average=RectangleAverager<GLuint>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_INT:
					average=RectangleAverager<GLint>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				
				case GL_FLOAT:
					average=RectangleAverager<GLfloat>::averageRect(image,xmin,xmax,ymin,ymax);
					break;
				}
			
			/* Show the sampling result: */
			Misc::formattedUserNote("PipetteTool: Extracted RGBA color:\n%f\n%f\n%f\n%f",average[0],average[1],average[2],average[3]);
			}
		}
	}

void ImageViewer::PipetteTool::frame(void)
	{
	if(dragging)
		{
		/* Update the current pixel position: */
		setPixelPos();
		}
	}

void ImageViewer::PipetteTool::display(GLContextData& contextData) const
	{
	if(dragging)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Temporarily go to navigation coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Assign a non-zero z value to draw above the image: */
		float z=0.05f;
		
		/* Draw the current dragging rectangle as haloed lines: */
		glLineWidth(3.0f);
		glColor(Vrui::getBackgroundColor());
		glBegin(GL_LINE_LOOP);
		glVertex3f(float(x0),float(y0),z);
		glVertex3f(float(x),float(y0),z);
		glVertex3f(float(x),float(y),z);
		glVertex3f(float(x0),float(y),z);
		glEnd();
		glLineWidth(1.0f);
		glColor(Vrui::getForegroundColor());
		glBegin(GL_LINE_LOOP);
		glVertex3f(float(x0),float(y0),z);
		glVertex3f(float(x),float(y0),z);
		glVertex3f(float(x),float(y),z);
		glVertex3f(float(x0),float(y),z);
		glEnd();
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

/***********************************************************
Static elements of class ImageViewer::HomographySamplerTool:
***********************************************************/

ImageViewer::HomographySamplerToolFactory* ImageViewer::HomographySamplerTool::factory=0;

/***************************************************
Methods of class ImageViewer::HomographySamplerTool:
***************************************************/

ImageViewer::Point ImageViewer::HomographySamplerTool::calcPixelPos(void) const
	{
	/* Get the first button slot's device ray: */
	Vrui::Ray ray=getButtonDeviceRay(0);
	
	/* Transform the ray to navigational space: */
	ray.transform(Vrui::getInverseNavigationTransformation());
	
	/* Intersect the ray with the z=0 plane: */
	if(ray.getOrigin()[2]*ray.getDirection()[2]<Vrui::Scalar(0))
		{
		Vrui::Scalar lambda=-ray.getOrigin()[2]/ray.getDirection()[2];
		Vrui::Point pos=ray(lambda);
		return Point(pos[0],pos[1]);
		}
	else
		return Point::origin;
	}

namespace {

class LDKernel // Minimization kernel to calculate lens distortion parameters
	{
	/* Embedded classes: */
	public:
	typedef ImageViewer::Scalar Scalar; // Scalar type for optimization space
	typedef ImageViewer::Point Point; // Type for points
	typedef ImageViewer::Vector Vector; // Type for vectors
	static const unsigned int numVariables=4; // Lens distortion model parameters
	typedef Geometry::ComponentArray<Scalar,4> VariableVector; // Minimization state
	static const unsigned int numFunctionsInBatch=4; // Number of quad edges to straighten out
	
	/* Elements: */
	const Point* quad; // Array of four quad corner points
	const Point* edge; // Array of four quad edge midpoints
	Scalar imageScale2; // Scale factor to normalize the image's size
	
	/* Current lens distortion parameters: */
	Point center; // Position of distortion center in pixel space
	Scalar kappas[2]; // Radial distortion coefficients
	
	/* Private methods: */
	Point invLdc(const Point& distorted) const // Transforms a position from image space into rectified image space; used during optimization
		{
		/*******************************************************************
		Still using the two-dimensional Newton-Raphson from the original
		distortion formula here, although the absence of rhos means we could
		use a one-dimensional inversion on r alone:
		*******************************************************************/
		
		/* Start iteration at the distorted point: */
		Point p=distorted;
		for(int iteration=0;iteration<20;++iteration)
			{
			/* Calculate the function value at p: */
			Vector d=p-center;
			Scalar r2=d.sqr();
			Scalar div=Scalar(1)+(kappas[0]+kappas[1]*r2)*r2;
			Point fp;
			fp[0]=center[0]+d[0]/div-distorted[0];
			fp[1]=center[1]+d[1]/div-distorted[1];
			
			/* Bail out if close enough: */
			if(fp.sqr()<Scalar(1.0e-32))
				break;
			
			/* Calculate the function derivative at p: */
			Scalar fpd[2][2];
			Scalar div2=Math::sqr(div);
			Scalar divp=(Scalar(2)*kappas[0]+Scalar(4)*kappas[1]*r2)/div2;
			fpd[0][0]=div/div2-d[0]*divp*d[0]; // d fp[0] / d p[0]
			fpd[1][0]=fpd[0][1]=-d[0]*divp*d[1]; // d fp[0] / d p[1] == d fp[1] / d p[0]
			fpd[1][1]=div/div2-d[1]*divp*d[1]; // d fp[0] / d p[0]
			
			/* Perform the Newton-Raphson step: */
			Scalar det=fpd[0][0]*fpd[1][1]-fpd[0][1]*fpd[1][0];
			p[0]-=(fpd[1][1]*fp[0]-fpd[0][1]*fp[1])/det;
			p[1]-=(fpd[0][0]*fp[1]-fpd[1][0]*fp[0])/det;
			}
		
		return p;
		}
	Point ldc(const Point& undistorted) const // Converts a position from rectified image space into image space; used during sampling
		{
		Vector d=undistorted-center;
		Scalar r2=d.sqr();
		Scalar div=Scalar(1)+(kappas[0]+kappas[1]*r2)*r2; // Quadratic radial distortion formula in r^2
		return Point(center[0]+d[0]/div,center[1]+d[1]/div);
		}
	
	/* Constructors and destructors: */
	LDKernel(const Point* sQuad,const Point* sEdge,const unsigned int* imageSize)
		:quad(sQuad),edge(sEdge)
		{
		/* Calculate a scale factor for pixel coordinates to "normalize" the meaning of the higher-order rho terms: */
		imageScale2=Math::sqr(Scalar(3))/(Scalar(imageSize[0])*Scalar(imageSize[1]));
		
		/* Initialize the lens distortion parameters: */
		for(int i=0;i<2;++i)
			center[i]=Scalar(imageSize[i])/Scalar(2);
		kappas[1]=kappas[0]=Scalar(0);
		}
	
	/* Methods: */
	VariableVector getState(void) const
		{
		return VariableVector(center[0],center[1],kappas[0],kappas[1]);
		}
	void setState(const VariableVector& newState)
		{
		for(int i=0;i<2;++i)
			center[i]=newState[i];
		for(int i=0;i<2;++i)
			kappas[i]=newState[2+i];
		}
	unsigned int getNumBatches(void) const
		{
		return 1;
		}
	void calcValueBatch(unsigned int batchIndex,Scalar values[numFunctionsInBatch])
		{
		/* Transform all points using the inverse lens distortion correction formula: */
		Point uquad[4],uedge[4];
		for(int i=0;i<4;++i)
			{
			uquad[i]=invLdc(quad[i]);
			uedge[i]=invLdc(edge[i]);
			}
		
		/* Calculate the non-straightness of the quad's edges: */
		for(int i=0;i<4;++i)
			{
			Vector e=uquad[(i+1)%4]-uquad[i];
			Vector d=uedge[i]-uquad[i];
			values[i]=(d[0]*e[1]-d[1]*e[0])/e.mag();
			}
		}
	};

}

void ImageViewer::HomographySamplerTool::resample(void)
	{
	/* Access the displayed image: */
	const Images::BaseImage& image(application->textures.getTexture(0U).getImage());
	
	/* Use a simple lens distortion model to straighten out the current quad: */
	LDKernel kernel(quad,edge,image.getSize());
	#if 0
	for(int i=0;i<4;++i)
		std::cout<<quad[i]<<" --> "<<kernel.invLdc(quad[i])<<std::endl;
	for(int i=0;i<4;++i)
		std::cout<<edge[i]<<" --> "<<kernel.invLdc(edge[i])<<std::endl;
	#endif
	Math::SimplexMinimizer<LDKernel> ldc;
	for(int i=0;i<2;++i)
		ldc.initialSimplexSize[i]=Scalar(image.getSize(i))/Scalar(8);
	ldc.initialSimplexSize[2]=Scalar(0.01)*kernel.imageScale2;
	ldc.initialSimplexSize[3]=ldc.initialSimplexSize[2]*kernel.imageScale2;
	ldc.maxNumIterations=100000;
	Scalar residual=ldc.minimize(kernel);
	#if 0
	std::cout<<"Lens distortion correction residual = "<<residual<<std::endl;
	std::cout<<"Lens distortion correction coefficients: ";
	std::cout<<"center = "<<kernel.center<<", ";
	std::cout<<"kappas = ["<<kernel.kappas[0]<<", "<<kernel.kappas[1]<<"]"<<std::endl;
	for(int i=0;i<4;++i)
		std::cout<<quad[i]<<" --> "<<kernel.invLdc(quad[i])<<std::endl;
	for(int i=0;i<4;++i)
		std::cout<<edge[i]<<" --> "<<kernel.invLdc(edge[i])<<std::endl;
	#endif
	
	/* Calculate a sampling homography based on the distortion-corrected quad: */
	Point rect[4];
	rect[0]=Point(0,0);
	rect[1]=Point(size[0],0);
	rect[2]=Point(size[0],size[1]);
	rect[3]=Point(0,size[1]);
	Point uquad[4];
	for(int i=0;i<4;++i)
		uquad[i]=kernel.invLdc(quad[i]);
	
	/* Create a homogeneous linear system: */
	Math::Matrix a(9,9,0.0);
	for(int corner=0;corner<4;++corner)
		{
		/* Enter the corner's two linear equations into the matrix: */
		for(int eqi=0;eqi<2;++eqi)
			{
			unsigned int row=corner*2+eqi;
			for(int i=0;i<2;++i)
				a(row,eqi*3+i)=double(rect[corner][i]);
			a(row,eqi*3+2)=1.0;
			for(int i=0;i<2;++i)
				a(row,6+i)=-uquad[corner][eqi]*rect[corner][i];
			a(row,8)=-uquad[corner][eqi];
			}
		}
	
	/* Solve the homogeneous linear system: */
	Math::Matrix h=a.kernel();
	if(h.getNumColumns()!=1)
		{
		Misc::userError("ImageViewer::HomographySampleTool: Cannot calculate projective undistortion");
		return;
		}
	
	/* Create the homography from sampling rectangle space to distortion-corrected image space: */
	Geometry::ProjectiveTransformation<Scalar,2> hom;
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			hom.getMatrix()(i,j)=h(i*3+j);
	
	/* Sample the quad: */
	Images::RGBImage sample(size[0],size[1]);
	Images::RGBImage::Color* pPtr=sample.modifyPixels();
	int maxx=int(image.getSize(0))-2;
	int maxy=int(image.getSize(1))-2;
	for(unsigned int y=0;y<size[1];++y)
		for(unsigned int x=0;x<size[0];++x,++pPtr)
			{
			/* Transform the sample pixel position from sampling rectangle space into distortion-corrected image space: */
			Point up=hom.transform(Point(Scalar(x)+Scalar(0.5),Scalar(y)+Scalar(0.5)));
			
			/* Distortion-correct the sample pixel: */
			Point dp=kernel.ldc(up);
			
			/* Interpolate the image's pixel value at the sample position: */
			dp[0]-=Scalar(0.5);
			int cx=Math::clamp(int(Math::floor(dp[0])),0,maxx);
			Scalar dx=dp[0]-Scalar(cx);
			dp[1]-=Scalar(0.5);
			int cy=Math::clamp(int(Math::floor(dp[1])),0,maxy);
			Scalar dy=dp[1]-Scalar(cy);
			Color c00=application->getPixel(cx,cy);
			Color c01=application->getPixel(cx+1,cy);
			Color c0;
			for(int i=0;i<4;++i)
				c0[i]=GLfloat(c00[i]*(Scalar(1)-dx)+c01[i]*dx);
			Color c10=application->getPixel(cx,cy+1);
			Color c11=application->getPixel(cx+1,cy+1);
			Color c1;
			for(int i=0;i<4;++i)
				c1[i]=GLfloat(c10[i]*(Scalar(1)-dx)+c11[i]*dx);
			Color c;
			for(int i=0;i<4;++i)
				c[i]=GLfloat(c0[i]*(Scalar(1)-dy)+c1[i]*dy);
			
			/* Assign the sample image's pixel value: */
			for(int i=0;i<3;++i)
				(*pPtr)[i]=GLubyte(Math::clamp(Math::floor(c[i]+0.5f),0.0f,255.0f));
			}
	
	/* Save the sampled image: */
	// std::cout<<"Writing image file SampledImage.png"<<std::endl;
	Images::writeImageFile(sample,"SampledImage.png");
	}

void ImageViewer::HomographySamplerTool::draw(void) const
	{
	/* Assign a non-zero z value to draw above the image: */
	Scalar z(0.05);
	
	/* Check if the quad has been fully defined: */
	if(numVertices<4)
		{
		/* Draw the partial quad: */
		glBegin(GL_LINE_STRIP);
		for(int i=0;i<numVertices;++i)
			glVertex(quad[i][0],quad[i][1],z);
		glEnd();
		}
	else
		{
		/* Draw the full quad: */
		glBegin(GL_LINE_LOOP);
		for(int i=0;i<4;++i)
			{
			glVertex(quad[i][0],quad[i][1],z);
			glVertex(edge[i][0],edge[i][1],z);
			}
		glEnd();
		}
	
	/* Calculate the radius of the quad's vertex picking handles: */
	Scalar pickRadius=Vrui::getPointPickDistance();
	
	/* Draw the current quad's vertex picking handles: */
	for(int i=0;i<numVertices;++i)
		{
		glBegin(GL_LINE_LOOP);
		for(int j=0;j<32;++j)
			{
			Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(j)/Scalar(32);
			glVertex(quad[i][0]+Math::cos(angle)*pickRadius,quad[i][1]+Math::sin(angle)*pickRadius,z);
			}
		glEnd();
		}
	
	/* Check if the quad has been fully defined: */
	if(numVertices==4)
		{
		/* Draw the current quad's edge midpoint picking handles: */
		for(int i=0;i<4;++i)
			{
			glBegin(GL_LINE_LOOP);
			for(int j=0;j<32;++j)
				{
				Scalar angle=Scalar(2)*Math::Constants<Scalar>::pi*Scalar(j)/Scalar(32);
				glVertex(edge[i][0]+Math::cos(angle)*pickRadius,edge[i][1]+Math::sin(angle)*pickRadius,z);
				}
			glEnd();
			}
		}
	}

void ImageViewer::HomographySamplerTool::initClass(void)
	{
	/* Create a factory object for the homography sampler tool class: */
	factory=new HomographySamplerToolFactory("HomographySamplerTool","Resample Quad",0,*Vrui::getToolManager());
	
	/* Set the homography sampler tool class' input layout: */
	factory->setNumButtons(2);
	factory->setButtonFunction(0,"Drag Quad Vertex");
	factory->setButtonFunction(1,"Resample Quad");
	
	/* Register the homography sampler tool class with the Vrui tool manager: */
	Vrui::getToolManager()->addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	}

ImageViewer::HomographySamplerTool::HomographySamplerTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 numVertices(0),dragging(false)
	{
	/* Initialize the result image size: */
	for(int i=0;i<2;++i)
		size[i]=1024;
	}

const Vrui::ToolFactory* ImageViewer::HomographySamplerTool::getFactory(void) const
	{
	return factory;
	}

void ImageViewer::HomographySamplerTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	/* Check which button was pressed or released: */
	if(buttonSlotIndex==0)
		{
		/* Check whether the drag button was pressed or released: */
		if(cbData->newButtonState)
			{
			if(numVertices<4)
				{
				/* Create a new quad vertex at the current device position: */
				quad[numVertices]=calcPixelPos();
				dragging=true;
				dragIndex=numVertices;
				dragOffset=Vector::zero;
				++numVertices;
				
				/* Check if the quad is now fully defined: */
				if(numVertices==4)
					{
					/* Initialize the edge midpoint positions: */
					for(int i=0;i<4;++i)
						edge[i]=Geometry::mid(quad[i],quad[(i+1)%4]);
					}
				}
			else
				{
				/* Try picking an existing quad vertex or edge midpoint: */
				Point devicePos=calcPixelPos();
				int bestIndex=0;
				Scalar bestDist2=Geometry::sqrDist(quad[0],devicePos);
				for(int i=1;i<4;++i)
					{
					Scalar dist2=Geometry::sqrDist(quad[i],devicePos);
					if(bestDist2>dist2)
						{
						bestIndex=i;
						bestDist2=dist2;
						}
					}
				for(int i=0;i<4;++i)
					{
					Scalar dist2=Geometry::sqrDist(edge[i],devicePos);
					if(bestDist2>dist2)
						{
						bestIndex=4+i;
						bestDist2=dist2;
						}
					}
				if(bestDist2<Math::sqr(Vrui::getPointPickDistance()))
					{
					/* Start dragging the picked vertex or edge midpoint: */
					dragging=true;
					dragIndex=bestIndex;
					dragOffset=(dragIndex<4?quad[dragIndex]:edge[dragIndex-4])-devicePos;
					}
				}
			}
		else
			{
			/* Stop dragging: */
			dragging=false;
			}
		}
	else
		{
		/* Check whether the sample button was released: */
		if(!cbData->newButtonState)
			{
			/* Resample the current quad if it is fully defined: */
			if(numVertices==4)
				resample();
			else
				Misc::userError("ImageViewer::HomographySampleTool: Cannot resample from partial quad");
			}
		}
	}

void ImageViewer::HomographySamplerTool::frame(void)
	{
	if(dragging)
		{
		/* Check whether a vertex or edge midpoint is being dragged: */
		if(dragIndex<4)
			{
			/* Update the dragged vertex's position: */
			quad[dragIndex]=calcPixelPos()+dragOffset;
			
			/* Check if the quad has been fully defined: */
			if(numVertices==4)
				{
				/* Update the positions of the edge midpoints adjacent to the dragged vertex: */
				edge[dragIndex]=Geometry::mid(quad[dragIndex],quad[(dragIndex+1)%4]);
				edge[(dragIndex+3)%4]=Geometry::mid(quad[(dragIndex+3)%4],quad[dragIndex]);
				}
			}
		else
			{
			/* Update the dragged edge midpoint's position: */
			edge[dragIndex-4]=calcPixelPos()+dragOffset;
			}
		}
	}

void ImageViewer::HomographySamplerTool::display(GLContextData& contextData) const
	{
	if(numVertices>0)
		{
		/* Set up OpenGL state: */
		glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
		glDisable(GL_LIGHTING);
		
		/* Temporarily go to navigation coordinates: */
		Vrui::goToNavigationalSpace(contextData);
		
		/* Draw the current quad as haloed lines: */
		glLineWidth(3.0f);
		glColor(Vrui::getBackgroundColor());
		draw();
		glLineWidth(1.0f);
		glColor(Vrui::getForegroundColor());
		draw();
		
		/* Go back to physical coordinates: */
		glPopMatrix();
		
		/* Restore OpenGL state: */
		glPopAttrib();
		}
	}

/****************************
Methods of class ImageViewer:
****************************/

namespace {

template <class ScalarParam>
class PixelExtractor
	{
	/* Methods: */
	public:
	static GLColor<GLfloat,4> getPixel(const Images::BaseImage& image,unsigned int x,unsigned int y)
		{
		const ScalarParam* imgPtr=static_cast<const ScalarParam*>(image.getPixels());
		imgPtr+=(y*image.getWidth()+x)*image.getNumChannels();
		
		/* Retrieve the pixel's channels: */
		GLColor<GLfloat,4> result(0.0f,0.0f,0.0f,0.0f);
		for(int channel=0;channel<image.getNumChannels();++channel,++imgPtr)
			result[channel]=GLfloat(*imgPtr);
		
		/* Swizzle the result color to better represent the image's intent: */
		switch(image.getFormat())
			{
			case GL_LUMINANCE:
				result[2]=result[1]=result[0];
				result[3]=1.0f;
				break;
			
			case GL_LUMINANCE_ALPHA:
				result[3]=result[1];
				result[2]=result[1]=result[0];
				break;
			
			case GL_RGB:
				result[3]=1.0f;
				break;
			}
		
		return result;
		}
	};

}

ImageViewer::Color ImageViewer::getPixel(unsigned int x,unsigned int y) const
	{
	switch(image->getScalarType())
		{
		case GL_UNSIGNED_BYTE:
			return PixelExtractor<GLubyte>::getPixel(*image,x,y);
		
		case GL_UNSIGNED_SHORT:
			return PixelExtractor<GLushort>::getPixel(*image,x,y);
		
		case GL_SHORT:
			return PixelExtractor<GLshort>::getPixel(*image,x,y);
		
		case GL_UNSIGNED_INT:
			return PixelExtractor<GLuint>::getPixel(*image,x,y);
		
		case GL_INT:
			return PixelExtractor<GLint>::getPixel(*image,x,y);
			break;
		
		case GL_FLOAT:
			return PixelExtractor<GLfloat>::getPixel(*image,x,y);
		
		default:
			return Color(0.0f,0.0f,0.0f,1.0f);
		}
	}

ImageViewer::ImageViewer(int& argc,char**& argv)
	:Vrui::Application(argc,argv)
	{
	/* Parse the command line: */
	const char* imageFileName=0;
	bool printInfo=false;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"p")==0)
				printInfo=true;
			}
		else if(imageFileName==0)
			imageFileName=argv[i];
		}
	if(imageFileName==0)
		throw std::runtime_error("ImageViewer: No image file name provided");
	
	/* Load the image into the texture set: */
	Images::BaseImage loadImage=Images::readGenericImageFile(imageFileName);
	Images::TextureSet::Texture& tex=textures.addTexture(loadImage,GL_TEXTURE_2D,loadImage.getInternalFormat(),0U);
	image=&tex.getImage();
	
	if(printInfo)
		{
		/* Display image size and format: */
		char messageText[2048];
		const char* componentScalarType=0;
		switch(image->getScalarType())
			{
			case GL_BYTE:
				componentScalarType="signed 8-bit integer";
				break;
			
			case GL_UNSIGNED_BYTE:
				componentScalarType="unsigned 8-bit integer";
				break;
			
			case GL_SHORT:
				componentScalarType="signed 16-bit integer";
				break;
			
			case GL_UNSIGNED_SHORT:
				componentScalarType="unsigned 16-bit integer";
				break;
			
			case GL_INT:
				componentScalarType="signed 32-bit integer";
				break;
			
			case GL_UNSIGNED_INT:
				componentScalarType="unsigned 32-bit integer";
				break;
			
			case GL_FLOAT:
				componentScalarType="32-bit floating-point number";
				break;
			
			case GL_DOUBLE:
				componentScalarType="64-bit floating-point number";
				break;
			
			default:
				componentScalarType="<unknown>";
			}
		Misc::formattedUserNote("Image: %s\nSize: %u x %u pixels\nFormat: %u %s of %u %s%s\nComponent type: %s",imageFileName,image->getSize(0),image->getSize(1),image->getNumChannels(),image->getNumChannels()!=1?"channels":"channel",image->getChannelSize(),image->getChannelSize()!=1?"bytes":"byte",image->getNumChannels()!=1?" each":"",componentScalarType);
		}
	
	/* Set clamping and filtering parameters for mip-mapped linear interpolation: */
	tex.setMipmapRange(0,1000);
	tex.setWrapModes(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
	tex.setFilterModes(GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR);
	
	/* Initialize the tool class: */
	PipetteTool::initClass();
	HomographySamplerTool::initClass();
	}

ImageViewer::~ImageViewer(void)
	{
	}

void ImageViewer::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	
	/* Get the texture set's GL state: */
	Images::TextureSet::GLState* texGLState=textures.getGLState(contextData);
	
	/* Bind the texture object: */
	const Images::TextureSet::GLState::Texture& tex=texGLState->bindTexture(0U);
	const Images::BaseImage& image=tex.getImage();
	
	/* Query the range of texture coordinates: */
	const GLfloat* texMin=tex.getTexCoordMin();
	const GLfloat* texMax=tex.getTexCoordMax();
	
	/* Draw the image: */
	glBegin(GL_QUADS);
	glTexCoord2f(texMin[0],texMin[1]);
	glVertex2i(0,0);
	glTexCoord2f(texMax[0],texMin[1]);
	glVertex2i(image.getSize(0),0);
	glTexCoord2f(texMax[0],texMax[1]);
	glVertex2i(image.getSize(0),image.getSize(1));
	glTexCoord2f(texMin[0],texMax[1]);
	glVertex2i(0,image.getSize(1));
	glEnd();
	
	/* Protect the texture object: */
	glBindTexture(GL_TEXTURE_2D,0);
	
	/* Draw the image's backside: */
	glDisable(GL_TEXTURE_2D);
	glMaterial(GLMaterialEnums::FRONT,GLMaterial(GLMaterial::Color(0.7f,0.7f,0.7f)));
	
	glBegin(GL_QUADS);
	glNormal3f(0.0f,0.0f,-1.0f);
	glVertex2i(0,0);
	glVertex2i(0,image.getSize(1));
	glVertex2i(image.getSize(0),image.getSize(1));
	glVertex2i(image.getSize(0),0);
	glEnd();
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void ImageViewer::resetNavigation(void)
	{
	/* Access the image: */
	const Images::BaseImage& image=textures.getTexture(0U).getImage();
	
	/* Reset the Vrui navigation transformation: */
	Vrui::Scalar w(image.getSize(0));
	Vrui::Scalar h(image.getSize(1));
	Vrui::Point center(Math::div2(w),Math::div2(h),Vrui::Scalar(0.05));
	Vrui::Scalar size=Math::sqrt(Math::sqr(w)+Math::sqr(h));
	Vrui::setNavigationTransformation(center,size,Vrui::Vector(0,1,0));
	}

/* Create and execute an application object: */
VRUI_APPLICATION_RUN(ImageViewer)
