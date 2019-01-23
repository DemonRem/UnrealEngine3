/*=============================================================================
UnPhysicalMaterial.h: Helper functions for PhysicalMaterial 
Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

// NOTE: move the PhysicalMaterial classes into here

#ifndef _UN_PHYSICAL_MATERIAL_H_
#define _UN_PHYSICAL_MATERIAL_H_

#include "EnginePhysicsClasses.h"

/**
 * This will search a StaticMeshActor and look at allof his materials and attempt to find
 * a physical material.  It will find the first one and then return.
 **/
template< typename HIT_DATA, typename OUT_STRUCT >
UBOOL FindPhysicalMaterialOnStaticMeshActor( const HIT_DATA& HitData, OUT_STRUCT& OutHitInfo )
{
	UBOOL Retval = FALSE;

	AStaticMeshActor* TheStaticMesh = Cast<AStaticMeshActor>(HitData.Actor);

	if( TheStaticMesh != NULL 
		&& TheStaticMesh->StaticMeshComponent != NULL 
		&& TheStaticMesh->StaticMeshComponent->StaticMesh != NULL
		)
	{
		for( INT ElementIndex = 0; ElementIndex < TheStaticMesh->StaticMeshComponent->StaticMesh->LODModels(0).Elements.Num(); ElementIndex++ )
		{
			UMaterialInterface* MaterialInterface = TheStaticMesh->StaticMeshComponent->GetMaterial(ElementIndex);

			if( MaterialInterface != NULL)
			{
				UPhysicalMaterial* PhysMaterial = MaterialInterface->GetPhysicalMaterial();

				if( PhysMaterial != NULL )
				{
					OutHitInfo.PhysMaterial = PhysMaterial;
					Retval = TRUE;
					break;
				}
			}

			UMaterial* Material = MaterialInterface ? MaterialInterface->GetMaterial() : NULL;

			if( Material != NULL && Material->PhysMaterial != NULL )
			{
				OutHitInfo.PhysMaterial = Material->PhysMaterial;
				Retval = TRUE;
				break;
			}
		}
	}

	return Retval;
}


/**
 * This is a helper function which will set the PhysicalMaterial based on
 * whether or not we have a PhysMaterial set from a Skeletal Mesh or if we
 * are getting it from the Environment.
 *
 **/
template< typename HIT_DATA, typename OUT_STRUCT >
void DetermineCorrectPhysicalMaterial( const HIT_DATA& HitData, OUT_STRUCT& OutHitInfo )
{
	// check to see if this has a Physical Material Override.  If it does then use that
	if( ( HitData.Component != NULL )
		&& ( HitData.Component->PhysMaterialOverride != NULL )
		)
	{
		OutHitInfo.PhysMaterial = HitData.Component->PhysMaterialOverride;
	}
	// if the physical material is already set then we know we hit a skeletal mesh and should return that PhysMaterial
	else if( HitData.PhysMaterial != NULL )
	{
		OutHitInfo.PhysMaterial = HitData.PhysMaterial;
	}
	// else we need to look at the Material and use that for our PhysMaterial.
	// The GetPhysicalMaterial is virtual and will return the correct Material for all
	// Material Types
	// The PhysMaterial may still be null
	else if( HitData.Material != NULL )
	{
		OutHitInfo.PhysMaterial = HitData.Material->GetPhysicalMaterial();
	}
	// we have the case of a mesh inside a blocking volume when that mesh has no collision
	else if( FindPhysicalMaterialOnStaticMeshActor( HitData, OutHitInfo ) == TRUE )
	{
		// the FindPhysicalMaterialOnStaticMeshActor did the work
	}	
	else
	{
		OutHitInfo.PhysMaterial = NULL;
	}
}


#endif // _UN_PHYSICAL_MATERIAL_H_


