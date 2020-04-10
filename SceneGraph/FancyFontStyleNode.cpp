/***********************************************************************
FancyFontStyleNode - Class for nodes defining the appearance and layout
of fancy 3D text, rendered as solid polyhedral characters using high-
quality outline fonts.
Copyright (c) 2020 Oliver Kreylos

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

#include <SceneGraph/FancyFontStyleNode.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include <Misc/UTF8.h>
#include <Misc/ThrowStdErr.h>
#include <Math/Math.h>
#include <Math/Constants.h>
#include <Geometry/PolygonTriangulator.h>
#include <GL/gl.h>
#include <GL/Extensions/GLARBVertexBufferObject.h>
#include <SceneGraph/Config.h>
#include <SceneGraph/VRMLFile.h>

namespace SceneGraph {

namespace {

/**************
Helper classes:
**************/

template <class ScalarParam,int dimensionParam>
class ComponentArrayHasher // Class to hash component arrays
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar;
	static const int dimension=dimensionParam;
	typedef Geometry::ComponentArray<ScalarParam,dimensionParam> ComponentArray;
	
	/* Methods: */
	static size_t hash(const ComponentArray& value,size_t tableSize)
		{
		union Converter // Union to convert a floating-point value into a size_t
			{
			/* Elements: */
			public:
			Scalar scalar;
			size_t index;
			} converter;
		
		converter.index=0;
		converter.scalar=value[0];
		size_t rawHash=converter.index;
		for(int i=1;i<dimension;++i)
			{
			converter.index=0;
			converter.scalar=value[i];
			rawHash=rawHash*17+converter.index;
			}
		
		return rawHash%tableSize;
		}
	};

}

/*****************************************************
Declaration of class FancyFontStyleNode::GlyphCreator:
*****************************************************/

class FancyFontStyleNode::GlyphCreator
	{
	/* Embedded classes: */
	public:
	typedef Geometry::PolygonTriangulator<GScalar> Triangulator;
	typedef Misc::HashTable<GPoint,Index,ComponentArrayHasher<GScalar,2> > FaceVertexIndexMap;
	
	/* Elements: */
	private:
	FT_Face ftFace; // Handle to the FreeType font face structure
	GScalar scale; // Scale factor from FreeType units to glyph units
	Index nextFaceIndex; // Index to assign to next created face vertex
	FaceVertexIndexMap faceVertexIndexMap; // Map from vertex positions to triangulator vertex indices
	FaceVertexList& faceVertices; // List of vertices defining the glyph's face
	Triangulator triangulator; // Helper object to triangulate the glyph's face
	IndexList& triangles; // List of triangles as vertex index triplets defining the glyph's face
	
	Index nextEdgeIndex; // Index to assign to next created edge vertex
	EdgeVertexList& edgeVertices; // List of vertices defining the glyph's outline
	IndexList& edges; // List of edges as vertex index pairs defining the glyph's outline
	
	GPoint pen; // Current pen position
	Index penFaceIndex; // Triangulator vertex index assigned to current pen position
	FT_Outline_Funcs ftOutlineFuncs; // Function table for FreeType outline processor
	GScalar epsilon2; // Approximation threshold for curve discretization
	
	/* Private methods: */
	Index getFaceIndex(const GPoint& vertex) // Returns a triangulator vertex index for the given vertex
		{
		#if 0
		/* Find the vertex in the vertex index map: */
		FaceVertexIndexMap::Iterator vIt=faceVertexIndexMap.findEntry(vertex);
		if(vIt.isFinished())
			{
			/* Create a new vertex and index: */
			Index result=nextFaceIndex;
			++nextFaceIndex;
			faceVertexIndexMap.setEntry(FaceVertexIndexMap::Entry(vertex,result));
			
			/* Store the scaled vertex: */
			faceVertices.push_back(GPoint(vertex[0]*scale,vertex[1]*scale));
			
			return result;
			}
		else
			{
			/* Return the found face vertex index: */
			return vIt->getDest();
			}
		#else
		/* Create a new vertex and index: */
		Index result=nextFaceIndex;
		++nextFaceIndex;
		
		/* Store the scaled vertex: */
		faceVertices.push_back(GPoint(vertex[0]*scale,vertex[1]*scale));
		
		return result;
		#endif
		}
	Index getEdgeIndex(const GVector& normal,const GPoint& point)
		{
		/* Create a new vertex and index: */
		Index result=nextEdgeIndex;
		++nextEdgeIndex;
		
		/* Store the scaled vertex: */
		edgeVertices.push_back(EdgeVertex(normal,GPoint(point[0]*scale,point[1]*scale)));
		
		return result;
		}
	static int moveToFunc(const FT_Vector* to,void* user); // Function to move the pen; called from FreeType library
	static int lineToFunc(const FT_Vector* to,void* user); // Function to draw a line; called from FreeType library
	void drawQuadratic(const GPoint& c0,const GPoint& p1);
	static int conicToFunc(const FT_Vector* control,const FT_Vector* to,void* user); // Function to draw a quadratic Bezier curve; called from FreeType library
	void drawCubic(const GPoint& c0,const GPoint& c1,const GPoint& p1);
	static int cubicToFunc(const FT_Vector* control1,const FT_Vector* control2,const FT_Vector* to,void* user); // Function to draw a cubic Bezier curve; called from FreeType library
	
	/* Constructors and destructors: */
	public:
	GlyphCreator(FT_Face sFtFace,GScalar sScale,FaceVertexList& sFaceVertices,IndexList& sTriangles,EdgeVertexList& sEdgeVertices,IndexList& sEdges,GScalar sEpsilon);
	
	/* Methods: */
	Glyph createGlyph(unsigned int glyphIndex);
	};

