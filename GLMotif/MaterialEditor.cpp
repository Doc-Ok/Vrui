/***********************************************************************
MaterialEditor - Class for composite widgets to display and edit OpenGL
material properties.
Copyright (c) 2013-2020 Oliver Kreylos

This file is part of the GLMotif Widget Library (GLMotif).

The GLMotif Widget Library is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GLMotif Widget Library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the GLMotif Widget Library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include <GLMotif/MaterialEditor.h>

#include <GLMotif/StyleSheet.h>
#include <GLMotif/HSVColorSelector.h>
#include <GLMotif/TextFieldSlider.h>

namespace GLMotif {

/*******************************
Methods of class MaterialEditor:
*******************************/

void MaterialEditor::componentChangedCallback(Misc::CallbackData* cbData)
	{
	if(trackedMaterial!=0)
		{
		/* Update the tracked material: */
		*trackedMaterial=material;
		}
	
	/* Call the value changed callbacks: */
	ValueChangedCallbackData myCbData(this,material);
	valueChangedCallbacks.call(&myCbData);
	}

MaterialEditor::MaterialEditor(const char* sName,Container* sParent,bool sManageChild)
	:RowColumn(sName,sParent,false),
	 material(GLMaterial::Color(0.8f,0.8f,0.8f),GLMaterial::Color(0.5f,0.5f,0.5f),16.0f),
	 trackedMaterial(0)
	{
	/* Create the composite widget layout: */
	setOrientation(VERTICAL);
	setPacking(PACK_TIGHT);
	setNumMinorWidgets(1);
	
	/* Create the child widgets: */
	RowColumn* row1=new RowColumn("Row1",this,false);
	row1->setOrientation(HORIZONTAL);
	row1->setPacking(PACK_TIGHT);
	row1->setNumMinorWidgets(2);
	
	HSVColorSelector* ambient=new HSVColorSelector("AmbientColorSelector",row1);
	ambient->track(material.ambient);
	ambient->getValueChangedCallbacks().add(this,&MaterialEditor::componentChangedCallback);
	
	new Label("AmbientLabel",row1,"Ambient");
	
	HSVColorSelector*diffuse=new HSVColorSelector("DiffuseColorSelector",row1);
	diffuse->track(material.diffuse);
	diffuse->getValueChangedCallbacks().add(this,&MaterialEditor::componentChangedCallback);
	
	new Label("DiffuseLabel",row1,"Diffuse");
	
	HSVColorSelector*emissive=new HSVColorSelector("EmissiveColorSelector",row1);
	emissive->track(material.emission);
	emissive->getValueChangedCallbacks().add(this,&MaterialEditor::componentChangedCallback);
	
	new Label("EmissiveLabel",row1,"Emissive");
	
	row1->manageChild();
	
	RowColumn* row2=new RowColumn("Row2",this,false);
	row2->setOrientation(HORIZONTAL);
	row2->setPacking(PACK_TIGHT);
	row2->setNumMinorWidgets(2);
	
	HSVColorSelector*specular=new HSVColorSelector("SpecularColorSelector",row2);
	specular->track(material.specular);
	specular->getValueChangedCallbacks().add(this,&MaterialEditor::componentChangedCallback);
	
	new Label("SpecularLabel",row2,"Specular");
	
	TextFieldSlider* shininess=new TextFieldSlider("ShininessSlider",row2,4,getStyleSheet()->fontHeight*5.0f);
	shininess->getTextField()->setFieldWidth(3);
	shininess->getTextField()->setPrecision(0);
	shininess->getTextField()->setFloatFormat(TextField::FIXED);
	shininess->setSliderMapping(TextFieldSlider::LINEAR);
	shininess->setValueType(TextFieldSlider::FLOAT);
	shininess->setValueRange(0.0,128.0,1.0);
	shininess->track(material.shininess);
	shininess->getValueChangedCallbacks().add(this,&MaterialEditor::componentChangedCallback);
	
	new Label("ShininessLabel",row2,"Shininess");
	
	row2->manageChild();
	
	if(sManageChild)
		manageChild();
	}

void MaterialEditor::updateVariables(void)
	{
	if(trackedMaterial!=0)
		{
		/* Update the displayed material: */
		material=*trackedMaterial;
		}
	
	/* Update all component widgets: */
	RowColumn::updateVariables();
	}

void MaterialEditor::setMaterial(const GLMaterial& newMaterial)
	{
	/* Update the current material properties: */
	material=newMaterial;
	
	if(trackedMaterial!=0)
		{
		/* Update the tracked material: */
		*trackedMaterial=newMaterial;
		}
	
	/* Update all component widgets: */
	RowColumn::updateVariables();
	}

void MaterialEditor::track(GLMaterial& newTrackedMaterial)
	{
	/* Change the tracked material variable: */
	trackedMaterial=&newTrackedMaterial;
	
	/* Update the displayed material: */
	material=*trackedMaterial;
	
	/* Update all component widgets: */
	RowColumn::updateVariables();
	}

}
