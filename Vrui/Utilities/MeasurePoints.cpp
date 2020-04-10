/***********************************************************************
MeasurePoints - Vrui application to measure sets of 3D positions using a
tracked VR input device.
Copyright (c) 2019-2020 Oliver Kreylos

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

#include <Vrui/Utilities/MeasurePoints.h>

#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <Misc/PrintInteger.h>
#include <Misc/FunctionCalls.h>
#include <Misc/SelfDestructArray.h>
#include <Misc/HashTable.h>
#include <Misc/MessageLogger.h>
#include <IO/Directory.h>
#include <IO/CSVSource.h>
#include <IO/OStream.h>
#include <Math/Random.h>
#include <Math/Math.h>
#include <Math/Matrix.h>
#include <Geometry/Box.h>
#include <Geometry/AffineTransformation.h>
#include <GL/gl.h>
#include <GL/GLColorTemplates.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <GLMotif/Popup.h>
#include <GLMotif/PopupMenu.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Separator.h>
#include <GLMotif/Button.h>
#include <GLMotif/ScrolledListBox.h>
#include <Vrui/Vrui.h>
#include <Vrui/CoordinateManager.h>
#include <Vrui/UIManager.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/VRDeviceClient.h>

/******************************************************
Declaration of class MeasurePoints::ProbeTipCalibrator:
******************************************************/

class MeasurePoints::ProbeTipCalibrator
	{
	/* Embedded classes: */
	private:
	typedef Geometry::AffineTransformation<Scalar,3> ATransform;
	
	/* Elements: */
	unsigned int numRansacIterations;
	Scalar maxInlierDist2;
	Scalar minInlierRatio;
	std::vector<ATransform> calibTransforms; // List of collected calibration transformations for residual calculation
	Point probeTipLocal,probeTipGlobal; // Intermediate calibration results while collecting samples
	
	/* Private methods: */
	Math::Matrix testSolve(const std::vector<ATransform>& as) const
		{
		/* Enter all samples into a least-squares linear system: */
		Math::Matrix ata(6,6,0.0);
		Math::Matrix atb(6,1,0.0);
		for(std::vector<ATransform>::const_iterator aIt=as.begin();aIt!=as.end();++aIt)
			{
			for(int i=0;i<3;++i)
				{
				/* Set up the equation: */
				double eq[7];
				for(int j=0;j<3;++j)
					{
					eq[j]=aIt->getMatrix()(i,j);
					eq[3+j]=i==j?-1.0:0.0;
					}
				eq[6]=-aIt->getMatrix()(i,3);
				
				/* Enter the equation into the least-squares system: */
				for(int j=0;j<6;++j)
					{
					for(int k=0;k<6;++k)
						ata(j,k)+=eq[j]*eq[k];
					atb(j)+=eq[j]*eq[6];
					}
				}
			}
		
		/* Solve for the local and global probe tip position: */
		Math::Matrix x=atb;
		x.divideFullPivot(ata);
		return x;
		}
	
	/* Constructors and destructors: */
	public:
	ProbeTipCalibrator(unsigned int sNumRansacIterations,Scalar sMaxInlierDist,Scalar sMinInlierRatio)
		:numRansacIterations(sNumRansacIterations),
		 maxInlierDist2(Math::sqr(sMaxInlierDist)),minInlierRatio(sMinInlierRatio)
		{
		reset();
		}
	
	/* Methods: */
	void reset(void)
		{
		calibTransforms.clear();
		}
	bool haveSamples(void) const
		{
		return !calibTransforms.empty();
		}
	void addSample(const ONTransform& sample)
		{
		/* Convert the orthonormal transformation to an affine transformation: */
		ATransform ct;
		sample.writeMatrix(ct.getMatrix());
		
		/* Store the affine transformation for later: */
		calibTransforms.push_back(ct);
		
		if(calibTransforms.size()>=3)
			{
			/* Find an intermediate solution using all samples: */
			Math::Matrix x=testSolve(calibTransforms);
			probeTipLocal=Point(x(0),x(1),x(2));
			probeTipGlobal=Point(x(3),x(4),x(5));
			}
		}
	Point calcCalibCenter(void) const
		{
		Point::AffineCombiner cc;
		for(std::vector<ATransform>::const_iterator ctIt=calibTransforms.begin();ctIt!=calibTransforms.end();++ctIt)
			cc.addPoint(ctIt->getOrigin());
		return cc.getPoint();
		}
	Point calcProbeTip(void) const
		{
		int numSamples=int(calibTransforms.size());
		if(numSamples<3)
			throw std::runtime_error("ProbeTipCalibrator::calcProbeTip: Not enough samples");
		
		Misc::SelfDestructArray<int> indices(numSamples);
		
		/* Calibrate using RANSAC: */
		Scalar bestResidual=Math::Constants<Scalar>::infinity;
		Point bestTip;
		int bestNumInliers=0;
		for(unsigned int ransac=0;ransac<numRansacIterations;++ransac)
			{
			try
				{
				/* Pick three random calibration transformations: */
				for(int i=0;i<numSamples;++i)
					indices[i]=i;
				std::vector<ATransform> tcts;
				tcts.reserve(3);
				for(int i=0;i<3;++i)
					{
					int pick=Math::randUniformCO(i,numSamples);
					std::swap(indices[i],indices[pick]);
					tcts.push_back(calibTransforms[indices[i]]);
					}
				
				/* Solve for the probe tip using the three selected samples: */
				Math::Matrix x=testSolve(tcts);
				Point lp(x(0),x(1),x(2));
				Point gp(x(3),x(4),x(5));
				
				/* Find the inlier set: */
				std::vector<ATransform> inliers;
				for(std::vector<ATransform>::const_iterator ctIt=calibTransforms.begin();ctIt!=calibTransforms.end();++ctIt)
					{
					Scalar dist2=Geometry::sqrDist(gp,ctIt->transform(lp));
					if(dist2<maxInlierDist2)
						inliers.push_back(*ctIt);
					}
				
				/* Check if the inlier set is sufficiently populated: */
				if(Scalar(inliers.size())>=Scalar(numSamples)*minInlierRatio)
					{
					/* Solve for the probe tip using the inlier set: */
					Math::Matrix x=testSolve(inliers);
					Point lp(x(0),x(1),x(2));
					Point gp(x(3),x(4),x(5));
					
					/* Calculate the inlier set's residual: */
					Scalar residual(0);
					for(std::vector<ATransform>::const_iterator aIt=inliers.begin();aIt!=inliers.end();++aIt)
						residual+=Geometry::sqrDist(gp,aIt->transform(lp));
					
					if(bestResidual>residual)
						{
						bestResidual=residual;
						bestTip=lp;
						bestNumInliers=int(inliers.size());
						}
					}
				}
			catch(const std::runtime_error&)
				{
				/* Ignore the error and try again... */
				}
			}
		
		std::cout<<"Probe tip calibration result: "<<bestTip[0]<<", "<<bestTip[1]<<", "<<bestTip[2]<<std::endl;
		std::cout<<"Solution residual for "<<bestNumInliers<<" inliers: "<<Math::sqrt(bestResidual/Scalar(bestNumInliers))<<std::endl;
		
		return bestTip;
		}
	void glRenderAction(GLContextData& contextData) const
		{
		/* Draw all calibration samples: */
		GLfloat frameSize=float(Vrui::getUiSize()*Vrui::getInverseNavigationTransformation().getScaling())*2.0f;
		glLineWidth(1.0f);
		for(std::vector<ATransform>::const_iterator ctIt=calibTransforms.begin();ctIt!=calibTransforms.end();++ctIt)
			{
			glPushMatrix();
			glMultMatrix(*ctIt);
			
			glBegin(GL_LINES);
			glColor3f(1.0f,0.0f,0.0f);
			glVertex3f(-frameSize,0.0f,0.0f);
			glVertex3f( frameSize,0.0f,0.0f);
			glColor3f(0.0f,1.0f,0.0f);
			glVertex3f(0.0f,-frameSize,0.0f);
			glVertex3f(0.0f, frameSize,0.0f);
			glColor3f(0.0f,0.0f,1.0f);
			glVertex3f(0.0f,0.0f,-frameSize);
			glVertex3f(0.0f,0.0f, frameSize);
			glEnd();
			
			glPopMatrix();
			}
		
		if(calibTransforms.size()>=3)
			{
			/* Draw the intermediate calibration result: */
			glPointSize(3.0f);
			glBegin(GL_POINTS);
			
			glColor3f(0.5f,0.5f,0.5f);
			for(std::vector<ATransform>::const_iterator ctIt=calibTransforms.begin();ctIt!=calibTransforms.end();++ctIt)
				glVertex(ctIt->transform(probeTipLocal));
			
			glColor3f(1.0f,1.0f,1.0f);
			glVertex(probeTipGlobal);
			
			glEnd();
			}
		}
	};

