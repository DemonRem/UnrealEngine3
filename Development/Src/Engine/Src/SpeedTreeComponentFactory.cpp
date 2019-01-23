/*=============================================================================
	SpeedTreeComponentFactory.cpp: SpeedTreeComponentFactory implementation.
	Copyright 1998-2007 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "SpeedTree.h"

IMPLEMENT_CLASS(USpeedTreeComponentFactory);

UPrimitiveComponent* USpeedTreeComponentFactory::CreatePrimitiveComponent( UObject* InOuter)
{
#if WITH_SPEEDTREE
	USpeedTreeComponent* NewComponent = ConstructObject<USpeedTreeComponent>(USpeedTreeComponent::StaticClass( ), InOuter);

	NewComponent->SpeedTree				= SpeedTreeComponent->SpeedTree;
	NewComponent->bUseLeaves			= SpeedTreeComponent->bUseLeaves;
	NewComponent->bUseBranches			= SpeedTreeComponent->bUseBranches;
	NewComponent->bUseFronds			= SpeedTreeComponent->bUseFronds;
	NewComponent->bUseBillboards		= SpeedTreeComponent->bUseBillboards;
	NewComponent->LodNearDistance		= SpeedTreeComponent->LodNearDistance;
	NewComponent->LodFarDistance		= SpeedTreeComponent->LodFarDistance;
	NewComponent->LodLevelOverride		= SpeedTreeComponent->LodLevelOverride;

	NewComponent->BranchMaterial		= SpeedTreeComponent->BranchMaterial;
	NewComponent->FrondMaterial			= SpeedTreeComponent->FrondMaterial;
	NewComponent->LeafMaterial			= SpeedTreeComponent->LeafMaterial;
	NewComponent->BillboardMaterial		= SpeedTreeComponent->BillboardMaterial;

	NewComponent->CollideActors			= SpeedTreeComponent->CollideActors;
	NewComponent->BlockActors			= SpeedTreeComponent->BlockActors;
	NewComponent->BlockZeroExtent		= SpeedTreeComponent->BlockZeroExtent;
	NewComponent->BlockNonZeroExtent	= SpeedTreeComponent->BlockNonZeroExtent;
	NewComponent->BlockRigidBody		= SpeedTreeComponent->BlockRigidBody;

	return NewComponent;
#else
	return NULL;
#endif
}

