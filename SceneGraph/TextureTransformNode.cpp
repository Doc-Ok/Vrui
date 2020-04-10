/***********************************************************************
TextureTransformNode - Class for nodes that apply an orthogonal
transformation to model texture coordinates provided by geometry nodes.
Copyright (c) 2018-2019 Oliver Kreylos

This file is part of the Simple Scene Graph Renderer (SceneGraph).

The Simple Scene Graph Renderer is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Simple Scene Graph Renderer is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Simple Scene Graph Renderer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <SceneGraph/TextureTransformNode.h>

#include <string.h>
#include <Math/Math.h>
#include <SceneGraph/EventTypes.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

/*************************************
Methods of class TextureTransformNode:
*************************************/

TextureTransformNode::TextureTransformNode(void)
	:center(Point2::origin),
	 rotation(0),scale(Size2(1,1)),
	 translation(Vector2::zero),
	 transform(GLRenderState::TextureTransform::identity)
	{
	}

const char* TextureTransformNode::getStaticClassName(void)
	{
	return "TextureTransform";
	}

const char* TextureTransformNode::getClassName(void) const
	{
	return "TextureTransform";
	}

EventOut* TextureTransformNode::getEventOut(const char* fieldName) const
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventOut(this,center);
	else if(strcmp(fieldName,"rotation")==0)
		return makeEventOut(this,rotation);
	else if(strcmp(fieldName,"scale")==0)
		return makeEventOut(this,scale);
	else if(strcmp(fieldName,"translation")==0)
		return makeEventOut(this,translation);
	else
		return AttributeNode::getEventOut(fieldName);
	}

EventIn* TextureTransformNode::getEventIn(const char* fieldName)
	{
	if(strcmp(fieldName,"center")==0)
		return makeEventIn(this,center);
	else if(strcmp(fieldName,"rotation")==0)
		return makeEventIn(this,rotation);
	else if(strcmp(fieldName,"scale")==0)
		return makeEventIn(this,scale);
	else if(strcmp(fieldName,"translation")==0)
		return makeEventIn(this,translation);
	else
		return AttributeNode::getEventIn(fieldName);
	}

void TextureTransformNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"center")==0)
		{
		vrmlFile.parseField(center);
		}
	else if(strcmp(fieldName,"rotation")==0)
		{
		vrmlFile.parseField(rotation);
		}
	else if(strcmp(fieldName,"scale")==0)
		{
		vrmlFile.parseField(scale);
		}
	else if(strcmp(fieldName,"translation")==0)
		{
		vrmlFile.parseField(translation);
		}
	else
		AttributeNode::parseField(fieldName,vrmlFile);
	}

void TextureTransformNode::update(void)
	{
	/* Calculate the texture transformation: */
	transform=GLRenderState::TextureTransform::translate(translation.getValue());
	if(center.getValue()!=Point2::origin)
		{
		transform*=GLRenderState::TextureTransform::translateFromOriginTo(center.getValue());
		transform*=GLRenderState::TextureTransform::scale(GLRenderState::TextureTransform::Scale(scale.getValue()[0],scale.getValue()[1],Scalar(1)));
		transform*=GLRenderState::TextureTransform::rotate(Rotation::rotateZ(rotation.getValue()));
		transform*=GLRenderState::TextureTransform::translateToOriginFrom(center.getValue());
		}
	else
		{
		transform*=GLRenderState::TextureTransform::scale(GLRenderState::TextureTransform::Scale(scale.getValue()[0],scale.getValue()[1],Scalar(1)));
		transform*=GLRenderState::TextureTransform::rotate(Rotation::rotateZ(rotation.getValue()));
		}
	}

void TextureTransformNode::setGLState(GLRenderState& renderState) const
	{
	/* Set the texture transformation: */
	renderState.setTextureTransform(transform);
	}

void TextureTransformNode::resetGLState(GLRenderState& renderState) const
	{
	/* Reset the texture transformation: */
	renderState.resetTextureTransform();
	}

}
