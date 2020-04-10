/***********************************************************************
PolygonTriangulator - Class to split a simple (non-self-intersecting)
non-convex polygon in the 2D plane into a set of triangles covering its
interior.
Copyright (c) 2018-2020 Oliver Kreylos

This file is part of the Templatized Geometry Library (TGL).

The Templatized Geometry Library is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Templatized Geometry Library is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Templatized Geometry Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef GEOMETRY_POLYGONTRIANGULATOR_INCLUDED
#define GEOMETRY_POLYGONTRIANGULATOR_INCLUDED

#include <vector>
#include <Misc/PriorityHeap.h>
#include <Misc/RedBlackTree.h>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace Geometry {

template <class ScalarParam>
class PolygonTriangulator
	{
	/* Embedded classes: */
	public:
	typedef ScalarParam Scalar; // Scalar type
	typedef Geometry::Point<Scalar,2> Point; // Type for 2D points
	typedef Geometry::Vector<Scalar,2> Vector; // Type for 2D vectors
	typedef unsigned int Index; // Type for polygon vertex indices
	typedef std::vector<Index> IndexList; // Type for lists of polygon vertex indices
	
	enum Error // Enumerated type for errors that can occur when triangulating a polygon
		{
		HoleInPolygon, // The polygon is not a closed loop of edges
		SelfIntersection // Two polygon edges intersect each other
		};
	
	private:
	struct Vertex:public Point // Structure for polygon vertices carrying indices
		{
		/* Elements: */
		public:
		Index i; // Vertex index
		
		/* Constructors and destructors: */
		Vertex(const Point& sPos,Index sI)
			:Point(sPos),i(sI)
			{
			}
		};
	
	struct Edge // Structure for undirected edges; are created such that the first vertex is below the second vertex
		{
		/* Elements: */
		public:
		Vertex v0,v1; // The edge's vertices, with v0[1]<v1[1]
		
		/* Constructors and destructors: */
		Edge(const Vertex& sV0,const Vertex& sV1) // Creates edge from two points, assumed to already be sorted
			:v0(sV0),v1(sV1)
			{
			}
		
		/* Methods: */
		Scalar calcIntercept(Scalar y) const; // Calculates the x coordinate of the edge's intersection with the given horizontal line, assumed to be inside the edge's y interval
		};
	
	struct EdgeEvent; // Structure representing an edge event, i.e., an edge starting, an edge ending, or two edges intersecting
	typedef Misc::PriorityHeap<EdgeEvent,EdgeEvent> EdgeEventPriorityList; // Type for priority lists of edge events
	struct PolygonVertex; // Structure for vertices defining the untriangulated polygon of an active interval
	struct ActiveEdge; // Structure representing a currently active edge
	class ActiveEdgeComp; // Comparison functor that compares edges based on their intersections with an imaginary sweep line
	typedef Misc::RedBlackTree<ActiveEdge,ActiveEdgeComp> ActiveEdgeList; // Type for sorted lists of active edges, implemented as a binary search tree
	typedef Misc::HashTable<const Edge*,typename ActiveEdgeList::iterator> ActiveEdgeMap; // Type for hash tables mapping currently active edges to their active edge representations
	
	/* Elements: */
	std::vector<Edge> edges; // List of edges defining the polygon to be triangulated
	
	/* Constructors and destructors: */
	public:
	PolygonTriangulator(void); // Creates an empty polygon triangulator
	
	/* Methods: */
	bool empty(void) const // Returns true if the polygon triangulator does not contain any edges
		{
		return edges.empty();
		}
	void addEdge(const Point& pos0,Index i0,const Point& pos1,Index i1) // Adds an edge to the polygon to be triangulated
		{
		/* Sort the edge by y coordinate and ignore it if it is horizontal: */
		if(pos0[1]<pos1[1])
			edges.push_back(Edge(Vertex(pos0,i0),Vertex(pos1,i1)));
		else if(pos1[1]<pos0[1])
			edges.push_back(Edge(Vertex(pos1,i1),Vertex(pos0,i0)));
		}
	void triangulate(IndexList& triangleVertexIndices) const; // Emits a series of triangles covering the interior of the polygon defined by the current list of edges as a list of vertex index triples into the given vector
	};

#if defined(GEOMETRY_NONSTANDARD_TEMPLATES) && !defined(GEOMETRY_POLYGONTRIANGULATOR_IMPLEMENTATION)
#include <Geometry/PolygonTriangulator.icpp>
#endif

}

#endif