/*************************************************
Methods of class FancyFontStyleNode::GlyphCreator:
*************************************************/

int FancyFontStyleNode::GlyphCreator::moveToFunc(const FT_Vector* to,void* user)
	{
	GlyphCreator* thisPtr=static_cast<GlyphCreator*>(user);
	
	/* Set the current pen position: */
	thisPtr->pen[0]=GScalar(to->x);
	thisPtr->pen[1]=GScalar(to->y);
	
	/* Get a triangulator vertex index for the new pen position: */
	thisPtr->penFaceIndex=thisPtr->getFaceIndex(thisPtr->pen);
	
	return 0;
	}

int FancyFontStyleNode::GlyphCreator::lineToFunc(const FT_Vector* to,void* user)
	{
	GlyphCreator* thisPtr=static_cast<GlyphCreator*>(user);
	
	/* Get a triangulator vertex index for the new pen position: */
	GPoint newPen(GScalar(to->x),GScalar(to->y));
	Index newPenFaceIndex=thisPtr->getFaceIndex(newPen);
	
	/* Add the line to the triangulator: */
	thisPtr->triangulator.addEdge(thisPtr->pen,thisPtr->penFaceIndex,newPen,newPenFaceIndex);
	
	/* Add the line to the glyph's outline: */
	GVector normal(thisPtr->pen[1]-newPen[1],newPen[0]-thisPtr->pen[0]);
	normal.normalize();
	thisPtr->edges.push_back(thisPtr->getEdgeIndex(normal,thisPtr->pen));
	thisPtr->edges.push_back(thisPtr->getEdgeIndex(normal,newPen));
	
	/* Move the pen: */
	thisPtr->pen=newPen;
	thisPtr->penFaceIndex=newPenFaceIndex;
	
	return 0;
	}

void FancyFontStyleNode::GlyphCreator::drawQuadratic(const FancyFontStyleNode::GPoint& c0,const FancyFontStyleNode::GPoint& p1)
	{
	/* Check if the quadratic Bezier curve is flat enough: */
	GVector d=p1-pen;
	GScalar dLen2=d.sqr();
	GVector n=Geometry::normal(d);
	GVector cp0=c0-pen;
	GScalar x0=cp0*d;
	
	/* Check if the curve can be approximated with a line segment: */
	if(x0>=GScalar(0)&&x0<=dLen2&&Math::sqr(cp0*n)<=epsilon2*dLen2)
		{
		/* Get a triangulator vertex index for the new pen position: */
		Index newPenFaceIndex=getFaceIndex(p1);
		
		/* Add the line segment to the triangulator: */
		triangulator.addEdge(pen,penFaceIndex,p1,newPenFaceIndex);
		
		/* Add the line segment to the glyph's outline: */
		GVector normal(c0[1]-p1[1],p1[0]-c0[0]);
		normal.normalize();
		edges.push_back(nextEdgeIndex-1); // Add the previous curve vertex's index
		edges.push_back(getEdgeIndex(normal,p1));
		
		/* Move the pen: */
		pen=p1;
		penFaceIndex=newPenFaceIndex;
		}
	else
		{
		/* Subdivide the curve: */
		GPoint i0=Geometry::mid(pen,c0);
		GPoint i2=Geometry::mid(c0,p1);
		GPoint i1=Geometry::mid(i0,i2);
		drawQuadratic(i0,i1);
		drawQuadratic(i2,p1);
		}
	}

int FancyFontStyleNode::GlyphCreator::conicToFunc(const FT_Vector* control,const FT_Vector* to,void* user)
	{
	GlyphCreator* thisPtr=static_cast<GlyphCreator*>(user);
	
	/* Draw a quadratic Bezier curve (not actually a conic, duh): */
	GPoint c0(GScalar(control->x),GScalar(control->y));
	GPoint p1(GScalar(to->x),GScalar(to->y));
	
	/* Store the initial vertex of the curve's outline: */
	GVector normal(thisPtr->pen[1]-c0[1],c0[0]-thisPtr->pen[0]);
	normal.normalize();
	thisPtr->getEdgeIndex(normal,thisPtr->pen);
	
	/* Subdivide the curve into flat segments: */
	thisPtr->drawQuadratic(c0,p1);
	
	return 0;
	}

