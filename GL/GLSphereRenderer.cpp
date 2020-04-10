/***********************************************************************
GLSphereRenderer - Class to render spheres as ray-cast impostors.
Copyright (c) 2019 Oliver Kreylos

This file is part of the OpenGL Support Library (GLSupport).

The OpenGL Support Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The OpenGL Support Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the OpenGL Support Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GL/GLSphereRenderer.h>

#include <string>
#include <Misc/PrintInteger.h>
#include <GL/GLLightTracker.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBGeometryShader4.h>
#include <GL/Extensions/GLARBVertexShader.h>

/*******************************************
Methods of class GLSphereRenderer::DataItem:
*******************************************/

GLSphereRenderer::DataItem::DataItem(void)
	:vertexShader(0),geometryShader(0),fragmentShader(0),shaderProgram(0),
	 settingsVersion(0),lightStateVersion(0)
	{
	/* Initialize required OpenGL extensions: */
	GLARBShaderObjects::initExtension();
	GLARBVertexShader::initExtension();
	GLARBGeometryShader4::initExtension();
	GLARBFragmentShader::initExtension();
	
	/* Create the shader objects: */
	vertexShader=glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	geometryShader=glCreateShaderObjectARB(GL_GEOMETRY_SHADER_ARB);
	fragmentShader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	shaderProgram=glCreateProgramObjectARB();
	
	/* Attach the shader objects to the shader program: */
	glAttachObjectARB(shaderProgram,vertexShader);
	glAttachObjectARB(shaderProgram,geometryShader);
	glAttachObjectARB(shaderProgram,fragmentShader);
	}

