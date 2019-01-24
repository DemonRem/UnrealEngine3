/*=============================================================================
LandscapeEdit.cpp: Landscape editing
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "EnginePhysicsClasses.h"
#include "UnTerrain.h"
#include "LandscapeDataAccess.h"
#include "LandscapeEdit.h"
#include "LandscapeRender.h"

#if WITH_EDITOR

//
// ULandscapeComponent
//
void ULandscapeComponent::Init(INT InBaseX,INT InBaseY,INT InComponentSizeQuads, INT InNumSubsections,INT InSubsectionSizeQuads)
{
	SectionBaseX = InBaseX;
	SectionBaseY = InBaseY;
	ComponentSizeQuads = InComponentSizeQuads;
	NumSubsections = InNumSubsections;
	SubsectionSizeQuads = InSubsectionSizeQuads;
	check(NumSubsections * SubsectionSizeQuads == ComponentSizeQuads);
	GetLandscapeActor()->XYtoComponentMap.Set(ALandscape::MakeKey(SectionBaseX,SectionBaseY), this);
}

void ULandscapeComponent::UpdateCachedBounds()
{
	FLandscapeComponentDataInterface CDI(this);

	FBox Box(0);
	for( INT y=0;y<ComponentSizeQuads+1;y++ )
	{
		for( INT x=0;x<ComponentSizeQuads+1;x++ )
		{
			Box += CDI.GetWorldVertex(x,y);
		}
	}
	CachedBoxSphereBounds = FBoxSphereBounds(Box);

	// Update collision component bounds
	ALandscape* LandscapeActor = GetLandscapeActor();
	QWORD ComponentKey = ALandscape::MakeKey(SectionBaseX,SectionBaseY);
	ULandscapeHeightfieldCollisionComponent* CollisionComponent = LandscapeActor->XYtoCollisionComponentMap.FindRef(ComponentKey);
	if( CollisionComponent )
	{
		CollisionComponent->Modify();
		CollisionComponent->CachedBoxSphereBounds = CachedBoxSphereBounds;	
		CollisionComponent->ConditionalUpdateTransform();
	}
}

void ULandscapeComponent::UpdateMaterialInstances()
{
	check(GIsEditor);

	ALandscape* Landscape = GetLandscapeActor();
	
	if( Landscape->LandscapeMaterial != NULL )
	{
		FString LayerKey = GetLayerAllocationKey();

		// debugf(TEXT("Looking for key %s"), *LayerKey);

		// Find or set a matching MIC in the Landscape's map.
		UMaterialInstanceConstant* CombinationMaterialInstance = Landscape->MaterialInstanceConstantMap.FindRef(*LayerKey);
		if( CombinationMaterialInstance == NULL || CombinationMaterialInstance->Parent != Landscape->LandscapeMaterial )
		{
			CombinationMaterialInstance = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), GetOutermost(), FName(*FString::Printf(TEXT("%s_%s"),*Landscape->GetName(),*LayerKey)), RF_Public);
			debugf(TEXT("Looking for key %s, making new combination %s"), *LayerKey, *CombinationMaterialInstance->GetName());
			Landscape->MaterialInstanceConstantMap.Set(*LayerKey,CombinationMaterialInstance);
			UBOOL bNeedsRecompile;
			CombinationMaterialInstance->SetParent(Landscape->LandscapeMaterial);

			UMaterial* ParentUMaterial = CombinationMaterialInstance->GetMaterial();

			if( ParentUMaterial && ParentUMaterial != GEngine->DefaultMaterial )
			{
				ParentUMaterial->SetMaterialUsage(bNeedsRecompile,MATUSAGE_Landscape);
				ParentUMaterial->SetMaterialUsage(bNeedsRecompile,MATUSAGE_StaticLighting);
			}

			FStaticParameterSet StaticParameters;
			CombinationMaterialInstance->GetStaticParameterValues(&StaticParameters);

			// Find weightmap mapping for each layer parameter, or disable if the layer is not used in this component.
			for( INT LayerParameterIdx=0;LayerParameterIdx<StaticParameters.TerrainLayerWeightParameters.Num();LayerParameterIdx++ )
			{
				FStaticTerrainLayerWeightParameter& LayerParameter = StaticParameters.TerrainLayerWeightParameters(LayerParameterIdx);
				LayerParameter.WeightmapIndex = INDEX_NONE;

				// Look through our allocations to see if we need this layer.
				// If not found, this component doesn't use the layer, and WeightmapIndex remains as INDEX_NONE.
				for( INT AllocIdx=0;AllocIdx<WeightmapLayerAllocations.Num();AllocIdx++ )
				{
					FWeightmapLayerAllocationInfo& Allocation = WeightmapLayerAllocations(AllocIdx);
					if( Allocation.LayerName == LayerParameter.ParameterName )
					{
						LayerParameter.WeightmapIndex = Allocation.WeightmapTextureIndex;
						LayerParameter.bOverride = TRUE;
						// debugf(TEXT(" Layer %s channel %d"), *LayerParameter.ParameterName.ToString(), LayerParameter.WeightmapIndex);
						break;
					}
				}
			}

			if (CombinationMaterialInstance->SetStaticParameterValues(&StaticParameters))
			{
				//mark the package dirty if a compile was needed
				CombinationMaterialInstance->MarkPackageDirty();
			}

			CombinationMaterialInstance->InitResources();
			CombinationMaterialInstance->UpdateStaticPermutation();
		}
		else
		{
			// debugf(TEXT("Looking for key %s, found combination %s"), *LayerKey, *CombinationMaterialInstance->GetName());
		}

		// Create the instance for this component, that will use the layer combination instance.
		if( MaterialInstance == NULL )
		{
			MaterialInstance = ConstructObject<UMaterialInstanceConstant>(UMaterialInstanceConstant::StaticClass(), GetOutermost(), FName(*FString::Printf(TEXT("%s_Inst"),*GetName())), RF_Public);
		}

		// For undo
		MaterialInstance->SetFlags(RF_Transactional);
		MaterialInstance->Modify();

		MaterialInstance->SetParent(CombinationMaterialInstance);

		FLinearColor Masks[4];
		Masks[0] = FLinearColor(1.f,0.f,0.f,0.f);
		Masks[1] = FLinearColor(0.f,1.f,0.f,0.f);
		Masks[2] = FLinearColor(0.f,0.f,1.f,0.f);
		Masks[3] = FLinearColor(0.f,0.f,0.f,1.f);

		// Set the layer mask
		for( INT AllocIdx=0;AllocIdx<WeightmapLayerAllocations.Num();AllocIdx++ )
		{
			FWeightmapLayerAllocationInfo& Allocation = WeightmapLayerAllocations(AllocIdx);
			MaterialInstance->SetVectorParameterValue(FName(*FString::Printf(TEXT("LayerMask_%s"),*Allocation.LayerName.ToString())), Masks[Allocation.WeightmapTextureChannel]);
		}

		// Set the weightmaps
		for( INT i=0;i<WeightmapTextures.Num();i++ )
		{
			// debugf(TEXT("Setting Weightmap%d = %s"), i, *WeightmapTextures(i)->GetName());
			MaterialInstance->SetTextureParameterValue(FName(*FString::Printf(TEXT("Weightmap%d"),i)), WeightmapTextures(i));
		}
		// Set the heightmap, if needed.
		MaterialInstance->SetTextureParameterValue(FName(TEXT("Heightmap")), HeightmapTexture);
	}
}

//
// LandscapeComponentAlphaInfo
//
struct FLandscapeComponentAlphaInfo
{
	INT LayerIndex;
	TArrayNoInit<BYTE> AlphaValues;

	// tor
	FLandscapeComponentAlphaInfo( ULandscapeComponent* InOwner, INT InLayerIndex )
	:	LayerIndex(InLayerIndex)
	,	AlphaValues(E_ForceInit)
	{
		AlphaValues.Empty(Square(InOwner->ComponentSizeQuads+1));
		AlphaValues.AddZeroed(Square(InOwner->ComponentSizeQuads+1));
	}

	UBOOL IsLayerAllZero() const
	{
		for( INT Index=0;Index<AlphaValues.Num();Index++ )
		{
			if( AlphaValues(Index) != 0 )
			{
				return FALSE;
			}
		}
		return TRUE;
	}
};

/**
 * Creates a collision component
 * @param HeightmapTextureMipData: heightmap data
 * @param ComponentX1, ComponentY1, ComponentX2, ComponentY2: region to update
 */
void ULandscapeComponent::UpdateCollisionComponent(FColor* HeightmapTextureMipData, INT ComponentX1, INT ComponentY1, INT ComponentX2, INT ComponentY2, UBOOL bUpdateBounds )
{
	ALandscape* LandscapeActor = GetLandscapeActor();
	QWORD ComponentKey = ALandscape::MakeKey(SectionBaseX,SectionBaseY);
	ULandscapeHeightfieldCollisionComponent* CollisionComponent = LandscapeActor->XYtoCollisionComponentMap.FindRef(ComponentKey);

	INT CollisionSubsectionSizeVerts = ((SubsectionSizeQuads+1)>>CollisionMipLevel);
	INT CollisionSubsectionSizeQuads = CollisionSubsectionSizeVerts-1;
	INT CollisionSizeVerts = NumSubsections*CollisionSubsectionSizeQuads+1;

	WORD* CollisionHeightData = NULL;
	UBOOL CreatedNew = FALSE;
	if( CollisionComponent )
	{
		CollisionComponent->Modify();
		if( bUpdateBounds )
		{
			CollisionComponent->CachedBoxSphereBounds = CachedBoxSphereBounds;	
			CollisionComponent->ConditionalUpdateTransform();
		}

		CollisionHeightData = (WORD*)CollisionComponent->CollisionHeightData.Lock(LOCK_READ_WRITE);
	}
	else
	{
		ComponentX1 = 0;
		ComponentY1 = 0;
		ComponentX2 = MAXINT;
		ComponentY2 = MAXINT;

		CollisionComponent = ConstructObject<ULandscapeHeightfieldCollisionComponent>(ULandscapeHeightfieldCollisionComponent::StaticClass(),GetLandscapeActor(),NAME_None,RF_Transactional);
		LandscapeActor->CollisionComponents.AddItem(CollisionComponent);
		LandscapeActor->XYtoCollisionComponentMap.Set(ComponentKey, CollisionComponent);

		CollisionComponent->SectionBaseX = SectionBaseX;
		CollisionComponent->SectionBaseY = SectionBaseY;
		CollisionComponent->CollisionSizeQuads = CollisionSubsectionSizeQuads * NumSubsections;
		CollisionComponent->CollisionScale = (FLOAT)(ComponentSizeQuads+1) / (FLOAT)(CollisionComponent->CollisionSizeQuads+1);
		CollisionComponent->CachedBoxSphereBounds = CachedBoxSphereBounds;
		CreatedNew = TRUE;

		// Reallocate raw collision data
		CollisionComponent->CollisionHeightData.Lock(LOCK_READ_WRITE);
		CollisionHeightData = (WORD*)CollisionComponent->CollisionHeightData.Realloc(Square(CollisionSizeVerts));
		appMemzero( CollisionHeightData,sizeof(WORD)*Square(CollisionSizeVerts));
	}

	INT HeightmapSizeU = HeightmapTexture->SizeX;
	INT HeightmapSizeV = HeightmapTexture->SizeY;
	INT MipSizeU = HeightmapSizeU >> CollisionMipLevel;
	INT MipSizeV = HeightmapSizeV >> CollisionMipLevel;

	// Ratio to convert update region coordinate to collision mip coordinates
	FLOAT CollisionQuadRatio = (FLOAT)CollisionSubsectionSizeQuads / (FLOAT)SubsectionSizeQuads;

	// XY offset into heightmap mip data
	INT HeightmapOffsetX = appRound(HeightmapScaleBias.Z * (FLOAT)HeightmapSizeU) >> CollisionMipLevel;
	INT HeightmapOffsetY = appRound(HeightmapScaleBias.W * (FLOAT)HeightmapSizeV) >> CollisionMipLevel;

	for( INT SubsectionY = 0;SubsectionY < NumSubsections;SubsectionY++ )
	{
		// Check if subsection is fully above or below the area we are interested in
		if( (ComponentY2 < SubsectionSizeQuads*SubsectionY) ||		// above
			(ComponentY1 > SubsectionSizeQuads*(SubsectionY+1)) )	// below
		{
			continue;
		}

		for( INT SubsectionX = 0;SubsectionX < NumSubsections;SubsectionX++ )
		{
			// Check if subsection is fully to the left or right of the area we are interested in
			if( (ComponentX2 < SubsectionSizeQuads*SubsectionX) ||		// left
				(ComponentX1 > SubsectionSizeQuads*(SubsectionX+1)) )	// right
			{
				continue;
			}

			// Area to update in subsection coordinates
			INT SubX1 = ComponentX1 - SubsectionSizeQuads*SubsectionX;
			INT SubY1 = ComponentY1 - SubsectionSizeQuads*SubsectionY;
			INT SubX2 = ComponentX2 - SubsectionSizeQuads*SubsectionX;
			INT SubY2 = ComponentY2 - SubsectionSizeQuads*SubsectionY;

			// Area to update in collision mip level coords
			INT CollisionSubX1 = appFloor( (FLOAT)SubX1 * CollisionQuadRatio );
			INT CollisionSubY1 = appFloor( (FLOAT)SubY1 * CollisionQuadRatio );
			INT CollisionSubX2 = appCeil(  (FLOAT)SubX2 * CollisionQuadRatio );
			INT CollisionSubY2 = appCeil(  (FLOAT)SubY2 * CollisionQuadRatio );

			// Clamp area to update
			INT VertX1 = Clamp<INT>(CollisionSubX1, 0, CollisionSubsectionSizeQuads);
			INT VertY1 = Clamp<INT>(CollisionSubY1, 0, CollisionSubsectionSizeQuads);
			INT VertX2 = Clamp<INT>(CollisionSubX2, 0, CollisionSubsectionSizeQuads);
			INT VertY2 = Clamp<INT>(CollisionSubY2, 0, CollisionSubsectionSizeQuads);

			for( INT VertY=VertY1;VertY<=VertY2;VertY++ )
			{
				for( INT VertX=VertX1;VertX<=VertX2;VertX++ )
				{
					// X/Y of the vertex we're looking indexed into the texture data
					INT TexX = HeightmapOffsetX + CollisionSubsectionSizeVerts * SubsectionX + VertX;
					INT TexY = HeightmapOffsetY + CollisionSubsectionSizeVerts * SubsectionY + VertY;
					FColor& TexData = HeightmapTextureMipData[ TexX + TexY * MipSizeU ];
			
					// this uses Quads as we don't want the duplicated vertices
					INT CompVertX = CollisionSubsectionSizeQuads * SubsectionX + VertX;
					INT CompVertY = CollisionSubsectionSizeQuads * SubsectionY + VertY;

					// Copy collision data
					WORD& CollisionHeight = CollisionHeightData[CompVertX+CompVertY*CollisionSizeVerts];
					WORD NewHeight = TexData.R<<8 | TexData.G;
					
					CollisionHeight = NewHeight;
				}
			}
		}
	}
	CollisionComponent->CollisionHeightData.Unlock();

	// If we updated an existing component, we need to reinitialize the physics
	if( !CreatedNew )
	{
		CollisionComponent->RecreateHeightfield();
	}
}