void FancyFontStyleNode::GlyphCreator::drawCubic(const FancyFontStyleNode::GPoint& c0,const FancyFontStyleNode::GPoint& c1,const FancyFontStyleNode::GPoint& p1)
	{
	/* Check if the cubic Bezier curve is flat enough: */
	GVector d=p1-pen;
	GScalar dLen2=d.sqr();
	GVector n=Geometry::normal(d);
	GVector cp0=c0-pen;
	GScalar x0=cp0*d;
	GVector cp1=c1-pen;
	GScalar x1=cp1*d;
	
	/* Check if the curve can be approximated with a line segment: */
	if(x0>=GScalar(0)&&x0<=dLen2&&x1>=GScalar(0)&&x1<=dLen2&&Math::sqr(cp0*n)<=epsilon2*dLen2&&Math::sqr(cp1*n)<=epsilon2*dLen2)
		{
		/* Get a triangulator vertex index for the new pen position: */
		Index newPenFaceIndex=getFaceIndex(p1);
		
		/* Add the line segment to the triangulator: */
		triangulator.addEdge(pen,penFaceIndex,p1,newPenFaceIndex);
		
		/* Add the line segment to the glyph's outline: */
		GVector normal(c1[1]-p1[1],p1[0]-c1[0]);
		normal.normalize();
		edges.push_back(nextEdgeIndex-1); // Add the previous curve vertex's index
		edges.push_back(getEdgeIndex(normal,p1));
		
		/* Move the pen: */
		pen=p1;
		penFaceIndex=newPenFaceIndex;
		}
	else
		{
		/* Subdivide the curve: */
		GPoint i0=Geometry::mid(pen,c0);
		GPoint m=Geometry::mid(c0,c1);
		GPoint i4=Geometry::mid(c1,p1);
		GPoint i1=Geometry::mid(i0,m);
		GPoint i3=Geometry::mid(m,i4);
		GPoint i2=Geometry::mid(i1,i3);
		drawCubic(i0,i1,i2);
		drawCubic(i3,i4,p1);
		}
	}

int FancyFontStyleNode::GlyphCreator::cubicToFunc(const FT_Vector* control1,const FT_Vector* control2,const FT_Vector* to,void* user)
	{
	GlyphCreator* thisPtr=static_cast<GlyphCreator*>(user);
	
	/* Draw a cubic Bezier curve: */
	GPoint c0(GScalar(control1->x),GScalar(control1->y));
	GPoint c1(GScalar(control2->x),GScalar(control2->y));
	GPoint p1(GScalar(to->x),GScalar(to->y));
	
	/* Store the initial vertex of the curve's outline: */
	GVector normal(thisPtr->pen[1]-c0[1],c0[0]-thisPtr->pen[0]);
	normal.normalize();
	thisPtr->getEdgeIndex(normal,thisPtr->pen);
	
	/* Subdivide the curve into flat segments: */
	thisPtr->drawCubic(c0,c1,p1);
	
	return 0;
	}

FancyFontStyleNode::GlyphCreator::GlyphCreator(FT_Face sFtFace,FancyFontStyleNode::GScalar sScale,FancyFontStyleNode::FaceVertexList& sFaceVertices,FancyFontStyleNode::IndexList& sTriangles,FancyFontStyleNode::EdgeVertexList& sEdgeVertices,FancyFontStyleNode::IndexList& sEdges,FancyFontStyleNode::GScalar sEpsilon)
	:ftFace(sFtFace),scale(sScale),
	 nextFaceIndex(0),faceVertexIndexMap(17),faceVertices(sFaceVertices),triangles(sTriangles),
	 nextEdgeIndex(0),edgeVertices(sEdgeVertices),edges(sEdges),
	 penFaceIndex(-1),epsilon2(Math::sqr(sEpsilon))
	{
	/* Initialize the outline function table: */
	ftOutlineFuncs.move_to=moveToFunc;
	ftOutlineFuncs.line_to=lineToFunc;
	ftOutlineFuncs.conic_to=conicToFunc;
	ftOutlineFuncs.cubic_to=cubicToFunc;
	ftOutlineFuncs.shift=0;
	ftOutlineFuncs.delta=0;
	}

