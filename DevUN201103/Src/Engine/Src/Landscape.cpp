/*=============================================================================
Landscape.cpp: New terrain rendering
Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnTerrain.h"
#include "LandscapeDataAccess.h"
#include "LandscapeRender.h"

IMPLEMENT_CLASS(ULandscapeComponent)


//
// ULandscapeComponent
//
void ULandscapeComponent::AddReferencedObjects(TArray<UObject*>& ObjectArray)
{
	Super::AddReferencedObjects(ObjectArray);
	if(LightMap != NULL)
	{
		LightMap->AddReferencedObjects(ObjectArray);
	}
}

void ULandscapeComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.Ver() >= VER_LANDSCAPECOMPONENT_LIGHTMAPS)
	{
		Ar << LightMap;
	}
}

#if WITH_EDITOR
/**
 * Generate a key for this component's layer allocations to use with MaterialInstanceConstantMap.
 */

IMPLEMENT_COMPARE_CONSTREF( FString, Landscape, { return A < B ? 1 : -1; } );

FString ULandscapeComponent::GetLayerAllocationKey() const
{
	FString Result;
	ALandscape* Landscape = GetLandscapeActor();

	// Sort the allocations
	TArray<FString> LayerStrings;
	for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
	{
		new(LayerStrings) FString( *FString::Printf(TEXT("%s_%d_"), *WeightmapLayerAllocations(LayerIdx).LayerName.ToString(), WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex) );
	}
	Sort<USE_COMPARE_CONSTREF(FString,Landscape)>( &LayerStrings(0), LayerStrings.Num() );

	for( INT LayerIdx=0;LayerIdx < LayerStrings.Num();LayerIdx++ )
	{
		Result += LayerStrings(LayerIdx);
	}
	return Result;
}

FString ULandscapeComponent::GetLayerDebugColorKey(INT& R, INT& G, INT& B) const
{
	FString Result;

	ALandscape* Landscape = GetLandscapeActor();
	R = INDEX_NONE, G = INDEX_NONE, B = INDEX_NONE;
	for ( INT LIndex = 0; LIndex < Landscape->LayerInfos.Num(); LIndex++)
	{
		if (Landscape->LayerInfos(LIndex).DebugColorChannel > 0)
		{
			for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )	
			{
				if (WeightmapLayerAllocations(LayerIdx).LayerName == Landscape->LayerInfos(LIndex).LayerName)
				{
					if ( Landscape->LayerInfos(LIndex).DebugColorChannel & 1 ) // R
					{
						R = (WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex*4+WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel);
					}
					if ( Landscape->LayerInfos(LIndex).DebugColorChannel & 2 ) // G
					{
						G = (WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex*4+WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel);
					}
					if ( Landscape->LayerInfos(LIndex).DebugColorChannel & 4 ) // B
					{
						B = (WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex*4+WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel);
					}
					break;
				}
			}		
		}
	}
	Result = *FString::Printf(TEXT("R.%d-G.%d-B.%d"), R, G, B);		
	return Result;
}

void ALandscape::UpdateDebugColorMaterial()
{
	GWarn->BeginSlowTask( *FString::Printf(TEXT("Compiling layer color combinations for %s"), *GetName()), TRUE);
	for (INT Idx = 0; Idx < LandscapeComponents.Num(); Idx++ )
	{
		if (LandscapeComponents(Idx) && LandscapeComponents(Idx)->EditToolRenderData)
		{
			LandscapeComponents(Idx)->EditToolRenderData->UpdateDebugColorMaterial();
		}
	}
	GWarn->EndSlowTask();
}

