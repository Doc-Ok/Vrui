/***********************************************************************
RedBlackTree - Self-balancing binary search tree using the red-black
property.
Copyright (c) 2018 Oliver Kreylos

This file is part of the Miscellaneous Support Library (Misc).

The Miscellaneous Support Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Miscellaneous Support Library is distributed in the hope that it
will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Miscellaneous Support Library; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef MISC_REDBLACKTREE_INCLUDED
#define MISC_REDBLACKTREE_INCLUDED

#include <utility>
#include <Misc/MallocAllocator.h>

namespace Misc {

template <class ContentParam>
class RBTreeStdCmp
	{
	/* Methods: */
	public:
	static bool lessEqual(const ContentParam& v1,const ContentParam& v2)
		{
		return v1<=v2;
		}
	};

template <class ContentParam,class ComparisonParam =RBTreeStdCmp<ContentParam>,template <class ItemParam> class AllocatorParam =MallocAllocator>
class RedBlackTree
	{
	/* Embedded classes: */
	public:
	typedef ContentParam Content; // Type of content stored in the tree
	typedef ComparisonParam Comparison; // Class providing a static <= comparison operator for tree contents
	
	private:
	struct Node // Structure representing a tree node
		{
		/* Elements: */
		public:
		bool black; // Flag if the node is black or red
		Node* parent; // Pointer to parent node; 0 for the root
		Node* left; // Pointer to the node's left child
		Node* right; // Pointer to the node's right child
		Content value; // Value stored in node
		
		/* Constructors and destructors: */
		public:
		Node(Node* sParent,const Content& sValue) // Creates a black leaf node under the given parent with the given value
			:black(true),
			 parent(sParent),left(0),right(0),value(sValue)
			{
			}
		};
	
	public:
	class iterator // Class to iterate through a tree's elements in sorting order
		{
		friend class RedBlackTree;
		
		/* Elements: */
		private:
		Node* node; // Currently pointed-to node
		
		/* Constructors and destructors: */
		public:
		iterator(void) // Dummy constructor
			{
			}
		iterator(Node* sNode) // Creates an iterator to the given node
			:node(sNode)
			{
			}
		
		/* Methods: */
		bool operator==(const iterator& other) // Equality operator
			{
			return node==other.node;
			}
		bool operator!=(const iterator& other) // Inequality operator
			{
			return node!=other.node;
			}
		Content& operator*(void) const // Returns the value of the pointed-to node as an L-value
			{
			return node->value;
			}
		Content* operator->(void) const // Ditto
			{
			return &node->value;
			}
		iterator& operator++(void) // Moves the iterator to the next element in sorting order
			{
			/* Check if the current node has a right child: */
			if(node->right!=0)
				{
				/* Move to the right once, and then to the left until there's no more left: */
				node=node->right;
				while(node->left!=0)
					node=node->left;
				}
			else
				{
				/* Move up the tree until there is no more tree or the current node is the parent's left child: */
				while(node->parent!=0&&node==node->parent->right)
					node=node->parent;
				
				/* Move up the tree one more time: */
				node=node->parent;
				}
			
			return *this;
			}
		iterator& operator+=(int step) // Moves the iterator the given number of steps in increasing sorting order
			{
			if(step>=0)
				{
				for(;step>0;--step)
					this->operator++();
				return *this;
				}
			else
				{
				for(;step<0;++step)
					this->operator--();
				return *this;
				}
			}
		iterator operator+(int step)
			{
			iterator result(node);
			result+=step;
			return result;
			}
		iterator& operator--(void) // Moves the iterator to the previous element in sorting order
			{
			/* Check if the current node has a left child: */
			if(node->left!=0)
				{
				/* Move to the left once, and then to the right until there's no more right: */
				node=node->left;
				while(node->right!=0)
					node=node->right;
				}
			else
				{
				/* Move up the tree until there is no more tree or the current node is the parent's right child: */
				while(node->parent!=0&&node==node->parent->left)
					node=node->parent;
				
				/* Move up the tree one more time: */
				node=node->parent;
				}
			
			return *this;
			}
		iterator& operator-=(int step) // Moves the iterator the given number of steps in decreasing sorting order
			{
			if(step>=0)
				{
				for(;step>0;--step)
					this->operator--();
				return *this;
				}
			else
				{
				for(;step<0;++step)
					this->operator++();
				return *this;
				}
			}
		iterator operator-(int step)
			{
			iterator result(node);
			result-=step;
			return result;
			}
		};
	
	/* Elements: */
	private:
	AllocatorParam<Node> allocator; // Memory allocator for tree nodes
	Node* root; // Pointer to the tree's root node; 0 for empty tree
	
	/* Private methods: */
	void clear(Node* node) // Recursive method to destroy the given node's subtree
		{
		if(node!=0)
			{
			clear(node->left);
			clear(node->right);
			allocator.destroy(node);
			}
		}
	
	/* Constructors and destructors: */
	public:
	RedBlackTree(void) // Creates an empty tree
		:root(0)
		{
		}
	private:
	RedBlackTree(const RedBlackTree& source); // Prohibit copy constructor
	RedBlackTree& operator=(const RedBlackTree& source); // Prohibit assignment operator
	public:
	~RedBlackTree(void) // Destroys the tree
		{
		/* Destroy all elements recursively: */
		clear(root);
		}
	
	/* Methods: */
	bool empty(void) const // Returns true if the tree contains no elements
		{
		/* Tree is empty if there is no root: */
		return root==0;
		}
	void clear(void) // Removes all elements from the tree
		{
		/* Call the recursive method: */
		clear(root);
		
		/* Reset the root pointer: */
		root=0;
		}
	iterator begin(void) // Returns an iterator to the first element in the tree in sorting order
		{
		/* Return an invalid iterator if the tree is empty: */
		if(root==0)
			return iterator(0);
		
		/* Return an iterator to the leftmost leaf in the tree: */
		Node* first=root;
		while(first->left!=0)
			first=first->left;
		return iterator(first);
		}
	iterator end(void) const // Returns an iterator behind the last element in the tree in sorting order
		{
		/* Just return an invalid iterator: */
		return iterator(0);
		}
	iterator find(const Content& searchValue) // Returns an iterator to any element matching the given value, or an invalid iterator if the element is not in the tree
		{
		Node* node=root;
		while(node!=0)
			{
			/* Check if the current node's value is less than or equal to the search value: */
			if(Comparison::lessEqual(node->value,searchValue))
				{
				/* Check if the current node's value is equal to the search value: */
				if(Comparison::lessEqual(searchValue,node->value))
					{
					/* Return an iterator to the current node: */
					return iterator(node);
					}
				else // Current node's value is less than search value
					{
					/* Go to the current node's right sub-tree: */
					node=node->right;
					}
				}
			else // Current node's value is larger than search value
				{
				/* Go to the current node's left sub-tree: */
				node=node->left;
				}
			}
		
		/* Return an invalid iterator: */
		return iterator(0);
		}
	iterator findFirst(const Content& searchValue) // Returns an iterator to the first element in traversal order matching the given value, or an invalid iterator if the element is not in the tree
		{
		Node* result=0;
		Node* node=root;
		while(node!=0)
			{
			/* Check if the current node's value is less than or equal to the search value: */
			if(Comparison::lessEqual(node->value,searchValue))
				{
				/* Check if the current node's value is equal to the search value: */
				if(Comparison::lessEqual(searchValue,node->value))
					{
					/* Mark the current node as a potential find: */
					result=node;
					
					/* Keep searching in the current node's left sub-tree anyway, in case there are more nodes with the same value ahead of it in traversal order: */
					node=node->left;
					}
				else // Current node's value is less than search value
					{
					/* Go to the current node's right sub-tree: */
					node=node->right;
					}
				}
			else // Current node's value is larger than search value
				{
				/* Go to the current node's left sub-tree: */
				node=node->left;
				}
			}
		
		/* Return an iterator to the last marked node: */
		return iterator(result);
		}
	template <class CompareFunctor>
	iterator findFirst(const CompareFunctor& comp) // Returns an iterator to the first element in traversal order matching the given compare functor, or an invalid iterator if no element equivalent to the functor is in the tree
		{
		Node* result=0;
		Node* node=root;
		while(node!=0)
			{
			/* Check if the current node's value is less than or equal to the search value: */
			if(comp.lessEqual(node->value))
				{
				/* Check if the current node's value is equal to the search value: */
				if(comp.lessEqual(node->value))
					{
					/* Mark the current node as a potential find: */
					result=node;
					
					/* Keep searching in the current node's left sub-tree anyway, in case there are more nodes with the same value ahead of it in traversal order: */
					node=node->left;
					}
				else // Current node's value is less than search value
					{
					/* Go to the current node's right sub-tree: */
					node=node->right;
					}
				}
			else // Current node's value is larger than search value
				{
				/* Go to the current node's left sub-tree: */
				node=node->left;
				}
			}
		
		/* Return an iterator to the last marked node: */
		return iterator(result);
		}
	std::pair<iterator,bool> insertUnique(const Content& newValue) // Inserts the given new value into the tree if it is not in there already and returns an iterator to the new or previously existing value and a flag whether the new value was actually inserted
		{
		/* Check if the tree is empty: */
		if(root==0)
			{
			/* Create the root and return an iterator to it: */
			root=new (allocator.allocate()) Node(0,newValue);
			return std::make_pair(iterator(root),true);
			}
		
		Node* node=root;
		while(true)
			{
			/* Check if the current node's value is less than or equal to the new value: */
			if(Comparison::lessEqual(node->value,newValue))
				{
				/* Check if the current node's value is equal to the search value: */
				if(Comparison::lessEqual(newValue,node->value))
					{
					/* The new value already exists in the tree; return an iterator to it: */
					return std::make_pair(iterator(node),false);
					}
				else
					{
					/* Check if the current node has a right child: */
					if(node->right!=0)
						{
						/* Insert in the current node's right sub-tree: */
						node=node->right;
						}
					else
						{
						/* Create the current node's right child and return an iterator to it: */
						node->right=new (allocator.allocate()) Node(node,newValue);
						return std::make_pair(iterator(node->right),true);
						}
					}
				}
			else // Current node's value is larger than new value
				{
				/* Check if the current node has a left child: */
				if(node->left!=0)
					{
					/* Insert in the current node's left sub-tree: */
					node=node->left;
					}
				else
					{
					/* Create the current node's left child and return an iterator to it: */
					node->left=new (allocator.allocate()) Node(node,newValue);
					return std::make_pair(iterator(node->left),true);
					}
				}
			}
		}
	iterator insertBefore(const Content& newValue) // Inserts the given new value into the tree before any other identical values in traversal order
		{
		/* Check if the tree is empty: */
		if(root==0)
			{
			/* Create the root and return an iterator to it: */
			root=new (allocator.allocate()) Node(0,newValue);
			return iterator(root);
			}
		
		Node* node=root;
		while(true)
			{
			/* Check if the current node's value is less than or equal to the new value: */
			if(Comparison::lessEqual(node->value,newValue))
				{
				/* Check if the current node's value is equal to the search value: */
				if(Comparison::lessEqual(newValue,node->value))
					{
					/* Check if the current node has a left child: */
					if(node->left!=0)
						{
						/* Insert in the current node's left sub-tree: */
						node=node->left;
						}
					else
						{
						/* Create the current node's left child and return an iterator to it: */
						node->left=new (allocator.allocate()) Node(node,newValue);
						return iterator(node->left);
						}
					}
				else
					{
					/* Check if the current node has a right child: */
					if(node->right!=0)
						{
						/* Insert in the current node's right sub-tree: */
						node=node->right;
						}
					else
						{
						/* Create the current node's right child and return an iterator to it: */
						node->right=new (allocator.allocate()) Node(node,newValue);
						return iterator(node->right);
						}
					}
				}
			else // Current node's value is larger than new value
				{
				/* Check if the current node has a left child: */
				if(node->left!=0)
					{
					/* Insert in the current node's left sub-tree: */
					node=node->left;
					}
				else
					{
					/* Create the current node's left child and return an iterator to it: */
					node->left=new (allocator.allocate()) Node(node,newValue);
					return iterator(node->left);
					}
				}
			}
		}
	iterator insertAfter(const Content& newValue) // Inserts the given new value into the tree after any other identical values in traversal order
		{
		/* Check if the tree is empty: */
		if(root==0)
			{
			/* Create the root and return an iterator to it: */
			root=new (allocator.allocate()) Node(0,newValue);
			return iterator(root);
			}
		
		Node* node=root;
		while(true)
			{
			/* Check if the current node's value is less than or equal to the new value: */
			if(Comparison::lessEqual(node->value,newValue))
				{
				/* Check if the current node has a right child: */
				if(node->right!=0)
					{
					/* Insert in the current node's right sub-tree: */
					node=node->right;
					}
				else
					{
					/* Create the current node's right child and return an iterator to it: */
					node->right=new (allocator.allocate()) Node(node,newValue);
					return iterator(node->right);
					}
				}
			else // Current node's value is larger than new value
				{
				/* Check if the current node has a left child: */
				if(node->left!=0)
					{
					/* Insert in the current node's left sub-tree: */
					node=node->left;
					}
				else
					{
					/* Create the current node's left child and return an iterator to it: */
					node->left=new (allocator.allocate()) Node(node,newValue);
					return iterator(node->left);
					}
				}
			}
		}
	template <class CompareFunctor>
	iterator insertAfter(const Content& newValue,const CompareFunctor& comp) // Inserts the given new value into the tree after any other identical values in traversal order as defined by the given comparison functor
		{
		/* Check if the tree is empty: */
		if(root==0)
			{
			/* Create the root and return an iterator to it: */
			root=new (allocator.allocate()) Node(0,newValue);
			return iterator(root);
			}
		
		Node* node=root;
		while(true)
			{
			/* Check if the current node's value is less than or equal to the new value: */
			if(comp.lessEqual(node->value,newValue))
				{
				/* Check if the current node has a right child: */
				if(node->right!=0)
					{
					/* Insert in the current node's right sub-tree: */
					node=node->right;
					}
				else
					{
					/* Create the current node's right child and return an iterator to it: */
					node->right=new (allocator.allocate()) Node(node,newValue);
					return iterator(node->right);
					}
				}
			else // Current node's value is larger than new value
				{
				/* Check if the current node has a left child: */
				if(node->left!=0)
					{
					/* Insert in the current node's left sub-tree: */
					node=node->left;
					}
				else
					{
					/* Create the current node's left child and return an iterator to it: */
					node->left=new (allocator.allocate()) Node(node,newValue);
					return iterator(node->left);
					}
				}
			}
		}
	void erase(iterator eraseIt) // Erases the value indicated by the given iterator, which will become invalid
		{
		/* Determine the number of children of the indicated node, and act accordingly: */
		Node* node=eraseIt.node;
		if(node->left!=0) // Node has a left child
			{
			if(node->right!=0) // Node has both a left and a right child
				{
				/* Find the rightmost node in the node's left sub-tree: */
				Node* child=node->left;
				if(child->right!=0) // The node's left child isn't already the rightmost node: */
					{
					/* Find the rightmost node: */
					do
						{
						child=child->right;
						}
					while(child->right!=0);
					
					/* Remove the rightmost child from its current place in the tree: */
					child->parent->right=child->left;
					if(child->left!=0)
						child->left->parent=child->parent;
					child->left=node->left;
					node->left->parent=child;
					}
				
				/* Replace the indicated node with the rightmost child: */
				if(node->parent!=0)
					{
					if(node->parent->left==node)
						node->parent->left=child;
					else
						node->parent->right=child;
					}
				else
					root=child;
				child->parent=node->parent;
				child->right=node->right;
				node->right->parent=child;
				
				/* Delete the node: */
				allocator.destroy(node);
				}
			else // Node does not have a right child
				{
				/* Point the node's parent to the node's left child: */
				if(node->parent!=0)
					{
					if(node->parent->left==node)
						node->parent->left=node->left;
					else
						node->parent->right=node->left;
					}
				else
					root=node->left;
				
				/* Point the node's left child to the parent: */
				if(node->left!=0)
					node->left->parent=node->parent;
				
				/* Delete the node: */
				allocator.destroy(node);
				}
			}
		else // Node does not have a left child
			{
			/* Point the node's parent to the node's right child: */
			if(node->parent!=0)
				{
				if(node->parent->left==node)
					node->parent->left=node->right;
				else
					node->parent->right=node->right;
				}
			else
				root=node->right;
			
			/* Point the node's right child to the parent: */
			if(node->right!=0)
				node->right->parent=node->parent;
			
			/* Delete the node: */
			allocator.destroy(node);
			}
		}
	};

}

#endif