FancyFontStyleNode::Glyph FancyFontStyleNode::GlyphCreator::createGlyph(unsigned int glyphIndex)
	{
	Glyph result;
	
	/* Access the character's glyph: */
	int ftError=FT_Load_Glyph(ftFace,glyphIndex,FT_LOAD_NO_HINTING|FT_LOAD_NO_BITMAP);
	if(ftError!=FT_Err_Ok)
		Misc::throwStdErr("FancyFontStyleNode::GlyphCreator::createGlyph: Unable to load glyph %u due to error %d",glyphIndex,ftError);
	
	/* Check if the glyph is an outline glyph: */
	FT_GlyphSlot slot=ftFace->glyph;
	if(slot->format!=FT_GLYPH_FORMAT_OUTLINE)
		Misc::throwStdErr("FancyFontStyleNode::GlyphCreator::createGlyph: Glyph %u is not an outline glyph",glyphIndex);
	
	/* Retrieve the glyph's bounding box: */
	FT_BBox glyphBox;
	FT_Outline_Get_BBox(&slot->outline,&glyphBox);
	result.box.min[0]=GScalar(glyphBox.xMin)*scale;
	result.box.min[1]=GScalar(glyphBox.yMin)*scale;
	result.box.max[0]=GScalar(glyphBox.xMax)*scale;
	result.box.max[1]=GScalar(glyphBox.yMax)*scale;
	
	/* Retrieve the glyph's advance vector: */
	result.advance[0]=GScalar(slot->advance.x)*scale;
	result.advance[1]=GScalar(slot->advance.y)*scale;
	
	/* Initialize the glyph's arrays: */
	result.firstFaceVertex=faceVertices.size();
	result.firstTriangle=triangles.size();
	result.firstEdgeVertex=edgeVertices.size();
	result.firstEdge=edges.size();
	
	/* Outline the glyph: */
	FT_Outline_Decompose(&slot->outline,&ftOutlineFuncs,this);
	
	/* Check if the glyph's outline contained any edges: */
	if(triangulator.empty())
		{
		/* Finalize the glyph's arrays: */
		result.numFaceVertices=faceVertices.size()-result.firstFaceVertex; // There might have been vertices even if there were no edges
		result.numTriangles=0;
		result.numEdgeVertices=0;
		result.numEdges=0;
		}
	else
		{
		/* Triangulate the text's interior polygon: */
		triangulator.triangulate(triangles);
		
		/* Finalize the glyph's arrays: */
		result.numFaceVertices=faceVertices.size()-result.firstFaceVertex;
		result.numTriangles=triangles.size()-result.firstTriangle;
		result.numEdgeVertices=edgeVertices.size()-result.firstEdgeVertex;
		result.numEdges=edges.size()-result.firstEdge;
		}
	
	return result;
	}

/*******************************************
Static elements of class FancyFontStyleNode:
*******************************************/

Threads::Mutex FancyFontStyleNode::ftLibraryMutex;
FT_Library FancyFontStyleNode::ftLibrary(0);
unsigned int FancyFontStyleNode::ftLibraryRefCount(0);

namespace {

/**************
Helper classes:
**************/

enum FontFamily // Enumerated type for font families
	{
	SERIF=0,SANS=1,TYPEWRITER=2
	};

enum FontStyle // Enumerated type for font styles
	{
	PLAIN=0,BOLD=1,ITALIC=2,BOLDITALIC=3
	};

/* Font file names for the possible combinations of families and styles: */
static const char* fontFileNames[3*4]=
	{
	SCENEGRAPH_CONFIG_FONT_SERIF_PLAIN,SCENEGRAPH_CONFIG_FONT_SERIF_BOLD,SCENEGRAPH_CONFIG_FONT_SERIF_ITALIC,SCENEGRAPH_CONFIG_FONT_SERIF_BOLDITALIC,
	SCENEGRAPH_CONFIG_FONT_SANS_PLAIN,SCENEGRAPH_CONFIG_FONT_SANS_BOLD,SCENEGRAPH_CONFIG_FONT_SANS_ITALIC,SCENEGRAPH_CONFIG_FONT_SANS_BOLDITALIC,
	SCENEGRAPH_CONFIG_FONT_TYPEWRITER_PLAIN,SCENEGRAPH_CONFIG_FONT_TYPEWRITER_BOLD,SCENEGRAPH_CONFIG_FONT_TYPEWRITER_ITALIC,SCENEGRAPH_CONFIG_FONT_TYPEWRITER_BOLDITALIC
	};

}

/***********************************
Methods of class FancyFontStyleNode:
***********************************/

FancyFontStyleNode::FancyFontStyleNode(void)
	:family("SERIF"),
	 style("PLAIN"),
	 language(""),
	 size(1),
	 spacing(1),
	 horizontal(true),
	 leftToRight(true),
	 topToBottom(true),
	 precision(1),
	 ftFace(0),scale(0.003125),epsilon(10.0f*precision.getValue()),
	 glyphMap(17)
	{
	/* Initialize the shared FreeType library object if there are no references yet, and acquire a reference: */
	{
	Threads::Mutex::Lock ftLibraryLock(ftLibraryMutex);
	if(ftLibraryRefCount==0)
		{
		/* Initialize the shared FreeType library object: */
		int error=FT_Init_FreeType(&ftLibrary);
		if(error!=FT_Err_Ok)
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode: Unable to initialize FreeType library due to error code %d",error);
		}
	++ftLibraryRefCount;
	}
	}

FancyFontStyleNode::~FancyFontStyleNode(void)
	{
	Threads::Mutex::Lock ftLibraryLock(ftLibraryMutex);
	
	/* Release the font face: */
	if(ftFace!=0)
		FT_Done_Face(ftFace);
	
	/* Release the reference to the shared FreeType library object, and destroy it if there are no references left: */
	--ftLibraryRefCount;
	if(ftLibraryRefCount==0)
		{
		/* Destroy the shared FreeType library object: */
		FT_Done_FreeType(ftLibrary);
		ftLibrary=0;
		}
	}

const char* FancyFontStyleNode::getStaticClassName(void)
	{
	return "FancyFontStyle";
	}

const char* FancyFontStyleNode::getClassName(void) const
	{
	return "FancyFontStyle";
	}

