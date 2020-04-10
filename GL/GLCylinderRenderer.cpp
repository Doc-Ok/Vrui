/***********************************************************************
GLCylinderRenderer - Class to render uncapped cylinders as ray-cast
impostors.
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

#include <GL/GLCylinderRenderer.h>

#include <string>
#include <Misc/PrintInteger.h>
#include <GL/GLLightTracker.h>
#include <GL/GLContextData.h>
#include <GL/Extensions/GLARBFragmentShader.h>
#include <GL/Extensions/GLARBGeometryShader4.h>
#include <GL/Extensions/GLARBVertexShader.h>

/*********************************************
Methods of class GLCylinderRenderer::DataItem:
*********************************************/

GLCylinderRenderer::DataItem::DataItem(void)
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

GLCylinderRenderer::DataItem::~DataItem(void)
	{
	/* Destroy the shader objects: */
	glDeleteObjectARB(vertexShader);
	glDeleteObjectARB(geometryShader);
	glDeleteObjectARB(fragmentShader);
	glDeleteObjectARB(shaderProgram);
	}

/***********************************
Methods of class GLCylinderRenderer:
***********************************/

void GLCylinderRenderer::compileShader(GLCylinderRenderer::DataItem* dataItem,const GLLightTracker& lightTracker) const
	{
	/* Create the impostor cylinder vertex shader source code: */
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
			/* Transform the axis end point to eye coordinates: */\n\
			gl_Position=gl_ModelViewMatrix*gl_Vertex;\n";
		}
	else
		{
		vertexShaderMain+="\
			/* Transform the axis end point to eye coordinates: */\n\
			gl_Position=vec4((gl_ModelViewMatrix*vec4(gl_Vertex.xyz,1.0)).xyz,gl_Vertex.w);\n";
		}
	vertexShaderMain+="\
		}\n";
	
	/* Compile the vertex shader and attach it to the cylinder shader program: */
	if(colorMaterial)
		glCompileShaderFromStrings(dataItem->vertexShader,2,vertexShaderVaryings.c_str(),vertexShaderMain.c_str());
	else
		glCompileShaderFromString(dataItem->vertexShader,vertexShaderMain.c_str());
	
	/* Create the impostor cylinder geometry shader source code: */
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
	if(colorMaterial)
		{
		geometryShaderVaryings+="\
		varying in vec4 inColor[2];\n\
		\n";
		}
	geometryShaderVaryings+="\
	varying out vec3 center;\n\
	varying out vec3 axis;\n\
	varying out float axis2;\n\
	varying out float radius2;\n\
	varying out vec3 dir;\n";
	if(colorMaterial)
		{
		geometryShaderVaryings+="\
		varying out vec4 color0;\n\
		varying out vec4 color1;\n\
		\n";
		}
	std::string geometryShaderMain="\
	void main()\n\
		{\n\
		/* Retrieve the cylinder's axis end points and radius in eye coordinates: */\n";
	if(fixedRadius)
		{
		geometryShaderMain+="\
		vec3 c0=gl_PositionIn[0].xyz/gl_PositionIn[0].w;\n\
		vec3 c1=gl_PositionIn[1].xyz/gl_PositionIn[1].w;\n\
		float r=fixedRadius;\n\
		\n";
		}
	else
		{
		geometryShaderMain+="\
		vec3 c0=gl_PositionIn[0].xyz;\n\
		vec3 c1=gl_PositionIn[1].xyz;\n\
		float r=mix(gl_PositionIn[0].w,gl_PositionIn[1].w,0.5)*modelViewScale;\n\
		\n";
		}
	#if 0
	geometryShaderMain+="\
		/* Sort the axis end points in lexicographic order to ensure that two cylinders with different orders are rendered exactly the same: */\n\
		if(c0.x>c1.x||(c0.x==c1.x&&(c0.y>c1.y||(c0.y==c1.y&&c0.z>c1.z))))\n\
			{\n\
			vec3 t=c0;\n\
			c0=c1;\n\
			c1=t;\n\
			}\n";
	#endif
	geometryShaderMain+="\
		vec3 c=mix(c0,c1,0.5);\n\
		vec3 a=c1-c;\n\
		float a2=dot(a,a);\n\
		float r2=r*r;\n\
		\n";
	if(colorMaterial)
		{
		geometryShaderMain+="\
		/* Retrieve the material color: */\n\
		vec4 col0=inColor[0];\n\
		vec4 col1=inColor[1];\n\
		\n";
		}
	
	geometryShaderMain+="\
		/* Calculate the impostor quad's primary axes: */\n\
		vec3 x=normalize(a);\n\
		vec3 y=normalize(cross(a,c));\n\
		\n\
		/* Calculate the impostor quad's width: */\n\
		vec3 d=cross(c,x);\n\
		float d2=dot(d,d);\n\
		float width=r*sqrt(d2/(d2-r2));\n\
		y*=width;\n\
		\n\
		/* Extend the impostor quad to the left and right: */\n\
		float aLen=sqrt(a2);\n\
		float dLen=sqrt(d2);\n\
		float eyex=-dot(c,x);\n";
	if(capped)
		{
		geometryShaderMain+="\
		if(eyex>-aLen)\n\
			c0-=x*((aLen+eyex)*r/(dLen-r));\n\
		else\n\
			c0-=x*((eyex+aLen)*r/(dLen+r));\n\
		if(eyex<aLen)\n\
			c1+=x*((aLen-eyex)*r/(dLen-r));\n\
		else\n\
			c1+=x*((eyex-aLen)*r/(dLen+r));\n";
		}
	else
		{
		geometryShaderMain+="\
		if(eyex>-aLen)\n\
			c0-=x*((aLen+eyex)*r/(dLen-r));\n\
		if(eyex<aLen)\n\
			c1+=x*((aLen-eyex)*r/(dLen-r));\n";
		}
	
	geometryShaderMain+="\
		\n\
		/* Emit the impostor quad's four vertices: */\n";
	for(int vertex=0;vertex<4;++vertex)
		{
		geometryShaderMain+="\
		center=c;\n\
		axis=a;\n\
		axis2=a2;\n\
		radius2=r2;\n\
		dir=c";
		geometryShaderMain.push_back(vertex/2+'0');
		geometryShaderMain.push_back(vertex%2==0?'+':'-');
		geometryShaderMain.append("y;\n");
		if(colorMaterial)
			{
			geometryShaderMain+="\
			color0=col0;\n\
			color1=col1;\n";
			}
		geometryShaderMain+="\
		gl_Position=gl_ProjectionMatrix*vec4(dir,1.0);\n\
		EmitVertex();\n";
		}
	geometryShaderMain+="\
		}\n";
	
	/* Compile the geometry shader and attach it to the cylinder shader program: */
	glCompileShaderFromStrings(dataItem->geometryShader,4,geometryShaderDeclarations.c_str(),geometryShaderUniforms.c_str(),geometryShaderVaryings.c_str(),geometryShaderMain.c_str());
	
	/* Set the geometry shader's parameters: */
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_VERTICES_OUT_ARB,4);
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_INPUT_TYPE_ARB,GL_LINES);
	glProgramParameteriARB(dataItem->shaderProgram,GL_GEOMETRY_OUTPUT_TYPE_ARB,GL_TRIANGLE_STRIP);
	
	/* Create the impostor cylinder fragment shader source code: */
	std::string fragmentShaderVaryings="\
	varying vec3 center;\n\
	varying vec3 axis;\n\
	varying float axis2;\n\
	varying float radius2;\n\
	varying vec3 dir;\n";
	if(colorMaterial)
		{
		fragmentShaderVaryings+="\
		varying vec4 color0; // Vertex color\n\
		varying vec4 color1; // Vertex color\n";
		}
	fragmentShaderVaryings.push_back('\n');
	std::string fragmentShaderFunctions;
	std::string fragmentShaderMain="\
	void main()\n\
		{\n\
		/* Calculate the intersection between the ray and the cylinder: */\n\
		vec3 mc0xmv1=cross(axis,center);\n\
		vec3 c0c1xmv1=cross(dir,axis);\n\
		float a=dot(c0c1xmv1,c0c1xmv1); // a from quadratic formula\n\
		if(a==0.0)\n\
			discard;\n\
		\n\
		float bh=dot(mc0xmv1,c0c1xmv1); // Half of b from quadratic formula\n\
		float c=dot(mc0xmv1,mc0xmv1)-radius2*axis2; // c from quadratic formula\n\
		float detq=bh*bh-a*c; // Quarter of discriminant\n\
		if(detq<=0.0)\n\
			discard;\n\
		float sqh=sqrt(detq); // Half of square root term\n\
		\n\
		/* Calculate the first intersection, where the ray enters the cylinder: */\n\
		float lambda=bh>=0.0?(-bh-sqh)/a:c/(-bh+sqh);\n\
		if(lambda<0.0)\n\
			discard;\n\
		\n\
		/* Calculate the intersection point: */\n\
		vec4 vertex;\n\
		vec3 normal;\n\
		\n\
		/* Calculate the vector from the edge midpoint to the intersection point and check it against the cylinder's height: */\n\
		float da=dot(dir,axis);\n\
		float ca=dot(center,axis);\n\
		float mcmv1=da*lambda-ca;\n";
	if(capped)
		{
		fragmentShaderMain+="\
		if(abs(mcmv1)<=axis2)\n\
			{\n\
			vertex=vec4(dir*lambda,1.0);\n\
			normal=vertex.xyz-center;\n\
			normal=normalize(normal-axis*(dot(normal,axis)/axis2));\n\
			}\n\
		else if(da*mcmv1<0.0)\n\
			{\n\
			lambda=mcmv1>=0.0?(ca+axis2)/da:(ca-axis2)/da;\n\
			vertex=vec4(dir*lambda,1.0);\n\
			vec3 dv=mcmv1>=0.0?vertex-center-axis:vertex-center+axis;\n\
			if(dot(dv,dv)>radius2)\n\
				discard;\n\
			normal=normalize(mcmv1>=0.0?axis:-axis);\n\
			}\n\
		else\n\
			discard;\n\
		\n";
		}
	else
		{
		fragmentShaderMain+="\
		if(abs(mcmv1)>axis2)\n\
			discard;\n\
		\n\
		vertex=vec4(dir*lambda,1.0);\n\
		normal=vertex.xyz-center;\n\
		normal=normalize(normal-axis*(dot(normal,axis)/axis2));\n\
		\n";
		}
	fragmentShaderMain+="\
		/* Calculate the intersection point's depth buffer value: */\n\
		vec4 vertexC=gl_ProjectionMatrix*vertex;\n\
		gl_FragDepth=0.5*(vertexC.z*gl_DepthRange.diff/vertexC.w+gl_DepthRange.near+gl_DepthRange.far);\n\
		\n\
		/* Calculate total illumination and initialize with global ambient term: */\n";
	if(colorMaterial)
		{
		if(bicolor)
			{
			fragmentShaderMain+="\
			vec4 color=mcmv1>=0.0?color1:color0;\n";
			}
		else
			{
			fragmentShaderMain+="\
			vec4 color=mix(color0,color1,(mcmv1+axis2)/(axis2*2.0));\n";
			}
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
	
	/* Compile the fragment shader and attach it to the cylinder shader program: */
	glCompileShaderFromStrings(dataItem->fragmentShader,3,fragmentShaderFunctions.c_str(),fragmentShaderVaryings.c_str(),fragmentShaderMain.c_str());
	
	/* Link the cylinder shader program: */
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

GLCylinderRenderer::GLCylinderRenderer(void)
	:fixedRadius(false),radius(0),
	 capped(false),
	 colorMaterial(false),bicolor(false),
	 settingsVersion(1)
	{
	}

GLCylinderRenderer::~GLCylinderRenderer(void)
	{
	}

void GLCylinderRenderer::initContext(GLContextData& contextData) const
	{
	/* Create context data item and store it in the GLContextData object: */
	DataItem* dataItem=new DataItem;
	contextData.addDataItem(this,dataItem);
	
	/* Create the initial cylinder shader program: */
	compileShader(dataItem,*contextData.getLightTracker());
	}

void GLCylinderRenderer::setFixedRadius(GLfloat newFixedRadius)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(!fixedRadius)
		++settingsVersion;
	fixedRadius=true;
	
	/* Store the new fixed radius: */
	radius=newFixedRadius;
	}