/**
* Generate mipmaps for height and tangent data.
* @param HeightmapTextureMipData - array of pointers to the locked mip data. 
*           This should only include the mips that are generated directly from this component's data
*           ie where each subsection has at least 2 vertices.
* @param ComponentX1 - region of texture to update in component space
* @param ComponentY1 - region of texture to update in component space
* @param ComponentX2 (optional) - region of texture to update in component space
* @param ComponentY2 (optional) - region of texture to update in component space
* @param TextureDataInfo - FLandscapeTextureDataInfo pointer, to notify of the mip data region updated.
*/
void ULandscapeComponent::GenerateHeightmapMips( TArray<FColor*>& HeightmapTextureMipData, INT ComponentX1/*=0*/, INT ComponentY1/*=0*/, INT ComponentX2/*=MAXINT*/, INT ComponentY2/*=MAXINT*/, struct FLandscapeTextureDataInfo* TextureDataInfo/*=NULL*/ )
{
	if( ComponentX2==MAXINT )
	{
		ComponentX2 = ComponentSizeQuads;
	}
	if( ComponentY2==MAXINT )
	{
		ComponentY2 = ComponentSizeQuads;
	}

	ALandscape* LandscapeActor = GetLandscapeActor();
	INT HeightmapSizeU = HeightmapTexture->SizeX;
	INT HeightmapSizeV = HeightmapTexture->SizeY;

	INT HeightmapOffsetX = appRound(HeightmapScaleBias.Z * (FLOAT)HeightmapSizeU);
	INT HeightmapOffsetY = appRound(HeightmapScaleBias.W * (FLOAT)HeightmapSizeV);

	for( INT SubsectionY = 0;SubsectionY < NumSubsections;SubsectionY++ )
	{
		// Check if subsection is fully above or below the area we are interested in
		if( (ComponentY2 < SubsectionSizeQuads*SubsectionY) ||		// above
			(ComponentY1 > SubsectionSizeQuads*(SubsectionY+1)) )	// below
		{
			continue;
		}

		for( INT SubsectionX = 0;SubsectionX < NumSubsections;SubsectionX++ )
		{
			// Check if subsection is fully to the left or right of the area we are interested in
			if( (ComponentX2 < SubsectionSizeQuads*SubsectionX) ||		// left
				(ComponentX1 > SubsectionSizeQuads*(SubsectionX+1)) )	// right
			{
				continue;
			}

			// Area to update in previous mip level coords
			INT PrevMipSubX1 = ComponentX1 - SubsectionSizeQuads*SubsectionX;
			INT PrevMipSubY1 = ComponentY1 - SubsectionSizeQuads*SubsectionY;
			INT PrevMipSubX2 = ComponentX2 - SubsectionSizeQuads*SubsectionX;
			INT PrevMipSubY2 = ComponentY2 - SubsectionSizeQuads*SubsectionY;

			INT PrevMipSubsectionSizeQuads = SubsectionSizeQuads;
			FLOAT InvPrevMipSubsectionSizeQuads = 1.f / (FLOAT)SubsectionSizeQuads;

			INT PrevMipSizeU = HeightmapSizeU;
			INT PrevMipSizeV = HeightmapSizeV;

			INT PrevMipHeightmapOffsetX = HeightmapOffsetX;
			INT PrevMipHeightmapOffsetY = HeightmapOffsetY;

			for( INT Mip=1;Mip<HeightmapTextureMipData.Num();Mip++ )
			{
				INT MipSizeU = HeightmapSizeU >> Mip;
				INT MipSizeV = HeightmapSizeV >> Mip;

				INT MipSubsectionSizeQuads = ((SubsectionSizeQuads+1)>>Mip)-1;
				FLOAT InvMipSubsectionSizeQuads = 1.f / (FLOAT)MipSubsectionSizeQuads;

				INT MipHeightmapOffsetX = HeightmapOffsetX>>Mip;
				INT MipHeightmapOffsetY = HeightmapOffsetY>>Mip;

				// Area to update in current mip level coords
				INT MipSubX1 = appFloor( (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubX1 * InvPrevMipSubsectionSizeQuads );
				INT MipSubY1 = appFloor( (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubY1 * InvPrevMipSubsectionSizeQuads );
				INT MipSubX2 = appCeil(  (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubX2 * InvPrevMipSubsectionSizeQuads );
				INT MipSubY2 = appCeil(  (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubY2 * InvPrevMipSubsectionSizeQuads );

				// Clamp area to update
				INT VertX1 = Clamp<INT>(MipSubX1, 0, MipSubsectionSizeQuads);
				INT VertY1 = Clamp<INT>(MipSubY1, 0, MipSubsectionSizeQuads);
				INT VertX2 = Clamp<INT>(MipSubX2, 0, MipSubsectionSizeQuads);
				INT VertY2 = Clamp<INT>(MipSubY2, 0, MipSubsectionSizeQuads);

				for( INT VertY=VertY1;VertY<=VertY2;VertY++ )
				{
					for( INT VertX=VertX1;VertX<=VertX2;VertX++ )
					{
						// Convert VertX/Y into previous mip's coords
						FLOAT PrevMipVertX = (FLOAT)PrevMipSubsectionSizeQuads * (FLOAT)VertX * InvMipSubsectionSizeQuads;
						FLOAT PrevMipVertY = (FLOAT)PrevMipSubsectionSizeQuads * (FLOAT)VertY * InvMipSubsectionSizeQuads;

#if 0
						// Validate that the vertex we skip wouldn't use the updated data in the parent mip.
						// Note this validation is doesn't do anything unless you change the VertY/VertX loops 
						// above to process all verts from 0 .. MipSubsectionSizeQuads.
						if( VertX < VertX1 || VertX > VertX2 )
						{
							check( appCeil(PrevMipVertX) < PrevMipSubX1 || appFloor(PrevMipVertX) > PrevMipSubX2 );
							continue;
						}

						if( VertY < VertY1 || VertY > VertY2 )
						{
							check( appCeil(PrevMipVertY) < PrevMipSubY1 || appFloor(PrevMipVertY) > PrevMipSubY2 );
							continue;
						}
#endif

						// X/Y of the vertex we're looking indexed into the texture data
						INT TexX = (MipHeightmapOffsetX) + (MipSubsectionSizeQuads+1) * SubsectionX + VertX;
						INT TexY = (MipHeightmapOffsetY) + (MipSubsectionSizeQuads+1) * SubsectionY + VertY;

						FLOAT fPrevMipTexX = (FLOAT)(PrevMipHeightmapOffsetX) + (FLOAT)((PrevMipSubsectionSizeQuads+1) * SubsectionX) + PrevMipVertX;
						FLOAT fPrevMipTexY = (FLOAT)(PrevMipHeightmapOffsetY) + (FLOAT)((PrevMipSubsectionSizeQuads+1) * SubsectionY) + PrevMipVertY;

						INT PrevMipTexX = appFloor(fPrevMipTexX);
						FLOAT fPrevMipTexFracX = appFractional(fPrevMipTexX);
						INT PrevMipTexY = appFloor(fPrevMipTexY);
						FLOAT fPrevMipTexFracY = appFractional(fPrevMipTexY);

						checkSlow( TexX >= 0 && TexX < MipSizeU );
						checkSlow( TexY >= 0 && TexY < MipSizeV );
						checkSlow( PrevMipTexX >= 0 && PrevMipTexX < PrevMipSizeU );
						checkSlow( PrevMipTexY >= 0 && PrevMipTexY < PrevMipSizeV );

						INT PrevMipTexX1 = Min<INT>( PrevMipTexX+1, PrevMipSizeU-1 );
						INT PrevMipTexY1 = Min<INT>( PrevMipTexY+1, PrevMipSizeV-1 );

						FColor* TexData = &(HeightmapTextureMipData(Mip))[ TexX + TexY * MipSizeU ];
						FColor *PreMipTexData00 = &(HeightmapTextureMipData(Mip-1))[ PrevMipTexX + PrevMipTexY * PrevMipSizeU ];
						FColor *PreMipTexData01 = &(HeightmapTextureMipData(Mip-1))[ PrevMipTexX + PrevMipTexY1 * PrevMipSizeU ];
						FColor *PreMipTexData10 = &(HeightmapTextureMipData(Mip-1))[ PrevMipTexX1 + PrevMipTexY * PrevMipSizeU ];
						FColor *PreMipTexData11 = &(HeightmapTextureMipData(Mip-1))[ PrevMipTexX1 + PrevMipTexY1 * PrevMipSizeU ];

						// Lerp height values
						WORD PrevMipHeightValue00 = PreMipTexData00->R << 8 | PreMipTexData00->G;
						WORD PrevMipHeightValue01 = PreMipTexData01->R << 8 | PreMipTexData01->G;
						WORD PrevMipHeightValue10 = PreMipTexData10->R << 8 | PreMipTexData10->G;
						WORD PrevMipHeightValue11 = PreMipTexData11->R << 8 | PreMipTexData11->G;
						WORD HeightValue = appRound(
							Lerp(
								Lerp( (FLOAT)PrevMipHeightValue00, (FLOAT)PrevMipHeightValue10, fPrevMipTexFracX),
								Lerp( (FLOAT)PrevMipHeightValue01, (FLOAT)PrevMipHeightValue11, fPrevMipTexFracX),
							fPrevMipTexFracY) );

						TexData->R = HeightValue >> 8;
						TexData->G = HeightValue & 255;

						// Lerp tangents
						TexData->B = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->B, (FLOAT)PreMipTexData10->B, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->B, (FLOAT)PreMipTexData11->B, fPrevMipTexFracX),
							fPrevMipTexFracY) );

						TexData->A = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->A, (FLOAT)PreMipTexData10->A, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->A, (FLOAT)PreMipTexData11->A, fPrevMipTexFracX),
							fPrevMipTexFracY) );
					}
				}

				// Record the areas we updated
				if( TextureDataInfo )
				{
					INT TexX1 = (MipHeightmapOffsetX) + (MipSubsectionSizeQuads+1) * SubsectionX + VertX1;
					INT TexY1 = (MipHeightmapOffsetY) + (MipSubsectionSizeQuads+1) * SubsectionY + VertY1;
					INT TexX2 = (MipHeightmapOffsetX) + (MipSubsectionSizeQuads+1) * SubsectionX + VertX2;
					INT TexY2 = (MipHeightmapOffsetY) + (MipSubsectionSizeQuads+1) * SubsectionY + VertY2;
					TextureDataInfo->AddMipUpdateRegion(Mip,TexX1,TexY1,TexX2,TexY2);
				}

				// Copy current mip values to prev as we move to the next mip.
				PrevMipSubsectionSizeQuads = MipSubsectionSizeQuads;
				InvPrevMipSubsectionSizeQuads = InvMipSubsectionSizeQuads;

				PrevMipSizeU = MipSizeU;
				PrevMipSizeV = MipSizeV;

				PrevMipHeightmapOffsetX = MipHeightmapOffsetX;
				PrevMipHeightmapOffsetY = MipHeightmapOffsetY;

				// Use this mip's area as we move to the next mip
				PrevMipSubX1 = MipSubX1;
				PrevMipSubY1 = MipSubY1;
				PrevMipSubX2 = MipSubX2;
				PrevMipSubY2 = MipSubY2;
			}
		}
	}
}

void ULandscapeComponent::CreateEmptyWeightmapMips(UTexture2D* WeightmapTexture)
{
	// Remove any existing mips.
	WeightmapTexture->Mips.Remove(1, WeightmapTexture->Mips.Num()-1);

	INT WeightmapSizeU = WeightmapTexture->SizeX;
	INT WeightmapSizeV = WeightmapTexture->SizeY;

	INT Mip = 1;
	for(;;)
	{
		INT MipSizeU = Max<INT>(WeightmapSizeU >> Mip,1);
		INT MipSizeV = Max<INT>(WeightmapSizeV >> Mip,1);

		// Allocate the mipmap
		FTexture2DMipMap* WeightMipMap = new(WeightmapTexture->Mips) FTexture2DMipMap;
		WeightMipMap->SizeX = MipSizeU;
		WeightMipMap->SizeY = MipSizeV;
		WeightMipMap->Data.Lock(LOCK_READ_WRITE);
		WeightMipMap->Data.Realloc(MipSizeU*MipSizeV*sizeof(FColor));
		WeightMipMap->Data.Unlock();

		if( MipSizeU == 1 && MipSizeV == 1 )
		{
			break;
		}

		Mip++;
	}
}

void ULandscapeComponent::GenerateWeightmapMips(INT InNumSubsections, INT InSubsectionSizeQuads, UTexture2D* WeightmapTexture, FColor* BaseMipData)
{
	// Remove any existing mips.
	WeightmapTexture->Mips.Remove(1, WeightmapTexture->Mips.Num()-1);

	// Stores pointers to the locked mip data
	TArray<FColor*> WeightmapTextureMipData;

	// Add the first mip's data
	WeightmapTextureMipData.AddItem( BaseMipData );

	INT WeightmapSizeU = WeightmapTexture->SizeX;
	INT WeightmapSizeV = WeightmapTexture->SizeY;

	INT Mip = 1;
	for(;;)
	{
		INT MipSizeU = Max<INT>(WeightmapSizeU >> Mip,1);
		INT MipSizeV = Max<INT>(WeightmapSizeV >> Mip,1);

		// Create the mipmap
		FTexture2DMipMap* WeightMipMap = new(WeightmapTexture->Mips) FTexture2DMipMap;
		WeightMipMap->SizeX = MipSizeU;
		WeightMipMap->SizeY = MipSizeV;
		WeightMipMap->Data.Lock(LOCK_READ_WRITE);
		WeightmapTextureMipData.AddItem( (FColor*)WeightMipMap->Data.Realloc(MipSizeU*MipSizeV*sizeof(FColor)) );

		if( MipSizeU == 1 && MipSizeV == 1 )
		{
			break;
		}

		Mip++;
	}

	// Update the newly created mips
	UpdateWeightmapMips( InNumSubsections, InSubsectionSizeQuads, WeightmapTexture, WeightmapTextureMipData );

	// Unlock all the new mips, but not the base mip's data
	for( INT i=1;i<WeightmapTextureMipData.Num();i++ )
	{
		WeightmapTexture->Mips(i).Data.Unlock();
	}
}

void ULandscapeComponent::UpdateWeightmapMips(INT InNumSubsections, INT InSubsectionSizeQuads, UTexture2D* WeightmapTexture, TArray<FColor*>& WeightmapTextureMipData, INT ComponentX1/*=0*/, INT ComponentY1/*=0*/, INT ComponentX2/*=MAXINT*/, INT ComponentY2/*=MAXINT*/, struct FLandscapeTextureDataInfo* TextureDataInfo/*=NULL*/)
{
	INT WeightmapSizeU = WeightmapTexture->SizeX;
	INT WeightmapSizeV = WeightmapTexture->SizeY;

	// Find the maximum mip where each texel's data comes from just one subsection.
	INT MaxWholeSubsectionMip = 1;
	INT Mip=1;
	for(;;)
	{
		INT MipSubsectionSizeQuads = ((InSubsectionSizeQuads+1)>>Mip)-1;

		INT MipSizeU = Max<INT>(WeightmapSizeU >> Mip,1);
		INT MipSizeV = Max<INT>(WeightmapSizeV >> Mip,1);

		// Mip must represent at least one quad to store valid weight data
		if( MipSubsectionSizeQuads > 0 )
		{
			MaxWholeSubsectionMip = Mip;
		}
		else
		{
			break;
		}

		Mip++;
	}

	// Update the mip where each texel's data comes from just one subsection.
	for( INT SubsectionY = 0;SubsectionY < InNumSubsections;SubsectionY++ )
	{
		// Check if subsection is fully above or below the area we are interested in
		if( (ComponentY2 < InSubsectionSizeQuads*SubsectionY) ||	// above
			(ComponentY1 > InSubsectionSizeQuads*(SubsectionY+1)) )	// below
		{
			continue;
		}

		for( INT SubsectionX = 0;SubsectionX < InNumSubsections;SubsectionX++ )
		{
			// Check if subsection is fully to the left or right of the area we are interested in
			if( (ComponentX2 < InSubsectionSizeQuads*SubsectionX) ||	// left
				(ComponentX1 > InSubsectionSizeQuads*(SubsectionX+1)) )	// right
			{
				continue;
			}

			// Area to update in previous mip level coords
			INT PrevMipSubX1 = ComponentX1 - InSubsectionSizeQuads*SubsectionX;
			INT PrevMipSubY1 = ComponentY1 - InSubsectionSizeQuads*SubsectionY;
			INT PrevMipSubX2 = ComponentX2 - InSubsectionSizeQuads*SubsectionX;
			INT PrevMipSubY2 = ComponentY2 - InSubsectionSizeQuads*SubsectionY;

			INT PrevMipSubsectionSizeQuads = InSubsectionSizeQuads;
			FLOAT InvPrevMipSubsectionSizeQuads = 1.f / (FLOAT)InSubsectionSizeQuads;

			INT PrevMipSizeU = WeightmapSizeU;
			INT PrevMipSizeV = WeightmapSizeV;

			for( INT Mip=1;Mip<=MaxWholeSubsectionMip;Mip++ )
			{
				INT MipSizeU = WeightmapSizeU >> Mip;
				INT MipSizeV = WeightmapSizeV >> Mip;

				INT MipSubsectionSizeQuads = ((InSubsectionSizeQuads+1)>>Mip)-1;
				FLOAT InvMipSubsectionSizeQuads = 1.f / (FLOAT)MipSubsectionSizeQuads;

				// Area to update in current mip level coords
				INT MipSubX1 = appFloor( (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubX1 * InvPrevMipSubsectionSizeQuads );
				INT MipSubY1 = appFloor( (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubY1 * InvPrevMipSubsectionSizeQuads );
				INT MipSubX2 = appCeil(  (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubX2 * InvPrevMipSubsectionSizeQuads );
				INT MipSubY2 = appCeil(  (FLOAT)MipSubsectionSizeQuads * (FLOAT)PrevMipSubY2 * InvPrevMipSubsectionSizeQuads );

				// Clamp area to update
				INT VertX1 = Clamp<INT>(MipSubX1, 0, MipSubsectionSizeQuads);
				INT VertY1 = Clamp<INT>(MipSubY1, 0, MipSubsectionSizeQuads);
				INT VertX2 = Clamp<INT>(MipSubX2, 0, MipSubsectionSizeQuads);
				INT VertY2 = Clamp<INT>(MipSubY2, 0, MipSubsectionSizeQuads);

				for( INT VertY=VertY1;VertY<=VertY2;VertY++ )
				{
					for( INT VertX=VertX1;VertX<=VertX2;VertX++ )
					{
						// Convert VertX/Y into previous mip's coords
						FLOAT PrevMipVertX = (FLOAT)PrevMipSubsectionSizeQuads * (FLOAT)VertX * InvMipSubsectionSizeQuads;
						FLOAT PrevMipVertY = (FLOAT)PrevMipSubsectionSizeQuads * (FLOAT)VertY * InvMipSubsectionSizeQuads;

						// X/Y of the vertex we're looking indexed into the texture data
						INT TexX = (MipSubsectionSizeQuads+1) * SubsectionX + VertX;
						INT TexY = (MipSubsectionSizeQuads+1) * SubsectionY + VertY;

						FLOAT fPrevMipTexX = (FLOAT)((PrevMipSubsectionSizeQuads+1) * SubsectionX) + PrevMipVertX;
						FLOAT fPrevMipTexY = (FLOAT)((PrevMipSubsectionSizeQuads+1) * SubsectionY) + PrevMipVertY;

						INT PrevMipTexX = appFloor(fPrevMipTexX);
						FLOAT fPrevMipTexFracX = appFractional(fPrevMipTexX);
						INT PrevMipTexY = appFloor(fPrevMipTexY);
						FLOAT fPrevMipTexFracY = appFractional(fPrevMipTexY);

						check( TexX >= 0 && TexX < MipSizeU );
						check( TexY >= 0 && TexY < MipSizeV );
						check( PrevMipTexX >= 0 && PrevMipTexX < PrevMipSizeU );
						check( PrevMipTexY >= 0 && PrevMipTexY < PrevMipSizeV );

						INT PrevMipTexX1 = Min<INT>( PrevMipTexX+1, PrevMipSizeU-1 );
						INT PrevMipTexY1 = Min<INT>( PrevMipTexY+1, PrevMipSizeV-1 );

						FColor* TexData = &(WeightmapTextureMipData(Mip))[ TexX + TexY * MipSizeU ];
						FColor *PreMipTexData00 = &(WeightmapTextureMipData(Mip-1))[ PrevMipTexX + PrevMipTexY * PrevMipSizeU ];
						FColor *PreMipTexData01 = &(WeightmapTextureMipData(Mip-1))[ PrevMipTexX + PrevMipTexY1 * PrevMipSizeU ];
						FColor *PreMipTexData10 = &(WeightmapTextureMipData(Mip-1))[ PrevMipTexX1 + PrevMipTexY * PrevMipSizeU ];
						FColor *PreMipTexData11 = &(WeightmapTextureMipData(Mip-1))[ PrevMipTexX1 + PrevMipTexY1 * PrevMipSizeU ];

						// Lerp weightmap data
						TexData->R = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->R, (FLOAT)PreMipTexData10->R, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->R, (FLOAT)PreMipTexData11->R, fPrevMipTexFracX),
							fPrevMipTexFracY) );
						TexData->G = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->G, (FLOAT)PreMipTexData10->G, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->G, (FLOAT)PreMipTexData11->G, fPrevMipTexFracX),
							fPrevMipTexFracY) );
						TexData->B = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->B, (FLOAT)PreMipTexData10->B, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->B, (FLOAT)PreMipTexData11->B, fPrevMipTexFracX),
							fPrevMipTexFracY) );
						TexData->A = appRound(
							Lerp(
							Lerp( (FLOAT)PreMipTexData00->A, (FLOAT)PreMipTexData10->A, fPrevMipTexFracX),
							Lerp( (FLOAT)PreMipTexData01->A, (FLOAT)PreMipTexData11->A, fPrevMipTexFracX),
							fPrevMipTexFracY) );
					}
				}

				// Record the areas we updated
				if( TextureDataInfo )
				{
					INT TexX1 = (MipSubsectionSizeQuads+1) * SubsectionX + VertX1;
					INT TexY1 = (MipSubsectionSizeQuads+1) * SubsectionY + VertY1;
					INT TexX2 = (MipSubsectionSizeQuads+1) * SubsectionX + VertX2;
					INT TexY2 = (MipSubsectionSizeQuads+1) * SubsectionY + VertY2;
					TextureDataInfo->AddMipUpdateRegion(Mip,TexX1,TexY1,TexX2,TexY2);
				}

				// Copy current mip values to prev as we move to the next mip.
				PrevMipSubsectionSizeQuads = MipSubsectionSizeQuads;
				InvPrevMipSubsectionSizeQuads = InvMipSubsectionSizeQuads;

				PrevMipSizeU = MipSizeU;
				PrevMipSizeV = MipSizeV;

				// Use this mip's area as we move to the next mip
				PrevMipSubX1 = MipSubX1;
				PrevMipSubY1 = MipSubY1;
				PrevMipSubX2 = MipSubX2;
				PrevMipSubY2 = MipSubY2;
			}
		}
	}

	// Handle mips that have texels from multiple subsections
	Mip=1;
	for(;;)
	{
		INT MipSubsectionSizeQuads = ((InSubsectionSizeQuads+1)>>Mip)-1;

		INT MipSizeU = Max<INT>(WeightmapSizeU >> Mip,1);
		INT MipSizeV = Max<INT>(WeightmapSizeV >> Mip,1);

		// Mip must represent at least one quad to store valid weight data
		if( MipSubsectionSizeQuads <= 0 )
		{
			INT PrevMipSizeU = WeightmapSizeU >> (Mip-1);
			INT PrevMipSizeV = WeightmapSizeV >> (Mip-1);

			// not valid weight data, so just average the texels of the previous mip.
			for( INT Y = 0;Y < MipSizeV;Y++ )
			{
				for( INT X = 0;X < MipSizeU;X++ )
				{
					FColor* TexData = &(WeightmapTextureMipData(Mip))[ X + Y * MipSizeU ];

					FColor *PreMipTexData00 = &(WeightmapTextureMipData(Mip-1))[ (X*2+0) + (Y*2+0)  * PrevMipSizeU ];
					FColor *PreMipTexData01 = &(WeightmapTextureMipData(Mip-1))[ (X*2+0) + (Y*2+1)  * PrevMipSizeU ];
					FColor *PreMipTexData10 = &(WeightmapTextureMipData(Mip-1))[ (X*2+1) + (Y*2+0)  * PrevMipSizeU ];
					FColor *PreMipTexData11 = &(WeightmapTextureMipData(Mip-1))[ (X*2+1) + (Y*2+1)  * PrevMipSizeU ];

					TexData->R = (((INT)PreMipTexData00->R + (INT)PreMipTexData01->R + (INT)PreMipTexData10->R + (INT)PreMipTexData11->R) >> 2);
					TexData->G = (((INT)PreMipTexData00->G + (INT)PreMipTexData01->G + (INT)PreMipTexData10->G + (INT)PreMipTexData11->G) >> 2);
					TexData->B = (((INT)PreMipTexData00->B + (INT)PreMipTexData01->B + (INT)PreMipTexData10->B + (INT)PreMipTexData11->B) >> 2);
					TexData->A = (((INT)PreMipTexData00->A + (INT)PreMipTexData01->A + (INT)PreMipTexData10->A + (INT)PreMipTexData11->A) >> 2);
				}
			}

			if( TextureDataInfo )
			{
				// These mip sizes are small enough that we may as well just update the whole mip.
				TextureDataInfo->AddMipUpdateRegion(Mip,0,0,MipSizeU-1,MipSizeV-1);
			}
		}

		if( MipSizeU == 1 && MipSizeV == 1 )
		{
			break;
		}

		Mip++;
	}
}