void FancyFontStyleNode::parseField(const char* fieldName,VRMLFile& vrmlFile)
	{
	if(strcmp(fieldName,"url")==0)
		vrmlFile.parseField(url);
	else if(strcmp(fieldName,"family")==0)
		vrmlFile.parseField(family);
	else if(strcmp(fieldName,"style")==0)
		vrmlFile.parseField(style);
	else if(strcmp(fieldName,"language")==0)
		vrmlFile.parseField(language);
	else if(strcmp(fieldName,"size")==0)
		vrmlFile.parseField(size);
	else if(strcmp(fieldName,"spacing")==0)
		vrmlFile.parseField(spacing);
	else if(strcmp(fieldName,"justify")==0)
		vrmlFile.parseField(justify);
	else if(strcmp(fieldName,"horizontal")==0)
		vrmlFile.parseField(horizontal);
	else if(strcmp(fieldName,"leftToRight")==0)
		vrmlFile.parseField(leftToRight);
	else if(strcmp(fieldName,"topToBottom")==0)
		vrmlFile.parseField(topToBottom);
	else if(strcmp(fieldName,"precision")==0)
		vrmlFile.parseField(precision);
	else
		Node::parseField(fieldName,vrmlFile);
	}

void FancyFontStyleNode::update(void)
	{
	/* Determine the name of the font file to load: */
	bool clearGlyphCache=false;
	std::string newFontFileName;
	if(!url.getValues().empty())
		{
		/* Load the font file of the given URL: */
		newFontFileName=url.getValue(0);
		}
	else
		{
		/* Extract the font family index: */
		FontFamily fontFamily=SERIF;
		if(family.getValues().empty())
			throw std::runtime_error("SceneGraph::FancyFontStyleNode::update: No font family defined");
		else if(family.getValue(0)=="SANS")
			fontFamily=SANS;
		else if(family.getValue(0)=="TYPEWRITER")
			fontFamily=TYPEWRITER;
		else if(family.getValue(0)!="SERIF")
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Invalid font family %s",family.getValue(0).c_str());
		
		/* Extract the font style index: */
		FontStyle fontStyle=PLAIN;
		if(style.getValue()=="BOLD")
			fontStyle=BOLD;
		else if(style.getValue()=="ITALIC")
			fontStyle=ITALIC;
		else if(style.getValue()=="BOLDITALIC")
			fontStyle=BOLDITALIC;
		else if(style.getValue()!="PLAIN")
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Invalid font style %s",style.getValue().c_str());
		
		/* Construct a font file name from font family and style: */
		newFontFileName=SCENEGRAPH_CONFIG_FONTDIR;
		newFontFileName.append(fontFileNames[fontFamily*4+fontStyle]);
		}
	
	/* Load the first font face in the requested font file: */
	if(fontFileName.empty()||fontFileName!=newFontFileName)
		{
		int ftError;
		{
		Threads::Mutex::Lock ftLibraryLock(ftLibraryMutex);
		if(ftFace!=0)
			FT_Done_Face(ftFace);
		ftError=FT_New_Face(ftLibrary,newFontFileName.c_str(),0,&ftFace);
		}
		if(ftError==FT_Err_Unknown_File_Format)
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Could not load font face from file %s due to unknown file format",newFontFileName.c_str());
		else if(ftError!=FT_Err_Ok)
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Could not load font face from file %s due to error %d",newFontFileName.c_str(),ftError);
		
		/* Set the font file name and clear the glyph cache: */
		fontFileName=newFontFileName;
		clearGlyphCache=true;
		}
	
	/* Load the face at a fixed character size and adjust the scaling factor to get the desired character size: */
	int ftError=FT_Set_Char_Size(ftFace,0,640,300,300);
	if(ftError!=FT_Err_Ok)
		Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Could not set font size due to error %d",ftError);
	GScalar newScale=GScalar(size.getValue()*0.0003125);
	if(scale!=newScale)
		{
		/* Set the font scale and clear the glyph cache: */
		scale=newScale;
		clearGlyphCache=true;
		}
	
	/* Calculate the glyph subdivision threshold: */
	GScalar newEpsilon=GScalar(10.0f*precision.getValue());
	if(epsilon!=newEpsilon)
		{
		/* Set the glyph subdivision threshold and clear the glyph cache: */
		epsilon=newEpsilon;
		clearGlyphCache=true;
		}
	
	/* Clear the glyph cache if font outlines changed: */
	if(clearGlyphCache)
		{
		faceVertices.clear();
		triangles.clear();
		edgeVertices.clear();
		edges.clear();
		glyphMap.clear();
		}
	
	/* Parse the string justifications: */
	justifications[0]=BEGIN;
	if(justify.getNumValues()>=1)
		{
		if(justify.getValue(0)=="FIRST")
			justifications[0]=FIRST;
		else if(justify.getValue(0)=="MIDDLE")
			justifications[0]=MIDDLE;
		else if(justify.getValue(0)=="END")
			justifications[0]=END;
		else if(justify.getValue(0)!="BEGIN")
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Invalid justification %s",justify.getValue(0).c_str());
		}
	justifications[1]=FIRST;
	if(justify.getNumValues()>=2)
		{
		if(justify.getValue(1)=="BEGIN")
			justifications[1]=BEGIN;
		else if(justify.getValue(1)=="MIDDLE")
			justifications[1]=MIDDLE;
		else if(justify.getValue(1)=="END")
			justifications[1]=END;
		else if(justify.getValue(1)!="FIRST")
			Misc::throwStdErr("SceneGraph::FancyFontStyleNode::update: Invalid justification %s",justify.getValue(1).c_str());
		}
	}