GLSphereRenderer::DataItem::~DataItem(void)
	{
	/* Destroy the shader objects: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(geometryShader);
	glDeleteObjectARB(fragmentShader);
	glDeleteObjectARB(shaderProgram);
	}

/*********************************
Methods of class GLSphereRenderer:
*********************************/

void GLSphereRenderer::compileShader(GLSphereRenderer::DataItem* dataItem,const GLLightTracker& lightTracker) const
	{
	/* Create the impostor sphere vertex shader source code: */
	std::string vertexShaderVaryings;
	std::string vertexShaderMain="\
	void main()\n\
		{\n";
	if(colorMaterial)
		{
		vertexShaderVaryings+="\
		varying vec4 inColor;\n\
		\n";
		
		vertexShaderMain+="\
			/* Copy the vertex color: */\n\
			inColor=gl_Color;\n\
			\n;";
		}
	if(fixedRadius)
		{
		vertexShaderMain+="\
			/* Transform the sphere's center point to eye coordinates: */\n\
			gl_Position=gl_ModelViewMatrix*gl_Vertex;\n";
		}
	else
		{
		vertexShaderMain+="\
			/* Transform the sphere's center point to eye coordinates: */\n\
			gl_Position=vec4((gl_ModelViewMatrix*vec4(gl_Vertex.xyz,1.0)).xyz,gl_Vertex.w);\n";
		}
	vertexShaderMain+="\
		}\n";
	
	/* Compile the vertex shader and attach it to the sphere shader program: */
	if(colorMaterial)
		glCompileShaderFromStrings(dataItem->vertexShader,2,vertexShaderVaryings.c_str(),vertexShaderMain.c_str());
	else
		glCompileShaderFromString(dataItem->vertexShader,vertexShaderMain.c_str());
	
	/* Create the impostor sphere geometry shader source code: */
	std::string geometryShaderDeclarations="\
	#version 120\n\
	#extension GL_ARB_geometry_shader4: enable\n\
	\n";
	std::string geometryShaderUniforms;
	if(fixedRadius)
		{
		geometryShaderUniforms+="\
		uniform float fixedRadius;\n\
		\n";
		}
	else
		{
		geometryShaderUniforms+="\
		uniform float modelViewScale;\n\
		\n";
		}
	std::string geometryShaderVaryings;
	std::string geometryShaderMain="\
	void main()\n\
		{\n";
	if(colorMaterial)
		{
		geometryShaderVaryings+="\
		varying in vec4 inColor[1];\n\
		\n";
		}
	geometryShaderVaryings+="\
	varying out vec3 center;\n\
	varying out float radius;\n\
	varying out vec3 dir;\n";
	if(colorMaterial)
		{
		geometryShaderVaryings+="\
		varying out vec4 color;\n\
		\n";
		}
	if(fixedRadius)
		{
		geometryShaderMain+="\
		/* Retrieve the sphere's center position and radius in eye coordinates: */\n\
		vec3 c=gl_PositionIn[0].xyz/gl_PositionIn[0].w;\n\
		float r=fixedRadius;\n\
		\n";
		}
	else
		{
		geometryShaderMain+="\
		/* Retrieve the sphere's center position and radius in eye coordinates: */\n\
		vec3 c=gl_PositionIn[0].xyz;\n\
		float r=gl_PositionIn[0].w*modelViewScale;\n\
		\n";
		}
	geometryShaderMain+="\
		/* Calculate the impostor quad's size: */\n\
		float d2=dot(c,c);\n\
		float impostorSize=r*sqrt(2.0*d2/(d2-r*r));\n\
		\n\
		/* Calculate the impostor quad's base vectors: */\n\
		vec3 x;\n\
		if(abs(c.x)>abs(c.y))\n\
			x=normalize(vec3(c.z,0.0,-c.x));\n\
		else\n\
			x=normalize(vec3(0.0,c.z,-c.y));\n\
		vec3 y=normalize(cross(x,c));\n\
		\n\
		/* Emit the quad's four vertices: */\n";
	for(int vertex=0;vertex<4;++vertex)
		{
		geometryShaderMain+="\
		center=c;\n\
		radius=r;\n\
		dir=c";
		geometryShaderMain.push_back(vertex%2!=0?'-':'+');
		geometryShaderMain.push_back(vertex==0||vertex==3?'y':'x');
		geometryShaderMain.append("*impostorSize;\n");
		if(colorMaterial)
			{
			geometryShaderMain+="\
			color=inColor[0];\n";
			}
		geometryShaderMain+="\
		gl_Position=gl_ProjectionMatrix*vec4(dir,1.0);\n\
		EmitVertex();\n";
		}
	geometryShaderMain+="\
		}\n";
	
	/* Compile the geometry shader and attach it to the sphere shader program: */
	glCompileShaderFromStrings(dataItem->geometryShader,4,geometryShaderDeclarations.c_str(),geometryShaderUniforms.c_str(),geometryShaderVaryings.c_str(),geometryShaderMain.c_str());
	
	/* Set the geometry shader's parameters: */
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_VERTICES_OUT_ARB,4);
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_INPUT_TYPE_ARB,GL_POINTS);
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_OUTPUT_TYPE_ARB,GL_TRIANGLE_STRIP);
	
	/* Create the impostor sphere fragment shader source code: */
	std::string fragmentShaderVaryings="\
	varying vec3 center; // Sphere center point\n\
	varying float radius; // Sphere radius in eye coordinates\n\
	varying vec3 dir; // Ray direction vector\n";
	if(colorMaterial)
		{
		fragmentShaderVaryings+="\
		varying vec4 color; // Vertex color\n";
		}
	fragmentShaderVaryings.push_back('\n');
	std::string fragmentShaderFunctions;
	std::string fragmentShaderMain="\
	void main()\n\
		{\n\
		/* Intersect the ray and the sphere: */\n\
		float a=dot(dir,dir);\n\
		float b=-dot(dir,center);\n\
		float c=dot(center,center)-radius*radius;\n\
		float det=b*b-c*a;\n\
		if(det<0.0)\n\
			discard;\n\
		float lambda=(-b-sqrt(det))/a;\n\
		if(lambda<-1.0)\n\
			discard;\n\
		\n\
		/* Calculate the intersection point and normal vector: */\n\
		vec4 vertex=vec4(dir*lambda,1.0);\n\
		vec3 normal=normalize(vertex.xyz-center);\n\
		\n\
		/* Calculate the intersection point's depth buffer value: */\n\
		vec4 vertexC=gl_ProjectionMatrix*vertex;\n\
		gl_FragDepth=0.5*(vertexC.z*gl_DepthRange.diff/vertexC.w+gl_DepthRange.near+gl_DepthRange.far);\n\
		\n\
		/* Calculate total illumination and initialize with global ambient term: */\n";
	if(colorMaterial)
		{
		fragmentShaderMain+="\
			vec4 ambientDiffuseAccum=gl_LightModel.ambient*color+gl_FrontMaterial.emission;\n";
		}
	else
		{
		fragmentShaderMain+="\
			vec4 ambientDiffuseAccum=gl_LightModel.ambient*gl_FrontMaterial.ambient+gl_FrontMaterial.emission;\n";
		}
	fragmentShaderMain+="\
		vec4 specularAccum=vec4(0.0,0.0,0.0,0.0);\n\
		\n\
		/* Accumulate all enabled light sources: */\n";
	
	/* Create light application functions for all enabled light sources: */
	for(int lightIndex=0;lightIndex<lightTracker.getMaxNumLights();++lightIndex)
		if(lightTracker.getLightState(lightIndex).isEnabled())
			{
			/* Create the light accumulation function: */
			fragmentShaderFunctions+=lightTracker.createAccumulateLightFunction(lightIndex);
			
			/* Call the light application function from the fragment shader's main function: */
			fragmentShaderMain+="\
			accumulateLight";
			char liBuffer[12];
			fragmentShaderMain.append(Misc::print(lightIndex,liBuffer+11));
			if(colorMaterial)
				fragmentShaderMain+="(vertex,normal,color,color,gl_FrontMaterial.specular,gl_FrontMaterial.shininess,ambientDiffuseAccum,specularAccum);\n";
			else
				fragmentShaderMain+="(vertex,normal,gl_FrontMaterial.ambient,gl_FrontMaterial.diffuse,gl_FrontMaterial.specular,gl_FrontMaterial.shininess,ambientDiffuseAccum,specularAccum);\n";
			}
	
	/* Finalize the fragment shader's main function: */
	fragmentShaderMain+="\
		\n\
		/* Compute the final fragment color: */\n\
		gl_FragColor=ambientDiffuseAccum+specularAccum;\n\
		}\n";
	
	/* Compile the fragment shader and attach it to the sphere shader program: */
	glCompileShaderFromStrings(dataItem->fragmentShader,3,fragmentShaderFunctions.c_str(),fragmentShaderVaryings.c_str(),fragmentShaderMain.c_str());
	
	/* Link the sphere shader program: */
	glLinkAndTestShader(dataItem->shaderProgram);
	
	/* Retrieve the shader program's uniform variable locations: */
	if(fixedRadius)
		dataItem->shaderProgramUniforms[0]=glGetUniformLocationARB(dataItem->shaderProgram,"fixedRadius");
	else
		dataItem->shaderProgramUniforms[0]=glGetUniformLocationARB(dataItem->shaderProgram,"modelViewScale");
	
	/* Mark the shader program as up-to-date: */
	dataItem->settingsVersion=settingsVersion;
	dataItem->lightStateVersion=lightTracker.getVersion();
	}