void GLCylinderRenderer::setVariableRadius(void)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(fixedRadius)
		++settingsVersion;
	fixedRadius=false;
	}

void GLCylinderRenderer::setCapped(bool newCapped)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(capped!=newCapped)
		++settingsVersion;
	capped=newCapped;
	}

void GLCylinderRenderer::setColorMaterial(bool newColorMaterial)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(colorMaterial!=newColorMaterial)
		++settingsVersion;
	colorMaterial=newColorMaterial;
	}

void GLCylinderRenderer::setBicolor(bool newBicolor)
	{
	/* Invalidate the shader program if the flag changed value: */
	if(bicolor!=newBicolor)
		++settingsVersion;
	bicolor=newBicolor;
	}

void GLCylinderRenderer::enable(GLfloat modelViewScale,GLContextData& contextData) const
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
	
	/* Check if all cylinders use the same model-space radius: */
	if(fixedRadius)
		{
		/* Upload the current model-space radius: */
		glUniform1fARB(dataItem->shaderProgramUniforms[0],radius*modelViewScale);
		}
	else
		{
		/* Upload the current modelview scale: */
		glUniform1fARB(dataItem->shaderProgramUniforms[0],modelViewScale);
		}
	}

void GLCylinderRenderer::disable(GLContextData& contextData) const
	{
	/* Deactivate the shader program: */
	glUseProgramObjectARB(0);
	}