FancyFontStyleNode::GBox FancyFontStyleNode::prepareStrings(const MFString& strings,bool front,bool outline,bool back,size_t& numVertices,size_t& numIndices)
	{
	/* Process the string list once to create all required character glyphs: */
	for(MFString::ValueList::const_iterator ssIt=strings.getValues().begin();ssIt!=strings.getValues().end();++ssIt)
		{
		/* Create the string's glyphs: */
		std::string::const_iterator sIt=ssIt->begin();
		while(sIt!=ssIt->end())
			{
			/* Get the next Unicode character index, assuming the string is UTF-8 encoded: */
			unsigned int characterCode=Misc::UTF8::decodeNoCheck(sIt,ssIt->end());
			
			/* Retrieve the Unicode character's glyph index in the font face: */
			unsigned int glyphIndex=FT_Get_Char_Index(ftFace,characterCode);
			
			/* Check if the character's glyph is not yet in the cache: */
			if(!glyphMap.isEntry(glyphIndex))
				{
				/* Add the character's outline and face triangulation to the cache: */
				GlyphCreator glyphCreator(ftFace,scale,faceVertices,triangles,edgeVertices,edges,epsilon);
				glyphMap.setEntry(GlyphMap::Entry(glyphIndex,glyphCreator.createGlyph(glyphIndex)));
				}
			}
		}
	
	/* Process the string list again to calculate its bounding box and count the total number of vertices and triangle vertex indices required to render them: */
	GScalar lineSpacing=GScalar(ftFace->size->metrics.height*spacing.getValue())*scale;
	GBox result=GBox::empty;
	GVector offset=GVector::zero;
	size_t numFaceVertices=0;
	size_t numTriangles=0;
	size_t numEdgeVertices=0;
	size_t numEdges=0;
	for(MFString::ValueList::const_iterator ssIt=strings.getValues().begin();ssIt!=strings.getValues().end();++ssIt)
		{
		/* Calculate the strings' bounding box and count their vertices and triangle indices: */
		unsigned int prevGlyphIndex=~0x0U;
		std::string::const_iterator sIt=ssIt->begin();
		while(sIt!=ssIt->end())
			{
			/* Get the next Unicode character index, assuming the string is UTF-8 encoded: */
			unsigned int characterCode=Misc::UTF8::decodeNoCheck(sIt,ssIt->end());
			
			/* Retrieve the Unicode character's glyph index in the font face: */
			unsigned int glyphIndex=FT_Get_Char_Index(ftFace,characterCode);
			
			/* Retrieve the character's glyph: */
			const Glyph& glyph=glyphMap.getEntry(glyphIndex).getDest();
			
			if(prevGlyphIndex!=~0x0U)
				{
				/* Check for kerning with the previous character: */
				FT_Vector kerning;
				if(FT_Get_Kerning(ftFace,prevGlyphIndex,glyphIndex,FT_KERNING_UNFITTED,&kerning)==FT_Err_Ok)
					{
					offset[0]+=GScalar(kerning.x)*scale;
					offset[1]+=GScalar(kerning.y)*scale;
					}
				}
			
			/* Add the glyph's bounding box to the strings' bounding box: */
			GBox glyphBox=glyph.box;
			result.addBox(glyphBox.shift(offset));
			
			/* Count the glyph's vertices and indices: */
			numFaceVertices+=glyph.numFaceVertices;
			numTriangles+=glyph.numTriangles;
			numEdgeVertices+=glyph.numEdgeVertices;
			numEdges+=glyph.numEdges;
			
			/* Advance the pen position: */
			offset+=glyph.advance;
			
			prevGlyphIndex=glyphIndex;
			}
		
		/* Go to the next line: */
		offset[0]=GScalar(0);
		offset[1]-=lineSpacing;
		}
	
	/* Calculate the final number of required vertices and indices: */
	numVertices=0;
	numIndices=0;
	if(front)
		{
		numVertices+=numFaceVertices;
		numIndices+=numTriangles;
		}
	if(outline)
		{
		numVertices+=numEdgeVertices*2;
		numIndices+=numEdges*3;
		}
	if(back)
		{
		numVertices+=numFaceVertices;
		numIndices+=numTriangles;
		}
	
	/* Adjust the bounding box based on the selected justification: */
	GScalar width=result.max[0]-result.min[0];
	GScalar height=result.max[1]-result.min[1];
	switch(justifications[0])
		{
		case FIRST:
		case BEGIN:
			result.min[0]=GScalar(0);
			result.max[0]=width;
			break;
		
		case MIDDLE:
			result.min[0]=-Math::div2(width);
			result.max[0]=Math::div2(width);
			break;
		
		case END:
			result.min[0]=-width;
			result.max[0]=GScalar(0);
			break;
		}
	switch(justifications[1])
		{
		case FIRST:
			/* Do nothing */
			break;
		
		case BEGIN:
			result.min[1]=-height;
			result.max[1]=GScalar(0);
			break;
		
		case MIDDLE:
			result.min[1]=-Math::div2(height);
			result.max[1]=Math::div2(height);
			break;
		
		case END:
			result.min[1]=GScalar(0);
			result.max[1]=height;
			break;
		}
	
	return result;
	}

