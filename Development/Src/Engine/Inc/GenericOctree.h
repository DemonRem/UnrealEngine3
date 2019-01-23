/*=============================================================================
	GenericOctree.h: Generic octree implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef __GENERIC_OCTREE_H__
#define __GENERIC_OCTREE_H__

/** A concise iteration over the children of an octree node. */
#define FOREACH_OCTREE_CHILD_NODE(ChildX,ChildY,ChildZ) \
	for( \
		INT ChildX = 0, \
			ChildY = 0, \
			ChildZ = 0, \
			ChildIndex = 0; \
		ChildIndex < 8; \
		ChildIndex++, \
		ChildX = (ChildIndex & 4) >> 2, \
		ChildY = (ChildIndex & 2) >> 1, \
		ChildZ = (ChildIndex & 1) \
	)

/** A reference to a child of an octree node. */
class FOctreeChildNodeRef
{
public:

	BITFIELD X : 1;
	BITFIELD Y : 1;
	BITFIELD Z : 1;

	/** Initialization constructor. */
	FOctreeChildNodeRef(INT InX,INT InY,INT InZ):
		X(InX),
		Y(InY),
		Z(InZ)
	{}
};

/** The context of an octree node, derived from the traversal of the tree. */
class FOctreeNodeContext
{
public:

	/** The bounding box of the octree node. */
	FBox BoundingBox;

	/** The center of the octree node. */
	FVector Center;

	/** The extent of the octree node. */
	FVector Extent;

	/** The number of steps up the tree to the root node. */
	INT Depth;

	/** Root node initialization constructor. */
	FOctreeNodeContext(const FBox& InBoundingBox):
		BoundingBox(InBoundingBox),
		Center(InBoundingBox.GetCenter()),
		Extent(InBoundingBox.GetExtent()),
		Depth(0)
	{}

	/** Child node initialization constructor. */
	FOctreeNodeContext(const FOctreeNodeContext& ParentContext,FOctreeChildNodeRef ChildRef):
		Depth(ParentContext.Depth + 1)
	{
		BoundingBox.Min = ParentContext.BoundingBox.Min + FVector(ChildRef.X * ParentContext.Extent.X,ChildRef.Y * ParentContext.Extent.Y,ChildRef.Z * ParentContext.Extent.Z);
		BoundingBox.Max = BoundingBox.Min + ParentContext.Extent;
		const FVector ChildExtent = ParentContext.Extent * 0.5f;;
		Center = BoundingBox.Min + ChildExtent;
		Extent = ChildExtent;
	}

	/** Determines whether a point is in a child or not. */
	UBOOL IsInChild(FOctreeChildNodeRef ChildRef,const FVector& Point) const;
};

/** An octree. */
template<typename ElementType,typename OctreeSemantics>
class TOctree
{
public:

	/** A node in the octree. */
	class FNode
	{
	public:

		friend class TOctree::TConstIterator;

		/** Initialization constructor. */
		FNode():
			bIsLeaf(TRUE)
		{
			FOREACH_OCTREE_CHILD_NODE(ChildX,ChildY,ChildZ)
			{
				Children[ChildX][ChildY][ChildZ] = NULL;
			}
		}

		/** Destructor. */
		~FNode()
		{
			FOREACH_OCTREE_CHILD_NODE(ChildX,ChildY,ChildZ)
			{
				delete Children[ChildX][ChildY][ChildZ];
			}
		}

		/**
		 * Adds an element to this subtree of the octree.
		 * @param Context - The context of this node.
		 * @param Element - The element to add.
		 * @param ElementBoundingBox - The element's bounding box.
		 */
		void AddElement(const FOctreeNodeContext& Context,typename TTypeInfo<ElementType>::ConstInitType Element,const FBox& ElementBoundingBox);

		// Accessors.
		const TArray<ElementType>& GetElements() const { return Elements; }
		UBOOL IsLeaf() const { return bIsLeaf; }
		UBOOL HasChild(FOctreeChildNodeRef ChildRef) const { return Children[ChildRef.X][ChildRef.Y][ChildRef.Z] != NULL; }

	private:

		/** The meshes in this node. */
		TArray<ElementType> Elements;

		/** The children of the node. */
		FNode* Children[2][2][2];