//
// ALandscape
//

#define MAX_LANDSCAPE_SUBSECTIONS 3

void ALandscape::GetComponentsInRegion(INT X1, INT Y1, INT X2, INT Y2, TSet<ULandscapeComponent*>& OutComponents)
{
	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{		
			ULandscapeComponent* Component = XYtoComponentMap.FindRef(ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads));
			if( Component )
			{
				OutComponents.Add(Component);
			}
		}
	}
}

UBOOL ALandscape::ImportFromOldTerrain(ATerrain* OldTerrain)
{
	if( !OldTerrain )
	{
		return FALSE;
	}

	// Landscape doesn't support rotation...
	if( !OldTerrain->Rotation.IsZero() )
	{
		appMsgf(AMT_OK,TEXT("The Terrain actor has Non-zero rotation, and Landscape doesn't support rotation"), *OldTerrain->GetName());
		return FALSE;
	}

	// Work out how many subsections we need.
	// Until we've got a SubsectionSizeQuads that's one less than a power of 2
	// Keep adding new sections, or until we run out of 
	ComponentSizeQuads = OldTerrain->MaxComponentSize;
	NumSubsections = 1;
	SubsectionSizeQuads = ComponentSizeQuads;
	while( ((SubsectionSizeQuads) & (SubsectionSizeQuads+1)) != 0 || NumSubsections*SubsectionSizeQuads != ComponentSizeQuads )
	{
		if( NumSubsections > MAX_LANDSCAPE_SUBSECTIONS || ComponentSizeQuads / NumSubsections < 1 )
		{
			appMsgf(AMT_OK,TEXT("The Terrain actor %s's MaxComponentSize must be an 1x, 2x or 3x multiple of one less than a power of two"), *OldTerrain->GetName());
			return FALSE;
		}

		// try adding another subsection.
		NumSubsections++;
		SubsectionSizeQuads = ComponentSizeQuads / NumSubsections;
	}

	// Should check after changing NumSubsections
	if( NumSubsections > MAX_LANDSCAPE_SUBSECTIONS || ComponentSizeQuads / NumSubsections < 1 )
	{
		appMsgf(AMT_OK,TEXT("The Terrain actor %s's MaxComponentSize must be an 1x, 2x or 3x multiple of one less than a power of two"), *OldTerrain->GetName());
		return FALSE;
	}

	debugf(TEXT("%s: using ComponentSizeQuads=%d, NumSubsections=%d, SubsectionSizeQuads=%d"), *OldTerrain->GetName(), ComponentSizeQuads, NumSubsections, SubsectionSizeQuads);

	// Validate old terrain.
	if( OldTerrain->NumPatchesX % OldTerrain->MaxComponentSize != 0 ||
		OldTerrain->NumPatchesX % OldTerrain->MaxComponentSize != 0 )
	{
		appMsgf(AMT_OK,TEXT("The Terrain actor %s's NumPatchesX/Y must be multiples of MaxComponentSize"), *OldTerrain->GetName());
		return FALSE;
	}
	if( OldTerrain->MaxTesselationLevel > 1 )
	{
		appMsgf(AMT_OK,TEXT("The Terrain actor %s's MaxTesselationLevel must be set to 1."), *OldTerrain->GetName());
		return FALSE;
	}

	GWarn->BeginSlowTask( *FString::Printf(TEXT("Converting terrain %s"), *OldTerrain->GetName()), TRUE);

	// Create and import terrain material
	UMaterial* LandscapeUMaterial = ConstructObject<UMaterial>(UMaterial::StaticClass(), GetOutermost(), FName(*FString::Printf(TEXT("%s_Material"),*GetName())), RF_Public|RF_Standalone);
	LandscapeMaterial = LandscapeUMaterial;

	TMap<EMaterialProperty, UMaterialExpressionTerrainLayerWeight*> MaterialPropertyLastLayerWeightExpressionMap;
	
	INT YOffset = 0;

	for( INT LayerIndex=0;LayerIndex<OldTerrain->Layers.Num();LayerIndex++ )
	{
		UTerrainLayerSetup*	Setup = OldTerrain->Layers(LayerIndex).Setup;
		if( Setup )
		{
			if( Setup->Materials.Num() == 1)
			{
				FTerrainFilteredMaterial& FilteredTerrainMaterial = Setup->Materials(0);
				if( FilteredTerrainMaterial.Material )
				{
					UMaterial* Material = Cast<UMaterial>(FilteredTerrainMaterial.Material->Material);
					if( Material == NULL )
					{
						debugf(TEXT("%s's Material is not a plain UMaterial, skipping..."), *FilteredTerrainMaterial.Material->GetName());
						continue;
					}

					FLandscapeLayerInfo Info(FName(*FString::Printf(TEXT("Layer%d"),LayerIndex)));
					LayerInfos.AddItem(Info);

					TArray<UMaterialExpression*> NewExpressions;
					TArray<UMaterialExpression*> NewComments;

					// Copy all the expression from the material to our new material
					UMaterialExpression::CopyMaterialExpressions(Material->Expressions, Material->EditorComments, LandscapeUMaterial, NewExpressions, NewComments);

					for( INT CommentIndex=0;CommentIndex<NewComments.Num();CommentIndex++ )
					{
						// Move comments
						UMaterialExpression* Comment = NewComments(CommentIndex);
						Comment->MaterialExpressionEditorX += 200;
						Comment->MaterialExpressionEditorY += YOffset;
					}

					UMaterialExpressionTerrainLayerCoords* LayerTexCoords = NULL;

					for( INT ExpressionIndex=0;ExpressionIndex<NewExpressions.Num();ExpressionIndex++ )
					{
						UMaterialExpression* Expression = NewExpressions(ExpressionIndex);

						// Move expressions
						Expression->MaterialExpressionEditorX += 200;
						Expression->MaterialExpressionEditorY += YOffset;

						// Fix up texture coordinates for this layer
						UMaterialExpressionTextureSample* TextureSampleExpression = Cast<UMaterialExpressionTextureSample>(Expression);
						if( TextureSampleExpression != NULL) // && TextureSampleExpression->Coordinates.Expression == NULL)
						{
							if( LayerTexCoords == NULL )
							{
								LayerTexCoords = ConstructObject<UMaterialExpressionTerrainLayerCoords>( UMaterialExpressionTerrainLayerCoords::StaticClass(), LandscapeUMaterial, NAME_None, RF_Transactional );
								LandscapeUMaterial->Expressions.AddItem(LayerTexCoords);
								LayerTexCoords->MaterialExpressionEditorX = Expression->MaterialExpressionEditorX + 120;
								LayerTexCoords->MaterialExpressionEditorY = Expression->MaterialExpressionEditorY + 48;
								check((INT)TMT_MAX == (INT)TCMT_MAX);
								//!! TODO make these parameters
								LayerTexCoords->MappingType		= FilteredTerrainMaterial.Material->MappingType;
								LayerTexCoords->MappingScale	= FilteredTerrainMaterial.Material->MappingScale;
								LayerTexCoords->MappingRotation = FilteredTerrainMaterial.Material->MappingRotation;
								LayerTexCoords->MappingPanU		= FilteredTerrainMaterial.Material->MappingPanU;
								LayerTexCoords->MappingPanV		= FilteredTerrainMaterial.Material->MappingPanV;
							}

							TextureSampleExpression->Coordinates.Expression = LayerTexCoords;
						}
					}

					INT NumPropertiesWeighted = 0;
					for( INT PropertyIndex=0;PropertyIndex < MP_MAX;PropertyIndex++ )
					{
						FExpressionInput* PropertyInput = Material->GetExpressionInputForProperty((EMaterialProperty)PropertyIndex);
						if( PropertyInput->Expression )
						{
							// Need to construct a new UMaterialExpressionTerrainLayerWeight to blend in this layer for this input.
							UMaterialExpressionTerrainLayerWeight* WeightExpression = ConstructObject<UMaterialExpressionTerrainLayerWeight>( UMaterialExpressionTerrainLayerWeight::StaticClass(), LandscapeUMaterial, NAME_None, RF_Transactional );
							LandscapeUMaterial->Expressions.AddItem(WeightExpression);
							WeightExpression->ConditionallyGenerateGUID(TRUE);

							WeightExpression->MaterialExpressionEditorX = 200 + 32 * NumPropertiesWeighted++;
							WeightExpression->MaterialExpressionEditorY = YOffset;
							YOffset += 64;

							// Connect the previous layer's weight blend for this material property as the Base, or NULL if there was none.
							WeightExpression->Base.Expression = MaterialPropertyLastLayerWeightExpressionMap.FindRef((EMaterialProperty)PropertyIndex);

							// Connect this layer to the Layer input.
							WeightExpression->Layer = *PropertyInput;

							// Remap the expression it points to, so we're working on the copy.
							INT ExpressionIndex = Material->Expressions.FindItemIndex(PropertyInput->Expression);
							check(ExpressionIndex != INDEX_NONE);					
							WeightExpression->Layer.Expression = NewExpressions(ExpressionIndex);
							WeightExpression->ParameterName = FName(*FString::Printf(TEXT("Layer%d"),LayerIndex));
							
							// Remember this weight expression as the last layer for this material property
							MaterialPropertyLastLayerWeightExpressionMap.Set((EMaterialProperty)PropertyIndex, WeightExpression);
						}
					}

					for( INT ExpressionIndex=0;ExpressionIndex<NewExpressions.Num();ExpressionIndex++ )
					{
						UMaterialExpression* Expression = NewExpressions(ExpressionIndex);

						if( Expression->MaterialExpressionEditorY > YOffset )
						{
							YOffset = Expression->MaterialExpressionEditorY;
						}
					}

					YOffset += 64;
				}
			}
			else
			{
				debugf(TEXT("%s is a multi-layer filtered material, skipping..."), *Setup->GetName());
			}
		}
	}

	// Assign all the material inputs
	for( INT PropertyIndex=0;PropertyIndex < MP_MAX;PropertyIndex++ )
	{
		UMaterialExpressionTerrainLayerWeight* WeightExpression = MaterialPropertyLastLayerWeightExpressionMap.FindRef((EMaterialProperty)PropertyIndex);
		if( WeightExpression )
		{
			FExpressionInput* PropertyInput = LandscapeUMaterial->GetExpressionInputForProperty((EMaterialProperty)PropertyIndex);
			check(PropertyInput);
			PropertyInput->Expression = WeightExpression;
		}
	}

	DrawScale = OldTerrain->DrawScale;
	DrawScale3D = OldTerrain->DrawScale3D;

	// import!
	INT VertsX = OldTerrain->NumPatchesX+1;
	INT VertsY = OldTerrain->NumPatchesY+1;

	// Copy height data
	WORD* HeightData = new WORD[VertsX*VertsY];
	for( INT Y=0;Y<VertsY;Y++ )
	{
		for( INT X=0;X<VertsX;X++ )
		{
			HeightData[Y*VertsX + X] = OldTerrain->Height(X,Y);
		}
	}

	TArray<FLandscapeLayerInfo> ImportLayerInfos;
	TArray<BYTE*> ImportAlphaDataPointers;

	// Copy over Alphamap data from old terrain
	for( INT LayerIndex=0;LayerIndex<OldTerrain->Layers.Num();LayerIndex++ )
	{
		INT AlphaMapIndex = OldTerrain->Layers(LayerIndex).AlphaMapIndex;

		if( AlphaMapIndex != -1 || LayerIndex == 0 )
		{
			BYTE* AlphaData = new BYTE[VertsX*VertsY];
			ImportAlphaDataPointers.AddItem(AlphaData);
			new(ImportLayerInfos) FLandscapeLayerInfo(FName(*FString::Printf(TEXT("Layer%d"),LayerIndex)));
			//ImportLayerNames.AddItem(FName(*FString::Printf(TEXT("Layer%d"),LayerIndex)));

			if( AlphaMapIndex == -1 || LayerIndex == 0 )
			{
				// First layer doesn't have an alphamap, as it's completely opaque.
				appMemset( AlphaData, 255, VertsX*VertsY );
			}
			else
			{
				check( OldTerrain->AlphaMaps(AlphaMapIndex).Data.Num() == VertsX * VertsY );
				appMemcpy(AlphaData, &OldTerrain->AlphaMaps(AlphaMapIndex).Data(0), VertsX * VertsY);
			}
		}
	}

	// import heightmap and weightmap
	Import(VertsX, VertsY, ComponentSizeQuads, NumSubsections, SubsectionSizeQuads, HeightData, ImportLayerInfos, &ImportAlphaDataPointers(0));

	delete[] HeightData;
	for( INT i=0;i<ImportAlphaDataPointers.Num();i++)
	{
		delete []ImportAlphaDataPointers(i);
	}
	ImportAlphaDataPointers.Empty();

	GWarn->EndSlowTask();

	return TRUE;
}

// A struct to remember where we have spare texture channels.
struct FWeightmapTextureAllocation
{
	INT X;
	INT Y;
	INT ChannelsInUse;
	UTexture2D* Texture;
	FColor* TextureData;

	FWeightmapTextureAllocation( INT InX, INT InY, INT InChannels, UTexture2D* InTexture, FColor* InTextureData )
		:	X(InX)
		,	Y(InY)
		,	ChannelsInUse(InChannels)
		,	Texture(InTexture)
		,	TextureData(InTextureData)
	{}
};

// A struct to hold the info about each texture chunk of the total heightmap
struct FHeightmapInfo
{
	INT HeightmapSizeU;
	INT HeightmapSizeV;
	UTexture2D* HeightmapTexture;
	TArray<FColor*> HeightmapTextureMipData;
};