/**************************************
Static elements of class MeasurePoints:
**************************************/

const MeasurePoints::Color MeasurePoints::pointSetColors[]=
	{
	Color(1.0f,0.0f,0.0f),Color(0.0f,1.0f,0.0f),Color(0.0f,0.0f,1.0f),
	Color(1.0f,1.0f,0.0f),Color(0.0f,1.0f,1.0f),Color(1.0f,0.0f,1.0f),
	Color(1.0f,0.5f,0.0f),Color(0.5f,1.0f,0.0f),Color(0.0f,1.0f,0.5f),
	Color(0.0f,0.5f,1.0f),Color(0.5f,0.0f,1.0f),Color(1.0f,0.0f,0.5f)
	};

/******************************
Methods of class MeasurePoints:
******************************/

void MeasurePoints::trackingCallback(Vrui::VRDeviceClient* client)
	{
	/* Check if there is no active device: */
	const Vrui::VRDeviceState& state=deviceClient->getState();
	if(triggerButtonIndex<0)
		{
		/* Check if any buttons are pressed: */
		for(int i=0;i<state.getNumButtons();++i)
			if(state.getButtonState(i))
				{
				triggerButtonIndex=i;
				triggerButtonIndexChanged=true;
				triggerButtonState=true;
				break;
				}
		
		/* Bail out if still no active device: */
		if(triggerButtonIndex<0)
			return;
		}
	
	/* Retrieve the active device and the current device state: */
	const Vrui::VRDeviceDescriptor& device=*buttonDevices[triggerButtonIndex];
	const Vrui::VRDeviceState::TrackerState::PositionOrientation& po=state.getTrackerState(device.trackerIndex).positionOrientation;
	bool valid=state.getTrackerValid(device.trackerIndex);
	if(valid)
		trackerFrame.postNewValue(po);
	
	/* Check if we are currently calibrating or sampling for a measurement: */
	if(numSamplesLeft>0)
		{
		/* Check if the active tracker is currently valid: */
		if(valid)
			{
			/* Accumulate the next measurement sample: */
			sampleAccumulator.addPoint(po.transform(probeTip));
			--numSamplesLeft;
			
			/* Check if the current measurement is complete: */
			if(numSamplesLeft==0)
				{
				/* Store the new measurement in the active point set: */
				{
				Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
				pointSets[activePointSet].points.push_back(sampleAccumulator.getPoint());
				}
				
				if(device.numHapticFeatures>0)
					{
					/* Notify the user with a haptic tick: */
					deviceClient->hapticTick(device.hapticFeatureIndices[0],50,100,255);
					}
				}
			}
		else
			{
			/* Cancel the current measurement: */
			numSamplesLeft=0;
			Vrui::requestUpdate();
			}
		}
	
	/* Get the state of the trigger button: */
	bool newTriggerButtonState=state.getButtonState(triggerButtonIndex);
	
	/* Check if the trigger button was pressed in idle mode: */
	if(numSamplesLeft==0&&newTriggerButtonState&&!triggerButtonState)
		{
		Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
		if(calibrator!=0)
			{
			/* Check if the active tracker is currently valid: */
			if(valid)
				{
				/* Add a calibration sample: */
				calibrator->addSample(po);
				
				if(device.numHapticFeatures>0)
					{
					/* Notify the user with a haptic tick: */
					deviceClient->hapticTick(device.hapticFeatureIndices[0],50,100,255);
					}
				}
			}
		else
			{
			/* Start a new measurement: */
			numSamplesLeft=numSamples;
			sampleAccumulator.reset();
			}
		}
	triggerButtonState=newTriggerButtonState;
	
	if(valid)
		{
		/* Wake up the main thread: */
		Vrui::requestUpdate();
		}
	}

