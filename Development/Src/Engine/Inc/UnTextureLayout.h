/*=============================================================================
	UnTextureLayout.h: Texture space allocation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_UNTEXTURELAYOUT
#define _INC_UNTEXTURELAYOUT

/**
 * An incremental texture space allocator.
 * For best results, add the elements ordered descending in size.
 */
class FTextureLayout
{
public:

	/**
	 * Minimal initialization constructor.
	 * @param	MinSizeX - The minimum width of the texture.
	 * @param	MinSizeY - The minimum height of the texture.
	 * @param	MaxSizeX - The maximum width of the texture.
	 * @param	MaxSizeY - The maximum height of the texture.
	 * @param	InPowerOfTwoSize - True if the texture size must be a power of two.
	 */
	FTextureLayout(UINT MinSizeX,UINT MinSizeY,UINT MaxSizeX,UINT MaxSizeY,bool InPowerOfTwoSize = false):
		SizeX(MinSizeX),
		SizeY(MinSizeY),
		PowerOfTwoSize(InPowerOfTwoSize)
	{
		new(Nodes) FTextureLayoutNode(0,0,MaxSizeX,MaxSizeY);
	}

	/**
	 * Finds a free area in the texture large enough to contain a surface with the given size.
	 * If a large enough area is found, it is marked as in use, the output parameters OutBaseX and OutBaseY are
	 * set to the coordinates of the upper left corner of the free area and the function return true.
	 * Otherwise, the function returns false and OutBaseX and OutBaseY remain uninitialized.
	 * @param	OutBaseX - If the function succeedes, contains the X coordinate of the upper left corner of the free area on return.
	 * @param	OutBaseY - If the function succeedes, contains the Y coordinate of the upper left corner of the free area on return.
	 * @param	ElementSizeX - The size of the surface to allocate in horizontal pixels.
	 * @param	ElementSizeY - The size of the surface to allocate in vertical pixels.
	 * @return	True if succeeded, false otherwise.
	 */
	UBOOL AddElement(UINT* OutBaseX,UINT* OutBaseY,UINT ElementSizeX,UINT ElementSizeY)
	{
		// Try allocating space without enlarging the texture.
		INT	NodeIndex = AddSurfaceInner(0,ElementSizeX,ElementSizeY,false);
		if(NodeIndex == INDEX_NONE)
		{
			// Try allocating space which might enlarge the texture.
			NodeIndex = AddSurfaceInner(0,ElementSizeX,ElementSizeY,true);
		}

		if(NodeIndex != INDEX_NONE)
		{
			FTextureLayoutNode&	Node = Nodes(NodeIndex);
			Node.Used = true;
			*OutBaseX = Node.MinX;
			*OutBaseY = Node.MinY;
			if(PowerOfTwoSize)
			{
				SizeX = Max<UINT>(SizeX,1 << appCeilLogTwo(Node.MinX + ElementSizeX));
				SizeY = Max<UINT>(SizeY,1 << appCeilLogTwo(Node.MinY + ElementSizeY));
			}
			else
			{
				SizeX = Max<UINT>(SizeX,Node.MinX + ElementSizeX);
				SizeY = Max<UINT>(SizeY,Node.MinY + ElementSizeY);
			}
			return 1;
		}
		else
			return 0;

	}

	/**
	 * Returns the minimum texture width which will contain the allocated surfaces.
	 */
	UINT GetSizeX() const { return SizeX; }

	/**
	 * Returns the minimum texture height which will contain the allocated surfaces.
	 */
	UINT GetSizeY() const { return SizeY; }

private:

	struct FTextureLayoutNode
	{
		INT		ChildA,
				ChildB;
		WORD	MinX,
				MinY,
				SizeX,
				SizeY;
		bool	Used;

		FTextureLayoutNode(WORD InMinX,WORD InMinY,WORD InSizeX,WORD InSizeY):
			ChildA(INDEX_NONE),
			ChildB(INDEX_NONE),
			MinX(InMinX),
			MinY(InMinY),
			SizeX(InSizeX),
			SizeY(InSizeY),
			Used(false)
		{}
	};

	UINT SizeX;
	UINT SizeY;
	bool PowerOfTwoSize;
	TIndirectArray<FTextureLayoutNode> Nodes;

	INT AddSurfaceInner(INT NodeIndex,UINT ElementSizeX,UINT ElementSizeY,bool AllowTextureEnlargement)
	{
		FTextureLayoutNode&	Node = Nodes(NodeIndex);

		if(Node.ChildA != INDEX_NONE)
		{
			check(Node.ChildB != INDEX_NONE);
			INT	Result = AddSurfaceInner(Node.ChildA,ElementSizeX,ElementSizeY,AllowTextureEnlargement);
			if(Result != INDEX_NONE)
				return Result;
			return AddSurfaceInner(Node.ChildB,ElementSizeX,ElementSizeY,AllowTextureEnlargement);
		}
		else
		{
			if(Node.Used)
				return INDEX_NONE;

			if(Node.SizeX < ElementSizeX || Node.SizeY < ElementSizeY)
				return INDEX_NONE;

			if(!AllowTextureEnlargement)
			{
				if(Node.MinX + ElementSizeX > SizeX || Node.MinY + ElementSizeY > SizeY)
				{
					// If this is an attempt to allocate space without enlarging the texture, and this node cannot hold the element
					// without enlarging the texture, fail.
					return INDEX_NONE;
				}
			}

			if(Node.SizeX == ElementSizeX && Node.SizeY == ElementSizeY)
				return NodeIndex;

			UINT	ExcessWidth = Node.SizeX - ElementSizeX,
					ExcessHeight = Node.SizeY - ElementSizeY;

			if(ExcessWidth > ExcessHeight)
			{
				Node.ChildA = Nodes.Num();
                new(Nodes) FTextureLayoutNode(Node.MinX,Node.MinY,ElementSizeX,Node.SizeY);

				Node.ChildB = Nodes.Num();
				new(Nodes) FTextureLayoutNode(Node.MinX + ElementSizeX,Node.MinY,Node.SizeX - ElementSizeX,Node.SizeY);
			}
			else
			{
				Node.ChildA = Nodes.Num();
                new(Nodes) FTextureLayoutNode(Node.MinX,Node.MinY,Node.SizeX,ElementSizeY);

				Node.ChildB = Nodes.Num();
				new(Nodes) FTextureLayoutNode(Node.MinX,Node.MinY + ElementSizeY,Node.SizeX,Node.SizeY - ElementSizeY);
			}

			return AddSurfaceInner(Node.ChildA,ElementSizeX,ElementSizeY,AllowTextureEnlargement);
		}
	}
};

#endif