#define HEIGHTDATA(X,Y) (HeightData[ Clamp<INT>(Y,0,VertsY) * VertsX + Clamp<INT>(X,0,VertsX) ])
void ALandscape::Import(INT VertsX, INT VertsY, INT InComponentSizeQuads, INT InNumSubsections, INT InSubsectionSizeQuads, WORD* HeightData, TArray<FLandscapeLayerInfo> ImportLayerInfos, BYTE* AlphaDataPointers[] )
{
	GWarn->BeginSlowTask( TEXT("Importing Landscape"), TRUE);

	ComponentSizeQuads = InComponentSizeQuads;
	NumSubsections = InNumSubsections;
	SubsectionSizeQuads = InSubsectionSizeQuads;

	MarkPackageDirty();

	INT NumPatchesX = (VertsX-1);
	INT NumPatchesY = (VertsY-1);

	INT NumSectionsX = NumPatchesX / ComponentSizeQuads;
	INT NumSectionsY = NumPatchesY / ComponentSizeQuads;

	LayerInfos = ImportLayerInfos;
	LandscapeComponents.Empty(NumSectionsX * NumSectionsY);

	for (INT Y = 0; Y < NumSectionsY; Y++)
	{
		for (INT X = 0; X < NumSectionsX; X++)
		{
			// The number of quads
			INT NumQuadsX = NumPatchesX;
			INT NumQuadsY = NumPatchesY;

			INT BaseX = X * ComponentSizeQuads;
			INT BaseY = Y * ComponentSizeQuads;

			ULandscapeComponent* LandscapeComponent = ConstructObject<ULandscapeComponent>(ULandscapeComponent::StaticClass(),this,NAME_None,RF_Transactional);
			LandscapeComponents.AddItem(LandscapeComponent);
			LandscapeComponent->Init(
				BaseX,BaseY,
				ComponentSizeQuads,
				NumSubsections,
				SubsectionSizeQuads
				);

#if 0
			// Propagate shadow/ lighting options from ATerrain to component.
			LandscapeComponent->CastShadow			= bCastShadow;
			LandscapeComponent->bCastDynamicShadow	= bCastDynamicShadow;
			LandscapeComponent->bForceDirectLightMap	= bForceDirectLightMap;
			LandscapeComponent->BlockRigidBody		= bBlockRigidBody;
			LandscapeComponent->bAcceptsDynamicLights	= bAcceptsDynamicLights;
			LandscapeComponent->LightingChannels		= LightingChannels;
			LandscapeComponent->PhysMaterialOverride	= TerrainPhysMaterialOverride;

			// Set the collision display options
			LandscapeComponent->bDisplayCollisionLevel = bShowingCollision;
#endif
		}
	}

#define MAX_HEIGHTMAP_TEXTURE_SIZE 512

	INT ComponentSizeVerts = NumSubsections * (SubsectionSizeQuads+1);
	INT ComponentsPerHeightmap = MAX_HEIGHTMAP_TEXTURE_SIZE / ComponentSizeVerts;

	// Count how many heightmaps we need and the X dimension of the final heightmap
	INT NumHeightmapsX = 1;
	INT FinalComponentsX = NumSectionsX;
	while( FinalComponentsX > ComponentsPerHeightmap )
	{
		FinalComponentsX -= ComponentsPerHeightmap;
		NumHeightmapsX++;
	}
	// Count how many heightmaps we need and the Y dimension of the final heightmap
	INT NumHeightmapsY = 1;
	INT FinalComponentsY = NumSectionsY;
	while( FinalComponentsY > ComponentsPerHeightmap )
	{
		FinalComponentsY -= ComponentsPerHeightmap;
		NumHeightmapsY++;
	}

	TArray<FHeightmapInfo> HeightmapInfos;

	for( INT HmY=0;HmY<NumHeightmapsY;HmY++ )
	{
		for( INT HmX=0;HmX<NumHeightmapsX;HmX++ )
		{
			FHeightmapInfo& HeightmapInfo = HeightmapInfos(HeightmapInfos.AddZeroed());

			// make sure the heightmap UVs are powers of two.
			HeightmapInfo.HeightmapSizeU = (1<<appCeilLogTwo( ((HmX==NumHeightmapsX-1) ? FinalComponentsX : ComponentsPerHeightmap)*ComponentSizeVerts ));
			HeightmapInfo.HeightmapSizeV = (1<<appCeilLogTwo( ((HmY==NumHeightmapsY-1) ? FinalComponentsY : ComponentsPerHeightmap)*ComponentSizeVerts ));

			// Construct the heightmap textures
			HeightmapInfo.HeightmapTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), GetOutermost(), NAME_None/*FName(TEXT("Heightmap"))*/, RF_Public|RF_Standalone);
			HeightmapInfo.HeightmapTexture->Init(HeightmapInfo.HeightmapSizeU,HeightmapInfo.HeightmapSizeV,PF_A8R8G8B8);
			HeightmapInfo.HeightmapTexture->SRGB = FALSE;
			HeightmapInfo.HeightmapTexture->CompressionNone = TRUE;
			HeightmapInfo.HeightmapTexture->MipGenSettings = TMGS_LeaveExistingMips;
			HeightmapInfo.HeightmapTexture->LODGroup = TEXTUREGROUP_Terrain_Heightmap;
			HeightmapInfo.HeightmapTexture->AddressX = TA_Clamp;
			HeightmapInfo.HeightmapTexture->AddressY = TA_Clamp;

			INT MipSubsectionSizeQuads = SubsectionSizeQuads;
			INT MipSizeU = HeightmapInfo.HeightmapSizeU;
			INT MipSizeV = HeightmapInfo.HeightmapSizeV;
			while( MipSizeU > 1 && MipSizeV > 1 && MipSubsectionSizeQuads >= 1 )
			{
				FColor* HeightmapTextureData;
				if( HeightmapInfo.HeightmapTextureMipData.Num() > 0 )	
				{
					// create subsequent mips
					FTexture2DMipMap* HeightMipMap = new(HeightmapInfo.HeightmapTexture->Mips) FTexture2DMipMap;
					HeightMipMap->SizeX = MipSizeU;
					HeightMipMap->SizeY = MipSizeV;
					HeightMipMap->Data.Lock(LOCK_READ_WRITE);
					HeightmapTextureData = (FColor*)HeightMipMap->Data.Realloc(MipSizeU*MipSizeV*sizeof(FColor));
				}
				else
				{
					HeightmapTextureData = (FColor*)HeightmapInfo.HeightmapTexture->Mips(0).Data.Lock(LOCK_READ_WRITE);
				}

				appMemzero( HeightmapTextureData, MipSizeU*MipSizeV*sizeof(FColor) );
				HeightmapInfo.HeightmapTextureMipData.AddItem(HeightmapTextureData);

				MipSizeU >>= 1;
				MipSizeV >>= 1;

				MipSubsectionSizeQuads = ((MipSubsectionSizeQuads + 1) >> 1) - 1;
			}
		}
	}

	// Calculate the normals for each of the two triangles per quad.
	FVector* VertexNormals = new FVector[(NumPatchesX+1)*(NumPatchesY+1)];
	appMemzero(VertexNormals, (NumPatchesX+1)*(NumPatchesY+1)*sizeof(FVector));
	for( INT QuadY=0;QuadY<NumPatchesY;QuadY++ )
	{
		for( INT QuadX=0;QuadX<NumPatchesX;QuadX++ )
		{
			FVector Vert00 = FVector(0.f,0.f,((FLOAT)HEIGHTDATA(QuadX+0, QuadY+0) - 32768.f)/128.f) * DrawScale3D;
			FVector Vert01 = FVector(0.f,1.f,((FLOAT)HEIGHTDATA(QuadX+0, QuadY+1) - 32768.f)/128.f) * DrawScale3D;
			FVector Vert10 = FVector(1.f,0.f,((FLOAT)HEIGHTDATA(QuadX+1, QuadY+0) - 32768.f)/128.f) * DrawScale3D;
			FVector Vert11 = FVector(1.f,1.f,((FLOAT)HEIGHTDATA(QuadX+1, QuadY+1) - 32768.f)/128.f) * DrawScale3D;

			FVector FaceNormal1 = ((Vert00-Vert10) ^ (Vert10-Vert11)).SafeNormal();
			FVector FaceNormal2 = ((Vert11-Vert01) ^ (Vert01-Vert00)).SafeNormal(); 

			// contribute to the vertex normals.
			VertexNormals[(QuadX+1 + (NumPatchesX+1)*(QuadY+0))] += FaceNormal1;
			VertexNormals[(QuadX+0 + (NumPatchesX+1)*(QuadY+1))] += FaceNormal2;
			VertexNormals[(QuadX+0 + (NumPatchesX+1)*(QuadY+0))] += FaceNormal1 + FaceNormal2;
			VertexNormals[(QuadX+1 + (NumPatchesX+1)*(QuadY+1))] += FaceNormal1 + FaceNormal2;
		}
	}

#define LANDSCAPE_WEIGHT_NORMALIZE_THRESHOLD	3

	// Weight values for each layer for each component.
	TArray<TArray<TArray<BYTE> > > ComponentWeightValues;
	ComponentWeightValues.AddZeroed(NumSectionsX*NumSectionsY);

	for (INT ComponentY = 0; ComponentY < NumSectionsY; ComponentY++)
	{
		for (INT ComponentX = 0; ComponentX < NumSectionsX; ComponentX++)
		{
			ULandscapeComponent* LandscapeComponent = LandscapeComponents(ComponentX + ComponentY*NumSectionsX);
			TArray<TArray<BYTE> >& WeightValues = ComponentWeightValues(ComponentX + ComponentY*NumSectionsX);

			// Assign the components' neighbors 
			LandscapeComponent->Neighbors[TCN_NW] = ComponentX==0 || ComponentY==0								? NULL : LandscapeComponents((ComponentX-1) + (ComponentY-1)*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_N]  = ComponentY==0												? NULL : LandscapeComponents((ComponentX  ) + (ComponentY-1)*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_NE] = ComponentX==NumSectionsX-1 || ComponentY==0					? NULL : LandscapeComponents((ComponentX+1) + (ComponentY-1)*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_W]  = ComponentX==0												? NULL : LandscapeComponents((ComponentX-1) + (ComponentY  )*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_E]  = ComponentX==NumSectionsX-1									? NULL : LandscapeComponents((ComponentX+1) + (ComponentY  )*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_SW] = ComponentX==0 || ComponentY==NumSectionsY-1					? NULL : LandscapeComponents((ComponentX-1) + (ComponentY+1)*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_S]  = ComponentY==NumSectionsY-1									? NULL : LandscapeComponents((ComponentX  ) + (ComponentY+1)*NumSectionsX);
			LandscapeComponent->Neighbors[TCN_SE] = ComponentX==NumSectionsX-1 || ComponentY==NumSectionsY-1	? NULL : LandscapeComponents((ComponentX+1) + (ComponentY+1)*NumSectionsX);

			// Import alphamap data into local array and check for unused layers for this component.
			TArray<FLandscapeComponentAlphaInfo> EditingAlphaLayerData;
			for( INT LayerIndex=0;LayerIndex<ImportLayerInfos.Num();LayerIndex++ )
			{
				FLandscapeComponentAlphaInfo* NewAlphaInfo = new(EditingAlphaLayerData) FLandscapeComponentAlphaInfo(LandscapeComponent, LayerIndex);

				for( INT AlphaY=0;AlphaY<=LandscapeComponent->ComponentSizeQuads;AlphaY++ )
				{
					BYTE* OldAlphaRowStart = &AlphaDataPointers[LayerIndex][ (AlphaY+LandscapeComponent->SectionBaseY) * VertsX + (LandscapeComponent->SectionBaseX) ];
					BYTE* NewAlphaRowStart = &NewAlphaInfo->AlphaValues(AlphaY * (LandscapeComponent->ComponentSizeQuads+1));
					appMemcpy(NewAlphaRowStart, OldAlphaRowStart, LandscapeComponent->ComponentSizeQuads+1);
				}						
			}

			for( INT AlphaMapIndex=0; AlphaMapIndex<EditingAlphaLayerData.Num();AlphaMapIndex++ )
			{
				if( EditingAlphaLayerData(AlphaMapIndex).IsLayerAllZero() )
				{
					EditingAlphaLayerData.Remove(AlphaMapIndex);
					AlphaMapIndex--;
				}
			}

			
			debugf(TEXT("%s needs %d alphamaps"), *LandscapeComponent->GetName(),EditingAlphaLayerData.Num());

			// Calculate weightmap weights for this component
			WeightValues.Empty(EditingAlphaLayerData.Num());
			WeightValues.AddZeroed(EditingAlphaLayerData.Num());
			LandscapeComponent->WeightmapLayerAllocations.Empty(EditingAlphaLayerData.Num());

			for( INT WeightLayerIndex=0; WeightLayerIndex<WeightValues.Num();WeightLayerIndex++ )
			{
				// Lookup the original layer name
				WeightValues(WeightLayerIndex) = EditingAlphaLayerData(WeightLayerIndex).AlphaValues;
				new(LandscapeComponent->WeightmapLayerAllocations) FWeightmapLayerAllocationInfo(ImportLayerInfos(EditingAlphaLayerData(WeightLayerIndex).LayerIndex).LayerName);
			}

			// Discard the temporary alpha data
			EditingAlphaLayerData.Empty();

			// For each layer...
			for( INT WeightLayerIndex=WeightValues.Num()-1; WeightLayerIndex>=0;WeightLayerIndex-- )
			{
				// ... multiply all lower layers'...
				for( INT BelowWeightLayerIndex=WeightLayerIndex-1; BelowWeightLayerIndex>=0;BelowWeightLayerIndex-- )
				{
					INT TotalWeight = 0;
					// ... values by...
					for( INT Idx=0;Idx<WeightValues(WeightLayerIndex).Num();Idx++ )
					{
						// ... one-minus the current layer's values
						INT NewValue = (INT)WeightValues(BelowWeightLayerIndex)(Idx) * (INT)(255 - WeightValues(WeightLayerIndex)(Idx)) / 255;
						WeightValues(BelowWeightLayerIndex)(Idx) = (BYTE)NewValue;
						TotalWeight += NewValue;
					}

					if( TotalWeight == 0 )
					{
						// Remove the layer as it has no contribution
						WeightValues.Remove(BelowWeightLayerIndex);
						LandscapeComponent->WeightmapLayerAllocations.Remove(BelowWeightLayerIndex);

						// The current layer has been re-numbered
						WeightLayerIndex--;
					}
				}
			}

			// Weight normalization for total should be 255...
			if (WeightValues.Num())
			{
				for( INT Idx=0;Idx<WeightValues(0).Num();Idx++ )
				{
					INT TotalWeight = 0;
					for( INT WeightLayerIndex = 0; WeightLayerIndex < WeightValues.Num(); WeightLayerIndex++ )
					{
						TotalWeight += (INT)WeightValues(WeightLayerIndex)(Idx);
					}
					if (TotalWeight == 0)
					{
						WeightValues(0)(Idx) = 255;
					}
					else if (255 - TotalWeight <= LANDSCAPE_WEIGHT_NORMALIZE_THRESHOLD) // small difference, currenlty 3...
					{
						WeightValues(0)(Idx) += 255 - TotalWeight;
					}
					else if (TotalWeight != 255)
					{
						// normalization...
						FLOAT Factor = 255.f/TotalWeight;
						TotalWeight = 0;
						for( INT WeightLayerIndex = 0; WeightLayerIndex < WeightValues.Num(); WeightLayerIndex++ )
						{
							WeightValues(WeightLayerIndex)(Idx) = (BYTE)(Factor * WeightValues(WeightLayerIndex)(Idx));
							TotalWeight += WeightValues(WeightLayerIndex)(Idx);
						}
						if (255 - TotalWeight)
						{
							WeightValues(0)(Idx) += 255 - TotalWeight;
						}
					}
				}
			}
		}
	}

	// Remember where we have spare texture channels.
	TArray<FWeightmapTextureAllocation> TextureAllocations;
		
	for (INT ComponentY = 0; ComponentY < NumSectionsY; ComponentY++)
	{
		for (INT ComponentX = 0; ComponentX < NumSectionsX; ComponentX++)
		{
			INT HmX = ComponentX / ComponentsPerHeightmap;
			INT HmY = ComponentY / ComponentsPerHeightmap;
			FHeightmapInfo& HeightmapInfo = HeightmapInfos(HmX + HmY * NumHeightmapsX);

			ULandscapeComponent* LandscapeComponent = LandscapeComponents(ComponentX + ComponentY*NumSectionsX);

			// Lookup array of weight values for this component.
			TArray<TArray<BYTE> >& WeightValues = ComponentWeightValues(ComponentX + ComponentY*NumSectionsX);

			// Heightmap offsets
			INT HeightmapOffsetX = (ComponentX - ComponentsPerHeightmap*HmX) * NumSubsections * (SubsectionSizeQuads+1);
			INT HeightmapOffsetY = (ComponentY - ComponentsPerHeightmap*HmY) * NumSubsections * (SubsectionSizeQuads+1);

			LandscapeComponent->HeightmapScaleBias = FVector4( 1.f / (FLOAT)HeightmapInfo.HeightmapSizeU, 1.f / (FLOAT)HeightmapInfo.HeightmapSizeV, (FLOAT)((HeightmapOffsetX)) / (FLOAT)HeightmapInfo.HeightmapSizeU, ((FLOAT)(HeightmapOffsetY)) / (FLOAT)HeightmapInfo.HeightmapSizeV );
			LandscapeComponent->HeightmapSubsectionOffset =  (FLOAT)(SubsectionSizeQuads+1) / (FLOAT)HeightmapInfo.HeightmapSizeU;

			LandscapeComponent->HeightmapTexture = HeightmapInfo.HeightmapTexture;

			// Layer UV panning to ensure consistent tiling across components
			LandscapeComponent->LayerUVPan = FVector2D( (FLOAT)(ComponentX*ComponentSizeQuads), (FLOAT)(ComponentY*ComponentSizeQuads) );

			// Weightmap is sized the same as the component
			INT WeightmapSize = (SubsectionSizeQuads+1) * NumSubsections;
			// Should be power of two
			check(((WeightmapSize-1) & WeightmapSize) == 0);

			LandscapeComponent->WeightmapScaleBias = FVector4( 1.f / (FLOAT)WeightmapSize, 1.f / (FLOAT)WeightmapSize, 0.5f / (FLOAT)WeightmapSize, 0.5f / (FLOAT)WeightmapSize);
			LandscapeComponent->WeightmapSubsectionOffset =  (FLOAT)(SubsectionSizeQuads+1) / (FLOAT)WeightmapSize;

			// Pointers to the texture data where we'll store each layer. Stride is 4 (FColor)
			TArray<BYTE*> WeightmapTextureDataPointers;

			debugf(TEXT("%s needs %d weightmap channels"), *LandscapeComponent->GetName(),WeightValues.Num());

			// Find texture channels to store each layer.
			INT LayerIndex = 0;
			while( LayerIndex < WeightValues.Num() )
			{
				INT RemainingLayers = WeightValues.Num()-LayerIndex;

				INT BestAllocationIndex = -1;

				// if we need less than 4 channels, try to find them somewhere to put all of them
				if( RemainingLayers < 4 )
				{
					INT BestDistSquared = MAXINT;
					for( INT TryAllocIdx=0;TryAllocIdx<TextureAllocations.Num();TryAllocIdx++ )
					{
						if( TextureAllocations(TryAllocIdx).ChannelsInUse + RemainingLayers <= 4 )
						{
							FWeightmapTextureAllocation& TryAllocation = TextureAllocations(TryAllocIdx);
							INT TryDistSquared = Square(TryAllocation.X-ComponentX) + Square(TryAllocation.Y-ComponentY);
							if( TryDistSquared < BestDistSquared )
							{
								BestDistSquared = TryDistSquared;
								BestAllocationIndex = TryAllocIdx;
							}
						}
					}
				}

				if( BestAllocationIndex != -1 )
				{
					FWeightmapTextureAllocation& Allocation = TextureAllocations(BestAllocationIndex);
					
					debugf(TEXT("  ==> Storing %d channels starting at %s[%d]"), RemainingLayers, *Allocation.Texture->GetName(), Allocation.ChannelsInUse );

					for( INT i=0;i<RemainingLayers;i++ )
					{
						LandscapeComponent->WeightmapLayerAllocations(LayerIndex+i).WeightmapTextureIndex = LandscapeComponent->WeightmapTextures.Num();
						LandscapeComponent->WeightmapLayerAllocations(LayerIndex+i).WeightmapTextureChannel = Allocation.ChannelsInUse;
						switch( Allocation.ChannelsInUse )
						{
						case 1:
							WeightmapTextureDataPointers.AddItem((BYTE*)&Allocation.TextureData->G);
							break;
						case 2:
							WeightmapTextureDataPointers.AddItem((BYTE*)&Allocation.TextureData->B);
							break;
						case 3:
							WeightmapTextureDataPointers.AddItem((BYTE*)&Allocation.TextureData->A);
							break;
						default:
							// this should not occur.
							check(0);

						}
						Allocation.ChannelsInUse++;
					}

					LayerIndex += RemainingLayers;
					LandscapeComponent->WeightmapTextures.AddItem(Allocation.Texture);
				}
				else
				{
					// We couldn't find a suitable place for these layers, so lets make a new one.
					UTexture2D* WeightmapTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), GetOutermost(), NAME_None, RF_Public|RF_Standalone);
					WeightmapTexture->Init(WeightmapSize,WeightmapSize,PF_A8R8G8B8);
					WeightmapTexture->SRGB = FALSE;
					WeightmapTexture->CompressionNone = TRUE;
					WeightmapTexture->MipGenSettings = TMGS_LeaveExistingMips;
					WeightmapTexture->AddressX = TA_Clamp;
					WeightmapTexture->AddressY = TA_Clamp;
					WeightmapTexture->LODGroup = TEXTUREGROUP_Terrain_Weightmap;
					FColor* MipData = (FColor*)WeightmapTexture->Mips(0).Data.Lock(LOCK_READ_WRITE);

					INT ThisAllocationLayers = Min<INT>(RemainingLayers,4);
					new(TextureAllocations) FWeightmapTextureAllocation(ComponentX,ComponentY,ThisAllocationLayers,WeightmapTexture,MipData);

					debugf(TEXT("  ==> Storing %d channels in new texture %s"), ThisAllocationLayers, *WeightmapTexture->GetName());

					WeightmapTextureDataPointers.AddItem((BYTE*)&MipData->R);
					LandscapeComponent->WeightmapLayerAllocations(LayerIndex+0).WeightmapTextureIndex = LandscapeComponent->WeightmapTextures.Num();
					LandscapeComponent->WeightmapLayerAllocations(LayerIndex+0).WeightmapTextureChannel = 0;

					if( ThisAllocationLayers > 1 )
					{
						WeightmapTextureDataPointers.AddItem((BYTE*)&MipData->G);
						LandscapeComponent->WeightmapLayerAllocations(LayerIndex+1).WeightmapTextureIndex = LandscapeComponent->WeightmapTextures.Num();
						LandscapeComponent->WeightmapLayerAllocations(LayerIndex+1).WeightmapTextureChannel = 1;

						if( ThisAllocationLayers > 2 )
						{
							WeightmapTextureDataPointers.AddItem((BYTE*)&MipData->B);
							LandscapeComponent->WeightmapLayerAllocations(LayerIndex+2).WeightmapTextureIndex = LandscapeComponent->WeightmapTextures.Num();
							LandscapeComponent->WeightmapLayerAllocations(LayerIndex+2).WeightmapTextureChannel = 2;

							if( ThisAllocationLayers > 3 )
							{
								WeightmapTextureDataPointers.AddItem((BYTE*)&MipData->A);
								LandscapeComponent->WeightmapLayerAllocations(LayerIndex+3).WeightmapTextureIndex = LandscapeComponent->WeightmapTextures.Num();
								LandscapeComponent->WeightmapLayerAllocations(LayerIndex+3).WeightmapTextureChannel = 3;
							}
						}
					}
					LandscapeComponent->WeightmapTextures.AddItem(WeightmapTexture);

					LayerIndex += ThisAllocationLayers;
				}
			}
			check(WeightmapTextureDataPointers.Num() == WeightValues.Num());

			FVector* WorldVerts = new FVector[Square(ComponentSizeQuads+1)];

			for( INT SubsectionY = 0;SubsectionY < NumSubsections;SubsectionY++ )
			{
				for( INT SubsectionX = 0;SubsectionX < NumSubsections;SubsectionX++ )
				{
					for( INT SubY=0;SubY<=SubsectionSizeQuads;SubY++ )
					{
						for( INT SubX=0;SubX<=SubsectionSizeQuads;SubX++ )
						{
							// X/Y of the vertex we're looking at in component's coordinates.
							INT CompX = SubsectionSizeQuads * SubsectionX + SubX;
							INT CompY = SubsectionSizeQuads * SubsectionY + SubY;

							// X/Y of the vertex we're looking indexed into the texture data
							INT TexX = (SubsectionSizeQuads+1) * SubsectionX + SubX;
							INT TexY = (SubsectionSizeQuads+1) * SubsectionY + SubY;

							INT WeightSrcDataIdx = CompY * (ComponentSizeQuads+1) + CompX;
							INT HeightTexDataIdx = (HeightmapOffsetX + TexX) + (HeightmapOffsetY + TexY) * (HeightmapInfo.HeightmapSizeU);

							INT WeightTexDataIdx = (TexX) + (TexY) * (WeightmapSize);

							// copy height and normal data
							WORD HeightValue = HEIGHTDATA(CompX + LandscapeComponent->SectionBaseX, CompY + LandscapeComponent->SectionBaseY);
							FVector Normal = VertexNormals[CompX+LandscapeComponent->SectionBaseX + (NumPatchesX+1)*(CompY+LandscapeComponent->SectionBaseY)].SafeNormal();

							HeightmapInfo.HeightmapTextureMipData(0)[HeightTexDataIdx].R = HeightValue >> 8;
							HeightmapInfo.HeightmapTextureMipData(0)[HeightTexDataIdx].G = HeightValue & 255;
							HeightmapInfo.HeightmapTextureMipData(0)[HeightTexDataIdx].B = appRound( 127.5f * (Normal.X + 1.f) );
							HeightmapInfo.HeightmapTextureMipData(0)[HeightTexDataIdx].A = appRound( 127.5f * (Normal.Y + 1.f) );

							for( INT WeightmapIndex=0;WeightmapIndex<WeightValues.Num(); WeightmapIndex++ )
							{
								WeightmapTextureDataPointers(WeightmapIndex)[WeightTexDataIdx*4] = WeightValues(WeightmapIndex)(WeightSrcDataIdx);
							}


							// Get world space verts
							FVector WorldVertex( (FLOAT)LandscapeComponent->SectionBaseX + FLOAT(CompX), (FLOAT)LandscapeComponent->SectionBaseY + FLOAT(CompY), ((FLOAT)HeightValue - 32768.f)/128.f );
							WorldVertex *= DrawScale3D * DrawScale;
							WorldVertex += Location;
							WorldVerts[(LandscapeComponent->ComponentSizeQuads+1) * CompY + CompX] = WorldVertex;
						}
					}
				}
			}

			// This could give us a tighter sphere bounds than just adding the points one by one.
			LandscapeComponent->CachedBoxSphereBounds = FBoxSphereBounds(WorldVerts, Square(ComponentSizeQuads+1));
			delete[] WorldVerts;


			// Update MaterialInstance
			LandscapeComponent->UpdateMaterialInstances();
		}
	}


	// Unlock the weightmaps' base mips
	for( INT AllocationIndex=0;AllocationIndex<TextureAllocations.Num();AllocationIndex++ )
	{
		UTexture2D* WeightmapTexture = TextureAllocations(AllocationIndex).Texture;
		FColor* BaseMipData = TextureAllocations(AllocationIndex).TextureData;

		// Generate mips for weightmaps
		ULandscapeComponent::GenerateWeightmapMips(NumSubsections, SubsectionSizeQuads, WeightmapTexture, BaseMipData);

		WeightmapTexture->Mips(0).Data.Unlock();
		WeightmapTexture->UpdateResource();
	}


	delete[] VertexNormals;

	// Generate mipmaps for the components, and create the collision components
	for (INT ComponentY = 0; ComponentY < NumSectionsY; ComponentY++)
	{
		for (INT ComponentX = 0; ComponentX < NumSectionsX; ComponentX++)
		{
			INT HmX = ComponentX / ComponentsPerHeightmap;
			INT HmY = ComponentY / ComponentsPerHeightmap;
			FHeightmapInfo& HeightmapInfo = HeightmapInfos(HmX + HmY * NumHeightmapsX);

			ULandscapeComponent* LandscapeComponent = LandscapeComponents(ComponentX + ComponentY*NumSectionsX);
			LandscapeComponent->GenerateHeightmapMips(HeightmapInfo.HeightmapTextureMipData);
			LandscapeComponent->UpdateCollisionComponent(HeightmapInfo.HeightmapTextureMipData(LandscapeComponent->CollisionMipLevel));
		}
	}

	for( INT HmIdx=0;HmIdx<HeightmapInfos.Num();HmIdx++ )
	{
		FHeightmapInfo& HeightmapInfo = HeightmapInfos(HmIdx);

		// Add remaining mips down to 1x1 to heightmap texture. These do not represent quads and are just a simple averages of the previous mipmaps. 
		// These mips are not used for sampling in the vertex shader but could be sampled in the pixel shader.
		INT Mip = HeightmapInfo.HeightmapTextureMipData.Num();
		INT MipSizeU = (HeightmapInfo.HeightmapTexture->SizeX) >> Mip;
		INT MipSizeV = (HeightmapInfo.HeightmapTexture->SizeY) >> Mip;
		while( MipSizeU > 1 && MipSizeV > 1 )
		{	
			// Create the mipmap
			FTexture2DMipMap* HeightmapMipMap = new(HeightmapInfo.HeightmapTexture->Mips) FTexture2DMipMap;
			HeightmapMipMap->SizeX = MipSizeU;
			HeightmapMipMap->SizeY = MipSizeV;
			HeightmapMipMap->Data.Lock(LOCK_READ_WRITE);
			HeightmapInfo.HeightmapTextureMipData.AddItem( (FColor*)HeightmapMipMap->Data.Realloc(MipSizeU*MipSizeV*sizeof(FColor)) );

			INT PrevMipSizeU = (HeightmapInfo.HeightmapTexture->SizeX) >> (Mip-1);
			INT PrevMipSizeV = (HeightmapInfo.HeightmapTexture->SizeY) >> (Mip-1);

			for( INT Y = 0;Y < MipSizeV;Y++ )
			{
				for( INT X = 0;X < MipSizeU;X++ )
				{
					FColor* TexData = &(HeightmapInfo.HeightmapTextureMipData(Mip))[ X + Y * MipSizeU ];

					FColor *PreMipTexData00 = &(HeightmapInfo.HeightmapTextureMipData(Mip-1))[ (X*2+0) + (Y*2+0)  * PrevMipSizeU ];
					FColor *PreMipTexData01 = &(HeightmapInfo.HeightmapTextureMipData(Mip-1))[ (X*2+0) + (Y*2+1)  * PrevMipSizeU ];
					FColor *PreMipTexData10 = &(HeightmapInfo.HeightmapTextureMipData(Mip-1))[ (X*2+1) + (Y*2+0)  * PrevMipSizeU ];
					FColor *PreMipTexData11 = &(HeightmapInfo.HeightmapTextureMipData(Mip-1))[ (X*2+1) + (Y*2+1)  * PrevMipSizeU ];

					TexData->R = (((INT)PreMipTexData00->R + (INT)PreMipTexData01->R + (INT)PreMipTexData10->R + (INT)PreMipTexData11->R) >> 2);
					TexData->G = (((INT)PreMipTexData00->G + (INT)PreMipTexData01->G + (INT)PreMipTexData10->G + (INT)PreMipTexData11->G) >> 2);
					TexData->B = (((INT)PreMipTexData00->B + (INT)PreMipTexData01->B + (INT)PreMipTexData10->B + (INT)PreMipTexData11->B) >> 2);
					TexData->A = (((INT)PreMipTexData00->A + (INT)PreMipTexData01->A + (INT)PreMipTexData10->A + (INT)PreMipTexData11->A) >> 2);
				}
			}

			Mip++;
			MipSizeU >>= 1;
			MipSizeV >>= 1;
		}

		for( INT i=0;i<HeightmapInfo.HeightmapTextureMipData.Num();i++ )
		{
			HeightmapInfo.HeightmapTexture->Mips(i).Data.Unlock();
		}
		HeightmapInfo.HeightmapTexture->UpdateResource();
	}

	// Update our new components
	ConditionalUpdateComponents();

	// Init RB physics for editor collision
	InitRBPhysEditor();

	GWarn->EndSlowTask();
}