void ULandscapeComponent::SetupActor()
{
	if( GIsEditor && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		ALandscape* Landscape = GetLandscapeActor();

		// Store the components in the map
		Landscape->XYtoComponentMap.Set(ALandscape::MakeKey(SectionBaseX,SectionBaseY), this);

		UBOOL bFixedWeightmapTextureIndex=FALSE;

		// Store the weightmap allocations in WeightmapUsageMap
		for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
		{
			FWeightmapLayerAllocationInfo& Allocation = WeightmapLayerAllocations(LayerIdx);

			// Fix up any problems caused by the layer deletion bug.
			if( Allocation.WeightmapTextureIndex >= WeightmapTextures.Num() )
			{
				Allocation.WeightmapTextureIndex = WeightmapTextures.Num()-1; 
				if( !bFixedWeightmapTextureIndex )
				{
					GWarn->MapCheck_Add( MCTYPE_WARNING, Landscape, *FString::Printf(TEXT("Fixed up incorrect layer weightmap texture index for %s."), *GetName() ), MCACTION_NONE );
					bFixedWeightmapTextureIndex = TRUE;
				}
			}

			UTexture2D* WeightmapTexture = WeightmapTextures(Allocation.WeightmapTextureIndex);
			FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(WeightmapTexture);
			if( Usage == NULL )
			{
				Usage = &Landscape->WeightmapUsageMap.Set(WeightmapTexture,FLandscapeWeightmapUsage());
			}

			// Detect a shared layer allocation, caused by a previous undo or layer deletion bugs
			if( Usage->ChannelUsage[Allocation.WeightmapTextureChannel] != NULL )
			{
				GWarn->MapCheck_Add( MCTYPE_WARNING, Landscape, *FString::Printf(TEXT("Fixed up shared weightmap texture for layer %s in component %s (shares with %s)."), *Allocation.LayerName.ToString(), *GetName(), *Usage->ChannelUsage[Allocation.WeightmapTextureChannel]->GetName() ), MCACTION_NONE );
				WeightmapLayerAllocations.Remove(LayerIdx);
				LayerIdx--;
				continue;
			}
			else
			{
				Usage->ChannelUsage[Allocation.WeightmapTextureChannel] = this;
			}
		}

		// Store the layer combination in the MaterialInstanceConstantMap
		if( MaterialInstance != NULL )
		{
			UMaterialInstanceConstant* CombinationMaterialInstance = Cast<UMaterialInstanceConstant>(MaterialInstance->Parent);
			if( CombinationMaterialInstance )
			{
				Landscape->MaterialInstanceConstantMap.Set(*GetLayerAllocationKey(),CombinationMaterialInstance);
			}
		}
	}
}

void ULandscapeComponent::PostLoad()
{
	Super::PostLoad();
	SetupActor();
}

void ULandscapeComponent::PostEditImport()
{
	Super::PostEditImport();
	SetupActor();
}
#endif

void ULandscapeComponent::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	if( GIsEditor && !HasAnyFlags(RF_ClassDefaultObject) )
	{
		ALandscape* Landscape = GetLandscapeActor();

		Landscape->XYtoComponentMap.Remove(ALandscape::MakeKey(SectionBaseX,SectionBaseY));

		// Remove any weightmap allocations from the Landscape Actor's map
		for( INT LayerIdx=0;LayerIdx < WeightmapLayerAllocations.Num();LayerIdx++ )
		{
			UTexture2D* WeightmapTexture = WeightmapTextures(WeightmapLayerAllocations(LayerIdx).WeightmapTextureIndex);
			FLandscapeWeightmapUsage* Usage = Landscape->WeightmapUsageMap.Find(WeightmapTexture);
			if( Usage != NULL )
			{
				Usage->ChannelUsage[WeightmapLayerAllocations(LayerIdx).WeightmapTextureChannel] = NULL;

				if( Usage->FreeChannelCount()==4 )
				{
					Landscape->WeightmapUsageMap.Remove(WeightmapTexture);
				}
			}
		}
	}

	if( EditToolRenderData != NULL )
	{
		// Ask render thread to destroy EditToolRenderData
		EditToolRenderData->Cleanup();
		EditToolRenderData = NULL;
	}
#endif
}

FPrimitiveSceneProxy* ULandscapeComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = NULL;
#if WITH_EDITOR
	if( EditToolRenderData == NULL )
	{
		EditToolRenderData = new FLandscapeEditToolRenderData(this);
	}
	Proxy = new FLandscapeComponentSceneProxy(this, EditToolRenderData);
#else
	Proxy = new FLandscapeComponentSceneProxy(this, NULL);
#endif
	return Proxy;
}

void ULandscapeComponent::UpdateBounds()
{
	Bounds = CachedBoxSphereBounds;
#if !CONSOLE
	if ( !GIsGame && Scene->GetWorld() == GWorld )
	{
		ULevel::TriggerStreamingDataRebuild();
	}
#endif
}

void ULandscapeComponent::SetParentToWorld(const FMatrix& ParentToWorld)
{
	Super::SetParentToWorld(FTranslationMatrix(FVector(SectionBaseX,SectionBaseY,0)) * ParentToWorld);
}

/** 
* Retrieves the materials used in this component 
* 
* @param OutMaterials	The list of used materials.
*/
void ULandscapeComponent::GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const
{
	if( MaterialInstance != NULL )
	{
		OutMaterials.AddItem(MaterialInstance);
	}
}


//
// ALandscape
//
void ALandscape::InitRBPhys()
{
#if WITH_NOVODEX
	if (!GWorld->RBPhysScene)
	{
		return;
	}
#endif // WITH_NOVODEX

	for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
	{
		ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
		if( Comp && Comp->IsAttached() )
		{
			Comp->InitComponentRBPhys(TRUE);
		}
	}
}