		/** TRUE if the meshes should be added directly to the node, rather than subdividing when possible. */
		BITFIELD bIsLeaf : 1;
	};

	/** A reference to an octree node, its context, and a read lock. */
	class FNodeReference
	{
	public:

		const FNode* Node;
		FOctreeNodeContext Context;

		/** Default constructor. */
		FNodeReference():
			Node(NULL),
			Context(FBox())
		{}

		/** Initialization constructor. */
		FNodeReference(const FNode* InNode,const FOctreeNodeContext& InContext):
			Node(InNode),
			Context(InContext)
		{}
	};

	/** An octree node iterator. */
	class TConstIterator
	{
	public:

		/** Pushes a child of the current node onto the stack of nodes to visit. */
		void PushChild(FOctreeChildNodeRef ChildRef)
		{
			check(NodeStack.Num() < MaxStackSize);
			NodeStack.AddItem(
				FNodeReference(
					CurrentNode.Node->Children[ChildRef.X][ChildRef.Y][ChildRef.Z],
					FOctreeNodeContext(CurrentNode.Context,ChildRef)
					));
		}

		/** Iterates to the next node. */
		void Advance()
		{
			if(NodeStack.Num())
			{
				CurrentNode = NodeStack(NodeStack.Num() - 1);
				NodeStack.Remove(NodeStack.Num() - 1);
			}
			else
			{
				CurrentNode = FNodeReference();
			}
		}

		/** Checks if there are any nodes left to iterate over. */
		UBOOL HasPendingNodes() const
		{
			return CurrentNode.Node != NULL;
		}

		/** Initialization constructor. */
		TConstIterator(const TOctree& Tree):
			CurrentNode(FNodeReference(&Tree.RootNode,Tree.RootNodeContext))
		{}

		// Accessors.
		const FNode& GetCurrentNode() const
		{
			return *CurrentNode.Node;
		}
		const FOctreeNodeContext& GetCurrentContext() const
		{
			return CurrentNode.Context;
		}

	private:

		/** The node that is currently being visited. */
		FNodeReference CurrentNode;

		// The maximum stack size is derived by assuming that all nodes on the stack will be siblings of a single path from root to leaf.
		enum { MaxStackSize = 7 * (OctreeSemantics::MaxNodeDepth - 1) + 8 };

		/** The nodes which are pending iteration. */
		TStaticArray<FNodeReference,MaxStackSize> NodeStack;
	};

	/**
	 * Adds an element to the octree.
	 * @param Element - The element to add.
	 */
	void AddElement(typename TTypeInfo<ElementType>::ConstInitType Element);

	/** Initialization constructor. */
	TOctree(const FBox& InBoundingBox):
		RootNodeContext(InBoundingBox)
	{}

private:

	FNode RootNode;
	FOctreeNodeContext RootNodeContext;
};

//
//	Octree implementation
//

inline UBOOL FOctreeNodeContext::IsInChild(FOctreeChildNodeRef ChildRef,const FVector& Point) const
{
	// Cull nodes on the opposite side of the X-axis division.
	if(ChildRef.X == 1 && Point.X < Center.X)
	{
		return FALSE;
	}
	else if(ChildRef.X == 0 && Point.X >= Center.X)
	{
		return FALSE;
	}

	// Cull nodes on the opposite side of the Y-axis division.
	if(ChildRef.Y == 1 && Point.Y < Center.Y)
	{
		return FALSE;
	}
	else if(ChildRef.Y == 0 && Point.Y >= Center.Y)
	{
		return FALSE;
	}

	// Cull nodes on the opposite side of the Z-axis division.
	if(ChildRef.Z == 1 && Point.Z < Center.Z)
	{
		return FALSE;
	}
	else if(ChildRef.Z == 0 && Point.Z >= Center.Z)
	{
		return FALSE;
	}

	return TRUE;
}