void FancyFontStyleNode::uploadStrings(const MFString& strings,Scalar depth,bool front,bool outline,bool back) const
	{
	/* Process the string list once to calculate all strings' bounding boxes and align them: */
	std::vector<GBox> boxes;
	boxes.reserve(strings.getNumValues());
	GScalar lineSpacing=GScalar(ftFace->size->metrics.height*spacing.getValue())*scale;
	GBox bbox=GBox::empty;
	GVector offset=GVector::zero;
	for(MFString::ValueList::const_iterator ssIt=strings.getValues().begin();ssIt!=strings.getValues().end();++ssIt)
		{
		/* Calculate the string's bounding box: */
		GBox box=GBox::empty;
		unsigned int prevGlyphIndex=~0x0U;
		std::string::const_iterator sIt=ssIt->begin();
		while(sIt!=ssIt->end())
			{
			/* Get the next Unicode character index, assuming the string is UTF-8 encoded: */
			unsigned int characterCode=Misc::UTF8::decodeNoCheck(sIt,ssIt->end());
			
			/* Retrieve the Unicode character's glyph index in the font face: */
			unsigned int glyphIndex=FT_Get_Char_Index(ftFace,characterCode);
			
			/* Retrieve the character's glyph, which was created during prepareStrings(...): */
			const Glyph& glyph=glyphMap.getEntry(glyphIndex).getDest();
			
			if(prevGlyphIndex!=~0x0U)
				{
				/* Check for kerning with the previous character: */
				FT_Vector kerning;
				if(FT_Get_Kerning(ftFace,prevGlyphIndex,glyphIndex,FT_KERNING_UNFITTED,&kerning)==FT_Err_Ok)
					{
					offset[0]+=GScalar(kerning.x)*scale;
					offset[1]+=GScalar(kerning.y)*scale;
					}
				}
			
			/* Add the glyph's bounding box to the string's bounding box: */
			GBox glyphBox=glyph.box;
			box.addBox(glyphBox.shift(offset));
			
			/* Advance the pen position: */
			offset+=glyph.advance;
			
			prevGlyphIndex=glyphIndex;
			}
		
		/* Store the box: */
		boxes.push_back(box);
		bbox.addBox(box);
		
		/* Go to the next line: */
		offset[0]=GScalar(0);
		offset[1]-=lineSpacing;
		}
	
	/* Prepare the bound vertex and index buffers for geometry upload: */
	GLVertex* vPtr=static_cast<GLVertex*>(glMapBufferARB(GL_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	GLIndex* iPtr=static_cast<GLIndex*>(glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,GL_WRITE_ONLY_ARB));
	
	/* Position the first string's baseline based on the vertical justification: */
	switch(justifications[1])
		{
		case FIRST:
			offset[1]=GScalar(0);
			break;
		
		case BEGIN:
			offset[1]=-bbox.max[1];
			break;
		
		case MIDDLE:
			offset[1]=-Math::mid(bbox.min[1],bbox.max[1]);
			break;
		
		case END:
			offset[1]=-bbox.min[1];
			break;
		}
	
	/* Process the string list again to upload the strings' 3D geometry: */
	GLVertex::Position::Scalar z(Math::div2(depth));
	GLIndex baseVertexIndex=0;
	std::vector<GBox>::iterator bIt=boxes.begin();
	for(MFString::ValueList::const_iterator ssIt=strings.getValues().begin();ssIt!=strings.getValues().end();++ssIt,++bIt)
		{
		/* Position the start of the string based on the horizontal justification: */
		switch(justifications[0])
			{
			case FIRST:
			case BEGIN:
				offset[0]=-bIt->min[0];
				break;
			
			case MIDDLE:
				offset[0]=-Math::mid(bIt->min[0],bIt->max[0]);
				break;
			
			case END:
				offset[0]=-bIt->max[0];
				break;
			}
		
		/* Upload the string's 3D geometry: */
		unsigned int prevGlyphIndex=~0x0U;
		std::string::const_iterator sIt=ssIt->begin();
		while(sIt!=ssIt->end())
			{
			/* Get the next Unicode character index, assuming the string is UTF-8 encoded: */
			unsigned int characterCode=Misc::UTF8::decodeNoCheck(sIt,ssIt->end());
			
			/* Retrieve the Unicode character's glyph index in the font face: */
			unsigned int glyphIndex=FT_Get_Char_Index(ftFace,characterCode);
			
			/* Retrieve the character's glyph, which was created during prepareStrings(...): */
			const Glyph& glyph=glyphMap.getEntry(glyphIndex).getDest();
			
			if(prevGlyphIndex!=~0x0U)
				{
				/* Check for kerning with the previous character: */
				FT_Vector kerning;
				if(FT_Get_Kerning(ftFace,prevGlyphIndex,glyphIndex,FT_KERNING_UNFITTED,&kerning)==FT_Err_Ok)
					{
					offset[0]+=GScalar(kerning.x)*scale;
					offset[1]+=GScalar(kerning.y)*scale;
					}
				}
			
			/* Upload the glyph's front- and back-face 3D geometry: */
			const GPoint* gfvBegin=faceVertices.data()+glyph.firstFaceVertex;
			const GPoint* gfvEnd=gfvBegin+glyph.numFaceVertices;
			const Index* giBegin=triangles.data()+glyph.firstTriangle;
			const Index* giEnd=giBegin+glyph.numTriangles;
			
			if(front)
				{
				/* Upload front-face vertices: */
				for(const GPoint* gfvPtr=gfvBegin;gfvPtr!=gfvEnd;++gfvPtr,++vPtr)
					{
					vPtr->normal=GLVertex::Normal(0,0,1);
					for(int i=0;i<2;++i)
						vPtr->position[i]=GLVertex::Position::Scalar((*gfvPtr)[i]+offset[i]);
					vPtr->position[2]=z;
					}
				
				/* Upload front-face triangle indices: */
				for(const Index* giPtr=giBegin;giPtr!=giEnd;++giPtr,++iPtr)
					*iPtr=baseVertexIndex+GLIndex(*giPtr);
				
				baseVertexIndex+=glyph.numFaceVertices;
				}
			
			if(back)
				{
				/* Upload back-face vertices: */
				for(const GPoint* gfvPtr=gfvBegin;gfvPtr!=gfvEnd;++gfvPtr,++vPtr)
					{
					vPtr->normal=GLVertex::Normal(0,0,-1);
					for(int i=0;i<2;++i)
						vPtr->position[i]=GLVertex::Position::Scalar((*gfvPtr)[i]+offset[i]);
					vPtr->position[2]=-z;
					}
				
				/* Upload back-face triangle indices: */
				for(const Index* giPtr=giBegin;giPtr!=giEnd;giPtr+=3,iPtr+=3)
					{
					/* Flip the back-side triangle's vertex order: */
					iPtr[0]=baseVertexIndex+GLIndex(giPtr[0]);
					iPtr[1]=baseVertexIndex+GLIndex(giPtr[2]);
					iPtr[2]=baseVertexIndex+GLIndex(giPtr[1]);
					}
				
				baseVertexIndex+=glyph.numFaceVertices;
				}
			
			if(outline)
				{
				/* Upload the glyph's outline 3D geometry: */
				const EdgeVertex* gevBegin=edgeVertices.data()+glyph.firstEdgeVertex;
				const EdgeVertex* gevEnd=gevBegin+glyph.numEdgeVertices;
				giBegin=edges.data()+glyph.firstEdge;
				giEnd=giBegin+glyph.numEdges;
				
				/* Upload outline vertices: */
				for(const EdgeVertex* gevPtr=gevBegin;gevPtr!=gevEnd;++gevPtr,vPtr+=2)
					{
					/* Upload the front-face vertex: */
					vPtr[0].normal=GLVertex::Normal(gevPtr->normal[0],gevPtr->normal[1],0);
					for(int i=0;i<2;++i)
						vPtr[0].position[i]=GLVertex::Position::Scalar(gevPtr->position[i]+offset[i]);
					vPtr[0].position[2]=z;
					
					/* Upload the back-face vertex: */
					vPtr[1].normal=GLVertex::Normal(gevPtr->normal[0],gevPtr->normal[1],0);
					for(int i=0;i<2;++i)
						vPtr[1].position[i]=GLVertex::Position::Scalar(gevPtr->position[i]+offset[i]);
					vPtr[1].position[2]=-z;
					}
				
				/* Upload side triangle indices: */
				for(const Index* giPtr=giBegin;giPtr!=giEnd;giPtr+=2,iPtr+=6)
					{
					/* Generate two side triangles: */
					iPtr[0]=baseVertexIndex+GLIndex(giPtr[0]*2+1);
					iPtr[1]=baseVertexIndex+GLIndex(giPtr[0]*2+0);
					iPtr[2]=baseVertexIndex+GLIndex(giPtr[1]*2+0);
					iPtr[3]=baseVertexIndex+GLIndex(giPtr[1]*2+0);
					iPtr[4]=baseVertexIndex+GLIndex(giPtr[1]*2+1);
					iPtr[5]=baseVertexIndex+GLIndex(giPtr[0]*2+1);
					}
				
				baseVertexIndex+=glyph.numEdgeVertices*2;
				}
			
			/* Advance the pen position: */
			offset+=glyph.advance;
			
			prevGlyphIndex=glyphIndex;
			}
		
		/* Go to the next line: */
		offset[1]-=lineSpacing;
		}
	
	/* Finalize the bound vertex and index buffers: */
	glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
	glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	}

}