void ALandscape::UpdateComponentsInternal(UBOOL bCollisionUpdate)
{
	Super::UpdateComponentsInternal(bCollisionUpdate);

	const FMatrix&	ActorToWorld = LocalToWorld();

	// Render components
	for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
	{
		ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
		if( Comp )
		{
			Comp->UpdateComponent(GWorld->Scene,this,ActorToWorld);
		}
	}

	// Collision components
	for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
	{
		ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
		if( Comp )
		{
			Comp->UpdateComponent(GWorld->Scene,this,ActorToWorld);
		}
	}
}

void ALandscape::ClearComponents()
{
	// wait until resources are released
	FlushRenderingCommands();

	Super::ClearComponents();

	// Render components
	for(INT ComponentIndex = 0;ComponentIndex < LandscapeComponents.Num();ComponentIndex++)
	{
		ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
		if (Comp)
		{
			Comp->ConditionalDetach();
		}
	}

	// Collision components
	for(INT ComponentIndex = 0; ComponentIndex < CollisionComponents.Num(); ComponentIndex++ )
	{
		ULandscapeHeightfieldCollisionComponent* Comp = CollisionComponents(ComponentIndex);
		if( Comp )
		{
			Comp->ConditionalDetach();
		}
	}
}


// FLandscapeWeightmapUsage serializer
FArchive& operator<<( FArchive& Ar, FLandscapeWeightmapUsage& U )
{
	return Ar << U.ChannelUsage[0] << U.ChannelUsage[1] << U.ChannelUsage[2] << U.ChannelUsage[3];
}

void ALandscape::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if( !(Ar.IsLoading() || Ar.IsSaving()) )
	{
		Ar << MaterialInstanceConstantMap;
	}

	// For Undo
	if( Ar.IsTransacting() )
	{
		Ar << WeightmapUsageMap;
	}

	// We do not serialize XYtoComponentMap as we don't want these references to hold a component in memory.
	// The references are automatically cleaned up by the components' BeginDestroy method.
}

void ALandscape::PostLoad()
{
	Super::PostLoad();

	// Temporary
	if( ComponentSizeQuads == 0 && LandscapeComponents.Num() > 0 )
	{
		ULandscapeComponent* Comp = LandscapeComponents(0);
		if( Comp )
		{
			ComponentSizeQuads = Comp->ComponentSizeQuads;
			SubsectionSizeQuads = Comp->SubsectionSizeQuads;	
			NumSubsections = Comp->NumSubsections;
		}
	}

	// For the LayerName legacy
	if (LayerNames_DEPRECATED.Num() && !LayerInfos.Num())
	{
		for (INT Idx = 0; Idx < LayerNames_DEPRECATED.Num(); Idx++)
		{
			new(LayerInfos) FLandscapeLayerInfo(LayerNames_DEPRECATED(Idx));
		}
	}

	// For data validation..
	for (INT Idx = 0; Idx < LayerInfos.Num(); Idx++)
	{
		LayerInfos(Idx).Hardness = Clamp<FLOAT>(LayerInfos(Idx).Hardness, 0.f, 1.f);
	}

#if WITH_EDITOR
	if( GIsEditor )
	{
		if (GetLinker() && (GetLinker()->Ver() < VER_CHANGED_LANDSCAPE_MATERIAL_PARAMS))
		{
			GWarn->BeginSlowTask( TEXT("Updating Landscape material combinations"), TRUE);

			// Clear any RF_Standalone flag for material instance constants in the level package
			// So it can be GC'd when it's no longer used.
			UObject* Pkg = GetOutermost();
			for ( TObjectIterator<UMaterialInstanceConstant> It; It; ++It )
			{
				if( (*It)->GetOutermost() == Pkg )
				{
					(*It)->ClearFlags(RF_Standalone);

					// Clear out the parent for any old MIC's
					(*It)->SetParent(NULL);
				}
			}
			
			// Recreate MIC's for all components
			for(INT ComponentIndex = 0; ComponentIndex < LandscapeComponents.Num(); ComponentIndex++ )
			{
				GWarn->StatusUpdatef( ComponentIndex, LandscapeComponents.Num(), TEXT("Updating Landscape material combinations") );
				ULandscapeComponent* Comp = LandscapeComponents(ComponentIndex);
				if( Comp )
				{
					Comp->UpdateMaterialInstances();
				}
			}

			// Clear out the thumbnail MICs so they're recreated
			for (INT LayerIdx = 0; LayerIdx < LayerInfos.Num(); LayerIdx++)
			{
				LayerInfos(LayerIdx).ThumbnailMIC = NULL;
			}

			GWarn->EndSlowTask();
		}
	}
#endif
}

void ALandscape::BeginDestroy()
{
	Super::BeginDestroy();

#if WITH_EDITOR
	if( DataInterface )
	{
		delete DataInterface;
		DataInterface = NULL;
	}
#endif
}

IMPLEMENT_CLASS(ALandscape);