void MeasurePoints::setActivePointSet(size_t newActivePointSet)
	{
	/* Set the active point set: */
	activePointSet=newActivePointSet;
	
	if(pointSetsDialog!=0)
		{
		/* Update the point sets dialog: */
		pointSetList->getListBox()->selectItem(activePointSet,true);
		PointSet& ps=pointSets[activePointSet];
		pointSetLabel->setString(ps.label.c_str());
		pointSetDraw->setToggle(ps.draw);
		pointSetColor->setCurrentColor(ps.color);
		}
	}

void MeasurePoints::startNewPointSet(void)
	{
	PointSet newPs;
	
	/* Create a numeric label: */
	char labelBuffer[6];
	newPs.label="Point Set ";
	newPs.label.append(Misc::print((unsigned int)(pointSets.size()+1),labelBuffer+5));
	
	#if 0
	/* Assign a random color: */
	for(int i=0;i<3;++i)
		newPs.color[i]=float(Math::randUniformCC(0.5,1.0));
	#else
	/* Assign a default color: */
	newPs.color=pointSetColors[pointSets.size()%12];
	#endif
	
	/* Draw the point set initially: */
	newPs.draw=true;
	
	/* Store the new point set: */
	pointSets.push_back(newPs);
	
	if(pointSetsDialog!=0)
		{
		/* Update the point sets dialog: */
		pointSetList->getListBox()->addItem(newPs.label.c_str());
		setActivePointSet(pointSets.size()-1);
		}
	else
		activePointSet=pointSets.size()-1;
	}

void MeasurePoints::loadPointSets(IO::Directory& directory,const char* fileName)
	{
	try
		{
		/* Open a CSV source for a file of the given name: */
		IO::CSVSource file(directory.openFile(fileName));
		
		/* Create a new point set list: */
		PointSetList newPointSets;
		
		/* Create a hash table mapping from point set indices to point sets: */
		Misc::HashTable<int,int> pointSetMap(5);
		
		/* Skip the header record: */
		file.skipRecord();
		
		/* Read all records from the CSV file: */
		while(!file.eof())
			{
			/* Parse the current record: */
			int pointSetIndex=file.readField<int>();
			std::string pointSetLabel=file.readField<std::string>();
			
			/* Find the point set to which this record belongs: */
			Misc::HashTable<int,int>::Iterator psIt=pointSetMap.findEntry(pointSetIndex);
			if(psIt.isFinished())
				{
				/* Create a new point set: */
				PointSet newPointSet;
				newPointSet.label=pointSetLabel;
				newPointSet.color=pointSetColors[newPointSets.size()%12];
				newPointSet.transform=OGTransform::identity;
				newPointSet.draw=true;
				
				/* Add the new point set to the new point set list: */
				pointSetMap.setEntry(Misc::HashTable<int,int>::Entry(pointSetIndex,int(newPointSets.size())));
				newPointSets.push_back(newPointSet);
				psIt=pointSetMap.findEntry(pointSetIndex);
				}
			
			file.readField<int>(); // Skip point index
			Point position;
			for(int i=0;i<3;++i)
				position[i]=file.readField<Scalar>();
			
			/* Check if the record was completely read: */
			if(!file.eor())
				throw std::runtime_error("Extra fields at end of point record");
			
			/* Add the new point to the point set: */
			newPointSets[psIt->getDest()].points.push_back(position);
			}
		
		/* Append the newly-read point sets to the list: */
		pointSets.insert(pointSets.end(),newPointSets.begin(),newPointSets.end());
		activePointSet=pointSets.size()-1;
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("MeasurePoints: Unable to load point sets from file %s due to exception %s",directory.getPath(fileName).c_str(),err.what());
		}
	}