GLSphereRenderer::GLSphereRenderer(void)
	:fixedRadius(false),radius(0),
	 colorMaterial(false),
	 settingsVersion(1)
	{
	}

GLSphereRenderer::~GLSphereRenderer(void)
	{
	}

void GLSphereRenderer::initContext(GLContextData& contextData) const
	{
	/* Create context data item and store it in the GLContextData object: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the initial sphere shader program: */
	compileShader(dataItem,*contextData.getLightTracker());
	}

void GLSphereRenderer::setFixedRadius(GLfloat newFixedRadius)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(!fixedRadius)
		++settingsVersion;
	fixedRadius=true;
	
	/* Store the new fixed radius: */
	radius=newFixedRadius;
	}

void GLSphereRenderer::setVariableRadius(void)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(fixedRadius)
		++settingsVersion;
	fixedRadius=false;
	}

void GLSphereRenderer::setColorMaterial(bool newColorMaterial)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(colorMaterial!=newColorMaterial)
		++settingsVersion;
	colorMaterial=newColorMaterial;
	}

void GLSphereRenderer::enable(GLfloat modelViewScale,GLContextData& contextData) const
	{
	/* Retrieve the context data item: */
	DataItem* dataItem=contextData.retrieveDataItem<DataItem>(this);
	
	/* Check if the shader program is up-to-date: */
	const GLLightTracker& lightTracker=*contextData.getLightTracker();
	if(dataItem->settingsVersion!=settingsVersion||dataItem->lightStateVersion!=lightTracker.getVersion())
		{
		/* Recompile the shader program: */
		compileShader(dataItem,lightTracker);
		}
	
	/* Activate the shader program: */
	glUseProgramObjectARB(dataItem->shaderProgram);
	
	/* Check if all spheres use the same model-space radius: */
	if(fixedRadius)
		{
		/* Upload the current model-sphere radius: */
		glUniform1fARB(dataItem->shaderProgramUniforms[0],radius*modelViewScale);
		}
	else
		{
		/* Upload the current modelview scale: */
		glUniform1fARB(dataItem->shaderProgramUniforms[0],modelViewScale);
		}
	}

void GLSphereRenderer::disable(GLContextData& contextData) const
	{
	/* Deactivate the shader program: */
	glUseProgramObjectARB(0);
	}
