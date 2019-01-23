/*=============================================================================
LandscapeDataAccess.h: Classes for the editor to access to Landscape data
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _LANDSCAPEDATAACCESS_H
#define _LANDSCAPEDATAACCESS_H


#define LANDSCAPE_VALIDATE_DATA_ACCESS 1
#define LANDSCAPE_ZSCALE	(1.0f/128.0f)

#if WITH_EDITOR

//
// FLandscapeDataInterface
//
struct FLandscapeDataInterface
{
private:

	struct FLockedMipDataInfo
	{
		FLockedMipDataInfo(void* InMipData)
		:	MipData(InMipData)
		,	LockCount(0)
		{}

		void* MipData;
		INT LockCount;
	};

public:
	// Constructor
	// @param bInAutoDestroy - shall we automatically clean up when the last 
	FLandscapeDataInterface()
	{}

	void* LockMip(UTexture2D* Texture, INT MipLevel)
	{
		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		if( MipInfo == NULL )
		{
			MipInfo = &LockedMipInfoMap.Set(Texture, TArray<FLockedMipDataInfo>() );
			MipInfo->AddZeroed( Texture->Mips.Num() );
		}

		if( (*MipInfo)(MipLevel).MipData == NULL )
		{
			(*MipInfo)(MipLevel).MipData = Texture->Mips(0).Data.Lock(LOCK_READ_ONLY);
			//Texture->Mips(0).Data.Unlock();
		}
		(*MipInfo)(MipLevel).LockCount++;
		//appDebugMessagef(TEXT("LockMip: %d"), (*MipInfo)(MipLevel).LockCount);

		return (*MipInfo)(MipLevel).MipData;
	}

	void UnlockMip(UTexture2D* Texture, INT MipLevel)
	{
		TArray<FLockedMipDataInfo>* MipInfo = LockedMipInfoMap.Find(Texture);
		check(MipInfo);

		if ((*MipInfo)(MipLevel).LockCount <= 0)
			return;
		(*MipInfo)(MipLevel).LockCount--;
		//appDebugMessagef(TEXT("UnlockMip: %d"), (*MipInfo)(MipLevel).LockCount);
		if( (*MipInfo)(MipLevel).LockCount == 0 )
		{
			check( (*MipInfo)(MipLevel).MipData != NULL );
			(*MipInfo)(MipLevel).MipData = NULL;
			Texture->Mips(0).Data.Unlock();
		}		
	}

private:
	TMap<UTexture2D*, TArray<FLockedMipDataInfo> > LockedMipInfoMap;
};

//
// FLandscapeComponentDataInterface
//
struct FLandscapeComponentDataInterface
{
	friend struct FLandscapeDataInterface;

	// tors
	FLandscapeComponentDataInterface(ULandscapeComponent* InComponent)
	:	Component(InComponent),
		ExpandQuadsX(0),
		ExpandQuadsY(0),
		SizeX(0),
		SizeY(0)
	{
		DataInterface = Component->GetLandscapeActor()->GetDataInterface();

		// Offset and stride for this component's data in heightmap texture
		HeightmapStride = Component->HeightmapTexture->SizeX;
		HeightmapComponentOffsetX = appRound( (FLOAT)Component->HeightmapTexture->SizeX * Component->HeightmapScaleBias.Z );
		HeightmapComponentOffsetY = appRound( (FLOAT)Component->HeightmapTexture->SizeY * Component->HeightmapScaleBias.W );
		HeightmapSubsectionOffset = Component->SubsectionSizeQuads + 1;

		SubsectionSizeVerts = Component->SubsectionSizeQuads + 1;

		// Weightmap
//		WeightmapStride = Component->WeightmapTexture->SizeX;
//		WeightmapOffsetX = appRound( (FLOAT)Component->WeightmapTexture->SizeX * Component->WeightmapScaleBias.Z );
//		WeightmapOffsetY = appRound( (FLOAT)Component->WeightmapTexture->SizeY * Component->WeightmapScaleBias.W );

		HeightMipData = (FColor*)DataInterface->LockMip(Component->HeightmapTexture,0);
	}

	~FLandscapeComponentDataInterface()
	{
		DataInterface->UnlockMip(Component->HeightmapTexture,0);
	}

	// Accessors
	void VertexIndexToXY(INT VertexIndex, INT& OutX, INT& OutY) const
	{
/*
		OutX = VertexIndex % (Component->ComponentSizeQuads + 2*ExpandQuadsX + 1);
		OutY = VertexIndex / (Component->ComponentSizeQuads + 2*ExpandQuadsY + 1);
*/
		OutX = VertexIndex % (Component->ComponentSizeQuads + 1);
		OutY = VertexIndex / (Component->ComponentSizeQuads + 1);

	}

	void QuadIndexToXY(INT QuadIndex, INT& OutX, INT& OutY) const
	{
/*
		OutX = QuadIndex % (Component->ComponentSizeQuads + 2*ExpandQuadsX);
		OutY = QuadIndex / (Component->ComponentSizeQuads + 2*ExpandQuadsY);
*/
		OutX = QuadIndex % (Component->ComponentSizeQuads);
		OutY = QuadIndex / (Component->ComponentSizeQuads);
	}

	void ComponentXYToSubsectionXY(INT CompX, INT CompY, INT& SubNumX, INT& SubNumY, INT& SubX, INT& SubY ) const
	{
		// We do the calculation as if we're looking for the previous vertex.
		// This allows us to pick up the last shared vertex of every subsection correctly.
		SubNumX = (CompX-1) / Component->SubsectionSizeQuads;
		SubNumY = (CompY-1) / Component->SubsectionSizeQuads;
		SubX = (CompX-1) % Component->SubsectionSizeQuads + 1;
		SubY = (CompY-1) % Component->SubsectionSizeQuads + 1;

		// If we're asking for the first vertex, the calculation above will lead
		// to a negative SubNumX/Y, so we need to fix that case up.
		if( SubNumX < 0 )
		{
			SubNumX = 0;
			SubX = 0;
		}

		if( SubNumY < 0 )
		{
			SubNumY = 0;
			SubY = 0;
		}
	}

	FColor* GetRawHeightData() const
	{
		return HeightMipData;
	}

	void UnlockRawHeightData() const
	{
		DataInterface->UnlockMip(Component->HeightmapTexture,0);
	}

	/* Return the raw heightmap data in an array size Square(ComponentSizeVerts) */
	void GetRawHeightmapDataSlow( TArray<FColor>& OutData )
	{
		INT ComponentSizeVerts = Component->ComponentSizeQuads+1;
		OutData.Empty(Square(ComponentSizeVerts));
		OutData.Add(Square(ComponentSizeVerts));

		for( INT SubsectionY = 0;SubsectionY < Component->NumSubsections;SubsectionY++ )
		{
			for( INT SubsectionX = 0;SubsectionX < Component->NumSubsections;SubsectionX++ )
			{
				for( INT SubY=0;SubY<SubsectionSizeVerts;SubY++ )
				{
					for( INT SubX=0;SubX<SubsectionSizeVerts;SubX++ )
					{
						// X/Y of the vertex we're looking at in component's coordinates.
						INT CompX = Component->SubsectionSizeQuads * SubsectionX + SubX;
						INT CompY = Component->SubsectionSizeQuads * SubsectionY + SubY;

						// UV coordinates of the data offset into the texture
						INT TexU = SubsectionSizeVerts * SubsectionX + SubX + HeightmapComponentOffsetX;
						INT TexV = SubsectionSizeVerts * SubsectionY + SubY + HeightmapComponentOffsetY;

						// Copy the data
						OutData(CompX + CompY * ComponentSizeVerts) = HeightMipData[TexU + TexV * HeightmapStride];
					}
				}
			}
		}
	}

	FORCEINLINE void CopyNeighborHeightMapData( FColor& OutData, FColor* NeighborHeightMap, INT SubsectionX, INT SubsectionY, INT NeigborPos, INT OrigSubX, INT OrigSubY )
	{
		INT ComponentSizeVerts = Component->ComponentSizeQuads+1;
		INT ExpandSize = ComponentSizeVerts + 2*ExpandQuadsX;

		if (NeighborHeightMap)
			//OutData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
			OutData = NeighborHeightMap[NeigborPos];
		else 
		{
			INT OrigU = SubsectionSizeVerts * SubsectionX + OrigSubX + HeightmapComponentOffsetX;
			INT OrigV = SubsectionSizeVerts * SubsectionY + OrigSubY + HeightmapComponentOffsetY;
			// Copy the original data
			OutData = HeightMipData[OrigU + OrigV * HeightmapStride];
		}
	}

	/* Return the raw heightmap data in an array size Square(ComponentSizeVerts) */
	void GetRawHeightmapData( TArray<FColor>& OutData )
	{
		// Heightmap would include extension...
		INT ComponentSizeVerts = Component->ComponentSizeQuads+1;
		INT ExpandSize = ComponentSizeVerts + 2*ExpandQuadsX;
		OutData.Empty(Square(ExpandSize));
		OutData.Add(Square(ExpandSize));

		// copy heightmap data for this component...
		for( INT SubsectionY = 0;SubsectionY < Component->NumSubsections;SubsectionY++ )
		{
			for( INT SubsectionX = 0;SubsectionX < Component->NumSubsections;SubsectionX++ )
			{
				for( INT SubY=0;SubY<SubsectionSizeVerts;SubY++ )
				{
					// X/Y of the vertex we're looking at in component's coordinates.
					INT CompX = Component->SubsectionSizeQuads * SubsectionX;
					INT CompY = Component->SubsectionSizeQuads * SubsectionY + SubY;

					// UV coordinates of the data offset into the texture
					INT TexU = SubsectionSizeVerts * SubsectionX + HeightmapComponentOffsetX;
					INT TexV = SubsectionSizeVerts * SubsectionY + SubY + HeightmapComponentOffsetY;

					// Copy the data
					appMemcpy( &OutData(CompX + ExpandQuadsX + (CompY + ExpandQuadsY) * ExpandSize), &HeightMipData[TexU + TexV * HeightmapStride], SubsectionSizeVerts * sizeof(FColor));
				}
			}
		}

		// assume that ExpandQuad size < SubsectionSizeQuads...
		check(ExpandQuadsX <= Component->SubsectionSizeQuads);
		check(ExpandQuadsY <= Component->SubsectionSizeQuads);

		BOOL bCopyBorderOnly = FALSE;

		INT NeighborHeightmapStride, NeighborHeightmapComponentOffsetX, NeighborHeightmapComponentOffsetY;
		NeighborHeightmapStride = NeighborHeightmapComponentOffsetX = NeighborHeightmapComponentOffsetY = 0;

		// Super ugly code to copy 8-neighbor heigh-map data to outdata.... 
		// copy right-bottom height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX + Component->ComponentSizeQuads, Component->SectionBaseY + Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = 0;
			INT SubsectionY = 0;
			INT OrigSubsectionX = Component->NumSubsections - 1 - SubsectionX;
			INT OrigSubsectionY = Component->NumSubsections - 1 - SubsectionY;

			for ( INT SubY = 1; SubY < ExpandQuadsY+1; SubY++ )
			{
				for( INT SubX = 1; SubX < ExpandQuadsX+1; SubX++ )
				{
					FColor& DestData = OutData( (SubX-1 + (ExpandQuadsX+ComponentSizeVerts)) + (SubY-1 + (ExpandQuadsY+ComponentSizeVerts)) * ExpandSize );
					if (Neighbor && !bCopyBorderOnly)
					{
						// UV coordinates of the data offset into the texture
						INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
						INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
						DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
					}
					else 
					{
						INT OrigU = SubsectionSizeVerts * OrigSubsectionX	+ SubsectionSizeVerts-1	+ HeightmapComponentOffsetX;
						INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ SubsectionSizeVerts-1 + HeightmapComponentOffsetY;
						// Copy the original data
						DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
					}
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy left-bottom height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX - Component->ComponentSizeQuads, Component->SectionBaseY + Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = Component->NumSubsections - 1;
			INT SubsectionY = 0;
			INT OrigSubsectionX = Component->NumSubsections - 1 - SubsectionX;
			INT OrigSubsectionY = Component->NumSubsections - 1 - SubsectionY;

			for ( INT SubY = 1; SubY < ExpandQuadsY+1; SubY++ )
			{
				for( INT SubX = (SubsectionSizeVerts-ExpandQuadsX-1); SubX < SubsectionSizeVerts-1; SubX++ )
				{
					FColor& DestData = OutData( (SubX - (SubsectionSizeVerts-ExpandQuadsX-1)) + (SubY-1 + (ExpandQuadsY+ComponentSizeVerts)) * ExpandSize );
					if (Neighbor && !bCopyBorderOnly)
					{
						// UV coordinates of the data offset into the texture
						INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
						INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
						DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
					}
					else 
					{
						INT OrigU = SubsectionSizeVerts * OrigSubsectionX	+ 0	+ HeightmapComponentOffsetX;
						INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ SubsectionSizeVerts-1 + HeightmapComponentOffsetY;
						// Copy the original data
						DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
					}
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy right-top height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX + Component->ComponentSizeQuads, Component->SectionBaseY - Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = 0;
			INT SubsectionY = Component->NumSubsections - 1;
			INT OrigSubsectionX = Component->NumSubsections - 1 - SubsectionX;
			INT OrigSubsectionY = Component->NumSubsections - 1 - SubsectionY;

			for ( INT SubY = SubsectionSizeVerts-ExpandQuadsY-1; SubY < SubsectionSizeVerts-1; SubY++ )
			{
				for( INT SubX = 1; SubX < ExpandQuadsX+1; SubX++ )
				{
					FColor& DestData = OutData( (SubX-1 + (ExpandQuadsX+ComponentSizeVerts)) + (SubY - (SubsectionSizeVerts-ExpandQuadsY-1)) * ExpandSize );
					if (Neighbor && !bCopyBorderOnly)
					{
						// UV coordinates of the data offset into the texture
						INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
						INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
						DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
					}
					else 
					{
						INT OrigU = SubsectionSizeVerts * OrigSubsectionX	+ SubsectionSizeVerts-1	+ HeightmapComponentOffsetX;
						INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ 0 + HeightmapComponentOffsetY;
						// Copy the original data
						DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
					}
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy left-top height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX - Component->ComponentSizeQuads, Component->SectionBaseY - Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = Component->NumSubsections - 1;
			INT SubsectionY = Component->NumSubsections - 1;
			INT OrigSubsectionX = Component->NumSubsections - 1 - SubsectionX;
			INT OrigSubsectionY = Component->NumSubsections - 1 - SubsectionY;

			for ( INT SubY = SubsectionSizeVerts-ExpandQuadsY-1; SubY < SubsectionSizeVerts-1; SubY++ )
			{
				for( INT SubX = SubsectionSizeVerts-ExpandQuadsX-1; SubX < SubsectionSizeVerts-1; SubX++ )
				{
					FColor& DestData = OutData( (SubX - (SubsectionSizeVerts-ExpandQuadsX-1)) + (SubY - (SubsectionSizeVerts-ExpandQuadsY-1)) * ExpandSize );
					if (Neighbor && !bCopyBorderOnly)
					{
						// UV coordinates of the data offset into the texture
						INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
						INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
						DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
					}
					else 
					{
						INT OrigU = SubsectionSizeVerts * OrigSubsectionX	+ 0	+ HeightmapComponentOffsetX;
						INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ 0 + HeightmapComponentOffsetY;
						// Copy the original data
						DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
					}				
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy middle-bottom height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX, Component->SectionBaseY + Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionY = 0; //Component->NumSubsections - 1;
			INT OrigSubsectionY = (Component->NumSubsections - 1) - SubsectionY;

			for( INT SubsectionX = 0;SubsectionX < Component->NumSubsections;SubsectionX++ )
			{
				for ( INT SubY = 1; SubY < ExpandQuadsY+1; SubY++ )
				{
					for( INT SubX = 0; SubX < SubsectionSizeVerts; SubX++ )
					{
						INT CompX = Component->SubsectionSizeQuads * SubsectionX + SubX;
						FColor& DestData = OutData( (CompX + ExpandQuadsX) + (SubY-1 + (ExpandQuadsY + ComponentSizeVerts)) * ExpandSize );
						if (Neighbor && !bCopyBorderOnly)
						{
							// UV coordinates of the data offset into the texture
							INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
							INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
							DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
						}
						else 
						{
							INT OrigU = SubsectionSizeVerts * SubsectionX		+ SubX					+ HeightmapComponentOffsetX;
							INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ SubsectionSizeVerts-1 + HeightmapComponentOffsetY;
							// Copy the original data
							DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
						}
					}
				}
			}

			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy middle-top height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX, Component->SectionBaseY - Component->ComponentSizeQuads));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionY = Component->NumSubsections - 1;
			INT OrigSubsectionY = (Component->NumSubsections - 1) - SubsectionY;

			for( INT SubsectionX = 0;SubsectionX < Component->NumSubsections;SubsectionX++ )
			{
				for ( INT SubY = (SubsectionSizeVerts-ExpandQuadsY-1); SubY < SubsectionSizeVerts-1; SubY++ )
				{
					for( INT SubX = 0; SubX < SubsectionSizeVerts; SubX++ )
					{
						INT CompX = Component->SubsectionSizeQuads * SubsectionX + SubX;
						FColor& DestData = OutData( (CompX + ExpandQuadsX) + (SubY - (SubsectionSizeVerts-ExpandQuadsY-1)) * ExpandSize );
						if (Neighbor && !bCopyBorderOnly)
						{
							// UV coordinates of the data offset into the texture
							INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
							INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
							DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
						}
						else 
						{
							INT OrigU = SubsectionSizeVerts * SubsectionX		+ SubX	+ HeightmapComponentOffsetX;
							INT OrigV = SubsectionSizeVerts * OrigSubsectionY	+ 0		+ HeightmapComponentOffsetY;
							// Copy the original data
							DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
						}
					}
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy right-middle height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX + Component->ComponentSizeQuads, Component->SectionBaseY));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = 0;
			INT OrigSubsectionX = (Component->NumSubsections - 1) - SubsectionX;

			for( INT SubsectionY = 0;SubsectionY < Component->NumSubsections;SubsectionY++ )
			{
				for ( INT SubY = 0; SubY < SubsectionSizeVerts; SubY++ )
				{
					INT CompY = Component->SubsectionSizeQuads * SubsectionY + SubY;
					for( INT SubX = 1; SubX < ExpandQuadsX+1; SubX++ )
					{
						FColor& DestData = OutData( (SubX-1 + ExpandQuadsX + ComponentSizeVerts) + (CompY + ExpandQuadsY) * ExpandSize );
						if (Neighbor && !bCopyBorderOnly)
						{
							// UV coordinates of the data offset into the texture
							INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
							INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
							DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
						}
						else 
						{
							INT OrigU = SubsectionSizeVerts * OrigSubsectionX		+ SubsectionSizeVerts-1		+ HeightmapComponentOffsetX;
							INT OrigV = SubsectionSizeVerts * SubsectionY			+ SubY						+ HeightmapComponentOffsetY;
							// Copy the original data
							DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
						}
					}
				}
			}
			if (Neighbor) DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}

		// copy left-middle height-map..
		{
			ULandscapeComponent* Neighbor = Component->GetLandscapeActor()->XYtoComponentMap.FindRef
				(ALandscape::MakeKey(Component->SectionBaseX - Component->ComponentSizeQuads, Component->SectionBaseY));

			FColor* NeighborHeightMap = NULL;
			if (Neighbor)
			{
				NeighborHeightMap = (FColor*)DataInterface->LockMip(Neighbor->HeightmapTexture,0);
				NeighborHeightmapStride = Neighbor->HeightmapTexture->SizeX;
				NeighborHeightmapComponentOffsetX = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeX * Neighbor->HeightmapScaleBias.Z );
				NeighborHeightmapComponentOffsetY = appRound( (FLOAT)Neighbor->HeightmapTexture->SizeY * Neighbor->HeightmapScaleBias.W );
			}

			INT SubsectionX = Component->NumSubsections - 1;
			INT OrigSubsectionX = (Component->NumSubsections - 1) - SubsectionX;

			for( INT SubsectionY = 0;SubsectionY < Component->NumSubsections;SubsectionY++ )
			{
				for ( INT SubY = 0; SubY < SubsectionSizeVerts; SubY++ )
				{
					INT CompY = Component->SubsectionSizeQuads * SubsectionY + SubY;
					for( INT SubX = SubsectionSizeVerts-ExpandQuadsX-1; SubX < SubsectionSizeVerts-1; SubX++ )
					{
						FColor& DestData = OutData( (SubX - (SubsectionSizeVerts-ExpandQuadsX-1)) + (CompY + ExpandQuadsY) * ExpandSize );
						if (Neighbor && !bCopyBorderOnly)
						{
							// UV coordinates of the data offset into the texture
							INT TexU = SubsectionSizeVerts * SubsectionX + SubX + NeighborHeightmapComponentOffsetX;
							INT TexV = SubsectionSizeVerts * SubsectionY + SubY + NeighborHeightmapComponentOffsetY;
							DestData = NeighborHeightMap[TexU + TexV * NeighborHeightmapStride];
						}
						else 
						{
							INT OrigU = SubsectionSizeVerts * OrigSubsectionX		+ 0		+ HeightmapComponentOffsetX;
							INT OrigV = SubsectionSizeVerts * SubsectionY			+ SubY	+ HeightmapComponentOffsetY;
							// Copy the original data
							DestData = HeightMipData[OrigU + OrigV * HeightmapStride];
						}
					}
				}
			}
			if (Neighbor)  DataInterface->UnlockMip(Neighbor->HeightmapTexture, 0);
		}
	}

	FColor* GetHeightData( INT LocalX, INT LocalY ) const
	{
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		check(Component);
		check(LocalX >=0 && LocalY >=0 && LocalX < Component->ComponentSizeQuads+1 && LocalY < Component->ComponentSizeQuads+1);
#endif

		INT SubNumX;
		INT SubNumY;
		INT SubX;
		INT SubY;
		ComponentXYToSubsectionXY(LocalX, LocalY, SubNumX, SubNumY, SubX, SubY );
		
		return &HeightMipData[SubX + SubNumX*SubsectionSizeVerts + HeightmapComponentOffsetX + (SubY+SubNumY*SubsectionSizeVerts+HeightmapComponentOffsetY)*HeightmapStride];
	}

	WORD GetHeight( INT LocalX, INT LocalY ) const
	{
		FColor* Texel = GetHeightData(LocalX, LocalY);
		return (Texel->R << 8) + Texel->G;
	}

	WORD GetHeight( INT VertexIndex ) const
	{
		INT X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		return GetHeight( X, Y );
	}

	FVector GetWorldVertex( INT LocalX, INT LocalY ) const
	{
		return Component->LocalToWorld.TransformFVector( FVector( LocalX, LocalY, ((FLOAT)GetHeight(LocalX, LocalY) - 32768.f) * LANDSCAPE_ZSCALE ) );			
	}

	void GetWorldTangentVectors( INT LocalX, INT LocalY, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		FColor* Data = GetHeightData( LocalX, LocalY );
		WorldTangentZ.X = 2.f * (FLOAT)Data->B / 255.f - 1.f;
		WorldTangentZ.Y = 2.f * (FLOAT)Data->A / 255.f - 1.f;
		WorldTangentZ.Z = appSqrt(1.f - (Square(WorldTangentZ.X)+Square(WorldTangentZ.Y)));
		WorldTangentX = FVector(-WorldTangentZ.Z, 0.f, WorldTangentZ.X);
		WorldTangentY = FVector(0.f, WorldTangentZ.Z, -WorldTangentZ.Y);
		// Assume there is no rotation, so we don't need to do any LocalToWorld.
	}

	void GetWorldPositionTangents( INT LocalX, INT LocalY, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		FColor* Data = GetHeightData( LocalX, LocalY );

		WorldTangentZ.X = 2.f * (FLOAT)Data->B / 255.f - 1.f;
		WorldTangentZ.Y = 2.f * (FLOAT)Data->A / 255.f - 1.f;
		WorldTangentZ.Z = appSqrt(1.f - (Square(WorldTangentZ.X)+Square(WorldTangentZ.Y)));
		WorldTangentX = FVector(WorldTangentZ.Z, 0.f, -WorldTangentZ.X);
		WorldTangentY = WorldTangentZ ^ WorldTangentX;
		//WorldTangentX = FVector(-WorldTangentZ.Z, 0.f, WorldTangentZ.X);
		//WorldTangentY = FVector(0.f, WorldTangentZ.Z, -WorldTangentZ.Y);
		// Assume there is no rotation, so we don't need to do any LocalToWorld.

		//WORD Height = (Data->R << 8) + Data->G;
		INT ClampedLocalX = Clamp(LocalX-ExpandQuadsX, 0, Component->ComponentSizeQuads);
		INT ClampedLocalY = Clamp(LocalY-ExpandQuadsY, 0, Component->ComponentSizeQuads);

		const FColor* Data2 = GetHeightData( ClampedLocalX+ExpandQuadsX, ClampedLocalY+ExpandQuadsY );
		WORD Height = (Data2->R << 8) + Data2->G;

		WorldPos = Component->LocalToWorld.TransformFVector( FVector( ClampedLocalX, ClampedLocalY, ((FLOAT)Height - 32768.f) * LANDSCAPE_ZSCALE ) );			
	}

	FVector GetWorldVertex( INT VertexIndex ) const
	{
		INT X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		return GetWorldVertex( X, Y );
	}

	void GetWorldTangentVectors( INT VertexIndex, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		INT X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetWorldTangentVectors( X, Y, WorldTangentX, WorldTangentY, WorldTangentZ );
	}

	void GetWorldPositionTangents( INT VertexIndex, FVector& WorldPos, FVector& WorldTangentX, FVector& WorldTangentY, FVector& WorldTangentZ ) const
	{
		INT X, Y;
		VertexIndexToXY( VertexIndex, X, Y );
		GetWorldPositionTangents( X, Y, WorldPos, WorldTangentX, WorldTangentY, WorldTangentZ );
	}

	void GetTriangleIndices(INT QuadX,INT QuadY,INT TriNum,INT& OutI0,INT& OutI1,INT& OutI2) const
	{
		switch(TriNum)
		{
		case 0:
			OutI0 = (QuadX + 0) + (QuadY + 0) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI1 = (QuadX + 1) + (QuadY + 1) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI2 = (QuadX + 1) + (QuadY + 0) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			break;
		case 1:
			OutI0 = (QuadX + 0) + (QuadY + 0) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI1 = (QuadX + 0) + (QuadY + 1) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			OutI2 = (QuadX + 1) + (QuadY + 1) * (Component->ComponentSizeQuads+1 + 2*ExpandQuadsX);
			break;
#if LANDSCAPE_VALIDATE_DATA_ACCESS
		default:
			check(TriNum==0||TriNum==1);
			break;
#endif
		}
	}

	void GetTriangleIndices(INT QuadIndex,INT TriNum,INT& OutI0,INT& OutI1,INT& OutI2) const
	{
		INT X, Y;
		QuadIndexToXY( QuadIndex, X, Y );
		GetTriangleIndices(X, Y, TriNum, OutI0, OutI1, OutI2);
	}

private:
	struct FLandscapeDataInterface* DataInterface;
	ULandscapeComponent* Component;

	// offset of this component's data into heightmap texture
	INT HeightmapStride;
	INT HeightmapComponentOffsetX;
	INT HeightmapComponentOffsetY;
	INT HeightmapSubsectionOffset;
	FColor* HeightMipData;

//	INT WeightmapStride;
//	INT WeightmapOffsetX;
//	INT WeightmapOffsetY;

	INT SubsectionSizeVerts;

public:
	// for expension.... is this right position -0-?
	INT ExpandQuadsX;
	INT ExpandQuadsY;
	INT SizeX;
	INT SizeY;
};

#endif // WITH_EDITOR

#endif // _LANDSCAPEDATAACCESS_H