void MeasurePoints::savePointSets(IO::Directory& directory,const char* fileName) const
	{
	try
		{
		/* Open an output stream to a file of the given name: */
		IO::OStream file(directory.openFile(fileName,IO::File::WriteOnly));
		
		/* Write a header line: */
		file<<"\"Point Set Index\",\"Point Set Label\",\"Point Index\",\"Position X\",\"Position Y\",\"Position Z\""<<std::endl;
		
		/* Write all point sets: */
		{
		Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
		size_t pointSetIndex=1;
		for(PointSetList::const_iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt,++pointSetIndex)
			{
			/* Write all points in the point set: */
			size_t pointIndex=1;
			for(PointList::const_iterator pIt=psIt->points.begin();pIt!=psIt->points.end();++pIt,++pointIndex)
				file<<pointSetIndex<<",\""<<psIt->label<<"\","<<pointIndex<<','<<(*pIt)[0]<<','<<(*pIt)[1]<<','<<(*pIt)[2]<<std::endl;
			}
		}
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("MeasurePoints: Unable to save point sets to file %s due to exception %s",directory.getPath(fileName).c_str(),err.what());
		}
	}

void MeasurePoints::objectSnapCallback(Vrui::ObjectSnapperToolFactory::SnapRequest& snapRequest)
	{
	/* Snap all visible measured point sets: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	for(PointSetList::iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
		if(psIt->draw)
			{
			for(PointList::iterator pIt=psIt->points.begin();pIt!=psIt->points.end();++pIt)
				snapRequest.snapPoint(psIt->transform.transform(*pIt));
			}
	}

GLMotif::Popup* MeasurePoints::createPressButtonPrompt(void)
	{
	GLMotif::Popup* result=new GLMotif::Popup("PressButtonPromptPopup",Vrui::getWidgetManager());
	result->setTitle("Measure Points");
	GLMotif::RowColumn* labelBox=new GLMotif::RowColumn("LabelBox",result,false);
	labelBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	labelBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	
	new GLMotif::Label("Label1",labelBox,"Please press an input");
	new GLMotif::Label("Label2",labelBox,"device button to act");
	new GLMotif::Label("Label3",labelBox,"as measurement trigger");
	
	labelBox->manageChild();
	
	return result;
	}

void MeasurePoints::selectDeviceCallback(Misc::CallbackData* cbData)
	{
	/* Reset the trigger button index: */
	{
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	triggerButtonIndex=-1;
	}
	
	/* Show the button press prompt: */
	Vrui::popupPrimaryWidget(pressButtonPrompt);
	}

void MeasurePoints::calibrateCallback(Misc::CallbackData* cbData)
	{
	/* Start calibration procedure: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	if(calibrator==0)
		{
		/* Create a new probe tip calibrator: */
		calibrator=new ProbeTipCalibrator(1000,0.005,0.75);
		
		/* Show the calibration dialog: */
		Vrui::popupPrimaryWidget(calibrationDialog);
		}
	}

void MeasurePoints::loadPointSetsCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Load point sets: */
	loadPointSets(*cbData->selectedDirectory,cbData->selectedFileName);
	}

void MeasurePoints::savePointSetsCallback(GLMotif::FileSelectionDialog::OKCallbackData* cbData)
	{
	/* Save the point sets: */
	savePointSets(*cbData->selectedDirectory,cbData->selectedFileName);
	}

void MeasurePoints::showPointSetsDialogCallback(Misc::CallbackData* cbData)
	{
	Vrui::popupPrimaryWidget(pointSetsDialog);
	}

GLMotif::PopupMenu* MeasurePoints::createMainMenu(void)
	{
	GLMotif::PopupMenu* result=new GLMotif::PopupMenu("MainMenuPopup",Vrui::getWidgetManager());
	result->setTitle("Measure Points");
	
	/* Create a button to change the measurement device or trigger button: */
	GLMotif::Button* selectDeviceButton=result->addEntry("Select Input Device");
	selectDeviceButton->getSelectCallbacks().add(this,&MeasurePoints::selectDeviceCallback);
	
	/* Create a button to start probe tip calibration: */
	GLMotif::Button* calibrateButton=result->addEntry("Calibrate Probe Tip");
	calibrateButton->getSelectCallbacks().add(this,&MeasurePoints::calibrateCallback);
	
	/* Create buttons to manage point sets: */
	GLMotif::Button* loadPointSetsButton=result->addEntry("Load Point Sets");
	saveHelper.addLoadCallback(loadPointSetsButton,Misc::createFunctionCall(this,&MeasurePoints::loadPointSetsCallback));
	GLMotif::Button* savePointSetsButton=result->addEntry("Save Point Sets");
	saveHelper.addSaveCallback(savePointSetsButton,Misc::createFunctionCall(this,&MeasurePoints::savePointSetsCallback));
	GLMotif::Button* showPointSetsDialogButton=result->addEntry("Show Point Sets Dialog");
	showPointSetsDialogButton->getSelectCallbacks().add(this,&MeasurePoints::showPointSetsDialogCallback);
	
	result->manageMenu();
	return result;
	}

void MeasurePoints::calibrationAddSampleCallback(Misc::CallbackData* cbData)
	{
	/* Add the calibrated tracker's current pose to the calibrator: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	calibrator->addSample(trackerFrame.getLockedValue());
	}

void MeasurePoints::calibrationOkCallback(Misc::CallbackData* cbData)
	{
	/* Complete the current probe tip calibration: */
	{
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	try
		{
		/* Calculate the new probe tip: */
		probeTip=calibrator->calcProbeTip();
		}
	catch(const std::runtime_error& err)
		{
		Misc::formattedUserError("MeasurePoints: Unable to calibrate probe tip due to exception %s",err.what());
		}
	delete calibrator;
	calibrator=0;
	}
	
	/* Pop down the calibration dialog: */
	Vrui::popdownPrimaryWidget(calibrationDialog);
	}