UBOOL ALandscape::GetLandscapeExtent(INT& MinX, INT& MinY, INT& MaxX, INT& MaxY)
{
	MinX=MAXINT;
	MinY=MAXINT;
	MaxX=-MAXINT;
	MaxY=-MAXINT;

	// Find range of entire landscape
	for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
	{
		ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
		if( Comp )
		{
			if( Comp->SectionBaseX < MinX )
			{
				MinX = Comp->SectionBaseX;
			}
			if( Comp->SectionBaseY < MinY )
			{
				MinY = Comp->SectionBaseY;
			}
			if( Comp->SectionBaseX+ComponentSizeQuads > MaxX )
			{
				MaxX = Comp->SectionBaseX+ComponentSizeQuads;
			}
			if( Comp->SectionBaseY+ComponentSizeQuads > MaxY )
			{
				MaxY = Comp->SectionBaseY+ComponentSizeQuads;
			}
		}
	}
	return (MinX != MAXINT);
}

void ALandscape::Export(TArray<FString>& Filenames)
{
	check( Filenames.Num() > 0 );

	INT MinX=MAXINT;
	INT MinY=MAXINT;
	INT MaxX=-MAXINT;
	INT MaxY=-MAXINT;

	if( !GetLandscapeExtent(MinX,MinY,MaxX,MaxY) )
	{
		return;
	}

	GWarn->BeginSlowTask( TEXT("Exporting Landscape"), TRUE);

	FLandscapeEditDataInterface LandscapeEdit(this);

	TArray<BYTE> HeightData;
	HeightData.AddZeroed((1+MaxX-MinX)*(1+MaxY-MinY)*sizeof(WORD));
	LandscapeEdit.GetHeightData(MinX,MinY,MaxX,MaxY,(WORD*)&HeightData(0),0);
	appSaveArrayToFile(HeightData,*Filenames(0));

	for( INT i=1;i<Filenames.Num();i++ )
	{
		TArray<BYTE> WeightData;
		WeightData.AddZeroed((1+MaxX-MinX)*(1+MaxY-MinY));
		LandscapeEdit.GetWeightData(LayerInfos(i-1).LayerName, MinX,MinY,MaxX,MaxY,&WeightData(0),0);
		appSaveArrayToFile(WeightData,*Filenames(i));
	}

	GWarn->EndSlowTask();
}

void ALandscape::DeleteLayer(FName LayerName)
{
	GWarn->BeginSlowTask( TEXT("Deleting Layer"), TRUE);

	// Remove data from all components
	FLandscapeEditDataInterface LandscapeEdit(this);
	LandscapeEdit.DeleteLayer(LayerName);

	// Remove from array
	for( INT LayerIdx=0;LayerIdx<LayerInfos.Num();LayerIdx++ )
	{
		if( LayerInfos(LayerIdx).LayerName == LayerName )
		{
			LayerInfos.Remove(LayerIdx);
			break;
		}
	}

	GWarn->EndSlowTask();
}

void ALandscape::InitRBPhysEditor()
{
	InitRBPhys();
}

void ALandscape::PostEditMove(UBOOL bFinished)
{
	Super::PostEditMove(bFinished);

	if( bFinished )
	{
		for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
		{
			ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
			if( Comp )
			{
				Comp->UpdateCachedBounds();
				Comp->UpdateBounds();

				// Reattach all component
				FComponentReattachContext ReattachContext(Comp);
			}
		}

		// Update collision
		for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
		{
			ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
			if( Comp )
			{
				Comp->RecreateHeightfield();
			}
		}
	}
}

void ALandscape::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	UBOOL ChangedMaterial = FALSE;
	UBOOL NeedsRecalcBoundingBox = FALSE;
	if( PropertyName == FName(TEXT("LandscapeMaterial")) )
	{
		for (INT LayerIdx = 0; LayerIdx < LayerInfos.Num(); LayerIdx++)
		{
			LayerInfos(LayerIdx).ThumbnailMIC = NULL;
		}
		ChangedMaterial = TRUE;
	
		// Clear the parents out of combination material instances
		for( TMap<FString ,UMaterialInstanceConstant*>::TIterator It(MaterialInstanceConstantMap); It; ++It )
		{
			It.Value()->SetParent(NULL);
		}
		
		// Remove our references to any material instances
		MaterialInstanceConstantMap.Empty();
	}
	else if( PropertyName == FName(TEXT("DrawScale")) )
	{
		NeedsRecalcBoundingBox = TRUE;
	}
	else if ( GIsEditor && PropertyName == FName(TEXT("StreamingDistanceMultiplier")) )
	{
		// Recalculate in a few seconds.
		ULevel::TriggerStreamingDataRebuild();
	}
	else if ( GIsEditor && PropertyName == FName(TEXT("Hardness")) )
	{
		for (INT LayerIdx = 0; LayerIdx < LayerInfos.Num(); LayerIdx++)
		{
			LayerInfos(LayerIdx).Hardness = Clamp<FLOAT>(LayerInfos(LayerIdx).Hardness, 0.f, 1.f);
		}
	}

	for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
	{
		ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
		if( Comp )
		{
			if( NeedsRecalcBoundingBox )
			{
				Comp->UpdateCachedBounds();
				Comp->UpdateBounds();
			}

			if( ChangedMaterial )
			{
				// Update the MIC
				Comp->UpdateMaterialInstances();
			}

			// Reattach all components
			FComponentReattachContext ReattachContext(Comp);
		}
	}

	// Update collision
	if( NeedsRecalcBoundingBox )
	{
		for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
		{
			ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
			if( Comp )
			{
				Comp->RecreateHeightfield();
			}
		}
	}
}

void ALandscape::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	UBOOL NeedsRecalcBoundingBox = FALSE;

	if ( PropertyChangedEvent.PropertyChain.Num() > 0 )
	{
		UProperty* OutermostProperty = PropertyChangedEvent.PropertyChain.GetHead()->GetValue();
		if ( OutermostProperty != NULL )
		{
			const FName PropertyName = OutermostProperty->GetFName();
			if( PropertyName == FName(TEXT("DrawScale3D")) )
			{
				NeedsRecalcBoundingBox = TRUE;
			}
		}
	}

	if( NeedsRecalcBoundingBox )
	{
		for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
		{
			ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
			if( Comp )
			{
				Comp->UpdateCachedBounds();
				Comp->UpdateBounds();

				// Reattach all components
				FComponentReattachContext ReattachContext(Comp);
			}
		}
	}

	// Update collision
	if( NeedsRecalcBoundingBox )
	{
		for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
		{
			ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
			if( Comp )
			{
				Comp->RecreateHeightfield();
			}
		}
	}
}

struct FLandscapeDataInterface* ALandscape::GetDataInterface()
{
	if( DataInterface == NULL )
	{ 
		DataInterface = new FLandscapeDataInterface();
	}

	return DataInterface;
}


//
// FLandscapeEditDataInterface
//
FLandscapeEditDataInterface::FLandscapeEditDataInterface(ALandscape* InLandscape)
:	Landscape(InLandscape)
,	ComponentSizeQuads(InLandscape->ComponentSizeQuads)
,	SubsectionSizeQuads(InLandscape->SubsectionSizeQuads)
,	ComponentNumSubsections(InLandscape->NumSubsections)
,	DrawScale(InLandscape->DrawScale3D * InLandscape->DrawScale)
{
}

FLandscapeEditDataInterface::~FLandscapeEditDataInterface()
{
	Flush();
}

void FLandscapeEditDataInterface::Flush()
{
	UBOOL bNeedToWaitForUpdate = FALSE;

	// Update all textures
	for( TMap<UTexture2D*, FLandscapeTextureDataInfo*>::TIterator It(TextureDataMap); It;  ++It )
	{
		if( It.Value()->UpdateTextureData() )
		{
			bNeedToWaitForUpdate = TRUE;
		}
	}

	if( bNeedToWaitForUpdate )
	{
		FlushRenderingCommands();
	}

	// delete all the FLandscapeTextureDataInfo allocations
	for( TMap<UTexture2D*, FLandscapeTextureDataInfo*>::TIterator It(TextureDataMap); It;  ++It )
	{
		delete It.Value();
	}

	TextureDataMap.Empty();	// FLandscapeTextureDataInfo destructors will unlock any texture data
}


void FLandscapeEditDataInterface::GetComponentsInRegion(INT X1, INT Y1, INT X2, INT Y2, TSet<ULandscapeComponent*>& OutComponents)
{
	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{		
			ULandscapeComponent* Component = Landscape->XYtoComponentMap.FindRef(ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads));

			if( Component )
			{
				OutComponents.Add(Component);
			}
		}
	}
}