template<typename ElementType,typename OctreeSemantics>
void TOctree<ElementType,OctreeSemantics>::FNode::AddElement(const FOctreeNodeContext& Context,typename TTypeInfo<ElementType>::ConstInitType Element,const FBox& ElementBoundingBox)
{
	UBOOL bAddElementToThisNode = FALSE;
	UBOOL bSplitThisNode = FALSE;
	UBOOL bIntersectsChildX[2] = { TRUE, TRUE };
	UBOOL bIntersectsChildY[2] = { TRUE, TRUE };
	UBOOL bIntersectsChildZ[2] = { TRUE, TRUE };

	{
		if(bIsLeaf)
		{
			// If this is a leaf, check if adding this element would turn it into a node by overflowing its element list.
			if(Elements.Num() + 1 > OctreeSemantics::MaxElementsPerNode && Context.Depth < OctreeSemantics::MaxNodeDepth)
			{
				bSplitThisNode = TRUE;
			}
			else
			{
				// If the leaf has room for the new element, simply add it to the list.
				bAddElementToThisNode = TRUE;
			}
		}
		else
		{
			// If this isn't a leaf, find the children which the mesh intersects.

			// Cull nodes on the opposite side of the X-axis division.
			if(ElementBoundingBox.Min.X > Context.Center.X)
			{
				bIntersectsChildX[0] = FALSE;
			}
			else if(ElementBoundingBox.Max.X < Context.Center.X)
			{
				bIntersectsChildX[1] = FALSE;
			}
			else
			{
				// If the element crosses one of the center planes, it would end up in multiple children, so it should be added directly to this node.
				bAddElementToThisNode = TRUE;
			}

			// Cull nodes on the opposite side of the Y-axis division.
			if(ElementBoundingBox.Min.Y > Context.Center.Y)
			{
				bIntersectsChildY[0] = FALSE;
			}
			else if(ElementBoundingBox.Max.Y < Context.Center.Y)
			{
				bIntersectsChildY[1] = FALSE;
			}
			else
			{
				// If the element crosses one of the center planes, it would end up in multiple children, so it should be added directly to this node.
				bAddElementToThisNode = TRUE;
			}

			// Cull nodes on the opposite side of the Z-axis division.
			if(ElementBoundingBox.Min.Z > Context.Center.Z)
			{
				bIntersectsChildZ[0] = FALSE;
			}
			else if(ElementBoundingBox.Max.Z < Context.Center.Z)
			{
				bIntersectsChildZ[1] = FALSE;
			}
			else
			{
				// If the element crosses one of the center planes, it would end up in multiple children, so it should be added directly to this node.
				bAddElementToThisNode = TRUE;
			}
		}
	}

	if(bAddElementToThisNode)
	{
		// Add the mesh to this node.
		new(Elements) ElementType(Element);
	}
	else if(bSplitThisNode)
	{
		// Copy the leaf's elements, remove them from the leaf, and turn it into a node.
		TArray<ElementType> ChildElements;
		{
			ChildElements = Elements;

			// Allow meshes to be added to children of this node.
			bIsLeaf = FALSE;
		}

		// Add the new element to the node.
		AddElement(Context,Element,ElementBoundingBox);

		// Re-add all of the node's child elements, potentially creating children of this node for them.
		for(INT ElementIndex = 0;ElementIndex < ChildElements.Num();ElementIndex++)
		{
			AddElement(Context,ChildElements(ElementIndex),OctreeSemantics::GetBoundingBox(ChildElements(ElementIndex)));
		}
	}
	else
	{
		// Add the mesh to any child nodes which is intersects.
		FOREACH_OCTREE_CHILD_NODE(ChildX,ChildY,ChildZ)
		{
			// Check that the mesh intersects this child.
			if(bIntersectsChildX[ChildX] && bIntersectsChildY[ChildY] && bIntersectsChildZ[ChildZ])
			{
				if(!Children[ChildX][ChildY][ChildZ])
				{
					Children[ChildX][ChildY][ChildZ] = new typename TOctree<ElementType,OctreeSemantics>::FNode();
				}

				// Add the mesh to the child node.
				Children[ChildX][ChildY][ChildZ]->AddElement(
					FOctreeNodeContext(Context,FOctreeChildNodeRef(ChildX,ChildY,ChildZ)),
					Element,
					ElementBoundingBox
					);
			}
		}
	}
}

template<typename ElementType,typename OctreeSemantics>
void TOctree<ElementType,OctreeSemantics>::AddElement(typename TTypeInfo<ElementType>::ConstInitType Element)
{
	RootNode.AddElement(RootNodeContext,Element,OctreeSemantics::GetBoundingBox(Element));
}

#endif