void MeasurePoints::calibrationCancelCallback(Misc::CallbackData* cbData)
	{
	{
	/* Cancel the current probe tip calibration: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	delete calibrator;
	calibrator=0;
	}
	
	/* Pop down the calibration dialog: */
	Vrui::popdownPrimaryWidget(calibrationDialog);
	}

GLMotif::PopupWindow* MeasurePoints::createCalibrationDialog(void)
	{
	GLMotif::PopupWindow* result=new GLMotif::PopupWindow("CalibrationDialogPopup",Vrui::getWidgetManager(),"Probe Tip Calibration");
	result->setResizableFlags(true,false);
	
	GLMotif::RowColumn* calibrationDialog=new GLMotif::RowColumn("CalibrationDialog",result,false);
	calibrationDialog->setOrientation(GLMotif::RowColumn::VERTICAL);
	calibrationDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	calibrationDialog->setNumMinorWidgets(1);
	
	new GLMotif::Label("InstructionLabel1",calibrationDialog,"Please collect at least three samples");
	new GLMotif::Label("InstructionLabel2",calibrationDialog,"with different orientations");
	
	GLMotif::RowColumn* resultBox=new GLMotif::RowColumn("ResultBox",calibrationDialog,false);
	resultBox->setOrientation(GLMotif::RowColumn::VERTICAL);
	resultBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	resultBox->setNumMinorWidgets(2);
	
	new GLMotif::Label("NumSamplesLabel",resultBox,"# samples");
	
	GLMotif::RowColumn* samplesBox=new GLMotif::RowColumn("SamplesBox",resultBox,false);
	samplesBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	samplesBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	samplesBox->setNumMinorWidgets(1);
	
	GLMotif::TextField* calibrationNumSamples=new GLMotif::TextField("CalibrationNumSamples",samplesBox,8);
	
	GLMotif::Button* sampleButton=new GLMotif::Button("SampleButton",samplesBox,"Sample");
	sampleButton->getSelectCallbacks().add(this,&MeasurePoints::calibrationAddSampleCallback);
	
	samplesBox->setColumnWeight(0,1.0f);
	samplesBox->manageChild();
	
	new GLMotif::Label("ProbeTipLabel",resultBox,"Probe Tip");
	
	GLMotif::RowColumn* probeTipBox=new GLMotif::RowColumn("ProbeTipBox",resultBox,false);
	probeTipBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	probeTipBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	probeTipBox->setNumMinorWidgets(1);
	
	GLMotif::TextField* calibrationProbeTipX=new GLMotif::TextField("CalibrationProbeTipX",probeTipBox,6);
	GLMotif::TextField* calibrationProbeTipY=new GLMotif::TextField("CalibrationProbeTipY",probeTipBox,6);
	GLMotif::TextField* calibrationProbeTipZ=new GLMotif::TextField("CalibrationProbeTipZ",probeTipBox,6);
	
	probeTipBox->manageChild();
	
	new GLMotif::Label("ResidualLabel",resultBox,"Residual");
	
	GLMotif::TextField* calibrationResidual=new GLMotif::TextField("CalibrationResidual",resultBox,8);
	
	resultBox->manageChild();
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",calibrationDialog,false);
	buttonMargin->setAlignment(GLMotif::Alignment::HCENTER);
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	buttonBox->setNumMinorWidgets(1);
	
	GLMotif::Button* okButton=new GLMotif::Button("OkButton",buttonBox,"OK");
	okButton->getSelectCallbacks().add(this,&MeasurePoints::calibrationOkCallback);
	
	GLMotif::Button* cancelButton=new GLMotif::Button("CancelButton",buttonBox,"Cancel");
	cancelButton->getSelectCallbacks().add(this,&MeasurePoints::calibrationCancelCallback);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	calibrationDialog->manageChild();
	return result;
	}

void MeasurePoints::pointSetListValueChangedCallback(GLMotif::ListBox::ValueChangedCallbackData* cbData)
	{
	if(cbData->interactive)
		{
		/* Activate the newly-selected point set: */
		Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
		setActivePointSet(cbData->newSelectedItem);
		}
	}

void MeasurePoints::addPointSetCallback(Misc::CallbackData* cbData)
	{
	/* Start a new point set: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	startNewPointSet();
	}

void MeasurePoints::deletePointSetCallback(Misc::CallbackData* cbData)
	{
	/* Delete the active point set: */
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	pointSets.erase(pointSets.begin()+activePointSet);
	pointSetList->getListBox()->removeItem(activePointSet);
	
	if(pointSets.empty())
		{
		/* Immediately start a new point set: */
		startNewPointSet();
		}
	else
		{
		/* Update the point sets dialog: */
		setActivePointSet(activePointSet<pointSets.size()?activePointSet:pointSets.size()-1);
		}
	}

void MeasurePoints::pointSetLabelValueChangedCallback(GLMotif::TextField::ValueChangedCallbackData* cbData)
	{
	/* Check if this is a final update: */
	if(cbData->confirmed)
		{
		/* Change the active point set's label: */
		pointSets[activePointSet].label=cbData->value;
		
		/* Update the list box: */
		pointSetList->getListBox()->setItem(activePointSet,cbData->value);
		}
	}

void MeasurePoints::pointSetDrawValueChangedCallback(GLMotif::ToggleButton::ValueChangedCallbackData* cbData)
	{
	/* Set the active point set's draw flag: */
	pointSets[activePointSet].draw=cbData->set;
	}

void MeasurePoints::pointSetColorValueChangedCallback(GLMotif::HSVColorSelector::ValueChangedCallbackData* cbData)
	{
	/* Set the active point set's color: */
	pointSets[activePointSet].color=cbData->newColor;
	}

void MeasurePoints::pointSetResetTransformCallback(Misc::CallbackData* cbData)
	{
	/* Reset the active point set's transformation: */
	pointSets[activePointSet].transform=OGTransform::identity;
	}

GLMotif::PopupWindow* MeasurePoints::createPointSetsDialog(void)
	{
	GLMotif::PopupWindow* result=new GLMotif::PopupWindow("PointSetsDialogPopup",Vrui::getWidgetManager(),"Point Sets");
	result->setCloseButton(true);
	result->popDownOnClose();
	
	GLMotif::RowColumn* pointSetsDialog=new GLMotif::RowColumn("PointSetsDialog",result,false);
	pointSetsDialog->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	pointSetsDialog->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	pointSetsDialog->setNumMinorWidgets(1);
	
	GLMotif::RowColumn* pointSetListPanel=new GLMotif::RowColumn("PointSetListPanel",pointSetsDialog,false);
	pointSetListPanel->setOrientation(GLMotif::RowColumn::VERTICAL);
	pointSetListPanel->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	pointSetListPanel->setNumMinorWidgets(1);
	
	pointSetList=new GLMotif::ScrolledListBox("PointSetList",pointSetListPanel,GLMotif::ListBox::ALWAYS_ONE,20,10);
	for(PointSetList::iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
		pointSetList->getListBox()->addItem(psIt->label.c_str());
	pointSetList->getListBox()->selectItem(activePointSet);
	pointSetList->getListBox()->getValueChangedCallbacks().add(this,&MeasurePoints::pointSetListValueChangedCallback);
	
	GLMotif::Margin* listButtonMargin=new GLMotif::Margin("ListButtonMargin",pointSetListPanel,false);
	listButtonMargin->setAlignment(GLMotif::Alignment::HCENTER);
	
	GLMotif::RowColumn* listButtonBox=new GLMotif::RowColumn("ListButtonBox",listButtonMargin,false);
	listButtonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	listButtonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	listButtonBox->setNumMinorWidgets(1);
	
	GLMotif::Button* addPointSetButton=new GLMotif::Button("AddPointSetButton",listButtonBox," + ");
	addPointSetButton->getSelectCallbacks().add(this,&MeasurePoints::addPointSetCallback);
	
	new GLMotif::Blind("Blind",listButtonBox);
	
	GLMotif::Button* deletePointSetButton=new GLMotif::Button("DeletePointSetButton",listButtonBox," - ");
	deletePointSetButton->getSelectCallbacks().add(this,&MeasurePoints::deletePointSetCallback);
	
	listButtonBox->manageChild();
	
	listButtonMargin->manageChild();
	
	pointSetListPanel->setRowWeight(0,1.0f);
	pointSetListPanel->manageChild();
	
	GLMotif::Margin* pointSetMargin=new GLMotif::Margin("PointSetMargin",pointSetsDialog,false);
	pointSetMargin->setAlignment(GLMotif::Alignment::VCENTER);
	
	GLMotif::RowColumn* pointSetPanel=new GLMotif::RowColumn("PointSetPanel",pointSetMargin,false);
	pointSetPanel->setOrientation(GLMotif::RowColumn::VERTICAL);
	pointSetPanel->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	pointSetPanel->setNumMinorWidgets(1);
	
	pointSetLabel=new GLMotif::TextField("LabelTextField",pointSetPanel,20);
	pointSetLabel->setHAlignment(GLFont::Left);
	pointSetLabel->setEditable(true);
	pointSetLabel->setString(pointSets[activePointSet].label.c_str());
	pointSetLabel->getValueChangedCallbacks().add(this,&MeasurePoints::pointSetLabelValueChangedCallback);
	
	GLMotif::RowColumn* drawColorBox=new GLMotif::RowColumn("DrawColorBox",pointSetPanel,false);
	drawColorBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	drawColorBox->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	drawColorBox->setNumMinorWidgets(1);
	
	GLMotif::Margin* drawToggleMargin=new GLMotif::Margin("DrawToggleMargin",drawColorBox,false);
	drawToggleMargin->setAlignment(GLMotif::Alignment::VCENTER);
	
	pointSetDraw=new GLMotif::ToggleButton("DrawToggle",drawToggleMargin,"Draw");
	pointSetDraw->setBorderType(GLMotif::Widget::PLAIN);
	pointSetDraw->setBorderWidth(0.0f);
	pointSetDraw->setToggle(pointSets[activePointSet].draw);
	pointSetDraw->getValueChangedCallbacks().add(this,&MeasurePoints::pointSetDrawValueChangedCallback);
	
	drawToggleMargin->manageChild();
	
	pointSetColor=new GLMotif::HSVColorSelector("DrawColor",drawColorBox);
	pointSetColor->setCurrentColor(pointSets[activePointSet].color);
	pointSetColor->getValueChangedCallbacks().add(this,&MeasurePoints::pointSetColorValueChangedCallback);
	
	drawColorBox->manageChild();
	
	GLMotif::Button* resetTransformButton=new GLMotif::Button("ResetTransformButton",pointSetPanel,"Reset Transform");
	resetTransformButton->getSelectCallbacks().add(this,&MeasurePoints::pointSetResetTransformCallback);
	
	pointSetPanel->setRowWeight(1,1.0f);
	pointSetPanel->manageChild();
	
	pointSetMargin->manageChild();
	
	pointSetsDialog->setColumnWeight(0,0.5f);
	pointSetsDialog->setColumnWeight(1,0.5f);
	pointSetsDialog->manageChild();
	
	return result;
	}

MeasurePoints::MeasurePoints(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 deviceClient(0),
	 trackingUnit(Geometry::LinearUnit::METER,1.0),probeTip(Point::origin),
	 triggerButtonIndex(-1),triggerButtonIndexChanged(false),triggerButtonState(false),
	 numSamples(10),numSamplesLeft(0),
	 calibrator(0),
	 saveHelper(Vrui::getWidgetManager(),"PointSets.csv",".csv"),
	 pressButtonPrompt(0),mainMenu(0),calibrationDialog(0),pointSetsDialog(0),
	 numberRenderer(GLfloat(Vrui::getUiSize())*2.0f,true)
	{
	/* Parse command line: */
	char defaultServerName[]="localhost:8555";
	char* serverName=defaultServerName;
	for(int i=1;i<argc;++i)
		{
		if(argv[i][0]=='-')
			{
			if(strcasecmp(argv[i]+1,"unit")==0)
				{
				if(i+2<argc)
					{
					const char* unitName=argv[i+1];
					double unitFactor=atof(argv[i+2]);
					try
						{
						/* Update the tracking unit: */
						trackingUnit=Geometry::LinearUnit(unitName,unitFactor);
						}
					catch(const std::runtime_error& err)
						{
						std::cerr<<"MeasurePoints: Ignoring command line option "<<argv[i]<<' '<<argv[i+1]<<' '<<argv[i+2]<<" due to exception "<<err.what()<<std::endl;
						}
					}
				else
					std::cerr<<"MeasurePoints: Ignoring incomplete command line option "<<argv[i]<<" <unit name> <unit factor>"<<std::endl;
				i+=2;
				}
			else if(strcasecmp(argv[i]+1,"numSamples")==0||strcasecmp(argv[i]+1,"ns")==0)
				{
				if(i+1<argc)
					numSamples=atoi(argv[i+1]);
				else
					std::cerr<<"MeasurePoints: Ignoring incomplete command line option "<<argv[i]<<" <num samples>"<<std::endl;
				++i;
				}
			else if(strcasecmp(argv[i]+1,"probeTip")==0||strcasecmp(argv[i]+1,"pt")==0)
				{
				if(i+3<argc)
					{
					for(int j=0;j<3;++j)
						probeTip[j]=Scalar(atof(argv[i+1+j]));
					}
				else
					std::cerr<<"MeasurePoints: Ignoring incomplete command line option "<<argv[i]<<" <x> <y> <z>"<<std::endl;
				i+=3;
				}
			else
				std::cerr<<"MeasurePoints: Ignoring command line option "<<argv[i]<<std::endl;
			}
		else if(serverName==defaultServerName)
			serverName=argv[i];
		else
			std::cerr<<"MeasurePoints: Ignoring command line argument "<<argv[i]<<std::endl;
		}
	
	/* Split the server name into hostname:port: */
	char* colonPtr=0;
	for(char* cPtr=serverName;*cPtr!='\0';++cPtr)
		if(*cPtr==':')
			colonPtr=cPtr;
	int portNumber=8555;
	if(colonPtr!=0)
		{
		portNumber=atoi(colonPtr+1);
		*colonPtr='\0';
		}
	
	/* Initialize device client: */
	deviceClient=new Vrui::VRDeviceClient(serverName,portNumber);
	
	/* Retrieve the number of buttons in the VRDeviceDaemon's namespace: */
	deviceClient->lockState();
	int numButtons=deviceClient->getState().getNumButtons();
	deviceClient->unlockState();
	
	/* Initialize the button device mapping array: */
	buttonDevices.reserve(numButtons);
	for(int i=0;i<numButtons;++i)
		buttonDevices.push_back(0);
	
	/* Point all devices' buttons to the devices containing them: */
	for(int i=0;i<deviceClient->getNumVirtualDevices();++i)
		{
		const Vrui::VRDeviceDescriptor& device=deviceClient->getVirtualDevice(i);
		for(int j=0;j<device.numButtons;++j)
			buttonDevices[device.buttonIndices[j]]=&device;
		}
	
	/* Start the first point set: */
	startNewPointSet();
	
	/* Activate the device client and start streaming: */
	deviceClient->activate();
	deviceClient->startStream(Misc::createFunctionCall(this,&MeasurePoints::trackingCallback));
	
	/* Create the button press prompt: */
	pressButtonPrompt=createPressButtonPrompt();
	Vrui::popupPrimaryWidget(pressButtonPrompt);
	
	/* Create the main menu: */
	mainMenu=createMainMenu();
	Vrui::setMainMenu(mainMenu);
	
	/* Create the probe tip calibration dialog: */
	calibrationDialog=createCalibrationDialog();
	
	/* Create the point set dialog: */
	pointSetsDialog=createPointSetsDialog();
	
	/* Set the linear unit of navigational space to the tracking unit: */
	Vrui::getCoordinateManager()->setUnit(trackingUnit);
	
	/* Create event tool classes: */
	addEventTool("Start New Point Set",0,0);
	addEventTool("Delete Last Point Set",0,1);
	addEventTool("Delete Last Point",0,2);
	addEventTool("Calibrate Probe Tip",0,3);
	
	/* Register a callback with the object snapper tool class: */
	Vrui::ObjectSnapperTool::addSnapCallback(Misc::createFunctionCall(this,&MeasurePoints::objectSnapCallback));
	}

MeasurePoints::~MeasurePoints(void)
	{
	/* Stop streaming and deactivate the device client: */
	deviceClient->stopStream();
	deviceClient->deactivate();
	
	/* Clean up: */
	delete deviceClient;
	delete calibrator;
	delete pressButtonPrompt;
	delete mainMenu;
	delete calibrationDialog;
	delete pointSetsDialog;
	}

void MeasurePoints::frame(void)
	{
	/* Lock the most recent tracking state of the trigger button device: */
	trackerFrame.lockNewValue();
	
	/* Check if the trigger button index changed since the last frame: */
	if(triggerButtonIndexChanged)
		{
		/* Pop down the button press prompt: */
		Vrui::popdownPrimaryWidget(pressButtonPrompt);
		
		/* Reset the navigation transformation: */
		resetNavigation();
		triggerButtonIndexChanged=false;
		}
	}

void MeasurePoints::display(GLContextData& contextData) const
	{
	/* Set up OpenGL state: */
	glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT|GL_POINT_BIT);
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glPointSize(3.0f);
	
	if(triggerButtonIndex>=0)
		{
		/* Draw the active tracker's current state: */
		glPushMatrix();
		glMultMatrix(pointSets[activePointSet].transform);
		glMultMatrix(trackerFrame.getLockedValue());
		
		GLfloat frameSize=float(Vrui::getUiSize()*Vrui::getInverseNavigationTransformation().getScaling())*2.0f;
		glBegin(GL_LINES);
		glColor3f(1.0f,0.0f,0.0f);
		glVertex3f(-frameSize,0.0f,0.0f);
		glVertex3f( frameSize,0.0f,0.0f);
		glColor3f(0.0f,1.0f,0.0f);
		glVertex3f(0.0f,-frameSize,0.0f);
		glVertex3f(0.0f, frameSize,0.0f);
		glColor3f(0.0f,0.0f,1.0f);
		glVertex3f(0.0f,0.0f,-frameSize);
		glVertex3f(0.0f,0.0f, frameSize);
		glEnd();
		
		glBegin(GL_POINTS);
		glColor3f(1.0f,1.0f,1.0f);
		glVertex(probeTip);
		glEnd();
		
		glPopMatrix();
		}
	
	{
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	
	if(calibrator!=0)
		calibrator->glRenderAction(contextData);
	
	/* Draw all measured point sets: */
	glBegin(GL_POINTS);
	for(PointSetList::const_iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
		if(psIt->draw)
			{
			glColor(psIt->color);
			for(PointList::const_iterator pIt=psIt->points.begin();pIt!=psIt->points.end();++pIt)
				glVertex(psIt->transform.transform(*pIt));
			}
	glEnd();
	
	/* Label all points: */
	Vrui::goToPhysicalSpace(contextData);
	GLNumberRenderer::Vector labelOffset(0.0f,GLfloat(Vrui::getUiSize()),0.0f);
	for(PointSetList::const_iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
		if(psIt->draw)
			{
			glColor(psIt->color);
			unsigned int index=0;
			for(PointList::const_iterator pIt=psIt->points.begin();pIt!=psIt->points.end();++pIt,++index)
				{
				glPushMatrix();
				glMultMatrix(Vrui::getUiManager()->calcHUDTransform(Vrui::getNavigationTransformation().transform(psIt->transform.transform(*pIt))));
				numberRenderer.drawNumber(labelOffset,index+1,contextData,0,-1);
				glPopMatrix();
				}
			}
	glPopMatrix();
	
	}
	
	/* Restore OpenGL state: */
	glPopAttrib();
	}

void MeasurePoints::resetNavigation(void)
	{
	Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
	
	if(calibrator!=0)
		{
		if(calibrator->haveSamples())
			{
			/* Center around the calibration position: */
			Vrui::setNavigationTransformation(calibrator->calcCalibCenter(),trackingUnit.getMeterFactor()*0.5);
			}
		}
	else
		{
		/* Calculate the bounding box of all measured points: */
		Geometry::Box<float,3> bbox=Geometry::Box<float,3>::empty;
		for(PointSetList::iterator psIt=pointSets.begin();psIt!=pointSets.end();++psIt)
			if(psIt->draw)
				{
				for(PointList::iterator pIt=psIt->points.begin();pIt!=psIt->points.end();++pIt)
					bbox.addPoint(psIt->transform.transform(*pIt));
				}
		
		/* Check if the box is empty: */
		if(bbox.isNull())
			{
			if(triggerButtonIndex>=0)
				{
				/* Center around the current trigger button device position: */
				Vrui::setNavigationTransformation(trackerFrame.getLockedValue().transform(probeTip),trackingUnit.getMeterFactor());
				}
			}
		else
			{
			/* Show the measured points: */
			Point mid=Geometry::mid(bbox.min,bbox.max);
			Scalar dist=Geometry::dist(mid,bbox.min);
			Vrui::setNavigationTransformation(mid,dist*Scalar(2.0));
			}
		}
	}

void MeasurePoints::eventCallback(Vrui::Application::EventID eventId,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	if(cbData->newButtonState)
		{
		switch(eventId)
			{
			case 0:
				{
				Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
				startNewPointSet();
				break;
				}
			
			case 1:
				{
				Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
				pointSets.pop_back();
				if(pointSets.empty())
					startNewPointSet();
				else if(activePointSet>=pointSets.size())
					activePointSet=pointSets.size()-1;
				break;
				}
			
			case 2:
				{
				Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
				if(!pointSets[activePointSet].points.empty())
					pointSets[activePointSet].points.pop_back();
				break;
				}
			
			case 3:
				{
				Threads::Mutex::Lock pointSetsLock(pointSetsMutex);
				if(calibrator!=0)
					{
					/* Finish the current probe tip calibration: */
					probeTip=calibrator->calcProbeTip();
					delete calibrator;
					calibrator=0;
					}
				else
					{
					/* Create a new probe tip calibrator: */
					calibrator=new ProbeTipCalibrator(1000,0.005,0.75);
					}
				break;
				}
			}
		}
	}

/*************
Main function:
*************/

VRUI_APPLICATION_RUN(MeasurePoints)