void FLandscapeEditDataInterface::SetHeightData(INT X1, INT Y1, INT X2, INT Y2, const WORD* Data, INT Stride, UBOOL CalcNormals, UBOOL CreateComponents)
{
	if( Stride==0 )
	{
		Stride = (1+X2-X1);
	}

	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	FVector* VertexNormals = NULL;
	if( CalcNormals )
	{
		// Calculate the normals for each of the two triangles per quad.
		// Note that the normals at the edges are not correct because they include normals
		// from triangles outside the current area. They are not updated
		INT NumVertsX = 1+X2-X1;
		INT NumVertsY = 1+Y2-Y1;
		VertexNormals = new FVector[NumVertsX*NumVertsY];
		appMemzero(VertexNormals, NumVertsX*NumVertsY*sizeof(FVector));

		for( INT Y=0;Y<NumVertsY-1;Y++ )
		{
			for( INT X=0;X<NumVertsX-1;X++ )
			{
				FVector Vert00 = FVector(0.f,0.f,((FLOAT)Data[(X+0) + Stride*(Y+0)] - 32768.f)/128.f) * DrawScale;
				FVector Vert01 = FVector(0.f,1.f,((FLOAT)Data[(X+0) + Stride*(Y+1)] - 32768.f)/128.f) * DrawScale;
				FVector Vert10 = FVector(1.f,0.f,((FLOAT)Data[(X+1) + Stride*(Y+0)] - 32768.f)/128.f) * DrawScale;
				FVector Vert11 = FVector(1.f,1.f,((FLOAT)Data[(X+1) + Stride*(Y+1)] - 32768.f)/128.f) * DrawScale;

				FVector FaceNormal1 = ((Vert00-Vert10) ^ (Vert10-Vert11)).SafeNormal();
				FVector FaceNormal2 = ((Vert11-Vert01) ^ (Vert01-Vert00)).SafeNormal(); 

				// contribute to the vertex normals.
				VertexNormals[(X+1 + NumVertsX*(Y+0))] += FaceNormal1;
				VertexNormals[(X+0 + NumVertsX*(Y+1))] += FaceNormal2;
				VertexNormals[(X+0 + NumVertsX*(Y+0))] += FaceNormal1 + FaceNormal2;
				VertexNormals[(X+1 + NumVertsX*(Y+1))] += FaceNormal1 + FaceNormal2;
			}
		}
	}

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{	
			QWORD ComponentKey = ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads);
			ULandscapeComponent* Component = Landscape->XYtoComponentMap.FindRef(ComponentKey);

			// if NULL, it was painted away
			if( Component==NULL )
			{
				if( CreateComponents )
				{
					// not yet implemented
					continue;
				}
				else
				{
					continue;
				}
			}

			FLandscapeTextureDataInfo* TexDataInfo = GetTextureDataInfo(Component->HeightmapTexture);
			FColor* HeightmapTextureData = (FColor*)TexDataInfo->GetMipData(0);

			// Find the texture data corresponding to this vertex
			INT SizeU = Component->HeightmapTexture->SizeX;
			INT SizeV = Component->HeightmapTexture->SizeY;
			INT HeightmapOffsetX = Component->HeightmapScaleBias.Z * (FLOAT)SizeU;
			INT HeightmapOffsetY = Component->HeightmapScaleBias.W * (FLOAT)SizeV;

			// Find coordinates of box that lies inside component
			INT ComponentX1 = Clamp<INT>(X1-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentY1 = Clamp<INT>(Y1-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentX2 = Clamp<INT>(X2-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentY2 = Clamp<INT>(Y2-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads);

			// Find subsection range for this box
			INT SubIndexX1 = Clamp<INT>((ComponentX1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);	// -1 because we need to pick up vertices shared between subsections
			INT SubIndexY1 = Clamp<INT>((ComponentY1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexX2 = Clamp<INT>(ComponentX2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexY2 = Clamp<INT>(ComponentY2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);

			// To adjust bounding box
			WORD MinHeight = MAXWORD;
			WORD MaxHeight = 0;

			for( INT SubIndexY=SubIndexY1;SubIndexY<=SubIndexY2;SubIndexY++ )
			{
				for( INT SubIndexX=SubIndexX1;SubIndexX<=SubIndexX2;SubIndexX++ )
				{
					// Find coordinates of box that lies inside subsection
					INT SubX1 = Clamp<INT>(ComponentX1-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads);
					INT SubY1 = Clamp<INT>(ComponentY1-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads);
					INT SubX2 = Clamp<INT>(ComponentX2-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads);
					INT SubY2 = Clamp<INT>(ComponentY2-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads);
				
					// Update texture data for the box that lies inside subsection
					for( INT SubY=SubY1;SubY<=SubY2;SubY++ )
					{
						for( INT SubX=SubX1;SubX<=SubX2;SubX++ )
						{
							INT LandscapeX = SubIndexX*SubsectionSizeQuads + ComponentIndexX*ComponentSizeQuads + SubX;
							INT LandscapeY = SubIndexY*SubsectionSizeQuads + ComponentIndexY*ComponentSizeQuads + SubY;
							checkSlow( LandscapeX >= X1 && LandscapeX <= X2 );
							checkSlow( LandscapeY >= Y1 && LandscapeY <= Y2 );

							// Find the input data corresponding to this vertex
							INT DataIndex = (LandscapeX-X1) + Stride * (LandscapeY-Y1);
							const WORD& Height = Data[DataIndex];

							// for bounding box
							if( Height < MinHeight )
							{
								MinHeight = Height;
							}
							if( Height > MaxHeight )
							{
								MaxHeight = Height;
							}

							INT TexX = HeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
							INT TexY = HeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;
							FColor& TexData = HeightmapTextureData[ TexX + TexY * SizeU ];

							// Update the texture
							TexData.R = Height >> 8;
							TexData.G = Height & 255;

							// Update normals if we're not on an edge vertex
							if( VertexNormals && LandscapeX > X1 && LandscapeX < X2 && LandscapeY > Y1 && LandscapeY < Y2 )
							{
								FVector Normal = VertexNormals[DataIndex].SafeNormal();
								TexData.B = appRound( 127.5f * (Normal.X + 1.f) );
								TexData.A = appRound( 127.5f * (Normal.Y + 1.f) );
							}
						}
					}

					// Record the areas of the texture we need to re-upload
					INT TexX1 = HeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX1;
					INT TexY1 = HeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY1;
					INT TexX2 = HeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX2;
					INT TexY2 = HeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY2;
					TexDataInfo->AddMipUpdateRegion(0,TexX1,TexY1,TexX2,TexY2);
				}
			}

			// See if we need to adjust the bounds. Note we never shrink the bounding box at this point
			FLOAT MinWorldZ = (((FLOAT)MinHeight - 32768.f)/128.f) * Landscape->DrawScale3D.Z * Landscape->DrawScale + Landscape->Location.Z;
			FLOAT MaxWorldZ = (((FLOAT)MaxHeight - 32768.f)/128.f) * Landscape->DrawScale3D.Z * Landscape->DrawScale + Landscape->Location.Z;

			UBOOL bUpdateBoxSphereBounds = FALSE;
			FBox Box = Component->CachedBoxSphereBounds.GetBox();
			if( MinWorldZ < Box.Min.Z )
			{
				Box.Min.Z = MinWorldZ;
				bUpdateBoxSphereBounds = TRUE;
			}
			if( MaxWorldZ > Box.Max.Z )
			{
				Box.Max.Z = MaxWorldZ;
				bUpdateBoxSphereBounds = TRUE;
			}

			if( bUpdateBoxSphereBounds )
			{
				Component->CachedBoxSphereBounds = FBoxSphereBounds(Box);
				Component->ConditionalUpdateTransform();
			}

			// Update mipmaps

			// Work out how many mips should be calculated directly from one component's data.
			// The remaining mips are calculated on a per texture basis.
			// eg if subsection is 7x7 quads, we need one 3 mips total: (8x8, 4x4, 2x2 verts)
			INT BaseNumMips = appCeilLogTwo(SubsectionSizeQuads+1);
			TArray<FColor*> MipData(BaseNumMips);
			MipData(0) = HeightmapTextureData;
			for( INT MipIdx=1;MipIdx<BaseNumMips;MipIdx++ )
			{
				MipData(MipIdx) = (FColor*)TexDataInfo->GetMipData(MipIdx);
			}
			Component->GenerateHeightmapMips( MipData, ComponentX1, ComponentY1, ComponentX2, ComponentY2, TexDataInfo );

			// Update collision
			Component->UpdateCollisionComponent(MipData(Component->CollisionMipLevel), ComponentX1, ComponentY1, ComponentX2, ComponentY2, bUpdateBoxSphereBounds );
		}
	}

	if( VertexNormals )
	{
		delete[] VertexNormals;
	}
}


template<typename TStoreData>
void FLandscapeEditDataInterface::GetHeightDataTempl(INT X1, INT Y1, INT X2, INT Y2, TStoreData& StoreData)
{
	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{		
			ULandscapeComponent* Component = Landscape->XYtoComponentMap.FindRef(ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads));

			FLandscapeTextureDataInfo* TexDataInfo = NULL;
			FColor* HeightmapTextureData = NULL;
			if( Component )
			{
				TexDataInfo = GetTextureDataInfo(Component->HeightmapTexture);
				HeightmapTextureData = (FColor*)TexDataInfo->GetMipData(0);
			}

			// Find coordinates of box that lies inside component
			INT ComponentX1 = Clamp<INT>(X1-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentY1 = Clamp<INT>(Y1-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentX2 = Clamp<INT>(X2-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentY2 = Clamp<INT>(Y2-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads+1);

			// Find subsection range for this box
			INT SubIndexX1 = Clamp<INT>((ComponentX1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);	// -1 because we need to pick up vertices shared between subsections
			INT SubIndexY1 = Clamp<INT>((ComponentY1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexX2 = Clamp<INT>(ComponentX2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexY2 = Clamp<INT>(ComponentY2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);

			for( INT SubIndexY=SubIndexY1;SubIndexY<=SubIndexY2;SubIndexY++ )
			{
				for( INT SubIndexX=SubIndexX1;SubIndexX<=SubIndexX2;SubIndexX++ )
				{
					// Find coordinates of box that lies inside subsection
					INT SubX1 = Clamp<INT>(ComponentX1-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads+1);
					INT SubY1 = Clamp<INT>(ComponentY1-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads+1);
					INT SubX2 = Clamp<INT>(ComponentX2-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads+1);
					INT SubY2 = Clamp<INT>(ComponentY2-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads+1);

					// Update texture data for the box that lies inside subsection
					for( INT SubY=SubY1;SubY<=SubY2;SubY++ )
					{
						for( INT SubX=SubX1;SubX<=SubX2;SubX++ )
						{
							INT LandscapeX = SubIndexX*SubsectionSizeQuads + ComponentIndexX*ComponentSizeQuads + SubX;
							INT LandscapeY = SubIndexY*SubsectionSizeQuads + ComponentIndexY*ComponentSizeQuads + SubY;

							// Find the input data corresponding to this vertex
							if( Component )
							{
								// Find the texture data corresponding to this vertex
								INT SizeU = Component->HeightmapTexture->SizeX;
								INT SizeV = Component->HeightmapTexture->SizeY;
								INT HeightmapOffsetX = Component->HeightmapScaleBias.Z * (FLOAT)SizeU;
								INT HeightmapOffsetY = Component->HeightmapScaleBias.W * (FLOAT)SizeV;

								INT TexX = HeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
								INT TexY = HeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;
								FColor& TexData = HeightmapTextureData[ TexX + TexY * SizeU ];

								WORD Height = (((WORD)TexData.R) << 8) | TexData.G;
								StoreData.Store(LandscapeX, LandscapeY, Height);
							}
							else
							{
								StoreData.StoreDefault(LandscapeX, LandscapeY);
							}

						}
					}
				}
			}
		}
	}
}

void FLandscapeEditDataInterface::GetHeightData(INT X1, INT Y1, INT X2, INT Y2, WORD* Data, INT Stride)
{
	if( Stride==0 )
	{
		Stride = (1+X2-X1);
	}

	struct FArrayStoreData
	{
		INT X1;
		INT Y1;
		WORD* Data;
		INT Stride;
		
		FArrayStoreData(INT InX1, INT InY1, WORD* InData, INT InStride)
		:	X1(InX1)
		,	Y1(InY1)
		,	Data(InData)
		,	Stride(InStride)
		{}

		inline void Store(INT LandscapeX, INT LandscapeY, WORD Height)
		{
			Data[ (LandscapeY-Y1) * Stride + (LandscapeX-X1) ] = Height;
		}

		inline void StoreDefault(INT LandscapeX, INT LandscapeY)
		{
			Data[ (LandscapeY-Y1) * Stride + (LandscapeX-X1) ] = 0;
		}
	};
	
	FArrayStoreData ArrayStoreData(X1, Y1, Data, Stride);
	GetHeightDataTempl(X1, Y1, X2, Y2, ArrayStoreData);
}

void FLandscapeEditDataInterface::GetHeightData(INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, WORD>& SparseData)
{
	struct FSparseStoreData
	{
		TMap<QWORD, WORD>& SparseData;
		
		FSparseStoreData(TMap<QWORD, WORD>& InSparseData)
		:	SparseData(InSparseData)
		{}

		inline void Store(INT LandscapeX, INT LandscapeY, WORD Height)
		{
			SparseData.Set(ALandscape::MakeKey(LandscapeX,LandscapeY), Height);
		}

		inline void StoreDefault(INT LandscapeX, INT LandscapeY)
		{
		}
	};

	FSparseStoreData SparseStoreData(SparseData);
	GetHeightDataTempl(X1, Y1, X2, Y2, SparseStoreData);
}

void FLandscapeEditDataInterface::DeleteLayer(FName LayerName)
{
	// Collect a map to all the layer infos
	TMap<FName, FLandscapeLayerInfo> LayerInfosMap;
	for (INT Idx = 0; Idx < Landscape->LayerInfos.Num(); Idx++)
	{
		LayerInfosMap.Set(Landscape->LayerInfos(Idx).LayerName, Landscape->LayerInfos(Idx));
	}

	for( INT ComponentIdx=0;ComponentIdx<Landscape->LandscapeComponents.Num();ComponentIdx++ )
	{
		ULandscapeComponent* Component = Landscape->LandscapeComponents(ComponentIdx);

		// Find the index for this layer in this component.
		INT DeleteLayerIdx = INDEX_NONE;
	
		for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
		{
			FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(LayerIdx);
			if( Allocation.LayerName == LayerName )
			{
				DeleteLayerIdx = LayerIdx;
			}
		}
		if( DeleteLayerIdx == INDEX_NONE )
		{
			// Layer not used for this component.
			continue;
		}

		FWeightmapLayerAllocationInfo& DeleteLayerAllocation = Component->WeightmapLayerAllocations(DeleteLayerIdx);
		INT DeleteLayerWeightmapTextureIndex = DeleteLayerAllocation.WeightmapTextureIndex;

		// See if we'll be able to remove the texture completely.
		UBOOL bCanRemoveLayerTexture = TRUE;
		for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
		{
			FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(LayerIdx);

			// check if we will be able to remove the texture also
			if( LayerIdx!=DeleteLayerIdx && Allocation.WeightmapTextureIndex == DeleteLayerWeightmapTextureIndex )
			{
				bCanRemoveLayerTexture = FALSE;
			}
		}


		// See if the deleted layer is a NoWeightBlend layer - if not, we don't have to worry about normalization
		FLandscapeLayerInfo* DeleteLayerInfo = LayerInfosMap.Find(LayerName);
		UBOOL bDeleteLayerIsNoWeightBlend = (DeleteLayerInfo && DeleteLayerInfo->bNoWeightBlend);

		if( !bDeleteLayerIsNoWeightBlend )
		{
			// Lock data for all the weightmaps
			TArray<FLandscapeTextureDataInfo*> TexDataInfos;
			for( INT WeightmapIdx=0;WeightmapIdx < Component->WeightmapTextures.Num();WeightmapIdx++ )
			{
				TexDataInfos.AddItem(GetTextureDataInfo(Component->WeightmapTextures(WeightmapIdx)));
			}

			// Channel remapping
			INT ChannelOffsets[4] = {STRUCT_OFFSET(FColor,R),STRUCT_OFFSET(FColor,G),STRUCT_OFFSET(FColor,B),STRUCT_OFFSET(FColor,A)};
	
			TArray<UBOOL> LayerNoWeightBlends;	// Array of NoWeightBlend flags
			TArray<BYTE*> LayerDataPtrs;		// Pointers to all layers' data 

			// Get the data for each layer
			for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
			{
				FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(LayerIdx);
				LayerDataPtrs.AddItem( (BYTE*)TexDataInfos(Allocation.WeightmapTextureIndex)->GetMipData(0) + ChannelOffsets[Allocation.WeightmapTextureChannel] );

				// Find the layer info and record if it is a bNoWeightBlend layer.
				FLandscapeLayerInfo* LayerInfo = LayerInfosMap.Find(Allocation.LayerName);
				LayerNoWeightBlends.AddItem( (LayerInfo && LayerInfo->bNoWeightBlend) );
			}

			// Find the texture data corresponding to this vertex
			INT SizeU = Component->WeightmapTextures(0)->SizeX;		// not exactly correct.  
			INT SizeV = Component->WeightmapTextures(0)->SizeY;
			INT WeightmapOffsetX = Component->WeightmapScaleBias.Z * (FLOAT)SizeU;
			INT WeightmapOffsetY = Component->WeightmapScaleBias.W * (FLOAT)SizeV;

			for( INT SubIndexY=0;SubIndexY<ComponentNumSubsections;SubIndexY++ )
			{
				for( INT SubIndexX=0;SubIndexX<ComponentNumSubsections;SubIndexX++ )
				{
					for( INT SubY=0;SubY<=SubsectionSizeQuads;SubY++ )
					{
						for( INT SubX=0;SubX<=SubsectionSizeQuads;SubX++ )
						{
							INT TexX = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
							INT TexY = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;
							INT TexDataIndex = 4 * (TexX + TexY * SizeU);

							// Calculate the sum of other layer weights
							INT OtherLayerWeightSum = 0;
							for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
							{
								if( LayerIdx != DeleteLayerIdx && LayerNoWeightBlends(LayerIdx)==FALSE )
								{
									OtherLayerWeightSum += LayerDataPtrs(LayerIdx)[TexDataIndex];
								}
							}

							if( OtherLayerWeightSum == 0 )
							{
								OtherLayerWeightSum = 255;
							}

							// Adjust other layer weights
							for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
							{
								if( LayerIdx != DeleteLayerIdx && LayerNoWeightBlends(LayerIdx)==FALSE )
								{
									BYTE& Weight = LayerDataPtrs(LayerIdx)[TexDataIndex];
									Weight = Clamp<INT>( appRound(255.f * (FLOAT)Weight/(FLOAT)OtherLayerWeightSum), 0, 255 );
								}
							}						
						}
					}
				}
			}

			// Update all the textures and mips
			for( INT Idx=0;Idx<Component->WeightmapTextures.Num();Idx++)
			{
				if( bCanRemoveLayerTexture && Idx==DeleteLayerWeightmapTextureIndex )
				{
					// We're going to remove this texture anyway, so don't bother updating
					continue;
				}

				UTexture2D* WeightmapTexture = Component->WeightmapTextures(Idx);
				FLandscapeTextureDataInfo* WeightmapDataInfo = TexDataInfos(Idx);

				INT NumMips = WeightmapTexture->Mips.Num();
				TArray<FColor*> WeightmapTextureMipData(NumMips);
				for( INT MipIdx=0;MipIdx<NumMips;MipIdx++ )
				{
					WeightmapTextureMipData(MipIdx) = (FColor*)WeightmapDataInfo->GetMipData(MipIdx);
				}

				ULandscapeComponent::UpdateWeightmapMips(Component->NumSubsections, Component->SubsectionSizeQuads, WeightmapTexture, WeightmapTextureMipData, 0, 0, MAXINT, MAXINT, WeightmapDataInfo);

				WeightmapDataInfo->AddMipUpdateRegion(0,0,0,WeightmapTexture->SizeX-1,WeightmapTexture->SizeY-1);
			}
		}

		// Mark the channel as unallocated, so we can reuse it later
		FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(Component->WeightmapTextures(DeleteLayerAllocation.WeightmapTextureIndex));
		check(Usage);
		Usage->ChannelUsage[DeleteLayerAllocation.WeightmapTextureChannel] = NULL;

		// Remove the layer
		Component->WeightmapLayerAllocations.Remove(DeleteLayerIdx);

		// If this layer was the last usage for this channel in this layer, we can remove it.
		if( bCanRemoveLayerTexture )
		{
			Component->WeightmapTextures.Remove(DeleteLayerWeightmapTextureIndex);

			// Adjust WeightmapTextureIndex index for other layers
			for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
			{
				FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(LayerIdx);

				if( Allocation.WeightmapTextureIndex > DeleteLayerWeightmapTextureIndex )
				{
					Allocation.WeightmapTextureIndex--;
				}

				check( Allocation.WeightmapTextureIndex < Component->WeightmapTextures.Num() );
			}
		}

		// Update the shaders for this component
		Component->UpdateMaterialInstances();
	}
}

void ULandscapeComponent::ReallocateWeightmaps(FLandscapeEditDataInterface* DataInterface)
{
	ALandscape* Landscape = GetLandscapeActor();
	
	INT NeededNewChannels=0;
	for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
	{
		if( WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex == 255 )
		{
			NeededNewChannels++;
		}
	}

	// All channels allocated!
	if( NeededNewChannels == 0 )
	{
		return;
	}

	Modify();
	Landscape->Modify();

	// debugf(TEXT("----------------------"));
	// debugf(TEXT("Component %s needs %d layers (%d new)"), *GetName(), WeightmapLayerAllocations.Num(), NeededNewChannels);

	// See if our existing textures have sufficient space
	INT ExistingTexAvailableChannels=0;
	for( INT TexIdx=0;TexIdx<WeightmapTextures.Num();TexIdx++ )
	{
		FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(WeightmapTextures(TexIdx));
		check(Usage);

		ExistingTexAvailableChannels += Usage->FreeChannelCount();

		if( ExistingTexAvailableChannels >= NeededNewChannels )
		{
			break;
		}
	}
	
	if( ExistingTexAvailableChannels >= NeededNewChannels )
	{
		// debugf(TEXT("Existing texture has available channels"));

		// Allocate using our existing textures' spare channels.
		for( INT TexIdx=0;TexIdx<WeightmapTextures.Num();TexIdx++ )
		{
			FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(WeightmapTextures(TexIdx));
			
			for( INT ChanIdx=0;ChanIdx<4;ChanIdx++ )
			{
				if( Usage->ChannelUsage[ChanIdx]==NULL )
				{
					for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
					{
						FWeightmapLayerAllocationInfo& AllocInfo = WeightmapLayerAllocations(LayerIdx);
						if( AllocInfo.WeightmapTextureIndex == 255 )
						{
							// Zero out the data for this texture channel
							if( DataInterface )
							{
								DataInterface->ZeroTextureChannel( WeightmapTextures(TexIdx), ChanIdx );
							}

							AllocInfo.WeightmapTextureIndex = TexIdx;
							AllocInfo.WeightmapTextureChannel = ChanIdx;
							Usage->ChannelUsage[ChanIdx] = this;
							NeededNewChannels--;

							if( NeededNewChannels == 0 )
							{
								return;
							}
						}
					}
				}
			}
		}
		// we should never get here.
		check(FALSE);
	}

	// debugf(TEXT("Reallocating."));

	// We are totally reallocating the weightmap
	INT TotalNeededChannels = WeightmapLayerAllocations.Num();
	INT CurrentLayer = 0;
	TArray<UTexture2D*> NewWeightmapTextures;
	while( TotalNeededChannels > 0 )
	{
		// debugf(TEXT("Still need %d channels"), TotalNeededChannels);

		UTexture2D* CurrentWeightmapTexture = NULL;
		FLandscapeWeightmapUsage* CurrentWeightmapUsage = NULL;

		if( TotalNeededChannels < 4 )
		{
			// debugf(TEXT("Looking for nearest"));

			// see if we can find a suitable existing weightmap texture with sufficient channels
			INT BestDistanceSquared = MAXINT;
			for( TMap<UTexture2D*,struct FLandscapeWeightmapUsage>::TIterator It(Landscape->WeightmapUsageMap); It; ++It )
			{
				FLandscapeWeightmapUsage* TryWeightmapUsage = &It.Value();
				if( TryWeightmapUsage->FreeChannelCount() >= TotalNeededChannels )
				{
					// See if this candidate is closer than any others we've found
					for( INT ChanIdx=0;ChanIdx<4;ChanIdx++ )
					{
						if( TryWeightmapUsage->ChannelUsage[ChanIdx] != NULL  )
						{
							INT TryDistanceSquared = Square(TryWeightmapUsage->ChannelUsage[ChanIdx]->SectionBaseX - SectionBaseX) + Square(TryWeightmapUsage->ChannelUsage[ChanIdx]->SectionBaseX - SectionBaseX);
							if( TryDistanceSquared < BestDistanceSquared )
							{
								CurrentWeightmapTexture = It.Key();
								CurrentWeightmapUsage = TryWeightmapUsage;
								BestDistanceSquared = TryDistanceSquared;
							}
						}
					}
				}
			}
		}

		UBOOL NeedsUpdateResource=FALSE;
		// No suitable weightmap texture
		if( CurrentWeightmapTexture == NULL )
		{
			MarkPackageDirty();

			// Weightmap is sized the same as the component
			INT WeightmapSize = (SubsectionSizeQuads+1) * NumSubsections;

			// We need a new weightmap texture
			CurrentWeightmapTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), GetOutermost(), NAME_None, RF_Public|RF_Standalone);
			CurrentWeightmapTexture->Init(WeightmapSize,WeightmapSize,PF_A8R8G8B8);
			CurrentWeightmapTexture->SRGB = FALSE;
			CurrentWeightmapTexture->CompressionNone = TRUE;
			CurrentWeightmapTexture->MipGenSettings = TMGS_LeaveExistingMips;
			CurrentWeightmapTexture->AddressX = TA_Clamp;
			CurrentWeightmapTexture->AddressY = TA_Clamp;
			CurrentWeightmapTexture->LODGroup = TEXTUREGROUP_Terrain_Weightmap;
			// Alloc dummy mips
			CreateEmptyWeightmapMips(CurrentWeightmapTexture);
			CurrentWeightmapTexture->UpdateResource();

			// Store it in the usage map
			CurrentWeightmapUsage = &Landscape->WeightmapUsageMap.Set(CurrentWeightmapTexture, FLandscapeWeightmapUsage());

			// debugf(TEXT("Making a new texture %s"), *CurrentWeightmapTexture->GetName());
		}

		NewWeightmapTextures.AddItem(CurrentWeightmapTexture);

		for( INT ChanIdx=0;ChanIdx<4 && TotalNeededChannels > 0;ChanIdx++ )
		{
			// debugf(TEXT("Finding allocation for layer %d"), CurrentLayer);

			if( CurrentWeightmapUsage->ChannelUsage[ChanIdx] == NULL  )
			{
				// Use this allocation
				FWeightmapLayerAllocationInfo& AllocInfo = WeightmapLayerAllocations(CurrentLayer);

				if( AllocInfo.WeightmapTextureIndex == 255 )
				{
					// New layer - zero out the data for this texture channel
					if( DataInterface )
					{
						DataInterface->ZeroTextureChannel( CurrentWeightmapTexture, ChanIdx );
						// debugf(TEXT("Zeroing out channel %s.%d"), *CurrentWeightmapTexture->GetName(), ChanIdx);
					}
				}
				else
				{
					UTexture2D* OldWeightmapTexture = WeightmapTextures(AllocInfo.WeightmapTextureIndex);

					// Copy the data
					if( DataInterface )
					{
						DataInterface->CopyTextureChannel( CurrentWeightmapTexture, ChanIdx, OldWeightmapTexture, AllocInfo.WeightmapTextureChannel );
						DataInterface->ZeroTextureChannel( OldWeightmapTexture, AllocInfo.WeightmapTextureChannel );
						// debugf(TEXT("Copying old channel (%s).%d to new channel (%s).%d"), *OldWeightmapTexture->GetName(), AllocInfo.WeightmapTextureChannel, *CurrentWeightmapTexture->GetName(), ChanIdx);
					}

					// Remove the old allocation
					FLandscapeWeightmapUsage* OldWeightmapUsage = Landscape->WeightmapUsageMap.Find(OldWeightmapTexture);
					OldWeightmapUsage->ChannelUsage[AllocInfo.WeightmapTextureChannel] = NULL;
				}

				// Assign the new allocation
				CurrentWeightmapUsage->ChannelUsage[ChanIdx] = this;
				AllocInfo.WeightmapTextureIndex = NewWeightmapTextures.Num()-1;
				AllocInfo.WeightmapTextureChannel = ChanIdx;
				CurrentLayer++;
				TotalNeededChannels--;
			}
		}
	}

	// Replace the weightmap textures
	WeightmapTextures = NewWeightmapTextures;

	// Update the mipmaps for the textures we edited
	for( INT Idx=0;Idx<WeightmapTextures.Num();Idx++)
	{
		UTexture2D* WeightmapTexture = WeightmapTextures(Idx);
		FLandscapeTextureDataInfo* WeightmapDataInfo = DataInterface->GetTextureDataInfo(WeightmapTexture);

		INT NumMips = WeightmapTexture->Mips.Num();
		TArray<FColor*> WeightmapTextureMipData(NumMips);
		for( INT MipIdx=0;MipIdx<NumMips;MipIdx++ )
		{
			WeightmapTextureMipData(MipIdx) = (FColor*)WeightmapDataInfo->GetMipData(MipIdx);
		}

		ULandscapeComponent::UpdateWeightmapMips(NumSubsections, SubsectionSizeQuads, WeightmapTexture, WeightmapTextureMipData, 0, 0, MAXINT, MAXINT, WeightmapDataInfo);
	}
}

// simple classes for the template....
namespace
{
	template<typename TDataType>
	struct FArrayStoreData
	{
		INT X1;
		INT Y1;
		TDataType* Data;
		INT Stride;
		INT ArraySize;

		FArrayStoreData(INT InX1, INT InY1, TDataType* InData, INT InStride)
			:	X1(InX1)
			,	Y1(InY1)
			,	Data(InData)
			,	Stride(InStride)
			,	ArraySize(1)
		{}

		inline void Store(INT LandscapeX, INT LandscapeY, BYTE Weight) {}
		inline void Store(INT LandscapeX, INT LandscapeY, BYTE Weight, INT LayerIdx) {}
		inline void PreInit(INT InArraySize) { ArraySize = InArraySize; }
	};

	void FArrayStoreData<BYTE>::Store(INT LandscapeX, INT LandscapeY, BYTE Weight)
	{
		Data[ (LandscapeY-Y1) * Stride + (LandscapeX-X1) ] = Weight;
	}

	// Data items should be initialized with ArraySize
	void FArrayStoreData<TArray<BYTE>>::Store(INT LandscapeX, INT LandscapeY, BYTE Weight, INT LayerIdx)
	{
		TArray<BYTE>& Value = Data[ ((LandscapeY-Y1) * Stride + (LandscapeX-X1)) ];
		if (Value.Num() != ArraySize)
		{
			Value.Empty(ArraySize);
			Value.AddZeroed(ArraySize);
		}
		Value(LayerIdx) = Weight;
	}

	template<typename TDataType>
	struct FSparseStoreData
	{
		TMap<QWORD, TDataType>& SparseData;
		INT ArraySize;

		FSparseStoreData(TMap<QWORD, TDataType>& InSparseData)
			:	SparseData(InSparseData)
			,	ArraySize(1)
		{}

		inline void Store(INT LandscapeX, INT LandscapeY, BYTE Weight) {}
		inline void Store(INT LandscapeX, INT LandscapeY, BYTE Weight, INT LayerIdx) {}
		inline void PreInit(INT InArraySize) { ArraySize = InArraySize; }
	};

	void FSparseStoreData<BYTE>::Store(INT LandscapeX, INT LandscapeY, BYTE Weight)
	{
		SparseData.Set(ALandscape::MakeKey(LandscapeX,LandscapeY), Weight);
	}

	void FSparseStoreData<TArray<BYTE>>::Store(INT LandscapeX, INT LandscapeY, BYTE Weight, INT LayerIdx)
	{
		TArray<BYTE>* Value = SparseData.Find(ALandscape::MakeKey(LandscapeX,LandscapeY));
		if (Value)
		{
			(*Value)(LayerIdx) = Weight;
		}
		else
		{
			TArray<BYTE> Value;
			Value.Empty(ArraySize);
			Value.AddZeroed(ArraySize);
			Value(LayerIdx) = Weight;
			SparseData.Set(ALandscape::MakeKey(LandscapeX,LandscapeY), Value);
		}
	}
};

void FLandscapeEditDataInterface::SetAlphaData(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, const BYTE* Data, INT Stride, UBOOL bWeightAdjust/* = TRUE */)
{
	if( Stride==0 )
	{
		Stride = (1+X2-X1);
	}

	// Collect a map to all the layer infos
	TMap<FName, FLandscapeLayerInfo> LayerInfosMap;
	for (INT Idx = 0; Idx < Landscape->LayerInfos.Num(); Idx++)
	{
		LayerInfosMap.Set(Landscape->LayerInfos(Idx).LayerName, Landscape->LayerInfos(Idx));
	}

	// Channel remapping
	INT ChannelOffsets[4] = {STRUCT_OFFSET(FColor,R),STRUCT_OFFSET(FColor,G),STRUCT_OFFSET(FColor,B),STRUCT_OFFSET(FColor,A)};

	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{	
			QWORD ComponentKey = ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads);
			ULandscapeComponent* Component = Landscape->XYtoComponentMap.FindRef(ComponentKey);

			// if NULL, it was painted away
			if( Component==NULL )
			{
				continue;
			}
			
			INT UpdateLayerIdx = INDEX_NONE;

			// If LayerName is passed in as NAME_None, we are updating all layers simultaneously.
			if( LayerName == NAME_None )
			{
				// If we have no layers at all, we need to allocate one.
				// Otherwise, we nominally choose the first layer.
				if( Component->WeightmapLayerAllocations.Num() > 0 )
				{
					UpdateLayerIdx = 0;
				}
			}
			else
			{
				for( INT LayerIdx=0;LayerIdx < Component->WeightmapLayerAllocations.Num();LayerIdx++ )
				{
					if( Component->WeightmapLayerAllocations(LayerIdx).LayerName == LayerName )
					{
						UpdateLayerIdx = LayerIdx;
						break;
					}
				}
			}

			// See if we need to reallocate our weightmaps
			if( UpdateLayerIdx == INDEX_NONE )
			{
				Component->Modify();
				UpdateLayerIdx = Component->WeightmapLayerAllocations.Num();
				new (Component->WeightmapLayerAllocations) FWeightmapLayerAllocationInfo(LayerName);
				Component->ReallocateWeightmaps(this);
				Component->UpdateMaterialInstances();
			}

			// Lock data for all the weightmaps
			TArray<FLandscapeTextureDataInfo*> TexDataInfos;
			for( INT WeightmapIdx=0;WeightmapIdx < Component->WeightmapTextures.Num();WeightmapIdx++ )
			{
				TexDataInfos.AddItem(GetTextureDataInfo(Component->WeightmapTextures(WeightmapIdx)));
			}

			TArray<BYTE*> LayerDataPtrs;		// Pointers to all layers' data 
			TArray<UBOOL> LayerNoWeightBlends;	// NoWeightBlend flags
			TArray<UBOOL> LayerEditDataAllZero; // Whether the data we are editing for this layer is all zero 
			TArray<UBOOL> LayerEditDataPrevNonzero; // Whether some data we are editing for this layer was previously non-zero 
			for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
			{
				FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(LayerIdx);

				LayerDataPtrs.AddItem( (BYTE*)TexDataInfos(Allocation.WeightmapTextureIndex)->GetMipData(0) + ChannelOffsets[Allocation.WeightmapTextureChannel] );

				// Find the layer info and record if it is a bNoWeightBlend layer.
				FLandscapeLayerInfo* LayerInfo = LayerInfosMap.Find(Allocation.LayerName);
				LayerNoWeightBlends.AddItem( (LayerInfo && LayerInfo->bNoWeightBlend) );
				LayerEditDataAllZero.AddItem(TRUE);
				LayerEditDataPrevNonzero.AddItem(FALSE);
			}

			for( INT LayerIdx=0;LayerIdx < Component->WeightmapLayerAllocations.Num();LayerIdx++ )
			{
				if( Component->WeightmapLayerAllocations(LayerIdx).LayerName == LayerName )
				{
					UpdateLayerIdx = LayerIdx;
					break;
				}
			}

			// Find the texture data corresponding to this vertex
			INT SizeU = Component->WeightmapTextures(0)->SizeX;		// not exactly correct.  
			INT SizeV = Component->WeightmapTextures(0)->SizeY;
			INT WeightmapOffsetX = Component->WeightmapScaleBias.Z * (FLOAT)SizeU;
			INT WeightmapOffsetY = Component->WeightmapScaleBias.W * (FLOAT)SizeV;

			// Find coordinates of box that lies inside component
			INT ComponentX1 = Clamp<INT>(X1-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentY1 = Clamp<INT>(Y1-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentX2 = Clamp<INT>(X2-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads);
			INT ComponentY2 = Clamp<INT>(Y2-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads);

			// Find subsection range for this box
			INT SubIndexX1 = Clamp<INT>((ComponentX1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);	// -1 because we need to pick up vertices shared between subsections
			INT SubIndexY1 = Clamp<INT>((ComponentY1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexX2 = Clamp<INT>(ComponentX2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexY2 = Clamp<INT>(ComponentY2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);

			for( INT SubIndexY=SubIndexY1;SubIndexY<=SubIndexY2;SubIndexY++ )
			{
				for( INT SubIndexX=SubIndexX1;SubIndexX<=SubIndexX2;SubIndexX++ )
				{
					// Find coordinates of box that lies inside subsection
					INT SubX1 = Clamp<INT>(ComponentX1-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads);
					INT SubY1 = Clamp<INT>(ComponentY1-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads);
					INT SubX2 = Clamp<INT>(ComponentX2-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads);
					INT SubY2 = Clamp<INT>(ComponentY2-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads);

					// Update texture data for the box that lies inside subsection
					for( INT SubY=SubY1;SubY<=SubY2;SubY++ )
					{
						for( INT SubX=SubX1;SubX<=SubX2;SubX++ )
						{
							INT LandscapeX = SubIndexX*SubsectionSizeQuads + ComponentIndexX*ComponentSizeQuads + SubX;
							INT LandscapeY = SubIndexY*SubsectionSizeQuads + ComponentIndexY*ComponentSizeQuads + SubY;
							checkSlow( LandscapeX >= X1 && LandscapeX <= X2 );
							checkSlow( LandscapeY >= Y1 && LandscapeY <= Y2 );

							// Find the input data corresponding to this vertex
							INT DataIndex = (LandscapeX-X1) + Stride * (LandscapeY-Y1);
							BYTE NewWeight = Data[DataIndex];

							// Adjust all layer weights
							INT TexX = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
							INT TexY = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;

							INT TexDataIndex = 4 * (TexX + TexY * SizeU);

							INT OtherLayerWeightSum = 0;

							if( bWeightAdjust )
							{
								// Adjust other layers' weights accordingly
								for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
								{
									BYTE ExistingWeight = LayerDataPtrs(LayerIdx)[TexDataIndex];
									// Exclude bNoWeightBlend layers
									if( LayerIdx != UpdateLayerIdx && LayerNoWeightBlends(LayerIdx)==FALSE )
									{
										OtherLayerWeightSum += ExistingWeight;
									}
								}

								if( OtherLayerWeightSum == 0 )
								{
									NewWeight = 255;
								}

								for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
								{
									BYTE& Weight = LayerDataPtrs(LayerIdx)[TexDataIndex];

									if( Weight != 0 )
									{
										LayerEditDataPrevNonzero(LayerIdx) = TRUE;
									}

									if( LayerIdx == UpdateLayerIdx )
									{
										Weight = NewWeight;
									}
									else
									{
										// Exclude bNoWeightBlend layers
										if( LayerNoWeightBlends(LayerIdx)==FALSE )
										{
											Weight = Clamp<INT>( appRound((FLOAT)(255 - NewWeight) * (FLOAT)Weight/(FLOAT)OtherLayerWeightSum), 0, 255 );
										}
									}

									if( Weight != 0 )
									{
										LayerEditDataAllZero(LayerIdx) = FALSE;
									}
								}
							}
							else
							{
								// Weight value set without adjusting other layers' weights
								
								// Apply weights to all layers simultaneously.
								if( LayerName == NAME_None ) 
								{
									for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
									{
										BYTE& Weight = LayerDataPtrs(LayerIdx)[TexDataIndex];

										if( Weight != 0 )
										{
											LayerEditDataPrevNonzero(LayerIdx) = TRUE;
										}

										// Find Index in LayerNames
										for( INT NameIdx = 0; NameIdx < Landscape->LayerInfos.Num(); NameIdx++ )
										{
											if( Landscape->LayerInfos(NameIdx).LayerName == Component->WeightmapLayerAllocations(LayerIdx).LayerName )
											{
												Weight = Data[DataIndex * Landscape->LayerInfos.Num() + NameIdx]; // Only for whole weight
												if( Weight != 0 )
												{
													LayerEditDataAllZero(LayerIdx) = FALSE;
												}
												break;
											}
										}
									}
								}
								else
								{
									BYTE& Weight = LayerDataPtrs(UpdateLayerIdx)[TexDataIndex];
									if( Weight != 0 )
									{
										LayerEditDataPrevNonzero(UpdateLayerIdx) = TRUE;
									}

									Weight = NewWeight;
									if( Weight != 0 )
									{
										LayerEditDataAllZero(UpdateLayerIdx) = FALSE;
									}
								}
							}
						}
					}

					// Record the areas of the texture we need to re-upload
					INT TexX1 = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX1;
					INT TexY1 = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY1;
					INT TexX2 = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX2;
					INT TexY2 = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY2;
					for( INT WeightmapIdx=0;WeightmapIdx < Component->WeightmapTextures.Num();WeightmapIdx++ )
					{
						TexDataInfos(WeightmapIdx)->AddMipUpdateRegion(0,TexX1,TexY1,TexX2,TexY2);
					}
				}
			}

			// Update mipmaps
			for( INT WeightmapIdx=0;WeightmapIdx < Component->WeightmapTextures.Num();WeightmapIdx++ )
			{
				UTexture2D* WeightmapTexture = Component->WeightmapTextures(WeightmapIdx);

				INT NumMips = WeightmapTexture->Mips.Num();
				TArray<FColor*> WeightmapTextureMipData(NumMips);
				for( INT MipIdx=0;MipIdx<NumMips;MipIdx++ )
				{
					WeightmapTextureMipData(MipIdx) = (FColor*)TexDataInfos(WeightmapIdx)->GetMipData(MipIdx);
				}

				ULandscapeComponent::UpdateWeightmapMips(ComponentNumSubsections, SubsectionSizeQuads, WeightmapTexture, WeightmapTextureMipData, ComponentX1, ComponentY1, ComponentX2, ComponentY2, TexDataInfos(WeightmapIdx));
			}

			// Check if we need to remove weightmap allocations for layers that were completely painted away
			UBOOL bRemovedLayer = FALSE;
			for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
			{
				if( LayerEditDataAllZero(LayerIdx) && LayerEditDataPrevNonzero(LayerIdx) )
				{
					// Check the data for the entire component and to see if it's all zero
					for( INT SubIndexY=0;SubIndexY<ComponentNumSubsections;SubIndexY++ )
					{
						for( INT SubIndexX=0;SubIndexX<ComponentNumSubsections;SubIndexX++ )
						{
							for( INT SubY=0;SubY<=SubsectionSizeQuads;SubY++ )
							{
								for( INT SubX=0;SubX<=SubsectionSizeQuads;SubX++ )
								{
									INT TexX = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
									INT TexY = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;
									INT TexDataIndex = 4 * (TexX + TexY * SizeU);

									// Stop the first time we see any non-zero data
									BYTE& Weight = LayerDataPtrs(LayerIdx)[TexDataIndex];
									if( Weight != 0 )
									{
										goto NextLayer;
									}
								}
							}
						}
					}
					
					// Mark the channel as unallocated, so we can reuse it later
					INT DeleteLayerWeightmapTextureIndex = Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex;
					FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(Component->WeightmapTextures(DeleteLayerWeightmapTextureIndex));
					check(Usage);
					Usage->ChannelUsage[Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel] = NULL;

					// Remove the layer as it's totally painted away.
					Component->WeightmapLayerAllocations.Remove(LayerIdx);
					LayerEditDataAllZero.Remove(LayerIdx);
					LayerEditDataPrevNonzero.Remove(LayerIdx);
					LayerDataPtrs.Remove(LayerIdx);

					// Check if the weightmap texture used by the layer we just removed is used by any other layer, and if so, remove the texture too
					UBOOL bCanRemoveLayerTexture = TRUE;
					for( INT OtherLayerIdx=0;OtherLayerIdx<Component->WeightmapLayerAllocations.Num();OtherLayerIdx++ )
					{
						if( Component->WeightmapLayerAllocations(OtherLayerIdx).WeightmapTextureIndex == DeleteLayerWeightmapTextureIndex )
						{
							bCanRemoveLayerTexture = FALSE;
							break;
						}
					}
					if( bCanRemoveLayerTexture )
					{
						Component->WeightmapTextures.Remove(DeleteLayerWeightmapTextureIndex);

						// Adjust WeightmapTextureChannel index for other layers
						for( INT OtherLayerIdx=0;OtherLayerIdx<Component->WeightmapLayerAllocations.Num();OtherLayerIdx++ )
						{
							FWeightmapLayerAllocationInfo& Allocation = Component->WeightmapLayerAllocations(OtherLayerIdx);
							if( Allocation.WeightmapTextureChannel > DeleteLayerWeightmapTextureIndex )
							{
								Allocation.WeightmapTextureChannel--;
							}
						}
					}

					bRemovedLayer = TRUE;
					LayerIdx--;
				}
				NextLayer:;
			}

			if( bRemovedLayer )
			{
				Component->UpdateMaterialInstances();
			}
		}
	}
}

template<typename TStoreData>
void FLandscapeEditDataInterface::GetWeightDataTempl(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, TStoreData& StoreData)
{
	// Find component range for this block of data
	INT ComponentIndexX1 = (X1-1) / ComponentSizeQuads;	// -1 because we need to pick up vertices shared between components
	INT ComponentIndexY1 = (Y1-1) / ComponentSizeQuads;
	INT ComponentIndexX2 = X2 / ComponentSizeQuads;
	INT ComponentIndexY2 = Y2 / ComponentSizeQuads;

	// Channel remapping
	INT ChannelOffsets[4] = {STRUCT_OFFSET(FColor,R),STRUCT_OFFSET(FColor,G),STRUCT_OFFSET(FColor,B),STRUCT_OFFSET(FColor,A)};

	for( INT ComponentIndexY=ComponentIndexY1;ComponentIndexY<=ComponentIndexY2;ComponentIndexY++ )
	{
		for( INT ComponentIndexX=ComponentIndexX1;ComponentIndexX<=ComponentIndexX2;ComponentIndexX++ )
		{		
			ULandscapeComponent* Component = Landscape->XYtoComponentMap.FindRef(ALandscape::MakeKey(ComponentIndexX*ComponentSizeQuads,ComponentIndexY*ComponentSizeQuads));

			if( !Component )
			{
				continue;
			}

			UTexture2D* WeightmapTexture = NULL;
			FLandscapeTextureDataInfo* TexDataInfo = NULL;
			BYTE* WeightmapTextureData = NULL;
			BYTE WeightmapChannelOffset = 0;
			TArray<FLandscapeTextureDataInfo*> TexDataInfos; // added for whole weight case...

			if (LayerName != NAME_None)
			{
				for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
				{
					if( Component->WeightmapLayerAllocations(LayerIdx).LayerName == LayerName )
					{
						WeightmapTexture = Component->WeightmapTextures(Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex);
						TexDataInfo = GetTextureDataInfo(WeightmapTexture);
						WeightmapTextureData = (BYTE*)TexDataInfo->GetMipData(0);
						WeightmapChannelOffset = ChannelOffsets[Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel];
						break;
					}
				}
			}
			else
			{
				// Lock data for all the weightmaps
				for( INT WeightmapIdx=0;WeightmapIdx < Component->WeightmapTextures.Num();WeightmapIdx++ )
				{
					TexDataInfos.AddItem(GetTextureDataInfo(Component->WeightmapTextures(WeightmapIdx)));
				}
			}

			// Find coordinates of box that lies inside component
			INT ComponentX1 = Clamp<INT>(X1-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentY1 = Clamp<INT>(Y1-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentX2 = Clamp<INT>(X2-ComponentIndexX*ComponentSizeQuads, 0, ComponentSizeQuads+1);
			INT ComponentY2 = Clamp<INT>(Y2-ComponentIndexY*ComponentSizeQuads, 0, ComponentSizeQuads+1);

			// Find subsection range for this box
			INT SubIndexX1 = Clamp<INT>((ComponentX1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);	// -1 because we need to pick up vertices shared between subsections
			INT SubIndexY1 = Clamp<INT>((ComponentY1-1) / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexX2 = Clamp<INT>(ComponentX2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);
			INT SubIndexY2 = Clamp<INT>(ComponentY2 / SubsectionSizeQuads,0,ComponentNumSubsections-1);

			for( INT SubIndexY=SubIndexY1;SubIndexY<=SubIndexY2;SubIndexY++ )
			{
				for( INT SubIndexX=SubIndexX1;SubIndexX<=SubIndexX2;SubIndexX++ )
				{
					// Find coordinates of box that lies inside subsection
					INT SubX1 = Clamp<INT>(ComponentX1-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads+1);
					INT SubY1 = Clamp<INT>(ComponentY1-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads+1);
					INT SubX2 = Clamp<INT>(ComponentX2-SubsectionSizeQuads*SubIndexX, 0, SubsectionSizeQuads+1);
					INT SubY2 = Clamp<INT>(ComponentY2-SubsectionSizeQuads*SubIndexY, 0, SubsectionSizeQuads+1);

					// Update texture data for the box that lies inside subsection
					for( INT SubY=SubY1;SubY<=SubY2;SubY++ )
					{
						for( INT SubX=SubX1;SubX<=SubX2;SubX++ )
						{
							INT LandscapeX = SubIndexX*SubsectionSizeQuads + ComponentIndexX*ComponentSizeQuads + SubX;
							INT LandscapeY = SubIndexY*SubsectionSizeQuads + ComponentIndexY*ComponentSizeQuads + SubY;

							if (LayerName != NAME_None)
							{
								// Find the input data corresponding to this vertex
								BYTE Weight;
								if( WeightmapTexture )
								{
									// Find the texture data corresponding to this vertex
									INT SizeU = WeightmapTexture->SizeX;
									INT SizeV = WeightmapTexture->SizeY;
									INT WeightmapOffsetX = Component->WeightmapScaleBias.Z * (FLOAT)SizeU;
									INT WeightmapOffsetY = Component->WeightmapScaleBias.W * (FLOAT)SizeV;

									INT TexX = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
									INT TexY = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;
									Weight = WeightmapTextureData[ 4 * (TexX + TexY * SizeU) + WeightmapChannelOffset ];
								}
								else
								{
									Weight = 0;
								}

								StoreData.Store(LandscapeX, LandscapeY, Weight);
							}
							else // Whole weight map case...
							{
								StoreData.PreInit(Landscape->LayerInfos.Num());
								for( INT LayerIdx=0;LayerIdx<Component->WeightmapLayerAllocations.Num();LayerIdx++ )
								{
									INT Idx = Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex;
									UTexture2D* WeightmapTexture = Component->WeightmapTextures(Idx);
									BYTE* WeightmapTextureData = (BYTE*)TexDataInfos(Idx)->GetMipData(0);
									BYTE WeightmapChannelOffset = ChannelOffsets[Component->WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel];

									// Find the texture data corresponding to this vertex
									INT SizeU = WeightmapTexture->SizeX;
									INT SizeV = WeightmapTexture->SizeY;
									INT WeightmapOffsetX = Component->WeightmapScaleBias.Z * (FLOAT)SizeU;
									INT WeightmapOffsetY = Component->WeightmapScaleBias.W * (FLOAT)SizeV;

									INT TexX = WeightmapOffsetX + (SubsectionSizeQuads+1) * SubIndexX + SubX;
									INT TexY = WeightmapOffsetY + (SubsectionSizeQuads+1) * SubIndexY + SubY;

									BYTE Weight = WeightmapTextureData[ 4 * (TexX + TexY * SizeU) + WeightmapChannelOffset ];

									// Find in index in LayerName
									for ( INT NameIdx = 0; NameIdx < Landscape->LayerInfos.Num(); NameIdx++ )
									{
										if (Landscape->LayerInfos(NameIdx).LayerName == Component->WeightmapLayerAllocations(LayerIdx).LayerName)
										{
											StoreData.Store(LandscapeX, LandscapeY, Weight, NameIdx);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void FLandscapeEditDataInterface::GetWeightData(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, BYTE* Data, INT Stride)
{
	if( Stride==0 )
	{
		Stride = (1+X2-X1);
	}
	FArrayStoreData<BYTE> ArrayStoreData(X1, Y1, Data, Stride);
	GetWeightDataTempl(LayerName, X1, Y1, X2, Y2, ArrayStoreData);
}

void FLandscapeEditDataInterface::GetWeightData(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, BYTE>& SparseData)
{
	FSparseStoreData<BYTE> SparseStoreData(SparseData);
	GetWeightDataTempl(LayerName, X1, Y1, X2, Y2, SparseStoreData);
}

void FLandscapeEditDataInterface::GetWeightData(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, TArray<BYTE>* Data, INT Stride)
{
	if( Stride==0 )
	{
		Stride = (1+X2-X1);
	}
	FArrayStoreData<TArray<BYTE>> ArrayStoreData(X1, Y1, Data, Stride);
	GetWeightDataTempl(LayerName, X1, Y1, X2, Y2, ArrayStoreData);
}

void FLandscapeEditDataInterface::GetWeightData(FName LayerName, INT X1, INT Y1, INT X2, INT Y2, TMap<QWORD, TArray<BYTE>>& SparseData)
{
	FSparseStoreData<TArray<BYTE>> SparseStoreData(SparseData);
	GetWeightDataTempl(LayerName, X1, Y1, X2, Y2, SparseStoreData);
}

FLandscapeTextureDataInfo* FLandscapeEditDataInterface::GetTextureDataInfo(UTexture2D* Texture)
{
	FLandscapeTextureDataInfo* Result = TextureDataMap.FindRef(Texture);
	if( !Result )
	{
		Result = TextureDataMap.Set(Texture, new FLandscapeTextureDataInfo(Texture));
	}
	return Result;
}

void FLandscapeEditDataInterface::CopyTextureChannel(UTexture2D* Dest, INT DestChannel, UTexture2D* Src, INT SrcChannel)
{
	FLandscapeTextureDataInfo* DestDataInfo = GetTextureDataInfo(Dest);
	FLandscapeTextureDataInfo* SrcDataInfo = GetTextureDataInfo(Src);
	INT MipSize = Dest->SizeX;
	check(Dest->SizeX == Dest->SizeY && Src->SizeX == Dest->SizeX);

	// Channel remapping
	INT ChannelOffsets[4] = {STRUCT_OFFSET(FColor,R),STRUCT_OFFSET(FColor,G),STRUCT_OFFSET(FColor,B),STRUCT_OFFSET(FColor,A)};

	for( INT MipIdx=0;MipIdx<DestDataInfo->NumMips();MipIdx++ )
	{
		BYTE* DestTextureData = (BYTE*)DestDataInfo->GetMipData(MipIdx) + ChannelOffsets[DestChannel];
		BYTE* SrcTextureData = (BYTE*)SrcDataInfo->GetMipData(MipIdx) + ChannelOffsets[SrcChannel];

		for( INT i=0;i<Square(MipSize);i++ )
		{
			DestTextureData[i*4] = SrcTextureData[i*4];
		}

		DestDataInfo->AddMipUpdateRegion(MipIdx, 0, 0, MipSize-1, MipSize-1);
		MipSize >>= 1;
	}
}

void FLandscapeEditDataInterface::ZeroTextureChannel(UTexture2D* Dest, INT DestChannel)
{
	FLandscapeTextureDataInfo* DestDataInfo = GetTextureDataInfo(Dest);
	INT MipSize = Dest->SizeX;
	check(Dest->SizeX == Dest->SizeY);

	// Channel remapping
	INT ChannelOffsets[4] = {STRUCT_OFFSET(FColor,R),STRUCT_OFFSET(FColor,G),STRUCT_OFFSET(FColor,B),STRUCT_OFFSET(FColor,A)};

	for( INT MipIdx=0;MipIdx<DestDataInfo->NumMips();MipIdx++ )
	{
		BYTE* DestTextureData = (BYTE*)DestDataInfo->GetMipData(MipIdx) + ChannelOffsets[DestChannel];

		for( INT i=0;i<Square(MipSize);i++ )
		{
			DestTextureData[i*4] = 0;
		}

		DestDataInfo->AddMipUpdateRegion(MipIdx, 0, 0, MipSize-1, MipSize-1);
		MipSize >>= 1;
	}
}

//
// FLandscapeTextureDataInfo
//

FLandscapeTextureDataInfo::FLandscapeTextureDataInfo(UTexture2D* InTexture)
:	Texture(InTexture)
{
	MipInfo.AddZeroed(Texture->Mips.Num());
	for( INT MipIdx=0;MipIdx<Texture->Mips.Num();MipIdx++ )
	{
		Texture->Mips(MipIdx).Data.ForceBulkDataResident();
	}

	if( Texture->bHasBeenLoadedFromPersistentArchive )
	{
		// We want to prevent the texture from being streamed again as we've just edited the data
		Texture->bHasBeenLoadedFromPersistentArchive = FALSE;

		// Update the entire resource, which will update all mip levels.
		Texture->UpdateResource();
	}

	Texture->SetFlags(RF_Transactional);
	Texture->Modify();
}

UBOOL FLandscapeTextureDataInfo::UpdateTextureData()
{
	UBOOL bNeedToWaitForUpdate = FALSE;

	for( INT i=0;i<MipInfo.Num();i++ )
	{
		if( MipInfo(i).MipData && MipInfo(i).MipUpdateRegions.Num()>0 )
		{
			Texture->UpdateTextureRegions( i, MipInfo(i).MipUpdateRegions.Num(), &MipInfo(i).MipUpdateRegions(0), ((Texture->SizeX)>>i)*sizeof(FColor), 4, (BYTE*)MipInfo(i).MipData, FALSE );
			bNeedToWaitForUpdate = TRUE;
		}
	}

	return bNeedToWaitForUpdate;
}

FLandscapeTextureDataInfo::~FLandscapeTextureDataInfo()
{
	// Unlock any mips still locked.
	for( INT i=0;i<MipInfo.Num();i++ )
	{
		if( MipInfo(i).MipData )
		{
			if( MipInfo(i).MipData )
			{
				Texture->Mips(i).Data.Unlock();
				MipInfo(i).MipData = NULL;
			}
		}
	}
	Texture->ClearFlags(RF_Transactional);
}

#endif //WITH_EDITOR